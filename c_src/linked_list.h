#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "common.h"
#include "transaction.h"

/* Flagged Transaction Linked List Node - exactly from original C code */
typedef struct FlagNode {
    Transaction transaction;
    char reason[MAX_REASON_LEN];
    struct FlagNode *next;
} FlagNode;

extern FlagNode *flaggedHead;

/* Linked list operations */
void add_flagged_transaction(const Transaction *t, const char *reason); // O(1) prepend
void append_flagged_transaction(const Transaction *t, const char *reason); // O(n) append
int delete_flagged_transaction(int transaction_id);
FlagNode* search_flagged_transaction(int transaction_id);
void display_flagged_transactions_console(void);
void clear_flagged_list(void);
int get_flagged_count(void);

void print_flagged_list_json(FILE *out);
void print_flagged_list_visual_json(FILE *out);

#endif /* LINKED_LIST_H */
