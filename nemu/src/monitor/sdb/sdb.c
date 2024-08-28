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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
word_t vaddr_read(vaddr_t addr, int len);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  int N = 1;
  if (args != NULL) {
    N = atoi(arg);
    if (N == 0 && N < 0 && arg[0] != '0') {
      printf("Invalid input: '%c'\n", arg[0]);
      return 0;
    }
  }
  cpu_exec(N);
  nemu_state.state = NEMU_STOP;
  return 0;
}

static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);
void info_wp();
void add_wp(char *expr, word_t val);
void delete_wp(int no);

static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: info [r | w]\n");
    return 0;
  }

  switch (arg[0]) {
    case 'r':
      isa_reg_display();
      break;
    case 'w':
      info_wp();
      break;
    default:
      printf("Unknown option: %s\n", arg);
  }
  return 0;
}

/* x [N] [EXPR]*/
static int cmd_x(char *args) {
  // get N
  char *token =strtok(args, " ");
  if (token == NULL) {
    printf("Invaild argument\n");
    return 0;
  }
  uint32_t N = (uint32_t) strtoul(token, NULL, 10);

  // get EXPR
  token = strtok(NULL, " ");
  if (token == NULL) {
    printf("Invaild expression\n");
    return 0;
  }

  bool success;
  word_t start_address = 0x80000000;
  start_address = expr(token, &success);
  if (!success) {
    printf("Invalid expression: %s\n", token);
    return 0;
  }

  for (word_t i = 0; i < N; i++) {
    word_t address = start_address + i * 4;
    printf("%x\n", vaddr_read(address, 4));
  }
  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("No expression provided\n");
    return 0;
  }

  bool success = true;
  word_t value = expr(args, &success);

  if (success) {
    printf("Result: %u (0x%x)\n", value, value);
  } else {
    printf("Invaild expression: %s\n", args);
  }
  return 0;
}

static int cmd_w(char *args) {
  if (args == NULL) {
    printf("No expression provided\n");
    return 0;
  }

  bool success;
  word_t value = expr(args, &success);

  if (!success) {
    printf("Expression evaluation error\n");
    return 0;
  } 

  add_wp(args, value);
  return 0;
}

static int cmd_d(char *args) {
  char *arg = strtok(args, " ");
  if (arg == NULL) {
    printf("Invaild No\n");
    return 0;
  }

  int no = strtol(arg, NULL, 10);
  delete_wp(no);
  return 0;
}


static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Execute N instructions in a single step and then pause", cmd_si },
  { "info", "Print program status\n"
    "Usage:\n"
    "-r: Print register status\n"
    "-w: Print watchpoint status\n", cmd_info },
  { "x", "Find the value of given expression, and output N consecutive 4-byte outputs in hexadecimal\n"
    "Usage:\n"
    "x [N] [EXPR]", cmd_x },
  { "p", "Evaluate the given expression and return its result in decimal and hexadecimal formats" , cmd_p },
  { "w", "Set a watchpoint at EXPR, and pause the program when it changes", cmd_w },
  { "d", "Delete watchpoint with serial number N", cmd_d }
  /* Add more commands */
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { 
          // also can change state here to achieve 'quit elegantly', but not the best choice
          return;
        }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void test_expr() {
  FILE *file = fopen("/home/peize/projects/myYSYX/nemu/tools/gen-expr/input", "r");
  if (file == NULL) {
    perror("Error reading the file");
    return;
  }

  size_t len = 0;
  ssize_t read;
  word_t true_answer;
  char *expression;
  bool success = false;
  while (true) {
    // read the true answer
    if (fscanf(file, "%u ", &true_answer) == -1) {
      break;
    }
    // read the expression
    read = getline(&expression, &len, file);
    if (read == -1) {
      break;  // reach the end or error
    }

    expression[read - 1] = '\0';  // remove newline character

    // get the result of expr
    word_t result = expr(expression, &success);
    assert(success);

    if (result != true_answer) {
      fprintf(stderr, "Test failed: Expected %u but got %u for expr: %s\n",
              true_answer, result, expression);
      assert(0);
    } else {
      printf("Test passed: %u = %u for expr: %s\n",
              true_answer, result, expression);
    }
  }
  fclose(file);
  if (expression) {
    free(expression);
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Test the expresssion interpreter. */
  // test_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
