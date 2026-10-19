#ifndef BST_H
#define BST_H

#include "common.h"
#include "transaction.h"

typedef struct BSTNode {
    double amount_key;
    Transaction *transactions;
    int tx_count;
    int tx_capacity;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

typedef struct {
    BSTNode *root;
    int total_nodes;
    int total_transactions;
} BST;

/* BST API */
BST* bst_create(void);
void bst_free(BST *tree);
void bst_insert(BST *tree, const Transaction *t);
BSTNode* bst_search(const BST *tree, double amount);
double bst_find_min(const BST *tree);
double bst_find_max(const BST *tree);

void bst_in_order(const BST *tree, FILE *json_out);
void bst_range_query(const BST *tree, double min_amount, double max_amount, FILE *json_out);
void bst_to_json(const BST *tree, FILE *out);

#endif /* BST_H */
