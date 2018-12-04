#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUMBER

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"[0-9]+", TK_NUMBER},    // number
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // sub 
  {"\\*", '*'},         // multiple
  {"\\/", '/'},         // division
  {"\\(", '('},         // left bracketed
  {"\\)", ')'},         // right bracketed
  {"==", TK_EQ}         // equal
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

uint32_t evaluate(int begin, int end, bool *success);

int compare(int pos_1, int pos_2, bool *success);
bool check_parentheses(int begin, int end);

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

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

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case ' ':
            break;
          case '+':
          case '-':
          case '*':
          case '/':
          case '(':
          case ')':
          case TK_NUMBER:
          case TK_EQ:
            tokens[nr_token].type = rules[i].token_type;
            break;
          default: TODO();
        }
        // Copy the token into token.str.
        assert(substr_len < 32); // Check the length.
        strncpy(tokens[nr_token].str, substr_start, substr_len);
        tokens[nr_token].str[substr_len] = 0; // Insert a string tail.
        nr_token++;

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

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  bool result;
  evaluate(0, nr_token - 1, &result);

  return 0;
}

uint32_t evaluate(int begin, int end, bool *success) {
  int i;
  int op = begin;
  int deep = 0;
  bool result;

  assert(begin >= 0 && end < 32); // Length of token array is 32.

  if (begin > end) {
    *success = false;
    return 0;
  } else if (begin == end) {
    *success = true;
    return atoi(tokens[begin].str);
  } else if (check_parentheses(begin, end) == true) {
    return evaluate(begin + 1, end - 1, success);
  } else {
    // Find the main operator.
    for (i = 0; i < nr_token; i++) {
      if (tokens[begin].type == '(') {
        deep++;
      } else if (tokens[begin].type == ')') {
        deep--;
      } else if (deep == 0) {
        op = compare(op, i, &result);
        if (result == false) {
          *success = false;
          return 0;
        }
      } else if (deep < 0) {
        // Invalid parentheses.
        *success = false;
        return 0;
      } else if (deep > 0) {
        compare(op, i, &result);
        if (result == false) {
          *success = false;
          return 0;
        }
      }
    }
    *success = true;
    switch(tokens[op].type) {
      case '+': return evaluate(begin, op - 1, success) + evaluate(op + 1, end, success);
      case '-': return evaluate(begin, op - 1, success) - evaluate(op + 1, end, success);
      case '*': return evaluate(begin, op - 1, success) * evaluate(op + 1, end, success);
      case '/': return evaluate(begin, op - 1, success) / evaluate(op + 1, end, success);
      default: assert(0);
    }
  }
}

// return the main operator.
int compare(int pos1, int pos2, bool *success) {
  /*
       +  -  *  /  <- token1
    +  T  T  T  T
    -  T  T  T  T
    *  F  F  T  T
    /  F  F  T  T
  */
  bool cmp[4][4] = {
    {true, true, true, true},
    {true, true, true, true},
    {false, false, true, true},
    {false, false, true, true}};
  int pos[2] = {pos1, pos2};
  int op[2];
  int i;

  for (i = 0; i < 2; i++) {
    switch(pos[i]) {
      case '+': op[i] = 0; break;
      case '-': op[i] = 1; break;
      case '*': op[i] = 2; break;
      case '/': op[i] = 3; break;
      default:
        *success = false;
        return pos1;
    }
  }
  *success = true;
  if (cmp[op[1]][op[0]]) {
    return pos[1];
  } else {
    return pos[0];
  }
}

bool check_parentheses(int begin, int end) {
  if (tokens[begin].type == '(' && tokens[end].type == ')') {
    return true;
  }
  return false;
}
