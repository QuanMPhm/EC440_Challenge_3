#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/*
This test case initializes a barrier of count 2, and starts main along with one more thread
Both the main thread and the child thread will count. When they reach their half-way point, they will wait on the barrier
The child thread should hit the barrier first, so you will see its print statements briefly at the start
The main thread will take longer to hit the barrier, so for a while you should only see print statements from main
Once both threads hit the barrier, it should reset, and both threads can keep counting freely until finish
*/

/* How many threads (aside from main) to create */
#define THREAD_CNT 1

/* pthread_join is not implemented in homework 2 */
#define HAVE_PTHREAD_JOIN 0

#define COUNTER_FACTOR 100000000

int changeme = 0; // Global data structure to change
pthread_barrier_t barrier; // Global barrier

void *count(void *arg) {
	unsigned long int c = (unsigned long int) arg;    
    

    // Waste time, counting up to half way
    int i;
	for (i = 0; i < c/2; i++) {
		if ((i % 10000000) == 0) {
			printf("id: 0x%lx counted to %d of %ld\n",
			       pthread_self(), i, c);
            // sleep(10);
            // nanosleep(&time_sleep, NULL);
        }
	}
    
    // Wait on barrier here
    pthread_barrier_wait(&barrier);
    
    // Edit dummy var
    printf("id: 0x%ld read changeme as %d\n", pthread_self(), changeme);
    changeme = pthread_self() * 10;
    printf("id: 0x%ld changed changeme to %d\n", pthread_self(), changeme);
    
    // Waste some more time to see normal scheduling in action and timing of barrier
	for (i = i; i < c; i++) {
		if ((i % 10000000) == 0) {
			printf("id: 0x%lx counted to %d of %ld\n",
			       pthread_self(), i, c);
		}
	}

	return NULL;
}

void * count2(void * arg) {
    int i;
    unsigned long int c = (unsigned long int) arg; 
    for (i = 0; i < c; i++) {
        if (i == c * 0.8) pthread_barrier_wait(&barrier);
        
		if ((i % 10000000) == 0) {
			printf("id: 0x%lx counted to %d of %ld\n",
			       pthread_self(), i, c);
		}
        
    }
    return NULL;
}


int main(int argc, char **argv) {
	pthread_t threads[THREAD_CNT + 1];
    pthread_barrier_init(&barrier, NULL, 3); // Init lock
	int i;
	for(i = 0; i < THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, count,
		               (void *)(intptr_t)((i + 1) * COUNTER_FACTOR));
	}
    
    pthread_create(&threads[i + 1], NULL, count2, (void *)(intptr_t)((i + 2) * COUNTER_FACTOR));

#if HAVE_PTHREAD_JOIN == 0
	
    // main thread will also change changeme after making thread. main should continue running while second thread is blocked on barrier
	count((void *)(intptr_t)((i + 2) * COUNTER_FACTOR));
    // Destroy lock
    pthread_barrier_destroy(&barrier);
#else
	/* Collect statuses of the other threads, waiting for them to finish */
	for(i = 0; i < THREAD_CNT; i++) {
		pthread_join(threads[i], NULL);
	}
#endif
	return 0;
}