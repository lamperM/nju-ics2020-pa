#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  
  char *watch_expr;
  uint32_t watch_value;

  /* TODO: Add more members if necessary */

} WP;

WP* new_wp(void);
void free_wp(WP *wp); 
WP* no_to_wp(int no); 
void watchpoint_display(void); 
#endif
