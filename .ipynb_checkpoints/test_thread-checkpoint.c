#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_mutex_t lock; // Global lock
pthread_mutex_t lock2; // Global lock

void *count(void *arg) {
	unsigned long int c = (unsigned long int) arg;    
    
    // Get lock, print out changeme, change it to pid * 10, wait by counting, exit
    pthread_mutex_lock(&lock);
    pthread_mutex_lock(&lock2);
    // struct timespec time_sleep = {.tv_nsec = 50000000};
    
    // Waste time
    int i;
	for (i = 0; i < c/2; i++) {
		if ((i % 10000000) == 0) {
			printf("id: 0x%lx counted to %d of %ld\n",
			       pthread_self(), i, c);
            // sleep(10);
            // nanosleep(&time_sleep, NULL);
        }
	}
    
    // Exit critical region
    pthread_mutex_unlock(&lock);
    pthread_mutex_unlock(&lock2);
    
    // Waste some more time to see normal scheduling in action
	for (i = i; i < c; i++) {
		if ((i % 10000000) == 0) {
			printf("id: 0x%lx counted to %d of %ld\n",
			       pthread_self(), i, c);
		}
	}

	return NULL;
}

int main() {
    pthread_t t_t = 0;
    pthread_t t_t2 = 0;
    pthread_mutex_init(&lock, NULL); // Init lock
    pthread_mutex_init(&lock2, NULL); // Init lock
    int res = pthread_create(&t_t, NULL, count, (void *)(intptr_t)((0 + 2) * 100000000));
    int res2 = pthread_create(&t_t2, NULL, count, (void *)(intptr_t)((0 + 2) * 100000000));
    printf("%d%d", res, res2);
    count((void *) (3 * 100000000));
    
    pthread_mutex_destroy(&lock);
    return 0;
}
