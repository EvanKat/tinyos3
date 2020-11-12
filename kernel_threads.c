
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_threads.h"


// Take the arguments of our process and ...

void start_new_thread()
{
  int exitval;
	//FIND the current PTCB (pointer to)

	//PTCB* ptcb = rlist_pop_back(&CURPROC->ptcb_list);
	rlnode* lptcb = rlist_pop_back(& CURPROC->ptcb_list);
  rlist_push_back(& CURPROC->ptcb_list, lptcb);

  Task call = lptcb->ptcb->task;
  int argl =lptcb->ptcb->argl;
  void* args = lptcb->ptcb->args;

  exitval = call(argl,args);
  Exit(exitval);
}

/**
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
	//Mutex lock?

  // Current process
  PCB* curproc=CURPROC;
  // Create and allocate new PTCB
  PTCB* ptcb_new=new_ptcb(task,argl,args);

	// proprietary, delete probably: ptcb_new->tcb->owner_pcb = curproc; // link PCB<-----TCB
	//the above is done in spawn_thread() already

	rlist_push_back(&curproc->ptcb_list, &ptcb_new->ptcb_list_node);// link PTCB-->other PTCB's-->PCB

  // Have to change main thread
	if(task!=NULL){
  ptcb_new->tcb = spawn_thread(curproc, start_new_thread); // Link link PTCB--->TCB
  ptcb_new->tcb->ptcb = ptcb_new; // link PTCB<-----TCB
  curproc->thread_count++; // Increase thread ounter
	}
	//Mutex unlock?
	return NOTHREAD;
	//return (Tid_t) ptcb_new->tcb; ??
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
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
	return -1;
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
