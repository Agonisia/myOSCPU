/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* Add more members if necessary */

  word_t value;
  char *expression;

} WP;

  /* Advantages: 
  * 1. Limit the scope to prevent naming comfilcts
  * 2. Static storage enable those variables thoughout whole life cycle
  */
static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* Implement the functionality of watchpoint */

WP* new_wp() {
  assert(free_ != NULL);

  WP* wp = free_;
  free_ = free_->next;

  wp -> next = head;
  head = wp;
  
  return wp;
}


void free_wp(WP* wp) {
  assert(free_ != NULL);

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

  wp->next = free_;
  free_ = wp;
}

void info_wp() {
  WP* wp = head;
  if (wp == NULL) {
    printf("No watchpoint\n");
    return;
  }

  printf("Num    Value     Expr\n");
  while (wp != NULL) {
    printf("%-3d %-12u %s\n", wp->NO, wp->value, wp->expression);
    wp = wp->next;
  }
}

void add_wp(char *expr, word_t val){
  WP* wp = new_wp();
  assert(wp != NULL);

  wp->expression = strdup(expr);
  wp->value = val;

  printf("Watchpoint %d: %s\n", wp->NO, expr);
}

void delete_wp(int no) {
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

      free_wp(wp);
      printf("Watchpoint %d deleted\n", no);
      return;
    }
    prev = wp;
    wp = wp->next;
  }

  printf("Watchpoint %d not found\n", no);
}

void check_wp() {
  WP* wp = head;
  while (wp != NULL) {
    bool success;
    word_t new_value = expr(wp->expression, &success);
    assert(success);
    // printf("%u\n", new_value);

    if (new_value != wp->value) {
      printf("Watchpoint %d triggered: %s\n", wp->NO, wp->expression);
      printf("Old value = %u\n", wp->value);
      printf("New value = %u\n", new_value);

      wp->value = new_value;
    }
    wp = wp->next;
  }
}

