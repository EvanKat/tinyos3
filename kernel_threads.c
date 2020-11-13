
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_threads.h"
#include "kernel_cc.h"


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
  if(task==NULL)
    return NOTHREAD; // In case of failure return NOTHREAD (tid 0)

  // Current process
  PCB* curproc=CURPROC;
  // Create and allocate new PTCB
  PTCB* ptcb_new=new_ptcb(task,argl,args);


	rlist_push_back(&curproc->ptcb_list, &ptcb_new->ptcb_list_node);// link PTCB-->other PTCB's-->PCB


  ptcb_new->tcb = spawn_thread(curproc, start_new_thread); // Link link PTCB--->TCB
  ptcb_new->tcb->ptcb = ptcb_new; // link PTCB<-----TCB
  curproc->thread_count++; // Increase thread ounter
  wakeup(ptcb_new->tcb); // Set to READY for Scheduler
	return (Tid_t) ptcb_new; // Return Tid_t of new thread
}

/**
TODO: Evaluate if needed
*/
// PTCB* find_PTCB(Tid_t tid){
  
//   TCB* curr_tcb = cur_thread();

//   rlnode head = CURPROC->ptcb_list;
 

//   // Find head of CURTHREAD->owner_PCB->PTCB_list
//   rlnode* ptcb_node = rlist_find(&head,(PTCB*) tid, NULL);

//   if (ptcb_node != NULL){
//     PTCB* ptcb = ptcb_node->ptcb;
//     return ptcb;
//   }

//   return NULL;
// }

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

  if(rlist_find(& CURPROC->ptcb_list, new_ptcb, NULL)!=NULL && tid != sys_ThreadSelf() && new_ptcb->detached == 0) {

    rcinc(new_ptcb); // increase ref counter by 1

    while (new_ptcb->exited != 1 && new_ptcb->detached != 1) // wait till new ptcb is exited or detached. 
    {
      kernel_wait(&new_ptcb->exit_cv, SCHED_USER);
    }

    if(exitval!= NULL ){ //add comments here/ ask around
        *exitval = new_ptcb->exitval;
    }

    rcdec(new_ptcb); //since it is detached or exited, decrease rf counter and possibly remove new_ptcb from list/free it

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

  // if (ptcb_found == NULL || ptcb_to_detach->exited == 1)
  //   return -1;

  // ptcb_to_detach->detached = 1;
  // kernel_broadcast(&ptcb_to_detach->exit_cv);
  // return 0;
  return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
  //Check 

}

/**
  @brief Decrements the refcount value at the pointed ptcb and check if islast

  The function rcdec() does not return anything, it only
  decrements the refcount value at the ptcb that the
  argument points to, and checks if that was the last
  reference to the struct, to deallocate it with free()

  @param ptcb The pointer to the ptcb
  */
void rcdec(PTCB* ptcb){
  ptcb->refcount --;
  if(ptcb->refcount == 0){
    rlist_remove(&ptcb->ptcb_list_node); //remove from list
    free(ptcb);
  }
}

/**
  @brief Incriments refcount of PTCB

  The function rcinc() does not return anything, it only
  increments the refcount value at the ptcb that the
  argument points to.

  @param ptcb The pointed ptcb
*/
void rcinc(PTCB* ptcb){
    ptcb->refcount++;
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

	rcinc(ptcb);  // increment refcount pointer

  return ptcb;
}




