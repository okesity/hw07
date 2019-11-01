// Author: Nat Tuck
// CS3650 starter code

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <semaphore.h>

#include "barrier.h"



barrier*
make_barrier(int nn)
{
    barrier* bb = malloc(sizeof(barrier));
    assert(bb != 0);
    pthread_mutex_init(&(bb->mutex), NULL);
    pthread_cond_init(&(bb->cond), NULL);

    bb->count = nn;
    bb->seen = 0;
    return bb;
}

void
barrier_wait(barrier* bb)
{
    pthread_mutex_lock(&(bb->mutex));
    bb->seen += 1;

    if (bb->seen >= bb->count) {
        puts("all entered!");
        fflush(stdout);
        pthread_cond_broadcast(&(bb->cond));
    }
    else {
        while(bb->seen < bb->count) {
            puts("waiting");
            pthread_cond_wait(&(bb->cond), &(bb->mutex));
        }
    }
    pthread_mutex_unlock(&(bb->mutex));
    return;
}

void
free_barrier(barrier* bb)
{
    free(bb);
}

