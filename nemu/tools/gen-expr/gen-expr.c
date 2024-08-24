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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define MAX_DEPTH 10
#define MAX_NUM 500

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

uint32_t choose(uint32_t n) {
  return rand() % n;
}

static void gen_rand_nums() {
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf) - 1, "%u", choose(MAX_NUM));
}

static void gen_rand_space() {
  if (choose(4)) {
    strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
  }
}

static void gen_rand_operator() {
  gen_rand_space();
  switch (choose(4)){
    case 0: strncat(buf, "+", sizeof(buf) - strlen(buf) - 1); break;
    case 1: strncat(buf, "-", sizeof(buf) - strlen(buf) - 1); break;
    case 2: strncat(buf, "*", sizeof(buf) - strlen(buf) - 1); break;
    case 3: strncat(buf, "/", sizeof(buf) - strlen(buf) - 1); break;
  }
  gen_rand_space();
}

static void gen_rand_expr(int depth) {
  // do something here 
  /* check if dive too deep or close to full*/ 
  if (depth > MAX_DEPTH || strlen(buf) > 65535) {
    return;
  }

  int r = rand() % (depth == 0 ? 2 : 3);
  switch (r)
  {
  case 0: // gen pure unsigned int
    gen_rand_nums();
    break;
  case 1: // gen with brackets
    strncat(buf, "(", sizeof(buf) - strlen(buf) - 1);
    gen_rand_space();
    gen_rand_expr(depth + 1);
    gen_rand_space();
    strncat(buf, ")", sizeof(buf) - strlen(buf) - 1);
    break;
  default:
    gen_rand_expr(depth + 1);
    gen_rand_operator();
    gen_rand_expr(depth + 1);
    break;
  }


}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf[0] = '\0';

    gen_rand_expr(0);

    snprintf(code_buf, sizeof(code_buf), code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr");
    if (ret != 0) {
      continue;
    }
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    uint32_t result;
    ret = fscanf(fp, "%u", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);

  }
  return 0;
}

