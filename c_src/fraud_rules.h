#ifndef FRAUD_RULES_H
#define FRAUD_RULES_H

#include "common.h"

typedef struct {
    double large_amount_threshold;
    int high_activity_threshold;
    int large_amount_weight;
    int high_activity_weight;
    int cycle_weight;
    int high_risk_threshold;
    int medium_risk_threshold;
    int enable_large_amount_rule;
    int enable_high_activity_rule;
    int enable_cycle_rule;
} FraudRuleConfig;

extern FraudRuleConfig g_fraud_config;

/* Rule Engine functions */
void init_default_fraud_rules(void);
void evaluate_transaction_fraud(void *tx_ptr);
void reevaluate_all_transactions(void);

void load_rules_config(const char *filepath);
void save_rules_config(const char *filepath);
void print_rules_config_json(FILE *out);

#endif /* FRAUD_RULES_H */
