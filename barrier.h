// Author: Nat Tuck
// CS3650 starter code

#ifndef BARRIER_H
#define BARRIER_H

#include <pthread.h>
#include <semaphore.h>


typedef struct barrier {
    // TODO: Need some synchronization stuff.
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int seen;
} barrier;

barrier* make_barrier(int nn);
void barrier_wait(barrier* bb, int id);
void free_barrier(barrier* bb);


#endif

