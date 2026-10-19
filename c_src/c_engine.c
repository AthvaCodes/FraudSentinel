#include "common.h"
#include "transaction.h"
#include "fraud_rules.h"
#include "linked_list.h"
#include "graph.h"
#include "queue.h"
#include "stack.h"
#include "bst.h"
#include "hash_map.h"
#include "algorithms.h"

/* Forward declaration of interactive CLI menu */
void run_interactive_console_menu(void);

static void ensure_data_loaded(void) {
    init_default_fraud_rules();
    load_rules_config(CONFIG_FILE_JSON);

    if (g_transaction_count == 0) {
        if (load_transactions_from_file(DATA_FILE_TXT) == 0) {
            seed_sample_transactions();
            save_transactions_to_file(DATA_FILE_TXT);
        }
    }

    if (!g_audit_stack) {
        g_audit_stack = stack_create();
    }
}

int main(int argc, char *argv[]) {
    ensure_data_loaded();

    if (argc < 2 || strcmp(argv[1], "--cli") == 0 || strcmp(argv[1], "-i") == 0) {
        run_interactive_console_menu();
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0 || strcmp(cmd, "seed") == 0) {
        seed_sample_transactions();
        save_transactions_to_file(DATA_FILE_TXT);
        fputs("{\"status\":\"success\",\"message\":\"Sample banking data seeded successfully\",\"count\":", stdout);
        fprintf(stdout, "%d}\n", g_transaction_count);
        return 0;
    }

    if (strcmp(cmd, "list") == 0) {
        print_all_transactions_json(stdout);
        fputc('\n', stdout);
        return 0;
    }

    if (strcmp(cmd, "stats") == 0) {
        TransactionStatistics stats = calculate_statistics();
        print_statistics_json(stdout, &stats);
        fputc('\n', stdout);
        return 0;
    }

    if (strcmp(cmd, "add") == 0) {
        if (argc < 5) {
            fputs("{\"status\":\"error\",\"message\":\"Usage: add <sender> <receiver> <amount> [category] [notes]\"}\n", stdout);
            return 1;
        }
        Transaction t;
        memset(&t, 0, sizeof(t));
        safe_strcpy(t.sender, argv[2], sizeof(t.sender));
        safe_strcpy(t.receiver, argv[3], sizeof(t.receiver));
        t.amount = atof(argv[4]);
        if (argc >= 6) safe_strcpy(t.category, argv[5], sizeof(t.category));
        if (argc >= 7) safe_strcpy(t.analyst_notes, argv[6], sizeof(t.analyst_notes));

        int new_id = add_transaction_record(&t);
        save_transactions_to_file(DATA_FILE_TXT);

        Transaction *created = find_transaction_by_id(new_id);
        fputs("{\"status\":\"success\",\"transaction\":", stdout);
        print_transaction_json(stdout, created);
        fputs("}\n", stdout);
        return 0;
    }

    if (strcmp(cmd, "update") == 0) {
        if (argc < 4) {
            fputs("{\"status\":\"error\",\"message\":\"Usage: update <id> <status> [notes]\"}\n", stdout);
            return 1;
        }
        int id = atoi(argv[2]);
        Transaction *existing = find_transaction_by_id(id);
        if (!existing) {
            fputs("{\"status\":\"error\",\"message\":\"Transaction ID not found\"}\n", stdout);
            return 1;
        }

        char old_status[MAX_STATUS_LEN];
        safe_strcpy(old_status, existing->review_status, sizeof(old_status));

        safe_strcpy(existing->review_status, argv[3], sizeof(existing->review_status));
        if (argc >= 5) {
            safe_strcpy(existing->analyst_notes, argv[4], sizeof(existing->analyst_notes));
        }

        // Push to audit stack
        stack_push(g_audit_stack, "STATUS_CHANGE", id, old_status, existing->review_status, existing->analyst_notes);

        save_transactions_to_file(DATA_FILE_TXT);

        fputs("{\"status\":\"success\",\"transaction\":", stdout);
        print_transaction_json(stdout, existing);
        fputs("}\n", stdout);
        return 0;
    }

    if (strcmp(cmd, "delete") == 0) {
        if (argc < 3) {
            fputs("{\"status\":\"error\",\"message\":\"Usage: delete <id>\"}\n", stdout);
            return 1;
        }
        int id = atoi(argv[2]);
        int res = delete_transaction_record(id);
        if (res) {
            save_transactions_to_file(DATA_FILE_TXT);
            fprintf(stdout, "{\"status\":\"success\",\"message\":\"Transaction %d deleted\"}\n", id);
        } else {
            fputs("{\"status\":\"error\",\"message\":\"Transaction not found\"}\n", stdout);
        }
        return 0;
    }

    if (strcmp(cmd, "undo") == 0) {
        StackNode popped;
        if (stack_pop(g_audit_stack, &popped)) {
            Transaction *t = find_transaction_by_id(popped.transaction_id);
            if (t) {
                safe_strcpy(t->review_status, popped.old_status, sizeof(t->review_status));
                save_transactions_to_file(DATA_FILE_TXT);
            }
            fputs("{\"status\":\"success\",\"message\":\"Reverted action\",\"action\":{", stdout);
            fprintf(stdout, "\"transaction_id\":%d,", popped.transaction_id);
            fputs("\"reverted_to\":", stdout); print_json_escaped_string(stdout, popped.old_status);
            fputs("}}\n", stdout);
        } else {
            fputs("{\"status\":\"error\",\"message\":\"Audit stack is empty. Nothing to undo.\"}\n", stdout);
        }
        return 0;
    }

    if (strcmp(cmd, "config_get") == 0) {
        print_rules_config_json(stdout);
        fputc('\n', stdout);
        return 0;
    }

    if (strcmp(cmd, "config_set") == 0) {
        if (argc < 9) {
            fputs("{\"status\":\"error\",\"message\":\"Usage: config_set <lg_amt> <hi_act> <lg_wt> <hi_wt> <cycle_wt> <hi_cut> <med_cut>\"}\n", stdout);
            return 1;
        }
        g_fraud_config.large_amount_threshold = atof(argv[2]);
        g_fraud_config.high_activity_threshold = atoi(argv[3]);
        g_fraud_config.large_amount_weight = atoi(argv[4]);
        g_fraud_config.high_activity_weight = atoi(argv[5]);
        g_fraud_config.cycle_weight = atoi(argv[6]);
        g_fraud_config.high_risk_threshold = atoi(argv[7]);
        g_fraud_config.medium_risk_threshold = atoi(argv[8]);

        save_rules_config(CONFIG_FILE_JSON);
        reevaluate_all_transactions();
        save_transactions_to_file(DATA_FILE_TXT);

        fputs("{\"status\":\"success\",\"message\":\"Rules updated and transactions re-evaluated\",\"config\":", stdout);
        print_rules_config_json(stdout);
        fputs("}\n", stdout);
        return 0;
    }

    /* DATA STRUCTURES LAB ENDPOINTS */

    if (strcmp(cmd, "graph") == 0) {
        TransactionGraph *g = graph_create();
        graph_build_from_transactions(g, g_transactions, g_transaction_count);
        graph_to_json(g, stdout);
        fputc('\n', stdout);
        graph_free(g);
        return 0;
    }

    if (strcmp(cmd, "graph_bfs") == 0) {
        const char *start = (argc >= 3) ? argv[2] : (g_transaction_count > 0 ? g_transactions[0].sender : "Aarav Sharma");
        TransactionGraph *g = graph_create();
        graph_build_from_transactions(g, g_transactions, g_transaction_count);
        graph_bfs(g, start, stdout);
        fputc('\n', stdout);
        graph_free(g);
        return 0;
    }

    if (strcmp(cmd, "graph_dfs") == 0) {
        const char *start = (argc >= 3) ? argv[2] : (g_transaction_count > 0 ? g_transactions[0].sender : "Aarav Sharma");
        TransactionGraph *g = graph_create();
        graph_build_from_transactions(g, g_transactions, g_transaction_count);
        graph_dfs(g, start, stdout);
        fputc('\n', stdout);
        graph_free(g);
        return 0;
    }

    if (strcmp(cmd, "graph_path") == 0) {
        if (argc < 4) {
            fputs("{\"status\":\"error\",\"message\":\"Usage: graph_path <start> <end>\"}\n", stdout);
            return 1;
        }
        TransactionGraph *g = graph_create();
        graph_build_from_transactions(g, g_transactions, g_transaction_count);
        graph_shortest_path(g, argv[2], argv[3], stdout);
        fputc('\n', stdout);
        graph_free(g);
        return 0;
    }

    if (strcmp(cmd, "graph_cycles") == 0) {
        TransactionGraph *g = graph_create();
        graph_build_from_transactions(g, g_transactions, g_transaction_count);
        graph_detect_cycles(g, stdout);
        fputc('\n', stdout);
        graph_free(g);
        return 0;
    }

    if (strcmp(cmd, "linked_list") == 0) {
        print_flagged_list_visual_json(stdout);
        fputc('\n', stdout);
        return 0;
    }

    if (strcmp(cmd, "queue") == 0) {
        FIFOQueue *q = queue_create();
        // Enqueue pending/flagged transactions into review queue
        for (int i = 0; i < g_transaction_count; i++) {
            if (g_transactions[i].is_flagged || strcmp(g_transactions[i].review_status, "PENDING") == 0) {
                queue_enqueue(q, &g_transactions[i]);
            }
        }
        queue_to_json(q, stdout);
        fputc('\n', stdout);
        queue_free(q);
        return 0;
    }

    if (strcmp(cmd, "stack") == 0) {
        stack_to_json(g_audit_stack, stdout);
        fputc('\n', stdout);
        return 0;
    }

    if (strcmp(cmd, "bst") == 0) {
        BST *tree = bst_create();
        for (int i = 0; i < g_transaction_count; i++) {
            bst_insert(tree, &g_transactions[i]);
        }
        bst_to_json(tree, stdout);
        fputc('\n', stdout);
        bst_free(tree);
        return 0;
    }

    if (strcmp(cmd, "bst_in_order") == 0) {
        BST *tree = bst_create();
        for (int i = 0; i < g_transaction_count; i++) {
            bst_insert(tree, &g_transactions[i]);
        }
        bst_in_order(tree, stdout);
        fputc('\n', stdout);
        bst_free(tree);
        return 0;
    }

    if (strcmp(cmd, "bst_range") == 0) {
        double min_val = (argc >= 3) ? atof(argv[2]) : 5000.0;
        double max_val = (argc >= 4) ? atof(argv[3]) : 20000.0;
        BST *tree = bst_create();
        for (int i = 0; i < g_transaction_count; i++) {
            bst_insert(tree, &g_transactions[i]);
        }
        bst_range_query(tree, min_val, max_val, stdout);
        fputc('\n', stdout);
        bst_free(tree);
        return 0;
    }

    if (strcmp(cmd, "hash_map") == 0) {
        HashMap *hm = hash_map_create();
        for (int i = 0; i < g_transaction_count; i++) {
            hash_map_put(hm, g_transactions[i].sender, g_transactions[i].amount, g_transactions[i].is_flagged);
            hash_map_put(hm, g_transactions[i].receiver, g_transactions[i].amount, g_transactions[i].is_flagged);
        }
        hash_map_to_json(hm, stdout);
        fputc('\n', stdout);
        hash_map_free(hm);
        return 0;
    }

    if (strcmp(cmd, "bubble_sort") == 0) {
        bubble_sort_transactions_trace(g_transactions, g_transaction_count, stdout);
        fputc('\n', stdout);
        return 0;
    }

    if (strcmp(cmd, "linear_search") == 0) {
        double target = (argc >= 3) ? atof(argv[2]) : 10000.0;
        linear_search_transactions_trace(g_transactions, g_transaction_count, target, stdout);
        fputc('\n', stdout);
        return 0;
    }

    if (strcmp(cmd, "binary_search") == 0) {
        double target = (argc >= 3) ? atof(argv[2]) : 10000.0;
        binary_search_transactions_trace(g_transactions, g_transaction_count, target, stdout);
        fputc('\n', stdout);
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    return 1;
}

/* ==========================================================
   INTERACTIVE CONSOLE MENU (Evolving original C project)
   ========================================================== */

void run_interactive_console_menu(void) {
    int choice = 0;
    printf("\n");
    printf("============================================================\n");
    printf("       BANK FRAUD TRANSACTION SYSTEM (C CORE ENGINE)\n");
    printf("           [Educational Prototype Demonstration]\n");
    printf("============================================================\n");

    do {
        printf("\n============== MENU ==============\n");
        printf("1.  Add Transaction\n");
        printf("2.  Display All Transactions\n");
        printf("3.  Search Transaction by ID\n");
        printf("4.  Search Account Transactions\n");
        printf("5.  Sort Transactions by Amount (Bubble Sort)\n");
        printf("6.  Detect Large Transactions (>= Rs. %.2f)\n", g_fraud_config.large_amount_threshold);
        printf("7.  Detect High Activity Accounts (>= %d Outgoing)\n", g_fraud_config.high_activity_threshold);
        printf("8.  Calculate Fraud Risk Scores\n");
        printf("9.  Display Flagged Transactions (Linked List)\n");
        printf("10. Transaction Statistics\n");
        printf("11. Complete Fraud Detection Report\n");
        printf("12. Save Transactions to File\n");
        printf("13. Re-seed Sample Dataset\n");
        printf("14. Data Structures Lab (C In-Memory Engines)\n");
        printf("15. Exit\n");
        printf("==================================\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // clear buffer
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        switch (choice) {
            case 1: {
                Transaction t;
                memset(&t, 0, sizeof(t));
                printf("\nEnter sender name: ");
                scanf("%63s", t.sender);
                printf("Enter receiver name: ");
                scanf("%63s", t.receiver);
                printf("Enter amount: ");
                scanf("%lf", &t.amount);
                if (t.amount <= 0) {
                    printf("\nAmount must be greater than zero.\n");
                    break;
                }
                int new_id = add_transaction_record(&t);
                save_transactions_to_file(DATA_FILE_TXT);
                printf("\nTransaction #%d created successfully!\n", new_id);
                if (t.amount >= g_fraud_config.large_amount_threshold) {
                    printf("WARNING: Large transaction detected! Flagged in linked list.\n");
                }
                break;
            }
            case 2: {
                printf("\n========================================================================================\n");
                printf("%-5s %-20s %-20s %-12s %-8s %-8s\n", "ID", "Sender", "Receiver", "Amount (Rs.)", "Risk", "Flagged");
                printf("========================================================================================\n");
                for (int i = 0; i < g_transaction_count; i++) {
                    Transaction *t = &g_transactions[i];
                    printf("%-5d %-20s %-20s Rs. %-8.2f %-8s %-8s\n",
                           t->id, t->sender, t->receiver, t->amount, t->risk_level, t->is_flagged ? "YES" : "NO");
                }
                printf("========================================================================================\n");
                break;
            }
            case 3: {
                int id;
                printf("\nEnter transaction ID: ");
                scanf("%d", &id);
                Transaction *t = find_transaction_by_id(id);
                if (t) {
                    printf("\n--- Transaction Found ---\n");
                    printf("ID         : %d\n", t->id);
                    printf("Sender     : %s\n", t->sender);
                    printf("Receiver   : %s\n", t->receiver);
                    printf("Amount     : Rs. %.2f\n", t->amount);
                    printf("Risk Level : %s (Score: %d)\n", t->risk_level, t->risk_score);
                    printf("Flagged    : %s (Reason: %s)\n", t->is_flagged ? "YES" : "NO", t->flag_reason);
                    printf("Status     : %s\n", t->review_status);
                } else {
                    printf("\nTransaction #%d not found.\n", id);
                }
                break;
            }
            case 4: {
                char account[MAX_NAME_LEN];
                printf("\nEnter account name: ");
                scanf("%63s", account);
                printf("\nTransactions involving %s:\n", account);
                printf("-------------------------------------------------------------------------\n");
                int found = 0;
                for (int i = 0; i < g_transaction_count; i++) {
                    Transaction *t = &g_transactions[i];
                    if (strcmp(t->sender, account) == 0 || strcmp(t->receiver, account) == 0) {
                        printf("ID: %-4d | %s -> %s | Rs. %-8.2f | Status: %s\n",
                               t->id, t->sender, t->receiver, t->amount, t->review_status);
                        found = 1;
                    }
                }
                if (!found) printf("No transactions found for account '%s'.\n", account);
                break;
            }
            case 5: {
                bubble_sort_transactions_trace(g_transactions, g_transaction_count, stdout);
                printf("\n\n(Bubble sort demonstration complete)\n");
                break;
            }
            case 6: {
                printf("\n========== LARGE TRANSACTIONS (>= Rs. %.2f) ==========\n", g_fraud_config.large_amount_threshold);
                int found = 0;
                for (int i = 0; i < g_transaction_count; i++) {
                    if (g_transactions[i].amount >= g_fraud_config.large_amount_threshold) {
                        printf("ID: %-4d | %s -> %s | Rs. %.2f\n",
                               g_transactions[i].id, g_transactions[i].sender, g_transactions[i].receiver, g_transactions[i].amount);
                        found = 1;
                    }
                }
                if (!found) printf("No large transactions found.\n");
                break;
            }
            case 7: {
                printf("\n========== HIGH ACTIVITY ACCOUNTS (>= %d Outgoing) ==========\n", g_fraud_config.high_activity_threshold);
                // Use hash map to count activity
                HashMap *hm = hash_map_create();
                for (int i = 0; i < g_transaction_count; i++) {
                    hash_map_put(hm, g_transactions[i].sender, g_transactions[i].amount, 0);
                }
                for (int b = 0; b < hm->num_buckets; b++) {
                    HashNode *curr = hm->buckets[b];
                    while (curr) {
                        if (curr->tx_count >= g_fraud_config.high_activity_threshold) {
                            printf("Account: %-25s | Outgoing Tx Count: %d | Total: Rs. %.2f\n",
                                   curr->key, curr->tx_count, curr->total_amount);
                        }
                        curr = curr->next;
                    }
                }
                hash_map_free(hm);
                break;
            }
            case 8: {
                printf("\n========== FRAUD RISK SCORES ==========\n");
                for (int i = 0; i < g_transaction_count; i++) {
                    Transaction *t = &g_transactions[i];
                    printf("ID: %-4d | Sender: %-20s | Amount: Rs. %-8.2f | Score: %d | Level: %s\n",
                           t->id, t->sender, t->amount, t->risk_score, t->risk_level);
                }
                break;
            }
            case 9: {
                display_flagged_transactions_console();
                break;
            }
            case 10: {
                TransactionStatistics stats = calculate_statistics();
                printf("\n========== TRANSACTION STATISTICS ==========\n");
                printf("Total Transactions : %d\n", stats.total_transactions);
                printf("Total Volume       : Rs. %.2f\n", stats.total_amount);
                printf("Average Amount     : Rs. %.2f\n", stats.average_amount);
                printf("Largest Transfer   : Rs. %.2f\n", stats.largest_amount);
                printf("Smallest Transfer  : Rs. %.2f\n", stats.smallest_amount);
                printf("Flagged Violations : %d\n", stats.flagged_count);
                printf("High Risk Count    : %d\n", stats.high_risk_count);
                printf("Medium Risk Count  : %d\n", stats.medium_risk_count);
                printf("Low Risk Count     : %d\n", stats.low_risk_count);
                break;
            }
            case 11: {
                printf("\n============================================\n");
                printf("         COMPLETE FRAUD REPORT\n");
                printf("============================================\n");
                TransactionStatistics stats = calculate_statistics();
                printf("Summary: %d transactions evaluated, %d flagged for review.\n",
                       stats.total_transactions, stats.flagged_count);
                display_flagged_transactions_console();
                break;
            }
            case 12: {
                save_transactions_to_file(DATA_FILE_TXT);
                printf("\nTransactions successfully saved to %s!\n", DATA_FILE_TXT);
                break;
            }
            case 13: {
                seed_sample_transactions();
                save_transactions_to_file(DATA_FILE_TXT);
                printf("\nSample transactions re-seeded successfully (%d records)!\n", g_transaction_count);
                break;
            }
            case 14: {
                printf("\n--- Data Structures Lab Quick Inspection ---\n");
                printf("1. Flagged Linked List Nodes: %d (Head: 0x%p)\n", get_flagged_count(), (void*)flaggedHead);
                TransactionGraph *g = graph_create();
                graph_build_from_transactions(g, g_transactions, g_transaction_count);
                printf("2. Transfer Network Graph: %d Vertices, %d Edges\n", g->vertex_count, g->edge_count);
                graph_free(g);
                printf("3. LIFO Audit Stack: %d actions logged (Total pushed: %d)\n",
                       stack_size(g_audit_stack), g_audit_stack ? g_audit_stack->total_pushed : 0);
                break;
            }
            case 15:
                save_transactions_to_file(DATA_FILE_TXT);
                printf("\nThank you for using the Bank Fraud Transaction System!\n");
                break;
            default:
                printf("\nInvalid choice. Please select 1-15.\n");
                break;
        }

    } while (choice != 15);
}
