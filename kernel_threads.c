
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_threads.h"
#include "kernel_cc.h"
#include "kernel_streams.h"

// Take the arguments of our process and ...

void start_new_thread()
{
  int exitval;
  TCB* current_tcb=cur_thread();
  PTCB* current_ptcb = current_tcb->ptcb;  //CAREFUL! may not work and  need find PTCB

  Task call = current_ptcb->task;
  int argl =current_ptcb->argl;
  void* args = current_ptcb->args;

  exitval = call(argl,args);
  ThreadExit(exitval);
}

/**
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
  // Current process
  PCB* curproc=CURPROC;

  if(task==NULL)
    return NOTHREAD; // In case of failure return NOTHREAD (tid 0)
  //Mutex lock?


  // Create and allocate new PTCB
  PTCB* ptcb_new=new_ptcb(task,argl,args);
  // proprietary, delete probably: ptcb_new->tcb->owner_pcb = curproc; // link PCB<-----TCB
  //the above is done in spawn_thread() already
  rlist_push_back(&curproc->ptcb_list, &ptcb_new->ptcb_list_node);// link PTCB-->other PTCB's-->PCB

  // Have to change main thread

  ptcb_new->tcb = spawn_thread(curproc, start_new_thread); // Link link PTCB--->TCB
  ptcb_new->tcb->ptcb = ptcb_new; // link PTCB<-----TCB
  curproc->thread_count++; // Increase thread ounter
  wakeup(ptcb_new->tcb); // Set to READY for Scheduler
  return (Tid_t) ptcb_new; // Return Tid_t of new thread
  
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) cur_thread()->ptcb;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
  PTCB* new_ptcb = (PTCB*) tid;

  if(rlist_find(&CURPROC->ptcb_list, new_ptcb, NULL) != NULL && tid != sys_ThreadSelf() && new_ptcb->detached == 0) {

    new_ptcb->refcount++; // increase ref counter by 1

    while (new_ptcb->exited != 1 && new_ptcb->detached != 1) // wait till new ptcb is exited or detached. 
    {
      kernel_wait(&new_ptcb->exit_cv, SCHED_USER);
    }

    new_ptcb->refcount--; //since it is detached or exited, decrease rf counter and possibly remove new_ptcb from list/free it

    if(exitval!= NULL ) //get the exitval
      *exitval = new_ptcb->exitval;

    if(new_ptcb->detached != 0)
      return -1;

    if(new_ptcb->refcount == 0){
      rlist_remove(&new_ptcb->ptcb_list_node); //remove from list
      free(new_ptcb);
    }
    return 0;
  }
  return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  if(tid == NOTHREAD)
    return -1;

  PTCB* ptcb_to_detach = (PTCB*) tid;

  if(rlist_find(& CURPROC->ptcb_list, ptcb_to_detach, NULL) != NULL && ptcb_to_detach->exited == 0){
    ptcb_to_detach->detached = 1;
    // Wake threads waiting for cv
    kernel_broadcast(&ptcb_to_detach->exit_cv);
    return 0;
  }
	
  return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

  PTCB* ptcb = (PTCB*) sys_ThreadSelf();
      
  ptcb->exited = 1;
  ptcb->exitval = exitval;
  kernel_broadcast(&ptcb->exit_cv);

  PCB* curproc = CURPROC;
  curproc->thread_count--;
  if ( curproc->thread_count == 0){

    if (get_pid(curproc)!=1){

    /* Reparent any children of the exiting process to the
       initial task */
      PCB* initpcb = get_pcb(1);
      while(!is_rlist_empty(& curproc->children_list)) {
        rlnode* child = rlist_pop_front(& curproc->children_list);
        child->pcb->parent = initpcb;
        rlist_push_front(& initpcb->children_list, child);
      }

      /* Add exited children to the initial task's exited list
         and signal the initial task */
      if(!is_rlist_empty(& curproc->exited_list)) {
        rlist_append(& initpcb->exited_list, &curproc->exited_list);
        kernel_broadcast(& initpcb->child_exit);
      }

      /* Put me into my parent's exited list */
      rlist_push_front(& curproc->parent->exited_list, &curproc->exited_node);
      kernel_broadcast(& curproc->parent->child_exit);
    }

    assert(is_rlist_empty(& curproc->children_list));
    assert(is_rlist_empty(& curproc->exited_list));
    /*
      Do all the other cleanup we want here, close files etc.
     */

    /* Release the args data */
    if(curproc->args) {
      free(curproc->args);
      curproc->args = NULL;
    }

    /* Clean up FIDT */
    for(int i=0;i<MAX_FILEID;i++) {
      if(curproc->FIDT[i] != NULL) {
        FCB_decref(curproc->FIDT[i]);
        curproc->FIDT[i] = NULL;
      }
    }

    /* Disconnect my main_thread */
    curproc->main_thread = NULL;

    /* Now, mark the process as exited. */
    curproc->pstate = ZOMBIE;
  }
  /* Bye-bye cruel world */
  kernel_sleep(EXITED, SCHED_USER);
}

/*This is the  function that allocates space and does the basic initialisation
	for a new PTCB. It is our intention to be */
PTCB* new_ptcb(Task task, int argl, void* args){
	PTCB*	ptcb = xmalloc(sizeof(PTCB));  // bbbbbbb space for the new PTCB
  //init fields here
	ptcb->exited=0;
	ptcb->detached=0;
	ptcb->task=task;
	ptcb->argl=argl;
  	ptcb->refcount=0;
	if(args!=NULL){
		ptcb->args=args;
	}
	else{ptcb->args=NULL;}

	rlnode_init(&ptcb->ptcb_list_node,ptcb);  //initialisation of ptcb_list_node
	ptcb->exit_cv = COND_INIT;

  return ptcb;
}
