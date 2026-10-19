#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "common.h"
#include "transaction.h"

/* Sorting Algorithms */
void bubble_sort_transactions_trace(Transaction *arr, int n, FILE *json_out);
void merge_sort_transactions(Transaction *arr, int n, long *comparison_count);

/* Searching Algorithms */
void linear_search_transactions_trace(const Transaction *arr, int n, double target_amount, FILE *json_out);
void binary_search_transactions_trace(const Transaction *arr, int n, double target_amount, FILE *json_out);

#endif /* ALGORITHMS_H */
