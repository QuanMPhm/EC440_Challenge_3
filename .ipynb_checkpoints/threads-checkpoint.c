#include "ec440threads.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <sys/time.h>
#include <signal.h>

/* You can support more threads. At least support this many. */
#define MAX_THREADS 128

/* Your stack should be this many bytes in size */
#define THREAD_STACK_SIZE 32767

/* Number of microseconds between scheduling events */
#define SCHEDULER_INTERVAL_USECS (50 * 1000)

/* Extracted from private libc headers. These are not part of the public
 * interface for jmp_buf.
 */
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7

/* thread_status identifies the current state of a thread. You can add, rename,
 * or delete these values. This is only a suggestion. */
enum thread_status
{
	TS_EXITED,
	TS_RUNNING,
	TS_READY
};

/* The thread control block stores information about a thread. You will
 * need one of this per thread.
 */
struct thread_control_block {
	/* TODO: add a thread ID */
    pthread_t thr_id;
	/* TODO: add information about its stack */
    void * thr_stack;
	/* TODO: add information about its registers */
    // long unsigned int * thr_registers[8];
    jmp_buf thr_registers;
	/* TODO: add information about the status (e.g., use enum thread_status) */
    enum thread_status thr_status;
	/* Add other information you need to manage this thread */
    
};

// Entry of the thread queue
struct thread_queue_block {
    struct thread_control_block * thread_block;    // TLB of thread
    struct thread_queue_block * next_thread;   // Points to next thread is circular queue
};

/*
Circular queue for scheduler
*/
struct thread_queue {
    int abs_thread_count;                            // Number of threads ever made
    int cur_thread_count;                            // Number of currently active threads
    struct thread_queue_block * first_thread;  // Points to "first" thread in circular queue
    // struct thread_queue_block * next_thread;  // Points to next thread to be considered for scheduling in circular queue
    struct thread_queue_block * now_thread;   // Points to currently running thread
    struct thread_queue_block * prev_thread;  // Points to previously ran thread (for pthread_exit purposes)
} global_queue;

// to supress compiler error
static void schedule(int signal) __attribute__((unused));

static void schedule(int signal)
{
	/* TODO: implement your round-robin scheduler 
	 * 1. Use setjmp() to update your currently-active thread's jmp_buf
	 *    You DON'T need to manually modify registers here.
	 * 2. Determine which is the next thread that should run
	 * 3. Switch to the next thread (use longjmp on that thread's jmp_buf)
	 */
    // printf("--- In scheduler\n");
    if (global_queue.cur_thread_count == 0) {
        // printf("That's a wrap!\n");
        exit(0);
    }
    if (setjmp(global_queue.now_thread->thread_block->thr_registers) == pthread_self()) return; 
    // If only 1 running thread, dont schedule
    if (global_queue.cur_thread_count == 1 && global_queue.now_thread->thread_block->thr_status == TS_RUNNING) return;   

    // Set current running thread to IS_READY and the previous thread
    if (global_queue.now_thread->thread_block->thr_status == TS_RUNNING) global_queue.now_thread->thread_block->thr_status = TS_READY;
    global_queue.prev_thread = global_queue.now_thread;
    
    struct thread_queue_block * next_thread_in_q = global_queue.now_thread->next_thread;
    enum thread_status next_thr_status = next_thread_in_q->thread_block->thr_status;
    
    // Find next available thread
    while (next_thr_status != TS_READY) {
        global_queue.prev_thread = next_thread_in_q;
        next_thread_in_q = next_thread_in_q->next_thread;
        next_thr_status = next_thread_in_q->thread_block->thr_status;
    }
    
    // Set next available thread to be current running thread
    global_queue.now_thread = next_thread_in_q;
    next_thread_in_q->thread_block->thr_status = TS_RUNNING;
    
    // Jump into wormhole
    longjmp(next_thread_in_q->thread_block->thr_registers, next_thread_in_q->thread_block->thr_id);
    
}

static void timer_handler(int sig) {
    // printf("--- In timer handler\n");
    schedule(sig);
    return;
}

static void exit_handler(void) {
    if (global_queue.cur_thread_count != 0) pthread_exit(NULL);
}

static void scheduler_init()
{
	/* TODO: do everything that is needed to initialize your scheduler. For example:
	 * - Allocate/initialize global threading data structures
	 * - Create a TCB for the main thread. Note: This is less complicated
	 *   than the TCBs you create for all other threads. In this case, your
	 *   current stack and registers are already exactly what they need to be!
	 *   Just make sure they are correctly referenced in your TCB.
	 * - Set up your timers to call schedule() at a 50 ms interval (SCHEDULER_INTERVAL_USECS)
	 */
    
    // Allocate Tcb and queue entry from main thread
    struct thread_control_block * main_tcb = malloc(sizeof(struct thread_control_block));
    struct thread_queue_block * main_qb = malloc(sizeof(struct thread_queue_block));
    
    // Init global queue
    global_queue.first_thread = main_qb;
    // global_queue.next_thread = thread_queue_block;
    global_queue.prev_thread = NULL;
    global_queue.now_thread = main_qb;
    global_queue.cur_thread_count = 1;
    global_queue.abs_thread_count = 1;
    
    main_tcb->thr_id = global_queue.abs_thread_count;
    main_tcb->thr_status = TS_RUNNING;
    main_qb->thread_block = main_tcb;
    main_qb->next_thread = main_qb;        // Pointing to myself :D
    
    
    // Setup timer
    struct sigaction new_action;
    new_action.sa_handler = timer_handler;
    new_action.sa_flags = SA_NODEFER;
    struct itimerval timer = {.it_value = {.tv_usec = SCHEDULER_INTERVAL_USECS}, .it_interval = {.tv_usec = SCHEDULER_INTERVAL_USECS}};
    if (sigaction(SIGALRM, &new_action, NULL)) printf("ERROR: Init signal handler failed\n");
    if (setitimer(ITIMER_REAL, &timer, NULL)) printf("ERROR: Start timer failed\n");
    
    // // Set main to return to scheduler
    // jmp_buf temp_buf;
    // setjmp(temp_buf);
    // long unsigned int rbp_val = ptr_demangle((unsigned long int) temp_buf[0].__jmpbuf[JB_RBP]);
    // *((long unsigned int *) rbp_val) = (long unsigned int) pthread_exit;
    atexit(exit_handler);
    
}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
    
    // printf("--- In pthread_create\n");
	// Create the timer and handler for the scheduler. Create thread 0.
	static bool is_first_call = true;
	if (is_first_call)
	{
		is_first_call = false;
		scheduler_init();
	}

	/* TODO: Return 0 on successful thread creation, non-zero for an error.
	 *       Be sure to set *thread on success.
	 * Hints:
	 * The general purpose is to create a TCB:
	 * - Create a stack.
	 * - Assign the stack pointer in the thread's registers. Important: where
	 *   within the stack should the stack pointer be? It may help to draw
	 *   an empty stack diagram to answer that question.
	 * - Assign the program counter in the thread's registers.
	 * - Wait... HOW can you assign registers of that new stack? 
	 *   1. call setjmp() to initialize a jmp_buf with your current thread
	 *   2. modify the internal data in that jmp_buf to create a new thread environment
	 *      env->__jmpbuf[JB_...] = ...
	 *      See the additional note about registers below
	 *   3. Later, when your scheduler runs, it will longjmp using your
	 *      modified thread environment, which will apply all the changes
	 *      you made here.
	 * - Remember to set your new thread as TS_READY, but only  after you
	 *   have initialized everything for the new thread.
	 * - Optionally: run your scheduler immediately (can also wait for the
	 *   next scheduling event).
	 */
	/*
	 * Setting registers for a new thread:
	 * When creating a new thread that will begin in start_routine, we
	 * also need to ensure that `arg` is passed to the start_routine.
	 * We cannot simply store `arg` in a register and set PC=start_routine.
	 * This is because the AMD64 calling convention keeps the first arg in
	 * the EDI register, which is not a register we control in jmp_buf.
	 * We provide a start_thunk function that copies R13 to RDI then jumps
	 * to R12, effectively calling function_at_R12(value_in_R13). So
	 * you can call your start routine with the given argument by setting
	 * your new thread's PC to be ptr_mangle(start_thunk), and properly
	 * assigning R12 and R13.
	 *
	 * Don't forget to assign RSP too! Functions know where to
	 * return after they finish based on the calling convention (AMD64 in
	 * our case). The address to return to after finishing start_routine
	 * should be the first thing you push on your stack.
	 */
    
    // Assume unlimited threads for now
    global_queue.cur_thread_count++;
    global_queue.abs_thread_count++;
    // Allocate Tcb and queue entry from new thread
    struct thread_control_block * new_tcb = malloc(sizeof(struct thread_control_block));
    struct thread_queue_block * new_qb = malloc(sizeof(struct thread_queue_block));
    new_tcb->thr_status = TS_READY;
    new_tcb->thr_id = global_queue.abs_thread_count;
    *thread = global_queue.abs_thread_count;
    
    // Put new thread after the currently running (New thread will be executed immediately)
    new_qb->thread_block = new_tcb;
    new_qb->next_thread = global_queue.now_thread->next_thread;
    global_queue.now_thread->next_thread = new_qb;
    
    // Create de la registers
    setjmp(new_tcb->thr_registers);    // Set new thread's reigsters initial values
    
    long unsigned int new_stack_ptr = (long unsigned int)  malloc(THREAD_STACK_SIZE);
    *((long unsigned int *) (new_stack_ptr + THREAD_STACK_SIZE)) = (long unsigned int) pthread_exit;   // Set top of stack to address of pthread_exit
    new_tcb->thr_registers[0].__jmpbuf[JB_RSP] = ptr_mangle(new_stack_ptr + THREAD_STACK_SIZE);  // Assign mangled rsp to beginning of malloc'ed mem for now
    new_tcb->thr_registers[0].__jmpbuf[JB_RBP] = new_tcb->thr_registers[0].__jmpbuf[JB_RSP];     // rbp = rsp
    new_tcb->thr_registers[0].__jmpbuf[JB_PC] = ptr_mangle((long unsigned int) start_thunk);
    new_tcb->thr_registers[0].__jmpbuf[JB_R12] = (long unsigned int) start_routine;     // Set real function's address
    new_tcb->thr_registers[0].__jmpbuf[JB_R13] = (long unsigned int) arg;               // Pass arg
    
    // ---- TEST ----
    // longjmp(new_tcb->thr_registers, 1);
    
    schedule(0);
	return 0;
}

void pthread_exit(void *value_ptr)
{
	/* TODO: Exit the current thread instead of exiting the entire process.
	 * Hints:
	 * - Release all resources for the current thread. CAREFUL though.
	 *   If you free() the currently-in-use stack then do something like
	 *   call a function or add/remove variables from the stack, bad things
	 *   can happen.
	 * - Update the thread's status to indicate that it has exited
	 */
    
    // printf("--- In pthread_exit\n");
    
//     // Remove thread from global queue
//     global_queue.prev_thread->next_thread =  global_queue.now_thread->next_thread;
    
    // // Free the thread TLB, saving the 32600-sized block to be freed last
    // void * dead_thr_stack_ptr = global_queue.now_thread->thread_block->thr_stack;
//     free(global_queue.now_thread->thread_block);
//     free(global_queue.now_thread);
    
//     // Set current running thread to same as previous thread, this means we'll need special checks when we do scheduling after a thread exit. Not clean
//     global_queue.now_thread = global_queue.prev_thread;   
    
//     free(dead_thr_stack_ptr);
    
    global_queue.cur_thread_count--;
    global_queue.now_thread->thread_block->thr_status = TS_EXITED;
    
    // Free the thread TLB, saving the 32600-sized block to be freed last
    // If this is not main() thread, don't free
    if (global_queue.now_thread->thread_block->thr_id != 1) {
        void * dead_thr_stack_ptr = global_queue.now_thread->thread_block->thr_stack;
        free(dead_thr_stack_ptr);
    }
    schedule(0);
    
    // Exit thread with value 0, ignore 
	exit(0);
}

pthread_t pthread_self(void)
{
	/* TODO: Return the current thread instead of -1
	 * Hint: this function can be implemented in one line, by returning
	 * a specific variable instead of -1.
	 */
    
	return global_queue.now_thread->thread_block->thr_id;
}

int pthread_mutex_init(
	pthread_mutex_t *restrict mutex,
	const pthread_mutexattr_t *restrict attr) 
{
    
}

int pthread_mutex_destroy(
	pthread_mutex_t *mutex) 
{

}

int pthread_mutex_lock(pthread_mutex_t *mutex) 
{

}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{

}


int pthread_barrier_init(
    pthread_barrier_t *restrict barrier,
    const pthread_barrierattr_t *restrict attr,
    unsigned count)
{

}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
}

int pthread_barrier_wait(pthread_barrier_t *barrier) 
{
}

static void lock() 
{}

static void unlock()
{}

/* Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */
