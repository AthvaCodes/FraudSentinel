#include "bst.h"

static BSTNode* create_node(const Transaction *t) {
    BSTNode *node = (BSTNode *)malloc(sizeof(BSTNode));
    if (!node) return NULL;

    node->amount_key = t->amount;
    node->tx_capacity = 4;
    node->tx_count = 1;
    node->transactions = (Transaction *)malloc(sizeof(Transaction) * node->tx_capacity);
    if (node->transactions) {
        node->transactions[0] = *t;
    }
    node->left = NULL;
    node->right = NULL;
    return node;
}

static void free_subtree(BSTNode *node) {
    if (!node) return;
    free_subtree(node->left);
    free_subtree(node->right);
    if (node->transactions) free(node->transactions);
    free(node);
}

BST* bst_create(void) {
    BST *tree = (BST *)malloc(sizeof(BST));
    if (!tree) return NULL;
    tree->root = NULL;
    tree->total_nodes = 0;
    tree->total_transactions = 0;
    return tree;
}

void bst_free(BST *tree) {
    if (!tree) return;
    free_subtree(tree->root);
    free(tree);
}

void bst_insert(BST *tree, const Transaction *t) {
    if (!tree || !t) return;

    if (tree->root == NULL) {
        tree->root = create_node(t);
        tree->total_nodes = 1;
        tree->total_transactions = 1;
        return;
    }

    BSTNode *curr = tree->root;
    while (1) {
        if (fabs(t->amount - curr->amount_key) < 0.001) {
            // Amount match: add to transaction array in this node
            if (curr->tx_count >= curr->tx_capacity) {
                curr->tx_capacity *= 2;
                curr->transactions = (Transaction *)realloc(curr->transactions, sizeof(Transaction) * curr->tx_capacity);
            }
            if (curr->transactions) {
                curr->transactions[curr->tx_count++] = *t;
            }
            tree->total_transactions++;
            return;
        } else if (t->amount < curr->amount_key) {
            if (curr->left == NULL) {
                curr->left = create_node(t);
                tree->total_nodes++;
                tree->total_transactions++;
                return;
            }
            curr = curr->left;
        } else {
            if (curr->right == NULL) {
                curr->right = create_node(t);
                tree->total_nodes++;
                tree->total_transactions++;
                return;
            }
            curr = curr->right;
        }
    }
}

BSTNode* bst_search(const BST *tree, double amount) {
    if (!tree) return NULL;
    BSTNode *curr = tree->root;
    while (curr) {
        if (fabs(amount - curr->amount_key) < 0.001) {
            return curr;
        } else if (amount < curr->amount_key) {
            curr = curr->left;
        } else {
            curr = curr->right;
        }
    }
    return NULL;
}

double bst_find_min(const BST *tree) {
    if (!tree || !tree->root) return 0.0;
    BSTNode *curr = tree->root;
    while (curr->left) curr = curr->left;
    return curr->amount_key;
}

double bst_find_max(const BST *tree) {
    if (!tree || !tree->root) return 0.0;
    BSTNode *curr = tree->root;
    while (curr->right) curr = curr->right;
    return curr->amount_key;
}

static void in_order_util(BSTNode *node, FILE *out, int *first) {
    if (!node) return;
    in_order_util(node->left, out, first);

    for (int i = 0; i < node->tx_count; i++) {
        if (!(*first)) fputc(',', out);
        *first = 0;
        print_transaction_json(out, &node->transactions[i]);
    }

    in_order_util(node->right, out, first);
}

void bst_in_order(const BST *tree, FILE *json_out) {
    if (!tree || !tree->root) {
        fputs("[]", json_out);
        return;
    }
    fputs("[", json_out);
    int first = 1;
    in_order_util(tree->root, json_out, &first);
    fputs("]", json_out);
}

static void range_query_util(BSTNode *node, double min_amount, double max_amount, FILE *out, int *first) {
    if (!node) return;

    if (min_amount <= node->amount_key) {
        range_query_util(node->left, min_amount, max_amount, out, first);
    }

    if (node->amount_key >= min_amount && node->amount_key <= max_amount) {
        for (int i = 0; i < node->tx_count; i++) {
            if (!(*first)) fputc(',', out);
            *first = 0;
            print_transaction_json(out, &node->transactions[i]);
        }
    }

    if (node->amount_key <= max_amount) {
        range_query_util(node->right, min_amount, max_amount, out, first);
    }
}

void bst_range_query(const BST *tree, double min_amount, double max_amount, FILE *json_out) {
    if (!tree || !tree->root) {
        fputs("[]", json_out);
        return;
    }
    fputs("[", json_out);
    int first = 1;
    range_query_util(tree->root, min_amount, max_amount, json_out, &first);
    fputs("]", json_out);
}

static void print_node_json(BSTNode *node, FILE *out) {
    if (!node) {
        fputs("null", out);
        return;
    }
    fputs("{", out);
    fprintf(out, "\"amount\":%.2f,", node->amount_key);
    fprintf(out, "\"count\":%d,", node->tx_count);

    fputs("\"transactions\":[", out);
    for (int i = 0; i < node->tx_count; i++) {
        if (i > 0) fputc(',', out);
        print_transaction_json(out, &node->transactions[i]);
    }
    fputs("],", out);

    fputs("\"left\":", out);
    print_node_json(node->left, out);
    fputc(',', out);

    fputs("\"right\":", out);
    print_node_json(node->right, out);

    fputs("}", out);
}

void bst_to_json(const BST *tree, FILE *out) {
    if (!tree || !tree->root) {
        fputs("{\"total_nodes\":0,\"total_transactions\":0,\"root\":null}", out);
        return;
    }
    fputs("{", out);
    fprintf(out, "\"total_nodes\":%d,", tree->total_nodes);
    fprintf(out, "\"total_transactions\":%d,", tree->total_transactions);
    fprintf(out, "\"min_amount\":%.2f,", bst_find_min(tree));
    fprintf(out, "\"max_amount\":%.2f,", bst_find_max(tree));
    fputs("\"root\":", out);
    print_node_json(tree->root, out);
    fputs("}", out);
}
