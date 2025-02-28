#include "monitor/sdb.h"
#include <readline/readline.h>
#include <readline/history.h>

#define NR_CMD ARRLEN(cmd_table)

static bool is_batch_mode = false;

static int cmd_help(char *args);
static int cmd_c(char *args);
static int cmd_q(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

void regex_init();
void wp_pool_init();
void wp_display();
void wp_add(char *expr, word_t val);
void wp_delete(int no);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit the simulation of Rockbottom", cmd_q },
  { "si", "Execute [N] instructions in a single step and then pause", cmd_si },
  { "info", "Print program status\n"
    "Usage:\n"
    "-r: Print register status\n"
    "-w: Print watchpoint status\n", cmd_info },
  { "x", "Find the value of given expression, and output [N] consecutive 4-byte outputs in hexadecimal\n"
    "Usage:\n"
    "x [N] [EXPR]", cmd_x },
  { "p", "Evaluate the given [EXPR] and return its result in decimal and hexadecimal formats" , cmd_p },
  { "w", "Set a watchpoint at [EXPR], and pause the program when it changes", cmd_w },
  { "d", "Delete watchpoint with serial number [N]", cmd_d }
  /* Add more commands */
};

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

static int cmd_c(char *args) {
  sim_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  sim_state.state = SIM_QUIT;
  return -1;
}

/* si [N] */
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
  sim_exec(N);
  return 0;
}

/* info [SUBCMD: r/w]*/
static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: info [r | w]\n");
    return 0;
  }

  switch (arg[0]) {
    case 'r':
      reg_display();
      break;
    case 'w':
      wp_display();
      break;
    default:
      printf("Unknown option: %s\n", arg);
  }
  return 0;
}

/* x [N] [EXPR] */
static int cmd_x(char *args) {
  // get N
  char *token = strtok(args, " ");
  if (token == NULL) {
    printf("Invaild argument\n");
    return 0;
  }
  uint32_t N = (uint32_t) strtoul(token, NULL, 10);

  // get EXPR
  char *remain = args + strlen(token) + 1;
  if (remain == NULL) {
    printf("Invaild expression\n");
    return 0;
  }

  bool success;
  vaddr_t start_address = 0x80000000;
  start_address = expr(remain, &success);
  if (!success) {
    printf("Invalid expression: %s\n", token);
    return 0;
  }

  for (int i = 0; i < N; i++) {
    vaddr_t address = start_address + i * 4;
    word_t data = vaddr_read(address, 4);
    printf("0x%x: 0x%x (%u)\n", address, data, data);
  }
  return 0;
}

/* p [EXPR] */
static int cmd_p(char *args) {
  if (args == NULL) {
    printf("No expression provided\n");
    return 0;
  }

  bool success = true;
  word_t value = expr(args, &success);

  if (success) {
    printf("Result: 0x%x (%u)\n", value, value);
  } else {
    printf("Invaild expression: %s\n", args);
  }
  return 0;
}

/* w [EXPR] */
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

  wp_add(args, value);
  return 0;
}

/* d [N] */
static int cmd_d(char *args) {
  char *arg = strtok(args, " ");
  if (arg == NULL) {
    printf("Invaild No\n");
    return 0;
  }

  int no = strtol(arg, NULL, 10);
  wp_delete(no);
  return 0;
}

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(npc) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
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

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { 
          return;
        }
        break;
      }
    }

    if (i == NR_CMD) { 
      printf("Unknown command '%s'\n", cmd); 
    }
  }
}

void sdb_init() {
  /* Compile the regular expressions. */
  regex_init();

  /* Initialize the watchpoint pool. */
  wp_pool_init();
}