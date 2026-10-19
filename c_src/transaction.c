#include "transaction.h"
#include "fraud_rules.h"
#include "linked_list.h"

Transaction g_transactions[MAX_TRANSACTIONS];
int g_transaction_count = 0;

void init_transactions(void) {
    g_transaction_count = 0;
    memset(g_transactions, 0, sizeof(g_transactions));
}

int add_transaction_record(const Transaction *t) {
    if (g_transaction_count >= MAX_TRANSACTIONS) {
        return -1;
    }

    Transaction record = *t;
    if (record.id <= 0) {
        int max_id = 0;
        for (int i = 0; i < g_transaction_count; i++) {
            if (g_transactions[i].id > max_id) max_id = g_transactions[i].id;
        }
        record.id = max_id + 1;
    }

    if (strlen(record.timestamp) == 0) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(record.timestamp, sizeof(record.timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    if (strlen(record.review_status) == 0) {
        safe_strcpy(record.review_status, "PENDING", sizeof(record.review_status));
    }

    if (strlen(record.category) == 0) {
        safe_strcpy(record.category, "Transfer", sizeof(record.category));
    }

    // Evaluate fraud rules and risk score
    evaluate_transaction_fraud(&record);

    g_transactions[g_transaction_count] = record;
    g_transaction_count++;

    // If flagged, automatically prepend to flagged linked list
    if (record.is_flagged) {
        add_flagged_transaction(&record, record.flag_reason);
    }

    return record.id;
}

int update_transaction_record(int id, const Transaction *updated) {
    for (int i = 0; i < g_transaction_count; i++) {
        if (g_transactions[i].id == id) {
            Transaction *t = &g_transactions[i];
            if (strlen(updated->sender) > 0) safe_strcpy(t->sender, updated->sender, sizeof(t->sender));
            if (strlen(updated->receiver) > 0) safe_strcpy(t->receiver, updated->receiver, sizeof(t->receiver));
            if (updated->amount > 0) t->amount = updated->amount;
            if (strlen(updated->category) > 0) safe_strcpy(t->category, updated->category, sizeof(t->category));
            if (strlen(updated->review_status) > 0) safe_strcpy(t->review_status, updated->review_status, sizeof(t->review_status));
            if (strlen(updated->analyst_notes) > 0) safe_strcpy(t->analyst_notes, updated->analyst_notes, sizeof(t->analyst_notes));

            evaluate_transaction_fraud(t);
            return 1;
        }
    }
    return 0;
}

int delete_transaction_record(int id) {
    int found_idx = -1;
    for (int i = 0; i < g_transaction_count; i++) {
        if (g_transactions[i].id == id) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) return 0;

    // Shift elements left
    for (int i = found_idx; i < g_transaction_count - 1; i++) {
        g_transactions[i] = g_transactions[i + 1];
    }
    g_transaction_count--;
    delete_flagged_transaction(id);
    return 1;
}

Transaction* find_transaction_by_id(int id) {
    for (int i = 0; i < g_transaction_count; i++) {
        if (g_transactions[i].id == id) {
            return &g_transactions[i];
        }
    }
    return NULL;
}

int get_sender_transaction_count(const char *sender) {
    int count = 0;
    for (int i = 0; i < g_transaction_count; i++) {
        if (strcmp(g_transactions[i].sender, sender) == 0) {
            count++;
        }
    }
    return count;
}

TransactionStatistics calculate_statistics(void) {
    TransactionStatistics stats;
    memset(&stats, 0, sizeof(stats));

    stats.total_transactions = g_transaction_count;
    if (g_transaction_count == 0) return stats;

    stats.largest_amount = g_transactions[0].amount;
    stats.smallest_amount = g_transactions[0].amount;

    for (int i = 0; i < g_transaction_count; i++) {
        const Transaction *t = &g_transactions[i];
        stats.total_amount += t->amount;

        if (t->amount > stats.largest_amount) stats.largest_amount = t->amount;
        if (t->amount < stats.smallest_amount) stats.smallest_amount = t->amount;

        if (t->is_flagged) stats.flagged_count++;

        if (strcmp(t->risk_level, "HIGH") == 0) stats.high_risk_count++;
        else if (strcmp(t->risk_level, "MEDIUM") == 0) stats.medium_risk_count++;
        else stats.low_risk_count++;

        if (strcmp(t->review_status, "PENDING") == 0) stats.pending_reviews++;
        else if (strcmp(t->review_status, "CLEARED") == 0) stats.cleared_reviews++;
        else if (strcmp(t->review_status, "BLOCKED") == 0) stats.blocked_reviews++;
    }

    stats.average_amount = stats.total_amount / g_transaction_count;
    return stats;
}

void save_transactions_to_file(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    if (!f) return;

    for (int i = 0; i < g_transaction_count; i++) {
        const Transaction *t = &g_transactions[i];
        fprintf(f, "%d|%s|%s|%.2f|%s|%s|%d|%s|%d|%s|%s|%s\n",
                t->id, t->sender, t->receiver, t->amount,
                t->timestamp, t->category, t->risk_score, t->risk_level,
                t->is_flagged, t->flag_reason, t->review_status, t->analyst_notes);
    }
    fclose(f);
}

int load_transactions_from_file(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    init_transactions();
    clear_flagged_list();

    char line[1024];
    while (fgets(line, sizeof(line), f) && g_transaction_count < MAX_TRANSACTIONS) {
        // Remove trailing newline
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        Transaction t;
        memset(&t, 0, sizeof(t));

        // Format: id|sender|receiver|amount|timestamp|category|risk_score|risk_level|is_flagged|flag_reason|review_status|analyst_notes
        char *token = strtok(line, "|");
        if (token) t.id = atoi(token);

        token = strtok(NULL, "|");
        if (token) safe_strcpy(t.sender, token, sizeof(t.sender));

        token = strtok(NULL, "|");
        if (token) safe_strcpy(t.receiver, token, sizeof(t.receiver));

        token = strtok(NULL, "|");
        if (token) t.amount = atof(token);

        token = strtok(NULL, "|");
        if (token) safe_strcpy(t.timestamp, token, sizeof(t.timestamp));

        token = strtok(NULL, "|");
        if (token) safe_strcpy(t.category, token, sizeof(t.category));

        token = strtok(NULL, "|");
        if (token) t.risk_score = atoi(token);

        token = strtok(NULL, "|");
        if (token) safe_strcpy(t.risk_level, token, sizeof(t.risk_level));

        token = strtok(NULL, "|");
        if (token) t.is_flagged = atoi(token);

        token = strtok(NULL, "|");
        if (token) safe_strcpy(t.flag_reason, token, sizeof(t.flag_reason));

        token = strtok(NULL, "|");
        if (token) safe_strcpy(t.review_status, token, sizeof(t.review_status));

        token = strtok(NULL, "|");
        if (token) safe_strcpy(t.analyst_notes, token, sizeof(t.analyst_notes));

        g_transactions[g_transaction_count] = t;
        g_transaction_count++;

        if (t.is_flagged) {
            add_flagged_transaction(&t, t.flag_reason);
        }
    }
    fclose(f);
    return g_transaction_count;
}

void seed_sample_transactions(void) {
    init_transactions();
    clear_flagged_list();

    struct SampleData {
        const char *sender;
        const char *receiver;
        double amount;
        const char *timestamp;
        const char *category;
        const char *status;
        const char *notes;
    } samples[] = {
        // Normal day-to-day transactions
        {"Aarav Sharma", "Priya Verma", 1500.00, "2026-10-18 09:15:00", "Dining", "CLEARED", "Verified dining split"},
        {"Ananya Iyer", "Kavita Rao", 3200.00, "2026-10-18 10:20:00", "Shopping", "CLEARED", "Regular e-commerce purchase"},
        {"Rohan Gupta", "Deepak Nair", 750.50, "2026-10-18 11:05:00", "Groceries", "CLEARED", "Supermarket payment"},
        {"Neha Reddy", "Pooja Joshi", 4800.00, "2026-10-18 11:45:00", "Utilities", "CLEARED", "Electricity bill settlement"},
        {"Siddharth Menon", "Amit Patel", 2200.00, "2026-10-18 13:10:00", "Services", "CLEARED", "Consulting fee"},

        // Rule 1 Trigger: Large Transactions (>= ₹10,000)
        {"Rajesh Singhania", "Nexus Holdings", 75000.00, "2026-10-18 14:00:00", "Business", "INVESTIGATING", "Large wire transfer awaiting invoice validation"},
        {"Vikram Malhotra", "Meera Kapoor", 25000.00, "2026-10-18 15:30:00", "Investment", "PENDING", "High value personal loan repayment"},
        {"Suresh Oberoi", "Sun Enterprises", 125000.00, "2026-10-18 16:15:00", "Real Estate", "BLOCKED", "Suspicious unverified overseas beneficiary"},
        {"Manish Agarwal", "Royal Jewelers", 14500.00, "2026-10-18 17:00:00", "Luxury Goods", "PENDING", "Gold purchase exceeding alert threshold"},

        // Rule 2 Trigger: High Activity Account (5+ outgoing transfers from same sender)
        {"Karan Desai", "Vendor Alpha", 850.00, "2026-10-19 08:00:00", "Vendor Pay", "PENDING", ""},
        {"Karan Desai", "Vendor Beta", 920.00, "2026-10-19 08:10:00", "Vendor Pay", "PENDING", ""},
        {"Karan Desai", "Vendor Gamma", 1100.00, "2026-10-19 08:25:00", "Vendor Pay", "PENDING", ""},
        {"Karan Desai", "Vendor Delta", 650.00, "2026-10-19 08:40:00", "Vendor Pay", "PENDING", ""},
        {"Karan Desai", "Vendor Epsilon", 1400.00, "2026-10-19 09:00:00", "Vendor Pay", "INVESTIGATING", "Rapid burst of payments across multiple accounts"},
        {"Karan Desai", "Vendor Zeta", 1800.00, "2026-10-19 09:15:00", "Vendor Pay", "INVESTIGATING", "6th outgoing transfer in under two hours"},

        // Dual Trigger: Both Large (>= ₹10,000) AND High Activity Sender (Risk Score 4+ -> HIGH)
        {"Karan Desai", "Offshore Vault", 18500.00, "2026-10-19 09:30:00", "Wire", "BLOCKED", "Both high activity burst and large amount violation"},

        // Money Laundering / Circular Smurfing Ring (A -> B -> C -> A)
        {"Mule Account A", "Mule Account B", 9800.00, "2026-10-19 10:00:00", "Transfer", "INVESTIGATING", "Suspected layer 1 of circular transfer cycle"},
        {"Mule Account B", "Mule Account C", 9650.00, "2026-10-19 10:30:00", "Transfer", "INVESTIGATING", "Suspected layer 2 of circular transfer cycle"},
        {"Mule Account C", "Mule Account A", 9500.00, "2026-10-19 11:00:00", "Transfer", "BLOCKED", "Circular ring completed back to origin node"},

        // Fan-Out Smurfing Mule (One source distributing to multiple micro receivers)
        {"Syndicate Hub", "Mule Account X", 4900.00, "2026-10-19 11:30:00", "Disbursement", "INVESTIGATING", "Sub-threshold structured distribution"},
        {"Syndicate Hub", "Mule Account Y", 4950.00, "2026-10-19 11:35:00", "Disbursement", "INVESTIGATING", "Sub-threshold structured distribution"},
        {"Syndicate Hub", "Mule Account Z", 4800.00, "2026-10-19 11:40:00", "Disbursement", "INVESTIGATING", "Sub-threshold structured distribution"}
    };

    int n_samples = sizeof(samples) / sizeof(samples[0]);
    for (int i = 0; i < n_samples; i++) {
        Transaction t;
        memset(&t, 0, sizeof(t));
        t.id = i + 1;
        safe_strcpy(t.sender, samples[i].sender, sizeof(t.sender));
        safe_strcpy(t.receiver, samples[i].receiver, sizeof(t.receiver));
        t.amount = samples[i].amount;
        safe_strcpy(t.timestamp, samples[i].timestamp, sizeof(t.timestamp));
        safe_strcpy(t.category, samples[i].category, sizeof(t.category));
        safe_strcpy(t.review_status, samples[i].status, sizeof(t.review_status));
        safe_strcpy(t.analyst_notes, samples[i].notes, sizeof(t.analyst_notes));

        evaluate_transaction_fraud(&t);
        g_transactions[g_transaction_count++] = t;

        if (t.is_flagged) {
            add_flagged_transaction(&t, t.flag_reason);
        }
    }
}

void print_transaction_json(FILE *out, const Transaction *t) {
    if (!t) {
        fputs("null", out);
        return;
    }
    fputs("{", out);
    fprintf(out, "\"id\":%d,", t->id);
    fputs("\"sender\":", out); print_json_escaped_string(out, t->sender); fputc(',', out);
    fputs("\"receiver\":", out); print_json_escaped_string(out, t->receiver); fputc(',', out);
    fprintf(out, "\"amount\":%.2f,", t->amount);
    fputs("\"timestamp\":", out); print_json_escaped_string(out, t->timestamp); fputc(',', out);
    fputs("\"category\":", out); print_json_escaped_string(out, t->category); fputc(',', out);
    fprintf(out, "\"risk_score\":%d,", t->risk_score);
    fputs("\"risk_level\":", out); print_json_escaped_string(out, t->risk_level); fputc(',', out);
    fprintf(out, "\"is_flagged\":%s,", t->is_flagged ? "true" : "false");
    fputs("\"flag_reason\":", out); print_json_escaped_string(out, t->flag_reason); fputc(',', out);
    fputs("\"review_status\":", out); print_json_escaped_string(out, t->review_status); fputc(',', out);
    fputs("\"analyst_notes\":", out); print_json_escaped_string(out, t->analyst_notes);
    fputs("}", out);
}

void print_all_transactions_json(FILE *out) {
    fputs("[", out);
    for (int i = 0; i < g_transaction_count; i++) {
        if (i > 0) fputc(',', out);
        print_transaction_json(out, &g_transactions[i]);
    }
    fputs("]", out);
}

void print_statistics_json(FILE *out, const TransactionStatistics *stats) {
    fputs("{", out);
    fprintf(out, "\"total_transactions\":%d,", stats->total_transactions);
    fprintf(out, "\"total_amount\":%.2f,", stats->total_amount);
    fprintf(out, "\"average_amount\":%.2f,", stats->average_amount);
    fprintf(out, "\"largest_amount\":%.2f,", stats->largest_amount);
    fprintf(out, "\"smallest_amount\":%.2f,", stats->smallest_amount);
    fprintf(out, "\"flagged_count\":%d,", stats->flagged_count);
    fprintf(out, "\"high_risk_count\":%d,", stats->high_risk_count);
    fprintf(out, "\"medium_risk_count\":%d,", stats->medium_risk_count);
    fprintf(out, "\"low_risk_count\":%d,", stats->low_risk_count);
    fprintf(out, "\"pending_reviews\":%d,", stats->pending_reviews);
    fprintf(out, "\"cleared_reviews\":%d,", stats->cleared_reviews);
    fprintf(out, "\"blocked_reviews\":%d", stats->blocked_reviews);
    fputs("}", out);
}
