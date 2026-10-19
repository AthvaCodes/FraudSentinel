#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#define MAX_TRANSACTIONS 500
#define MAX_NAME_LEN 64
#define MAX_REASON_LEN 128
#define MAX_STATUS_LEN 32
#define MAX_NOTES_LEN 256
#define MAX_CATEGORY_LEN 32
#define MAX_PATH_NODES 32

/* Default Rule Constants matching original C code */
#define DEFAULT_LARGE_AMOUNT 10000.0
#define DEFAULT_HIGH_ACTIVITY 5
#define DEFAULT_LARGE_AMOUNT_POINTS 2
#define DEFAULT_HIGH_ACTIVITY_POINTS 2
#define DEFAULT_CYCLE_POINTS 3
#define DEFAULT_HIGH_RISK_THRESHOLD 4
#define DEFAULT_MEDIUM_RISK_THRESHOLD 2

/* File paths */
#define DATA_FILE_TXT "data/transactions.txt"
#define DATA_FILE_JSON "data/transactions.json"
#define CONFIG_FILE_JSON "data/rules_config.json"
#define AUDIT_FILE_JSON "data/audit_log.json"

/* Safe string copy helper */
static inline void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

/* Escape string for JSON */
static inline void print_json_escaped_string(FILE *out, const char *s) {
    if (!s) {
        fputs("\"\"", out);
        return;
    }
    fputc('"', out);
    while (*s) {
        switch (*s) {
            case '"':  fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\b': fputs("\\b", out); break;
            case '\f': fputs("\\f", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if ((unsigned char)*s < 32) {
                    fprintf(out, "\\u%04x", (unsigned char)*s);
                } else {
                    fputc(*s, out);
                }
                break;
        }
        s++;
    }
    fputc('"', out);
}

#endif /* COMMON_H */
