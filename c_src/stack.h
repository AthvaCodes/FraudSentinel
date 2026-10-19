#ifndef STACK_H
#define STACK_H

#include "common.h"

typedef struct StackNode {
    char action_type[MAX_STATUS_LEN];   // "STATUS_CHANGE", "NOTE_ADDED", "CONFIG_CHANGE"
    int transaction_id;
    char old_status[MAX_STATUS_LEN];
    char new_status[MAX_STATUS_LEN];
    char timestamp[32];
    char details[MAX_NOTES_LEN];
    struct StackNode *next;
} StackNode;

typedef struct {
    StackNode *top;
    int size;
    int total_pushed;
    int total_popped;
} AuditStack;

extern AuditStack *g_audit_stack;

/* Stack API */
AuditStack* stack_create(void);
void stack_free(AuditStack *s);
int stack_push(AuditStack *s, const char *action_type, int tx_id, const char *old_status, const char *new_status, const char *details);
int stack_pop(AuditStack *s, StackNode *out_node);
int stack_peek(const AuditStack *s, StackNode *out_node);
int stack_is_empty(const AuditStack *s);
int stack_size(const AuditStack *s);
void stack_clear(AuditStack *s);

void stack_to_json(const AuditStack *s, FILE *out);

#endif /* STACK_H */
