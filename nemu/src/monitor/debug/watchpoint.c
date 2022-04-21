#include <stdlib.h>
#include "watchpoint.h"
#include "expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].watch_expr = NULL;
    wp_pool[i].watch_value = 0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(void) {
    WP *p = free_;
    if (NULL == p) {
        printf("No free WP!\n");
        assert(0);
    }
    free_ = free_->next;
    p->next = head;
    head = p;
    return head;
}

void free_wp(WP *wp) {
    assert(wp != NULL && head != NULL);
    WP *pre = head;

    printf("free_wp.NO: %d what: %s\n", wp->NO, wp->watch_expr);
    free(wp->watch_expr);
    wp->watch_value = 0;
    if (head == wp) {
        head = wp->next;
    } else {
        while(pre->next != wp && NULL != pre->next) 
            pre = pre->next;
        if (pre->next != wp) {
            printf("Free WP is not exist. Fix the bug!\n");
            assert(0);
        }
        pre->next = wp->next;
    }
    wp->next = free_;
    free_ = wp;
}
// TODO: test new_wp and free  
WP* no_to_wp(int no) {
    if (no > NR_WP) return NULL;
    for (WP *p = head; p != NULL; p = p->next) 
        if (p->NO == no) return p;

    return NULL;
}

/*
 * List all watchpoint(without order).
 */
void watchpoint_display(void) {
  for(WP *p = head; p != NULL; p = p->next) {
    printf("NO: %d, what:%s, value:%d\n", p->NO, p->watch_expr, p->watch_value);
  }
} 

/*
 * Call it after CPU execute an instruction.
 */
bool check_wp_changed(void) {
    WP *p = head;
    bool changed = false;
    
    for(; p != NULL; p = p->next) {
        char *e = p->watch_expr;
        uint32_t old_v = p->watch_value;
        bool success;
        uint32_t new_v = expr(e, &success);

        if (false == success) assert(0);
        if (new_v != old_v) {
            printf("Watchpoint %d is hitted, new value:%u\n", p->NO, new_v);
            p->watch_value = new_v;
            changed = true;
        }
    }

    return changed;
}
