#include "linked_list.h"

FlagNode *flaggedHead = NULL;

void add_flagged_transaction(const Transaction *t, const char *reason) {
    FlagNode *newNode = (FlagNode *)malloc(sizeof(FlagNode));
    if (!newNode) return;

    newNode->transaction = *t;
    safe_strcpy(newNode->reason, reason, sizeof(newNode->reason));

    // O(1) Prepend - exactly matching original C code:
    newNode->next = flaggedHead;
    flaggedHead = newNode;
}

void append_flagged_transaction(const Transaction *t, const char *reason) {
    FlagNode *newNode = (FlagNode *)malloc(sizeof(FlagNode));
    if (!newNode) return;

    newNode->transaction = *t;
    safe_strcpy(newNode->reason, reason, sizeof(newNode->reason));
    newNode->next = NULL;

    if (!flaggedHead) {
        flaggedHead = newNode;
        return;
    }

    FlagNode *curr = flaggedHead;
    while (curr->next) {
        curr = curr->next;
    }
    curr->next = newNode;
}

int delete_flagged_transaction(int transaction_id) {
    FlagNode *curr = flaggedHead;
    FlagNode *prev = NULL;

    while (curr) {
        if (curr->transaction.id == transaction_id) {
            if (prev == NULL) {
                flaggedHead = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}

FlagNode* search_flagged_transaction(int transaction_id) {
    FlagNode *curr = flaggedHead;
    while (curr) {
        if (curr->transaction.id == transaction_id) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void display_flagged_transactions_console(void) {
    FlagNode *current = flaggedHead;
    if (current == NULL) {
        printf("\nNo flagged transactions.\n");
        return;
    }

    printf("\n========== FLAGGED TRANSACTIONS ==========\n");
    while (current != NULL) {
        printf("\nTransaction ID : %d", current->transaction.id);
        printf("\nSender         : %s", current->transaction.sender);
        printf("\nReceiver       : %s", current->transaction.receiver);
        printf("\nAmount         : Rs. %.2f", current->transaction.amount);
        printf("\nReason         : %s\n", current->reason);
        current = current->next;
    }
}

void clear_flagged_list(void) {
    FlagNode *temp;
    while (flaggedHead != NULL) {
        temp = flaggedHead;
        flaggedHead = flaggedHead->next;
        free(temp);
    }
}

int get_flagged_count(void) {
    int count = 0;
    FlagNode *curr = flaggedHead;
    while (curr) {
        count++;
        curr = curr->next;
    }
    return count;
}

void print_flagged_list_json(FILE *out) {
    fputs("[", out);
    FlagNode *curr = flaggedHead;
    int first = 1;
    while (curr) {
        if (!first) fputc(',', out);
        first = 0;

        fputs("{", out);
        fputs("\"transaction\":", out);
        print_transaction_json(out, &curr->transaction);
        fputs(",\"reason\":", out);
        print_json_escaped_string(out, curr->reason);
        fputs("}", out);

        curr = curr->next;
    }
    fputs("]", out);
}

void print_flagged_list_visual_json(FILE *out) {
    fputs("{\"total_nodes\":", out);
    fprintf(out, "%d,", get_flagged_count());
    fputs("\"head_address\":", out);
    if (flaggedHead) {
        fprintf(out, "\"0x%p\",", (void*)flaggedHead);
    } else {
        fputs("\"NULL\",", out);
    }
    fputs("\"nodes\":[", out);

    FlagNode *curr = flaggedHead;
    int idx = 0;
    while (curr) {
        if (idx > 0) fputc(',', out);
        fputs("{", out);
        fprintf(out, "\"index\":%d,", idx);
        fprintf(out, "\"memory_address\":\"0x%p\",", (void*)curr);
        if (curr->next) {
            fprintf(out, "\"next_address\":\"0x%p\",", (void*)curr->next);
        } else {
            fputs("\"next_address\":null,", out);
        }
        fprintf(out, "\"transaction_id\":%d,", curr->transaction.id);
        fputs("\"sender\":", out); print_json_escaped_string(out, curr->transaction.sender); fputc(',', out);
        fputs("\"receiver\":", out); print_json_escaped_string(out, curr->transaction.receiver); fputc(',', out);
        fprintf(out, "\"amount\":%.2f,", curr->transaction.amount);
        fputs("\"reason\":", out); print_json_escaped_string(out, curr->reason);
        fputs("}", out);

        curr = curr->next;
        idx++;
    }
    fputs("]}", out);
}
