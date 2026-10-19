#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"
#include "transaction.h"

typedef struct QueueNode {
    Transaction transaction;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int size;
    int total_enqueued;
    int total_dequeued;
} FIFOQueue;

/* Queue API */
FIFOQueue* queue_create(void);
void queue_free(FIFOQueue *q);
int queue_enqueue(FIFOQueue *q, const Transaction *t); // O(1)
int queue_dequeue(FIFOQueue *q, Transaction *out_t);    // O(1)
int queue_peek(const FIFOQueue *q, Transaction *out_t); // O(1)
int queue_is_empty(const FIFOQueue *q);
int queue_size(const FIFOQueue *q);
void queue_clear(FIFOQueue *q);

void queue_to_json(const FIFOQueue *q, FILE *out);

#endif /* QUEUE_H */
