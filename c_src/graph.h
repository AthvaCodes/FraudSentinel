#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"
#include "transaction.h"

typedef struct EdgeNode {
    char target[MAX_NAME_LEN];
    double amount;
    int transaction_id;
    char timestamp[32];
    struct EdgeNode *next;
} EdgeNode;

typedef struct GraphVertex {
    char account[MAX_NAME_LEN];
    double total_sent;
    double total_received;
    int in_degree;
    int out_degree;
    int risk_score;
    int is_flagged;
    EdgeNode *edges; // Adjacency list of outgoing edges
    struct GraphVertex *next;
} GraphVertex;

typedef struct {
    GraphVertex *head;
    int vertex_count;
    int edge_count;
} TransactionGraph;

/* Graph API */
TransactionGraph* graph_create(void);
void graph_free(TransactionGraph *g);
GraphVertex* graph_find_or_add_vertex(TransactionGraph *g, const char *account);
void graph_add_edge(TransactionGraph *g, const char *sender, const char *receiver, double amount, int tx_id, const char *timestamp);
void graph_build_from_transactions(TransactionGraph *g, const Transaction *transactions, int count);

/* Graph Algorithms */
void graph_bfs(const TransactionGraph *g, const char *start_account, FILE *json_out);
void graph_dfs(const TransactionGraph *g, const char *start_account, FILE *json_out);
int graph_shortest_path(const TransactionGraph *g, const char *start, const char *end, FILE *json_out);
void graph_detect_cycles(const TransactionGraph *g, FILE *json_out);

/* Visual JSON Serialization */
void graph_to_json(const TransactionGraph *g, FILE *out);

#endif /* GRAPH_H */
