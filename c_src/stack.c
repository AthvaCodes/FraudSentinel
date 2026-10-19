#include "stack.h"

AuditStack *g_audit_stack = NULL;

AuditStack* stack_create(void) {
    AuditStack *s = (AuditStack *)malloc(sizeof(AuditStack));
    if (!s) return NULL;
    s->top = NULL;
    s->size = 0;
    s->total_pushed = 0;
    s->total_popped = 0;
    return s;
}

void stack_free(AuditStack *s) {
    if (!s) return;
    stack_clear(s);
    free(s);
}

int stack_push(AuditStack *s, const char *action_type, int tx_id, const char *old_status, const char *new_status, const char *details) {
    if (!s) return 0;

    StackNode *node = (StackNode *)malloc(sizeof(StackNode));
    if (!node) return 0;

    safe_strcpy(node->action_type, action_type, sizeof(node->action_type));
    node->transaction_id = tx_id;
    safe_strcpy(node->old_status, old_status ? old_status : "", sizeof(node->old_status));
    safe_strcpy(node->new_status, new_status ? new_status : "", sizeof(node->new_status));
    safe_strcpy(node->details, details ? details : "", sizeof(node->details));

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(node->timestamp, sizeof(node->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Push onto head (top of stack)
    node->next = s->top;
    s->top = node;

    s->size++;
    s->total_pushed++;
    return 1;
}

int stack_pop(AuditStack *s, StackNode *out_node) {
    if (!s || s->top == NULL) return 0;

    StackNode *temp = s->top;
    if (out_node) *out_node = *temp;

    s->top = s->top->next;
    free(temp);

    s->size--;
    s->total_popped++;
    return 1;
}

int stack_peek(const AuditStack *s, StackNode *out_node) {
    if (!s || s->top == NULL) return 0;
    if (out_node) *out_node = *(s->top);
    return 1;
}

int stack_is_empty(const AuditStack *s) {
    return (!s || s->top == NULL);
}

int stack_size(const AuditStack *s) {
    return s ? s->size : 0;
}

void stack_clear(AuditStack *s) {
    if (!s) return;
    StackNode dummy;
    while (stack_pop(s, &dummy));
}

void stack_to_json(const AuditStack *s, FILE *out) {
    if (!s) {
        fputs("{\"size\":0,\"items\":[]}", out);
        return;
    }

    fputs("{", out);
    fprintf(out, "\"size\":%d,", s->size);
    fprintf(out, "\"total_pushed\":%d,", s->total_pushed);
    fprintf(out, "\"total_popped\":%d,", s->total_popped);

    fputs("\"top\":", out);
    if (s->top) {
        fputs("{", out);
        fputs("\"action_type\":", out); print_json_escaped_string(out, s->top->action_type); fputc(',', out);
        fprintf(out, "\"transaction_id\":%d,", s->top->transaction_id);
        fputs("\"old_status\":", out); print_json_escaped_string(out, s->top->old_status); fputc(',', out);
        fputs("\"new_status\":", out); print_json_escaped_string(out, s->top->new_status); fputc(',', out);
        fputs("\"timestamp\":", out); print_json_escaped_string(out, s->top->timestamp); fputc(',', out);
        fputs("\"details\":", out); print_json_escaped_string(out, s->top->details);
        fputs("}", out);
    } else {
        fputs("null", out);
    }
    fputc(',', out);

    fputs("\"items\":[", out);
    StackNode *curr = s->top;
    int idx = 0;
    while (curr) {
        if (idx > 0) fputc(',', out);
        fputs("{", out);
        fprintf(out, "\"index_from_top\":%d,", idx);
        fprintf(out, "\"is_top\":%s,", (idx == 0) ? "true" : "false");
        fputs("\"action_type\":", out); print_json_escaped_string(out, curr->action_type); fputc(',', out);
        fprintf(out, "\"transaction_id\":%d,", curr->transaction_id);
        fputs("\"old_status\":", out); print_json_escaped_string(out, curr->old_status); fputc(',', out);
        fputs("\"new_status\":", out); print_json_escaped_string(out, curr->new_status); fputc(',', out);
        fputs("\"timestamp\":", out); print_json_escaped_string(out, curr->timestamp); fputc(',', out);
        fputs("\"details\":", out); print_json_escaped_string(out, curr->details);
        fputs("}", out);
        curr = curr->next;
        idx++;
    }
    fputs("]}", out);
}
