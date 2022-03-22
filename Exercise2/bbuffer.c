#include "bbuffer.h"
#include "sem.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct BNDBUF
{
    int *data;
    int size;
    int head;
    int tail;
    SEM *full;
    SEM *empty;
} BNDBUF;

BNDBUF *bb_init(unsigned int size)
{
    BNDBUF *ring_buffer = malloc(sizeof(struct BNDBUF));

    if (ring_buffer == NULL)
    {
        return NULL;
    }

    ring_buffer->size = size;
    ring_buffer->head = 0;
    ring_buffer->tail = 0;
    ring_buffer->data = malloc(sizeof(size));

    ring_buffer->full = sem_init(0);
    if (ring_buffer->full == NULL)
    {
        bb_del(ring_buffer);
        return NULL;
    }
    ring_buffer->empty = sem_init(size);
    if (ring_buffer->empty == NULL)
    {
        bb_del(ring_buffer);
        return NULL;
    }

    return ring_buffer;
}

void bb_del(BNDBUF *bb)
{
    sem_del(bb->full);
    sem_del(bb->empty);
    free(bb->data);
    free(bb);

    return;
}

int bb_get(BNDBUF *bb)
{
    int retrived_data;

    P(bb->full);

    bb->tail++;

    if (bb->tail >= bb->size)
    {
        bb->tail = 0;
    }
    retrived_data = bb->data[bb->tail];
    V(bb->empty);

    return retrived_data;
}

void bb_add(BNDBUF *bb, int fd)
{
    P(bb->empty);
    bb->head++;
    if (bb->head >= bb->size)
    {
        bb->head = 0;
    }
    bb->data[bb->head] = fd;
    V(bb->full);
}