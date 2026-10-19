#include "algorithms.h"

void bubble_sort_transactions_trace(Transaction *arr, int n, FILE *json_out) {
    if (!arr || n <= 0) {
        fputs("{\"algorithm\":\"Bubble Sort\",\"comparisons\":0,\"swaps\":0,\"steps\":[],\"sorted\":[]}", json_out);
        return;
    }

    // Make local copy so original array is unaffected if just simulating
    Transaction *copy = (Transaction *)malloc(sizeof(Transaction) * n);
    if (!copy) return;
    memcpy(copy, arr, sizeof(Transaction) * n);

    long comparisons = 0;
    long swaps = 0;

    fputs("{\"algorithm\":\"Bubble Sort\","
          "\"time_complexity\":\"O(n^2)\","
          "\"space_complexity\":\"O(1)\","
          "\"steps\":[", json_out);

    int step_idx = 0;
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            comparisons++;
            int did_swap = 0;
            if (copy[j].amount > copy[j + 1].amount) {
                Transaction temp = copy[j];
                copy[j] = copy[j + 1];
                copy[j + 1] = temp;
                swaps++;
                did_swap = 1;
            }

            // Record first 50 steps to keep JSON compact and responsive
            if (step_idx < 50) {
                if (step_idx > 0) fputc(',', json_out);
                fprintf(json_out, "{\"step\":%d,\"pass\":%d,\"comparing_indices\":[%d,%d],\"values\":[%.2f,%.2f],\"swapped\":%s}",
                        step_idx, i, j, j + 1, copy[j].amount, copy[j + 1].amount, did_swap ? "true" : "false");
                step_idx++;
            }
        }
    }

    fputs("],", json_out);
    fprintf(json_out, "\"total_comparisons\":%ld,\"total_swaps\":%ld,", comparisons, swaps);

    fputs("\"sorted\":[", json_out);
    for (int i = 0; i < n; i++) {
        if (i > 0) fputc(',', json_out);
        print_transaction_json(json_out, &copy[i]);
    }
    fputs("]}", json_out);

    free(copy);
}

static void merge(Transaction *arr, int l, int m, int r, long *comparisons) {
    int n1 = m - l + 1;
    int n2 = r - m;

    Transaction *L = (Transaction *)malloc(sizeof(Transaction) * n1);
    Transaction *R = (Transaction *)malloc(sizeof(Transaction) * n2);
    if (!L || !R) return;

    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        (*comparisons)++;
        if (L[i].amount <= R[j].amount) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}

static void merge_sort_helper(Transaction *arr, int l, int r, long *comparisons) {
    if (l < r) {
        int m = l + (r - l) / 2;
        merge_sort_helper(arr, l, m, comparisons);
        merge_sort_helper(arr, m + 1, r, comparisons);
        merge(arr, l, m, r, comparisons);
    }
}

void merge_sort_transactions(Transaction *arr, int n, long *comparison_count) {
    *comparison_count = 0;
    if (n > 1) {
        merge_sort_helper(arr, 0, n - 1, comparison_count);
    }
}

void linear_search_transactions_trace(const Transaction *arr, int n, double target_amount, FILE *json_out) {
    fputs("{\"algorithm\":\"Linear Search\","
          "\"time_complexity\":\"O(n)\","
          "\"target_amount\":", json_out);
    fprintf(json_out, "%.2f,\"steps\":[", target_amount);

    int found_idx = -1;
    int comparisons = 0;

    for (int i = 0; i < n; i++) {
        comparisons++;
        if (i > 0) fputc(',', json_out);
        fprintf(json_out, "{\"step\":%d,\"index\":%d,\"current_amount\":%.2f,\"match\":%s}",
                i, i, arr[i].amount, (fabs(arr[i].amount - target_amount) < 0.001) ? "true" : "false");

        if (fabs(arr[i].amount - target_amount) < 0.001) {
            found_idx = i;
            break;
        }
    }

    fputs("],", json_out);
    fprintf(json_out, "\"found\":%s,\"found_index\":%d,\"total_comparisons\":%d}",
            (found_idx != -1) ? "true" : "false", found_idx, comparisons);
}

void binary_search_transactions_trace(const Transaction *arr, int n, double target_amount, FILE *json_out) {
    fputs("{\"algorithm\":\"Binary Search\","
          "\"time_complexity\":\"O(log n)\","
          "\"target_amount\":", json_out);
    fprintf(json_out, "%.2f,\"steps\":[", target_amount);

    // Array must be sorted
    Transaction *sorted = (Transaction *)malloc(sizeof(Transaction) * n);
    if (!sorted) return;
    memcpy(sorted, arr, sizeof(Transaction) * n);
    long dummy = 0;
    merge_sort_transactions(sorted, n, &dummy);

    int low = 0;
    int high = n - 1;
    int step = 0;
    int found_idx = -1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (step > 0) fputc(',', json_out);
        fprintf(json_out, "{\"step\":%d,\"low\":%d,\"mid\":%d,\"high\":%d,\"mid_amount\":%.2f,\"match\":%s}",
                step, low, mid, high, sorted[mid].amount,
                (fabs(sorted[mid].amount - target_amount) < 0.001) ? "true" : "false");
        step++;

        if (fabs(sorted[mid].amount - target_amount) < 0.001) {
            found_idx = mid;
            break;
        } else if (sorted[mid].amount < target_amount) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    fputs("],", json_out);
    fprintf(json_out, "\"found\":%s,\"found_index\":%d,\"total_comparisons\":%d}",
            (found_idx != -1) ? "true" : "false", found_idx, step);

    free(sorted);
}
