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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <limits.h>

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_NUMS, TK_LB, TK_RB,
  TK_NEGATIVE,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
  {"\\*", '*'},         // multiply
  {"\\/", '/'},         // divide
  {"[0-9]+", TK_NUMS},  // numbers  
  {"\\(", TK_LB},       // left bracket
  {"\\)", TK_RB},       // right bracket
  {"==", TK_EQ},        // equal
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {}; // save compiled regex

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
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

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

typedef enum {
  PAREN_ENCLOSED,     // whole expr enclosed by brackets, brackets inside match exactly
  PAREN_NOT_ENCLOSED, // though epxr not completely enclosed, every brackets are matched, like (EXPR) "balabala" (EXPR)
  PAREN_NOT_MATCHED   // not enclosed and unmatched, throw it away
} Paren_Status;

static bool make_token(char *e) {
  int position = 0;
  int i;
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

          strncpy(tokens[nr_token].str, substr_start, substr_len);   // record str into tokens
          tokens[nr_token].str[substr_len] = '\0';   // add stop sign
          
          // devide zero detection 2
          if (nr_token > 0 && tokens[nr_token - 1].type == '/' && 
              tokens[nr_token].type == TK_NUMS && 
              atoi(tokens[nr_token].str) == 0) {
            panic("Attempted division by zero");
          }

          // check if - is negative sign or minus sign
          if (tokens[nr_token].type == '-') {
            if (nr_token == 0 ||
                tokens[nr_token - 1].type == TK_LB || 
                tokens[nr_token - 1].type == TK_NEGATIVE ||
                tokens[nr_token - 1].type == '-' ||
                tokens[nr_token - 1].type == '+' ||
                tokens[nr_token - 1].type == '*' ||
                tokens[nr_token - 1].type == '/') {
                  tokens[nr_token].type = TK_NEGATIVE;
                }
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

  return true;
}

static bool remove_redundant_parentheses(int *p, int *q) {
  while (*p < *q && tokens[*p].type == TK_LB && tokens[*q].type == TK_RB) {
    int layer = 0;
    bool is_redundant = true;
    for (int i = *p + 1; i < *q; i++) {
      if (tokens[i].type == TK_LB) {
        layer++;
      }
      if (tokens[i].type == TK_RB) {
        layer--;
      }
      if (layer < 0) {
        is_redundant = false;
        break;
      } 
    }
    if (is_redundant) {
      // remove outer bracket
      (*p)++;
      (*q)--;
    } else {
      break;
    }
  }
  return true;
}

Paren_Status check_parentheses(int p, int q) {
  if (tokens[p].type != TK_LB || tokens[q].type != TK_RB) {
    return PAREN_NOT_ENCLOSED;
  }
  
  // check if every bracket inside could match
  int layer = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_LB) {
      layer++;
    } else if (tokens[i].type == TK_RB) {
      layer--;
      if (layer == 0 && i != q) {
        // case1: the outer parentheses do not enclose the whole expr
        return PAREN_NOT_ENCLOSED;
      }
      if (layer < 0) {
        // case2: parentheses not paired
        return PAREN_NOT_MATCHED;
      }
    }
  }
  
  return layer == 0 ? PAREN_ENCLOSED : PAREN_NOT_MATCHED;

}

int get_operator_priority(int token_type) {
  /* return the priority according to each operatory's weight */
  switch (token_type)
  {
  case '+':
  case '-':
    return 1;
  case '*':
  case '/':
    return 2;
  default:
    return INT_MAX;
  }
}

int find_main_operator(int p, int q) {
  int op = -1;
  int min_priority = INT_MAX;
  int parentheses_layer = 0;

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_LB) {
      parentheses_layer++;
    } else if (tokens[i].type == TK_RB) {
      parentheses_layer--;
    } else if (parentheses_layer == 0) {
      // make sure filter and ignore minus sign while judging main op
      if (tokens[i].type == TK_NEGATIVE) {
        continue;
      }

      int priority = get_operator_priority(tokens[i].type);

      if (priority <= min_priority) {
        min_priority = priority;
        op = i;
      }
    }
  }

  return op;
}

#define MAX_DEPTH 2
#define STACK_SIZE 1000

typedef struct {
  int p, q;
} StackFrame;

word_t eval();

word_t eval_iter(int p, int q) {
  StackFrame stack[STACK_SIZE];
  int sp = 0; //pointer
  
  stack[sp++] = (StackFrame) {
    p, q
  };

  while (sp > 0) {
    StackFrame frame = stack[--sp];
    p = frame.p;
    q = frame.q;

    if (p > q) {
      panic("Bad expression 2");
      return 1;
    } else if (p == q) {
      if (tokens[p].type == TK_NUMS) {
        return atoi(tokens[p].str);
      }
    } else {
      Paren_Status status = check_parentheses(p, q);
      if (status == PAREN_ENCLOSED) {
        stack[sp++] = (StackFrame){p + 1, q - 1};
      } else if (status == PAREN_NOT_MATCHED) {
        panic("Parentheses not matched");
      } else {
        int op = find_main_operator(p, q);
        word_t val1, val2;

        stack[sp++] = (StackFrame){op + 1, q};
        stack[sp++] = (StackFrame){p, op - 1};
        
        sp--;
        val1 = eval(p, op - 1, MAX_DEPTH + 1);
        val2 = eval(op + 1, q, MAX_DEPTH + 1);

        if (tokens[op].type == '/' && val2 == 0) {
          panic("Division by zero detected");
        }

        switch (tokens[op].type) {
          case '+': return val1 + val2;
          case '-': return val1 - val2;
          case '*': return val1 * val2;
          case '/': return (sword_t)val1 / (sword_t)val2;
          default: panic("Unknown operator type: %d", tokens[op].type);
        }
      }
    }
  }
  return 0;
}

word_t eval(int p, int q, int depth) {

  if (depth > MAX_DEPTH) {
    // turn to iteraitve way 
    return eval_iter(p, q);
  }

  remove_redundant_parentheses(&p, &q);

  if (p > q) {
    /* Bad expression */
    panic("Bad expression");
    return 1;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (tokens[p].type == TK_NUMS) {
      return atoi(tokens[p].str);
    }
  } else {
    Paren_Status status = check_parentheses(p, q);
    if (status == PAREN_ENCLOSED) {
      /* The expression is surrounded by a matched pair of parentheses.
       *  If that is the case, just throw away the parentheses.
       */
      return eval(p + 1, q - 1, depth + 1); 
    } else if (status == PAREN_NOT_MATCHED) {
      panic("Parentheses not matched");
    }
    
    // for not enclosed type, keep recursive
    int op = find_main_operator(p, q);
      word_t val1 = eval(p, op - 1, depth + 1);
      word_t val2 = eval(op + 1, q, depth + 1);

      if (tokens[op].type == '/' && val2 == 0) {
      // devide zero detection 1
      panic("Deivsion by zero detected");
      }

      switch (tokens[op].type) {
        case '+': return val1 + val2;
        case '-': return val1 - val2;
        case '*': return val1 * val2;
        case '/': return (sword_t)val1 / (sword_t)val2;
        default: panic("Unknown operator type: %d", tokens[op].type);
      }
    }
  
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* Insert codes to evaluate the expression. */
  // backup option to detect devision by zero here 
  *success = true;
  return eval(0, nr_token - 1, 0);

  return 0;
}
