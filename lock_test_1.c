#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

/* How many threads (aside from main) to create */
#define THREAD_CNT 1

/* pthread_join is not implemented in homework 2 */
#define HAVE_PTHREAD_JOIN 0

/* Each counter goes up to a multiple of this value. If your test is too fast
 * use a bigger number. Too slow? Use a smaller number. See the comment about
 * sleeping in count() to avoid this size-tuning issue.
 */
#define COUNTER_FACTOR 100000000

int changeme = 0; // Global data structure to change
pthread_mutex_t * lock; // Global lock

void *count(void *arg) {
	unsigned long int c = (unsigned long int)arg;    
    
    // Get lock, print out changeme, change it to pid * 10, wait by counting, exit
    int res = pthread_mutex_lock(lock);
    printf("id: 0x%d read changeme as %d\n", pthread_self(), changeme);
    changme = pthread_self() * 10;
    printf("id: 0x%d changed changeme to %d\n", pthread_self(), changeme);
    
    // Waste time
    int i;
	for (i = 0; i < c/2; i++) {
		if ((i % 10000000) == 0) {
			printf("id: 0x%lx counted to %d of %ld\n",
			       pthread_self(), i, c);
		}
	}
    
    // Exit critical region
    res = pthread_mutex_unlock(lock);
    
    // Waste some more time to see normal scheduling in action
	for (i = i; i < c; i++) {
		if ((i % 10000000) == 0) {
			printf("id: 0x%lx counted to %d of %ld\n",
			       pthread_self(), i, c);
		}
	}

	return NULL;
}

int main(int argc, char **argv) {
	pthread_t threads[THREAD_CNT];
    lock = malloc(sizeof(pthread_mutex));
    int res = pthread_mutex_init(lock, NULL); // Init lock
	int i;
	for(i = 0; i < THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, count,
		               (void *)(intptr_t)((i + 1) * COUNTER_FACTOR));
	}

#if HAVE_PTHREAD_JOIN == 0
	
    // main thread will also change changeme after making thread. main should be blocked when second thread in crit region
	count((void *)(intptr_t)((i + 1) * COUNTER_FACTOR));
#else
	/* Collect statuses of the other threads, waiting for them to finish */
	for(i = 0; i < THREAD_CNT; i++) {
		pthread_join(threads[i], NULL);
	}
#endif
	return 0;
}