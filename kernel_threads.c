
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_threads.h"


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
  sys_ThreadExit(exitval);
}

/**
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
  if(task==NULL)
    return NOTHREAD; // In case of failure return NOTHREAD (tid 0)
  //Mutex lock?

  // Current process
  PCB* curproc=CURPROC;
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
	return (Tid_t) ptcb_new->tcb; // Return Tid_t of new thread
	//Mutex unlock?
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) cur_thread();
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
  /*
    Find thread's PTCB by tid (findptcb)




  */
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  PTCB* ptcb = find_ptcb(tid);

  if ( ptcb == NULL || ptcb->exited == 1 || tid == NOTHREAD )
    return -1;

  //TODO:

  ptcb->refcount = 1;
  ptcb->detached = 1;
  Cond_Broadcast(&ptcb->exit_cv);

  return 0;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

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
	PTCB*	ptcb = xmalloc(sizeof(PTCB));  // allocate space for the new PTCB
  //init fields here
	ptcb->exited=0;
	ptcb->detached=0;
	ptcb->task=task;
	ptcb->argl=argl;
	if(args!=NULL){
		ptcb->args=args;
	}
	else{ptcb->args=NULL;}

	rlnode_init(&ptcb->ptcb_list_node,ptcb);  //initialisation of ptcb_list_node
	//ptcb->exit_cv ??

	rcinc(ptcb);  // increment refcount pointer

  return ptcb;
}



PTCB* find_PTCB(Tid_t tid){
/**

*/
  TCB* curr_tcb = cur_thread();
  assert(curr_tcb != NULL);

  rlnode head = curr_tcb->owner_pcb->ptcb_list;
  assert(head != NULL);

  // Find head of CURTHREAD->owner_PCB->PTCB_list
  rlnode* ptcb_node = rlist_find(&head,(PTCB*) tid, NULL);
  assert(ptcb_node != NULL);
  if (ptcb_node != NULL){
    PTCB* ptcb = (PTCB *) ptcb_node->ptcb;
    return ptcb;
  }

  return NULL;
}
