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
    if (NULL == head)
        p->next = NULL;
    else
        p->next = head->next;
    head = p;
    free_ = free_->next;
    return head;
}

void free_wp(WP *wp) {
    assert(wp != NULL && head != NULL);
    WP *pre = head;

    while(pre->next != wp && NULL != pre->next) 
        pre = pre->next;
    if (pre->next != wp) {
        printf("Free WP is not exist. Fix the bug!\n");
        assert(0);
    }
    pre->next = wp->next;
    if (NULL == free_)
        wp->next = NULL;
    else 
        wp->next = free_->next;
    free_ = wp;
}
// TODO: test new_wp and free  
