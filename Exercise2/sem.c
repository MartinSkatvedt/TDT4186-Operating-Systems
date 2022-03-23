#include "sem.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct SEM
{
    volatile int counter;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} SEM;

SEM *sem_init(int initVal)
{
    // Allocate memory for struct
    SEM *semaphore = malloc(sizeof(struct SEM));

    // Return NULL if failed to allocate
    if (semaphore == NULL)
    {
        return NULL;
    }

    // Set counter to initVAL
    semaphore->counter = initVal;

    // Init mutex, if the function returns != 0 it failed and we return NULL
    if (pthread_mutex_init(&(semaphore->mutex), NULL) != 0)
    {
        sem_del(semaphore);
        return NULL;
    }

    // init condition variable, if the function returns != 0 it failed and we return NULL
    if (pthread_cond_init(&(semaphore->condition), NULL) != 0)
    {
        sem_del(semaphore);
        return NULL;
    }

    return semaphore;
}

int sem_del(SEM *sem)
{

    int error = 0;

    if (pthread_mutex_destroy(&(sem->mutex)) != 0)
    {
        error = -1;
    }

    if (pthread_cond_destroy(&(sem->condition)) != 0)
    {
        error = -1;
    }

    free(sem);
    return error;
}

// Wait
void P(SEM *semaphore)
{
    pthread_mutex_lock(&(semaphore->mutex));
    while (semaphore->counter == 0)
    {
        pthread_cond_wait(&(semaphore->condition), &(semaphore->mutex));
    }

    semaphore->counter--;
    pthread_mutex_unlock(&(semaphore->mutex));
}

// Signal
void V(SEM *semaphore)
{
    pthread_mutex_lock(&(semaphore->mutex));
    semaphore->counter++;

    if (semaphore->counter > 0)
    {
        pthread_cond_signal(&semaphore->condition);
    }

    pthread_mutex_unlock(&(semaphore->mutex));
}