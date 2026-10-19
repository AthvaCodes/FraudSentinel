#include "queue.h"

FIFOQueue* queue_create(void) {
    FIFOQueue *q = (FIFOQueue *)malloc(sizeof(FIFOQueue));
    if (!q) return NULL;
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    q->total_enqueued = 0;
    q->total_dequeued = 0;
    return q;
}

void queue_free(FIFOQueue *q) {
    if (!q) return;
    queue_clear(q);
    free(q);
}

int queue_enqueue(FIFOQueue *q, const Transaction *t) {
    if (!q || !t) return 0;

    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!new_node) return 0;

    new_node->transaction = *t;
    new_node->next = NULL;

    if (q->rear == NULL) {
        q->front = new_node;
        q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }

    q->size++;
    q->total_enqueued++;
    return 1;
}

int queue_dequeue(FIFOQueue *q, Transaction *out_t) {
    if (!q || q->front == NULL) return 0;

    QueueNode *temp = q->front;
    if (out_t) *out_t = temp->transaction;

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    q->size--;
    q->total_dequeued++;
    return 1;
}

int queue_peek(const FIFOQueue *q, Transaction *out_t) {
    if (!q || q->front == NULL) return 0;
    if (out_t) *out_t = q->front->transaction;
    return 1;
}

int queue_is_empty(const FIFOQueue *q) {
    return (!q || q->front == NULL);
}

int queue_size(const FIFOQueue *q) {
    return q ? q->size : 0;
}

void queue_clear(FIFOQueue *q) {
    if (!q) return;
    Transaction dummy;
    while (queue_dequeue(q, &dummy));
}

void queue_to_json(const FIFOQueue *q, FILE *out) {
    if (!q) {
        fputs("{\"size\":0,\"items\":[]}", out);
        return;
    }

    fputs("{", out);
    fprintf(out, "\"size\":%d,", q->size);
    fprintf(out, "\"total_enqueued\":%d,", q->total_enqueued);
    fprintf(out, "\"total_dequeued\":%d,", q->total_dequeued);

    fputs("\"front\":", out);
    if (q->front) {
        print_transaction_json(out, &q->front->transaction);
    } else {
        fputs("null", out);
    }
    fputc(',', out);

    fputs("\"rear\":", out);
    if (q->rear) {
        print_transaction_json(out, &q->rear->transaction);
    } else {
        fputs("null", out);
    }
    fputc(',', out);

    fputs("\"items\":[", out);
    QueueNode *curr = q->front;
    int idx = 0;
    while (curr) {
        if (idx > 0) fputc(',', out);
        fputs("{", out);
        fprintf(out, "\"position\":%d,", idx);
        fprintf(out, "\"is_front\":%s,", (idx == 0) ? "true" : "false");
        fprintf(out, "\"is_rear\":%s,", (curr->next == NULL) ? "true" : "false");
        fputs("\"transaction\":", out);
        print_transaction_json(out, &curr->transaction);
        fputs("}", out);
        curr = curr->next;
        idx++;
    }
    fputs("]}", out);
}
