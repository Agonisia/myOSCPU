#include "monitor/sdb.h"
#include <regex.h>
#include <limits.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_NUMS, TK_LB, TK_RB,
  TK_NEGATIVE, TK_POINTER,
  TK_AND, TK_OR, TK_NOT,
  TK_NOTEQ, TK_REG, TK_HEX,
  TK_LE, TK_GE, TK_LT, TK_GT,
  /* Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  /* Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},          // spaces
  {"==", TK_EQ},              // equal
  {"!=", TK_NOTEQ},           // not equal
  {"<=", TK_LE},              // less or equal
  {">=", TK_GE},              // greater or equal
  {"\\&\\&", TK_AND},         // and 
  {"\\|\\|", TK_OR},          // or
  {"!", TK_NOT},              // not   
  {"\\+", '+'},               // plus
  {"\\-", '-'},               // minus
  {"\\*", '*'},               // multiply
  {"\\/", '/'},               // divide
  {"0x[0-9a-fA-F]+", TK_HEX}, // hex numbers (must precede numbers)             
  {"[0-9]+", TK_NUMS},        // numbers  
  {"\\(", TK_LB},             // left bracket
  {"\\)", TK_RB},             // right bracket
  {"<", TK_LT},               // less than 
  {">", TK_GT},               // greater than 
  {"\\$\\w+", TK_REG},        // reg name
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {}; // save compiled regex

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void regex_init() {
  int i;
  char error_msg[128]; // contain error message
  int ret; // the result of whether compile success

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static int paren_pairs[64];
static int nr_token __attribute__((used))  = 0;
static Token tokens[64] __attribute__((used)) = {}; // change here, from 32 to 64

typedef enum {
  PAREN_ENCLOSED,     // whole expr enclosed by brackets, brackets inside match exactly
  PAREN_NOT_ENCLOSED, // though epxr not completely enclosed, every brackets are matched, like (EXPR) "balabala" (EXPR)
  PAREN_NOT_MATCHED   // not enclosed and unmatched, throw it away
} Paren_Status;

/* parse the input string into an array of tokens, handling negative signs, pointer symbols */
static bool make_token(char *e) {
  int position = 0;
  int i;
  int neg_count = 0;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        /* successful match regex && the match strat at the current pos (string offset = 0) */
        char *substr_start = e + position; // get str's initial position
        int substr_len = pmatch.rm_eo; // get str's length

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        /* record tokens*/
        if (rules[i].token_type != TK_NOTYPE) {

          tokens[nr_token].type = rules[i].token_type;

          /* check if need some tuncation, to prevent str overflow*/
          if (substr_len >= sizeof(tokens[nr_token].str)) {
            // cut off redundant str
            substr_len = sizeof(tokens[nr_token].str) - 1;
          }

          Assert(nr_token < 65536, "storage cant hold that much tokens");
          Assert(substr_len < sizeof(tokens[nr_token].str), "single token overflow");
          strncpy(tokens[nr_token].str, substr_start, substr_len);   // record str into tokens
          tokens[nr_token].str[substr_len] = '\0';   // add stop sign
          
          // check if - is negative sign or minus sign
          if (tokens[nr_token].type == '-') {
            if (nr_token == 0 ||
                tokens[nr_token - 1].type == TK_LB || 
                tokens[nr_token - 1].type == TK_NEGATIVE ||
                tokens[nr_token - 1].type == '-' ||
                tokens[nr_token - 1].type == '+' ||
                tokens[nr_token - 1].type == '*' ||
                tokens[nr_token - 1].type == '/') {
                  // count consecutive negative signs
                  neg_count++;
                  while (e[position] == '-') {
                    position++;
                    neg_count++;
                  }

                  // determine final type based on the count
                  if (neg_count % 2 == 0) {
                    continue;
                  } else {
                    tokens[nr_token].type = TK_NEGATIVE;
                    Log("A negative sign detected");
                  }

                  neg_count = 0; // reset the counter
                }
          }

          // check if * is multiply sign or pointer sign
          if (tokens[nr_token].type == '*') {
            if (nr_token == 0 || 
                tokens[nr_token - 1].type == TK_LB || 
                tokens[nr_token - 1].type == TK_NEGATIVE ||
                tokens[nr_token - 1].type == '-' ||
                tokens[nr_token - 1].type == '+' ||
                tokens[nr_token - 1].type == '*' ||
                tokens[nr_token - 1].type == '/') {
                  tokens[nr_token].type = TK_POINTER;
                  Log("A pointer sign decected");
                }
          }

          if (tokens[nr_token].type == TK_REG) {
            Log("A reg name detected");
          }

          nr_token++;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  // pre-check if parentheses are matched
  int stack[64], top = 0;
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == TK_LB) {
      stack[top++] = i;
    } else if (tokens[i].type == TK_RB) {
      if (top == 0) {
        printf("Mismatched parentheses\n");
        return false;
      }
      int left = stack[--top];
      paren_pairs[left] = i;
      paren_pairs[i] = left;
    }
  }
  if (top != 0) {
    printf("Mismatched parentheses\n");
    return false;
  }

  return true;
}

/* check whether the brackets are paired correctly and completely enclose the expression */
Paren_Status check_parentheses(int p, int q) {
  if (tokens[p].type != TK_LB || tokens[q].type != TK_RB) {
    return PAREN_NOT_ENCLOSED;
  }
  return (paren_pairs[p] == q) ? PAREN_ENCLOSED : PAREN_NOT_ENCLOSED;
}

/* return the priority according to each operatory's weight */
int get_operator_priority(int token_type) {
  switch (token_type) {
    case TK_OR:
      return 0;
    case TK_AND:
      return 1;
    case TK_EQ:
    case TK_NOTEQ:
      return 2;
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE:
      return 3;
    case '+':
    case '-':
      return 4;
    case '*':
    case '/':
      return 5;
    case TK_NEGATIVE:
    case TK_POINTER:
      return 6;
    default:
      return INT_MAX;
  }
}

bool is_operator(int type) {
  bool result = (type == '+' || type == '-' || type == '*' || type == '/' ||
          type == TK_EQ || type == TK_NOTEQ || type == TK_AND || 
          type == TK_OR || type == TK_LT || type == TK_LE || 
          type == TK_GT || type == TK_GE || type == TK_NOT ||
          type == TK_NEGATIVE || type == TK_POINTER);
  return result;
}

/* return the location of main operator, it cannot inside the brackets */
int find_main_operator(int p, int q) {
  int op = -1;
  int min_priority = INT_MAX;

  for (int i = p; i <= q;) {
    if (tokens[i].type == TK_LB) {
      i = paren_pairs[i] + 1; // skip the enclosed part
      continue;
    } 

    if (!is_operator(tokens[i].type)) {
      i++;
      continue;
    }

    int priority = get_operator_priority(tokens[i].type);
    if (priority < min_priority) {
      min_priority = priority;
      op = i;
    }
    i++;
  }

  return op;
}

bool is_unary_operator(int type) {
  return (type == TK_NEGATIVE || type == TK_POINTER || type == TK_NOT);
}

#define STACK_SIZE 1000
word_t vaddr_read(vaddr_t addr, int len);

typedef struct {
  int p, q;
  int stage; // 0: initial stage, 1: left operand processed, 2: right operand processed
  word_t left_val;
  int op;
} StackFrame;

/* evaluate expression value recursively, dealing divide-by-zero detection */
word_t eval(int p, int q) {
  StackFrame stack[STACK_SIZE];
  int sp = 0; // stack pointer
  word_t result = 0;
  bool error = false;
  
  stack[sp++] = (StackFrame){p, q, 0, 0, -1};

  while (sp > 0 && !error) {
    StackFrame *frame = &stack[sp-1];

    if (frame->stage == 0) {
      // initial stage, check if the expression is valid
      if (frame->p > frame->q) {
        error = true;
        sp--;
        continue;
      }

      Paren_Status status = check_parentheses(frame->p, frame->q);
      if (status == PAREN_ENCLOSED) {
        // handle enclosed expression
        frame->p++;
        frame->q--;
        continue;
      }

      int op = find_main_operator(frame->p, frame->q);
      if (op == -1) {
        /* Single token.
        * For now this token should be a number.
        * Return the value of the number.
        */
        if (tokens[frame->p].type == TK_NUMS) {
          result = atoi(tokens[frame->p].str);
        } else if (tokens[frame->p].type == TK_HEX) {
          result = strtol(tokens[frame->p].str, NULL, 16);
        } else if (tokens[frame->p].type == TK_REG) {
          bool suc = false;
          word_t val = reg_str2val(tokens[frame->p].str, &suc);
          if (suc == true) {
            result = val;
          } else {
            panic("Invalid register");
          }
        }
        sp--;
        continue;
      }

      if (is_unary_operator(tokens[op].type)) {
        // handel unary operator, stack the right operand
        frame->op = op;
        stack[sp++] = (StackFrame){op+1, frame->q, 0, 0, -1};
        frame->stage = 1;
      } else {
        // handle binary operator, deal with left operand first
        frame->op = op;
        stack[sp++] = (StackFrame){frame->p, op-1, 0, 0, -1};
        frame->stage = 2;
      }
    } else if (frame->stage == 1) {
      // unary operator
      word_t val = result;
      switch (tokens[frame->op].type) {
        case TK_NEGATIVE: result = -val; break;
        case TK_POINTER:  result = vaddr_read(val, 4); break;
        case TK_NOT:      result = !val; break;
        default: panic("Unknown unary operator type: %d", tokens[frame->op].type);
      }
      sp--;
    } else if (frame->stage == 2) {
      // binary operator, left operand processed, deal with right operand
      frame->left_val = result;
      stack[sp++] = (StackFrame){frame->op+1, frame->q, 0, 0, -1};
      frame->stage = 3;
    } else if (frame->stage == 3) {
      // binary operator, right operand processed, calculate the result
      word_t right_val = result;
      if (tokens[frame->op].type == '/' && right_val == 0) {
        error = true;
        sp--;
        Log("Division by zero detected"); 
        break;
      }
      switch (tokens[frame->op].type) {
        case '+': result = frame->left_val + right_val; break;
        case '-': result = frame->left_val - right_val; break;
        case '*': result = frame->left_val * right_val; break;
        case '/': result = (sword_t)frame->left_val / (sword_t)right_val; break;
        case TK_EQ: result = frame->left_val == right_val; break;
        case TK_NOTEQ: result = frame->left_val != right_val; break;
        case TK_LT: result = frame->left_val < right_val; break;
        case TK_LE: result = frame->left_val <= right_val; break;
        case TK_GT: result = frame->left_val > right_val; break;
        case TK_GE: result = frame->left_val >= right_val; break;
        case TK_AND: result = frame->left_val && right_val; break;
        case TK_OR: result = frame->left_val || right_val; break;
        default: panic("Unknown operator type: %d", tokens[frame->op].type);
      }
      sp--;
    }
  }

  if (error) {
    panic("Evaluation error");
  }
  return result;
}

/* parse and evaluate the value of an expression */
word_t expr(char *e, bool *success) {
  *success = false;
  if (!make_token(e)) {
    printf("Expression parse failed at: %s\n", e);
    return 0;
  }

  *success = true;
  return eval(0, nr_token - 1);
}