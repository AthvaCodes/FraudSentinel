#include "fraud_rules.h"
#include "transaction.h"
#include "linked_list.h"

FraudRuleConfig g_fraud_config = {
    .large_amount_threshold = DEFAULT_LARGE_AMOUNT,
    .high_activity_threshold = DEFAULT_HIGH_ACTIVITY,
    .large_amount_weight = DEFAULT_LARGE_AMOUNT_POINTS,
    .high_activity_weight = DEFAULT_HIGH_ACTIVITY_POINTS,
    .cycle_weight = DEFAULT_CYCLE_POINTS,
    .high_risk_threshold = DEFAULT_HIGH_RISK_THRESHOLD,
    .medium_risk_threshold = DEFAULT_MEDIUM_RISK_THRESHOLD,
    .enable_large_amount_rule = 1,
    .enable_high_activity_rule = 1,
    .enable_cycle_rule = 1
};

void init_default_fraud_rules(void) {
    g_fraud_config.large_amount_threshold = DEFAULT_LARGE_AMOUNT;
    g_fraud_config.high_activity_threshold = DEFAULT_HIGH_ACTIVITY;
    g_fraud_config.large_amount_weight = DEFAULT_LARGE_AMOUNT_POINTS;
    g_fraud_config.high_activity_weight = DEFAULT_HIGH_ACTIVITY_POINTS;
    g_fraud_config.cycle_weight = DEFAULT_CYCLE_POINTS;
    g_fraud_config.high_risk_threshold = DEFAULT_HIGH_RISK_THRESHOLD;
    g_fraud_config.medium_risk_threshold = DEFAULT_MEDIUM_RISK_THRESHOLD;
    g_fraud_config.enable_large_amount_rule = 1;
    g_fraud_config.enable_high_activity_rule = 1;
    g_fraud_config.enable_cycle_rule = 1;
}

void evaluate_transaction_fraud(void *tx_ptr) {
    Transaction *t = (Transaction *)tx_ptr;
    if (!t) return;

    int score = 0;
    char reasons[MAX_REASON_LEN] = "";
    int is_flagged = 0;

    // Rule 1: Large Transaction (Default: amount >= ₹10,000)
    if (g_fraud_config.enable_large_amount_rule && t->amount >= g_fraud_config.large_amount_threshold) {
        score += g_fraud_config.large_amount_weight;
        safe_strcpy(reasons, "Large transaction (>= threshold)", sizeof(reasons));
        is_flagged = 1;
    }

    // Rule 2: High Activity Account (Default: >= 5 outgoing transactions from sender)
    int activity = get_sender_transaction_count(t->sender);
    if (g_fraud_config.enable_high_activity_rule && activity >= g_fraud_config.high_activity_threshold) {
        score += g_fraud_config.high_activity_weight;
        if (strlen(reasons) > 0) {
            strncat(reasons, "; High activity account", sizeof(reasons) - strlen(reasons) - 1);
        } else {
            safe_strcpy(reasons, "High activity account", sizeof(reasons));
        }
        is_flagged = 1;
    }

    // Determine Risk Level (matching original C code)
    t->risk_score = score;
    t->is_flagged = is_flagged;
    safe_strcpy(t->flag_reason, reasons, sizeof(t->flag_reason));

    if (score >= g_fraud_config.high_risk_threshold) {
        safe_strcpy(t->risk_level, "HIGH", sizeof(t->risk_level));
    } else if (score >= g_fraud_config.medium_risk_threshold) {
        safe_strcpy(t->risk_level, "MEDIUM", sizeof(t->risk_level));
    } else {
        safe_strcpy(t->risk_level, "LOW", sizeof(t->risk_level));
    }
}

void reevaluate_all_transactions(void) {
    clear_flagged_list();
    for (int i = 0; i < g_transaction_count; i++) {
        evaluate_transaction_fraud(&g_transactions[i]);
        if (g_transactions[i].is_flagged) {
            add_flagged_transaction(&g_transactions[i], g_transactions[i].flag_reason);
        }
    }
}

void load_rules_config(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return;

    // Simple line parser
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[64];
        double val;
        if (sscanf(line, "%63[^=]=%lf", key, &val) == 2) {
            if (strcmp(key, "large_amount_threshold") == 0) g_fraud_config.large_amount_threshold = val;
            else if (strcmp(key, "high_activity_threshold") == 0) g_fraud_config.high_activity_threshold = (int)val;
            else if (strcmp(key, "large_amount_weight") == 0) g_fraud_config.large_amount_weight = (int)val;
            else if (strcmp(key, "high_activity_weight") == 0) g_fraud_config.high_activity_weight = (int)val;
            else if (strcmp(key, "cycle_weight") == 0) g_fraud_config.cycle_weight = (int)val;
            else if (strcmp(key, "high_risk_threshold") == 0) g_fraud_config.high_risk_threshold = (int)val;
            else if (strcmp(key, "medium_risk_threshold") == 0) g_fraud_config.medium_risk_threshold = (int)val;
            else if (strcmp(key, "enable_large_amount_rule") == 0) g_fraud_config.enable_large_amount_rule = (int)val;
            else if (strcmp(key, "enable_high_activity_rule") == 0) g_fraud_config.enable_high_activity_rule = (int)val;
            else if (strcmp(key, "enable_cycle_rule") == 0) g_fraud_config.enable_cycle_rule = (int)val;
        }
    }
    fclose(f);
}

void save_rules_config(const char *filepath) {
    FILE *f = fopen(filepath, "w");
    if (!f) return;

    fprintf(f, "large_amount_threshold=%.2f\n", g_fraud_config.large_amount_threshold);
    fprintf(f, "high_activity_threshold=%d\n", g_fraud_config.high_activity_threshold);
    fprintf(f, "large_amount_weight=%d\n", g_fraud_config.large_amount_weight);
    fprintf(f, "high_activity_weight=%d\n", g_fraud_config.high_activity_weight);
    fprintf(f, "cycle_weight=%d\n", g_fraud_config.cycle_weight);
    fprintf(f, "high_risk_threshold=%d\n", g_fraud_config.high_risk_threshold);
    fprintf(f, "medium_risk_threshold=%d\n", g_fraud_config.medium_risk_threshold);
    fprintf(f, "enable_large_amount_rule=%d\n", g_fraud_config.enable_large_amount_rule);
    fprintf(f, "enable_high_activity_rule=%d\n", g_fraud_config.enable_high_activity_rule);
    fprintf(f, "enable_cycle_rule=%d\n", g_fraud_config.enable_cycle_rule);
    fclose(f);
}

void print_rules_config_json(FILE *out) {
    fprintf(out, "{"
        "\"large_amount_threshold\":%.2f,"
        "\"high_activity_threshold\":%d,"
        "\"large_amount_weight\":%d,"
        "\"high_activity_weight\":%d,"
        "\"cycle_weight\":%d,"
        "\"high_risk_threshold\":%d,"
        "\"medium_risk_threshold\":%d,"
        "\"enable_large_amount_rule\":%s,"
        "\"enable_high_activity_rule\":%s,"
        "\"enable_cycle_rule\":%s"
        "}",
        g_fraud_config.large_amount_threshold,
        g_fraud_config.high_activity_threshold,
        g_fraud_config.large_amount_weight,
        g_fraud_config.high_activity_weight,
        g_fraud_config.cycle_weight,
        g_fraud_config.high_risk_threshold,
        g_fraud_config.medium_risk_threshold,
        g_fraud_config.enable_large_amount_rule ? "true" : "false",
        g_fraud_config.enable_high_activity_rule ? "true" : "false",
        g_fraud_config.enable_cycle_rule ? "true" : "false");
}
