#include "monitor/sdb.h"

#define NR_WP 32 // max num of watchpoint

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* Add more members if necessary */

  word_t value;
  char *expression;

} WP;

/* Advantages of static: 
* 1. Limit the scope to prevent naming comfilcts
* 2. Static storage enable those variables thoughout whole life cycle
*/
static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void wp_pool_init() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* Implement the functionality of watchpoint */

/* Get a watchpoint from pool and string it at the head of the list */
WP* wp_new() {
  Assert(free_ != NULL, "No idle watchpoints in pool");

  WP* wp = free_;
  free_ = free_->next;

  wp -> next = head;
  head = wp;
  
  return wp;
}

/* Find the watchpoint need to be free, and also free its expression member */
void wp_free(WP* wp) {
  Assert(wp != NULL, "The watchpoint to be released cannot be NULL");

  if (head == wp) {
    head = head->next;
  } else {
    WP* prev = head;
    while (prev != NULL && prev->next != wp) {
      prev = prev->next;
    }
    if (prev != NULL) {
      // remove wp
      prev->next = wp->next;
    }
  }

  // Free the expression string
  free(wp->expression);

  wp->next = free_;
  free_ = wp;
}

/* Print info of each watchpoint */
void wp_display() {
  WP* wp = head;
  if (wp == NULL) {
    printf("No watchpoint\n");
    return;
  }

  printf("Num    Value     Expr\n");
  while (wp != NULL) {
    printf("%-6d %-9u %s\n", wp->NO, wp->value, wp->expression);
    wp = wp->next;
  }
}

/* generate a watchpoint and set its expr and val as given value */
void wp_add(char *expr, word_t val){
  WP* wp = wp_new();
  Assert(wp != NULL, "Fail to add watchpoint, no idle watchpoint");

  wp->expression = strdup(expr);
  wp->value = val;

  printf("Watchpoint %d: %s\n", wp->NO, expr);
}

/* delete a watchpoint from list based on the given NO */
void wp_delete(int no) {
  Assert(no < NR_WP, "Invalid watchpoint No");

  WP* wp = head;
  WP* prev = NULL;

  while (wp != NULL) {
    if (wp->NO == no) {
      if (prev == NULL) {
        head = wp->next;
      } else {
        prev->next = wp->next;
      }

      wp_free(wp);
      printf("Watchpoint %d deleted\n", no);
      return;
    }
    prev = wp;
    wp = wp->next;
  }

  printf("Watchpoint %d not found\n", no);
}

/* iterate through the watchpoint list, check and report changes in expression values */
void wp_check() {
  WP* wp = head;
  while (wp != NULL) {
    bool success;
    word_t new_value = expr(wp->expression, &success);
    assert(success);

    if (new_value != wp->value) {
      printf("Watchpoint %d triggered: %s\n", wp->NO, wp->expression);
      printf("Old value = %u\n", wp->value);
      printf("New value = %u\n", new_value);

      wp->value = new_value;
      sim_state.state = SIM_STOP;
    }
    wp = wp->next;
  }
}
