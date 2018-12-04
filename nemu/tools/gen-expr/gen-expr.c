#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define MAX_INT 10
#define MAX_EXPR_NUMBER 20

// this should be enough
static char buf[65536];
static int len = 0;
static int expr_number = 0;

static void gen_num();
static void gen_op();

static inline void gen_rand_expr() {
  if (expr_number >= MAX_EXPR_NUMBER) {
    gen_num();
    return;
  }
  switch(rand() % 3) {
    case 0:
      gen_num();
      break;
    case 1:
      buf[len++] = '(';
      gen_rand_expr();
      buf[len++] = ')';
      break;
    default:
      gen_rand_expr();
      gen_op();
      gen_rand_expr();
      break;
  }
  buf[len] = '\0';
}
static void gen_num() {
  int _number = rand() % MAX_INT + 1;
  // printf("before gen_num: buf=%s, len=%d, number=%d\n", buf, len,_number);
  int _length = sprintf(buf + len, "%d", _number);
  len += _length;
  // printf("after gen_num: buf=%s, len=%d, number=%d\n", buf, len,_number);
}
static void gen_op() {
  int choose = rand() % 4;
  assert(len + 1 < 65536);
  switch(choose) {
    case 0: 
      buf[len++] = '+';
      break;
    case 1:
      buf[len++] = '-';
      break;
    case 2: 
      buf[len++] = '*';
      break;
    case 3: 
      buf[len++] = '/';
      break;
  }
}

static char code_buf[65536];
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen(".code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc .code.c -o .expr");
    if (ret != 0) continue;

    fp = popen("./.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
