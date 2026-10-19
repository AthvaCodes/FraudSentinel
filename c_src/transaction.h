#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "common.h"

typedef struct {
    int id;
    char sender[MAX_NAME_LEN];
    char receiver[MAX_NAME_LEN];
    double amount;
    char timestamp[32];
    char category[MAX_CATEGORY_LEN];
    int risk_score;
    char risk_level[MAX_STATUS_LEN]; // "LOW", "MEDIUM", "HIGH"
    int is_flagged;
    char flag_reason[MAX_REASON_LEN];
    char review_status[MAX_STATUS_LEN]; // "PENDING", "INVESTIGATING", "CLEARED", "BLOCKED"
    char analyst_notes[MAX_NOTES_LEN];
} Transaction;

typedef struct {
    int total_transactions;
    double total_amount;
    double average_amount;
    double largest_amount;
    double smallest_amount;
    int flagged_count;
    int high_risk_count;
    int medium_risk_count;
    int low_risk_count;
    int pending_reviews;
    int cleared_reviews;
    int blocked_reviews;
} TransactionStatistics;

/* Global transaction storage */
extern Transaction g_transactions[MAX_TRANSACTIONS];
extern int g_transaction_count;

/* Functions */
void init_transactions(void);
int add_transaction_record(const Transaction *t);
int update_transaction_record(int id, const Transaction *updated);
int delete_transaction_record(int id);
Transaction* find_transaction_by_id(int id);
int get_sender_transaction_count(const char *sender);
TransactionStatistics calculate_statistics(void);

void save_transactions_to_file(const char *filepath);
int load_transactions_from_file(const char *filepath);
void seed_sample_transactions(void);

void print_transaction_json(FILE *out, const Transaction *t);
void print_all_transactions_json(FILE *out);
void print_statistics_json(FILE *out, const TransactionStatistics *stats);

#endif /* TRANSACTION_H */
