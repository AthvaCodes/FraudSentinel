#include "graph.h"

TransactionGraph* graph_create(void) {
    TransactionGraph *g = (TransactionGraph *)malloc(sizeof(TransactionGraph));
    if (!g) return NULL;
    g->head = NULL;
    g->vertex_count = 0;
    g->edge_count = 0;
    return g;
}

void graph_free(TransactionGraph *g) {
    if (!g) return;
    GraphVertex *v = g->head;
    while (v) {
        EdgeNode *e = v->edges;
        while (e) {
            EdgeNode *temp_e = e;
            e = e->next;
            free(temp_e);
        }
        GraphVertex *temp_v = v;
        v = v->next;
        free(temp_v);
    }
    free(g);
}

GraphVertex* graph_find_or_add_vertex(TransactionGraph *g, const char *account) {
    if (!g || !account) return NULL;

    GraphVertex *curr = g->head;
    while (curr) {
        if (strcmp(curr->account, account) == 0) {
            return curr;
        }
        curr = curr->next;
    }

    // Add new vertex
    GraphVertex *new_v = (GraphVertex *)malloc(sizeof(GraphVertex));
    if (!new_v) return NULL;

    safe_strcpy(new_v->account, account, sizeof(new_v->account));
    new_v->total_sent = 0.0;
    new_v->total_received = 0.0;
    new_v->in_degree = 0;
    new_v->out_degree = 0;
    new_v->risk_score = 0;
    new_v->is_flagged = 0;
    new_v->edges = NULL;

    new_v->next = g->head;
    g->head = new_v;
    g->vertex_count++;
    return new_v;
}

void graph_add_edge(TransactionGraph *g, const char *sender, const char *receiver, double amount, int tx_id, const char *timestamp) {
    if (!g || !sender || !receiver) return;

    GraphVertex *v_sender = graph_find_or_add_vertex(g, sender);
    GraphVertex *v_receiver = graph_find_or_add_vertex(g, receiver);

    EdgeNode *e = (EdgeNode *)malloc(sizeof(EdgeNode));
    if (!e) return;

    safe_strcpy(e->target, receiver, sizeof(e->target));
    e->amount = amount;
    e->transaction_id = tx_id;
    safe_strcpy(e->timestamp, timestamp ? timestamp : "", sizeof(e->timestamp));

    e->next = v_sender->edges;
    v_sender->edges = e;

    v_sender->out_degree++;
    v_sender->total_sent += amount;

    v_receiver->in_degree++;
    v_receiver->total_received += amount;

    g->edge_count++;
}

void graph_build_from_transactions(TransactionGraph *g, const Transaction *transactions, int count) {
    for (int i = 0; i < count; i++) {
        const Transaction *t = &transactions[i];
        graph_add_edge(g, t->sender, t->receiver, t->amount, t->id, t->timestamp);

        GraphVertex *vs = graph_find_or_add_vertex(g, t->sender);
        if (t->risk_score > vs->risk_score) vs->risk_score = t->risk_score;
        if (t->is_flagged) vs->is_flagged = 1;

        GraphVertex *vr = graph_find_or_add_vertex(g, t->receiver);
        if (t->is_flagged && strcmp(t->risk_level, "HIGH") == 0) vr->is_flagged = 1;
    }
}

void graph_bfs(const TransactionGraph *g, const char *start_account, FILE *json_out) {
    if (!g || !start_account) {
        fputs("[]", json_out);
        return;
    }

    char visited[MAX_TRANSACTIONS][MAX_NAME_LEN];
    int visited_count = 0;

    char queue[MAX_TRANSACTIONS][MAX_NAME_LEN];
    int distances[MAX_TRANSACTIONS];
    char parents[MAX_TRANSACTIONS][MAX_NAME_LEN];
    int head = 0, tail = 0;

    // Enqueue start
    safe_strcpy(queue[tail], start_account, MAX_NAME_LEN);
    distances[tail] = 0;
    safe_strcpy(parents[tail], "START", MAX_NAME_LEN);
    tail++;

    safe_strcpy(visited[visited_count++], start_account, MAX_NAME_LEN);

    fputs("[", json_out);
    int step_idx = 0;

    while (head < tail) {
        char current[MAX_NAME_LEN];
        safe_strcpy(current, queue[head], MAX_NAME_LEN);
        int dist = distances[head];
        char parent[MAX_NAME_LEN];
        safe_strcpy(parent, parents[head], MAX_NAME_LEN);
        head++;

        if (step_idx > 0) fputc(',', json_out);
        fprintf(json_out, "{\"step\":%d,\"account\":", step_idx);
        print_json_escaped_string(json_out, current);
        fprintf(json_out, ",\"distance\":%d,\"parent\":", dist);
        print_json_escaped_string(json_out, parent);
        fputs("}", json_out);
        step_idx++;

        // Find vertex in graph
        GraphVertex *v = g->head;
        while (v && strcmp(v->account, current) != 0) v = v->next;

        if (v) {
            EdgeNode *e = v->edges;
            while (e) {
                int already_visited = 0;
                for (int i = 0; i < visited_count; i++) {
                    if (strcmp(visited[i], e->target) == 0) {
                        already_visited = 1;
                        break;
                    }
                }

                if (!already_visited && visited_count < MAX_TRANSACTIONS && tail < MAX_TRANSACTIONS) {
                    safe_strcpy(visited[visited_count++], e->target, MAX_NAME_LEN);
                    safe_strcpy(queue[tail], e->target, MAX_NAME_LEN);
                    distances[tail] = dist + 1;
                    safe_strcpy(parents[tail], current, MAX_NAME_LEN);
                    tail++;
                }
                e = e->next;
            }
        }
    }
    fputs("]", json_out);
}

void graph_dfs(const TransactionGraph *g, const char *start_account, FILE *json_out) {
    if (!g || !start_account) {
        fputs("[]", json_out);
        return;
    }

    char visited[MAX_TRANSACTIONS][MAX_NAME_LEN];
    int visited_count = 0;
    int step_idx = 0;

    fputs("[", json_out);

    void dfs_visit(const char *curr, int depth, const char *parent) {
        safe_strcpy(visited[visited_count++], curr, MAX_NAME_LEN);

        if (step_idx > 0) fputc(',', json_out);
        fprintf(json_out, "{\"step\":%d,\"account\":", step_idx++);
        print_json_escaped_string(json_out, curr);
        fprintf(json_out, ",\"depth\":%d,\"parent\":", depth);
        print_json_escaped_string(json_out, parent);
        fputs("}", json_out);

        GraphVertex *v = g->head;
        while (v && strcmp(v->account, curr) != 0) v = v->next;

        if (v) {
            EdgeNode *e = v->edges;
            while (e) {
                int already = 0;
                for (int i = 0; i < visited_count; i++) {
                    if (strcmp(visited[i], e->target) == 0) {
                        already = 1;
                        break;
                    }
                }
                if (!already && visited_count < MAX_TRANSACTIONS) {
                    dfs_visit(e->target, depth + 1, curr);
                }
                e = e->next;
            }
        }
    }

    dfs_visit(start_account, 0, "START");
    fputs("]", json_out);
}

int graph_shortest_path(const TransactionGraph *g, const char *start, const char *end, FILE *json_out) {
    if (!g || !start || !end) {
        fputs("{\"found\":false,\"path\":[]}", json_out);
        return 0;
    }

    if (strcmp(start, end) == 0) {
        fputs("{\"found\":true,\"hops\":0,\"path\":[", json_out);
        print_json_escaped_string(json_out, start);
        fputs("]}", json_out);
        return 1;
    }

    char queue[MAX_TRANSACTIONS][MAX_NAME_LEN];
    int parent_idx[MAX_TRANSACTIONS];
    int head = 0, tail = 0;

    safe_strcpy(queue[tail], start, MAX_NAME_LEN);
    parent_idx[tail] = -1;
    tail++;

    char visited[MAX_TRANSACTIONS][MAX_NAME_LEN];
    int visited_count = 0;
    safe_strcpy(visited[visited_count++], start, MAX_NAME_LEN);

    int target_idx = -1;

    while (head < tail) {
        char current[MAX_NAME_LEN];
        safe_strcpy(current, queue[head], MAX_NAME_LEN);

        if (strcmp(current, end) == 0) {
            target_idx = head;
            break;
        }

        GraphVertex *v = g->head;
        while (v && strcmp(v->account, current) != 0) v = v->next;

        if (v) {
            EdgeNode *e = v->edges;
            while (e) {
                int seen = 0;
                for (int i = 0; i < visited_count; i++) {
                    if (strcmp(visited[i], e->target) == 0) {
                        seen = 1;
                        break;
                    }
                }
                if (!seen && visited_count < MAX_TRANSACTIONS && tail < MAX_TRANSACTIONS) {
                    safe_strcpy(visited[visited_count++], e->target, MAX_NAME_LEN);
                    safe_strcpy(queue[tail], e->target, MAX_NAME_LEN);
                    parent_idx[tail] = head;
                    tail++;
                }
                e = e->next;
            }
        }
        head++;
    }

    if (target_idx == -1) {
        fputs("{\"found\":false,\"path\":[]}", json_out);
        return 0;
    }

    // Reconstruct path
    char path[MAX_PATH_NODES][MAX_NAME_LEN];
    int path_len = 0;
    int curr = target_idx;
    while (curr != -1 && path_len < MAX_PATH_NODES) {
        safe_strcpy(path[path_len++], queue[curr], MAX_NAME_LEN);
        curr = parent_idx[curr];
    }

    fputs("{\"found\":true,\"hops\":", json_out);
    fprintf(json_out, "%d,\"path\":[", path_len - 1);
    for (int i = path_len - 1; i >= 0; i--) {
        if (i < path_len - 1) fputc(',', json_out);
        print_json_escaped_string(json_out, path[i]);
    }
    fputs("]}", json_out);
    return 1;
}

void graph_detect_cycles(const TransactionGraph *g, FILE *json_out) {
    if (!g) {
        fputs("[]", json_out);
        return;
    }

    fputs("[", json_out);
    int cycle_count = 0;

    char path[16][MAX_NAME_LEN];
    double amounts[16];

    void find_cycles_from(const char *start_node, const char *curr_node, int depth) {
        if (depth > 6) return;

        GraphVertex *v = g->head;
        while (v && strcmp(v->account, curr_node) != 0) v = v->next;
        if (!v) return;

        EdgeNode *e = v->edges;
        while (e) {
            if (strcmp(e->target, start_node) == 0 && depth >= 2) {
                amounts[depth] = e->amount;
                double total_vol = 0.0;
                for (int i = 1; i <= depth; i++) total_vol += amounts[i];

                if (cycle_count > 0) fputc(',', json_out);
                fputs("{\"length\":", json_out);
                fprintf(json_out, "%d,\"total_volume\":%.2f,\"path\":[", depth + 1, total_vol);
                for (int i = 0; i < depth; i++) {
                    print_json_escaped_string(json_out, path[i]);
                    fputc(',', json_out);
                }
                print_json_escaped_string(json_out, start_node);
                fputs("],\"risk\":\"HIGH_MONEY_LAUNDERING_RING\"}", json_out);
                cycle_count++;
            } else {
                int already_in_path = 0;
                for (int i = 0; i < depth; i++) {
                    if (strcmp(path[i], e->target) == 0) {
                        already_in_path = 1;
                        break;
                    }
                }
                if (!already_in_path && depth < 6) {
                    safe_strcpy(path[depth], e->target, MAX_NAME_LEN);
                    amounts[depth] = e->amount;
                    find_cycles_from(start_node, e->target, depth + 1);
                }
            }
            e = e->next;
        }
    }

    GraphVertex *v = g->head;
    while (v) {
        safe_strcpy(path[0], v->account, MAX_NAME_LEN);
        find_cycles_from(v->account, v->account, 1);
        v = v->next;
    }

    fputs("]", json_out);
}

void graph_to_json(const TransactionGraph *g, FILE *out) {
    if (!g) {
        fputs("{\"nodes\":[],\"edges\":[]}", out);
        return;
    }

    fputs("{\"nodes\":[", out);
    GraphVertex *v = g->head;
    int first_v = 1;
    while (v) {
        if (!first_v) fputc(',', out);
        first_v = 0;

        fputs("{", out);
        fputs("\"id\":", out); print_json_escaped_string(out, v->account); fputc(',', out);
        fputs("\"label\":", out); print_json_escaped_string(out, v->account); fputc(',', out);
        fprintf(out, "\"total_sent\":%.2f,", v->total_sent);
        fprintf(out, "\"total_received\":%.2f,", v->total_received);
        fprintf(out, "\"in_degree\":%d,", v->in_degree);
        fprintf(out, "\"out_degree\":%d,", v->out_degree);
        fprintf(out, "\"risk_score\":%d,", v->risk_score);
        fprintf(out, "\"is_flagged\":%s", v->is_flagged ? "true" : "false");
        fputs("}", out);

        v = v->next;
    }
    fputs("],\"edges\":[", out);

    v = g->head;
    int first_e = 1;
    while (v) {
        EdgeNode *e = v->edges;
        while (e) {
            if (!first_e) fputc(',', out);
            first_e = 0;

            fputs("{", out);
            fputs("\"source\":", out); print_json_escaped_string(out, v->account); fputc(',', out);
            fputs("\"target\":", out); print_json_escaped_string(out, e->target); fputc(',', out);
            fprintf(out, "\"amount\":%.2f,", e->amount);
            fprintf(out, "\"transaction_id\":%d,", e->transaction_id);
            fputs("\"timestamp\":", out); print_json_escaped_string(out, e->timestamp);
            fputs("}", out);

            e = e->next;
        }
        v = v->next;
    }
    fputs("]}", out);
}
