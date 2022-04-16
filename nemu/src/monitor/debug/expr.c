#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h>
enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_OP_START,
  TK_PLUS,
  TK_SUB,
  TK_MUX,
  TK_DIV,
  TK_LBRKT,
  TK_RBRKT,
  TK_OP_END,

  TK_NUM,

  TK_INVAILD, // poison value
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
    
  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
  /* operation token */
  {"\\+", TK_PLUS},     // plus
  {"\\-", TK_SUB},      // sub
  {"\\*", TK_MUX},      // mux
  {"\\/", TK_DIV},      // div
  {"\\(", TK_LBRKT},    // (
  {"\\)", TK_RBRKT},    // )
  {"[0-9]+", TK_NUM},   // number

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )
//#define char_is_num(c)        (c >= '0' && c <= '9')
#define token_is_vaild_op(type) (type > TK_OP_START && type < TK_OP_END)
//#define token_is_op(type)     (type > TK_OP_START && type < TK_OP_END)
static regex_t re[NR_REGEX] = {};

bool test_expr(void);
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

  //test_expr();
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[65535] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

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

        memset(tokens[nr_token].str, 0, 32);
        switch (rules[i].token_type) {
          case TK_NOTYPE:
              nr_token--;
              break;
          case TK_PLUS:
              tokens[nr_token].type = TK_PLUS;
              break;
          case TK_EQ:
              break;
          case TK_SUB:
              tokens[nr_token].type = TK_SUB;
              break;
          case TK_MUX:
              tokens[nr_token].type = TK_MUX;
              break;
          case TK_DIV:
              tokens[nr_token].type = TK_DIV;
              break;
          case TK_LBRKT:
              tokens[nr_token].type = TK_LBRKT;
              break;
          case TK_RBRKT:
              tokens[nr_token].type = TK_RBRKT;
              break;
          case TK_NUM:
              tokens[nr_token].type = TK_NUM;
              if (substr_len >= 32) {
                  printf("number is longer than 32\n");
                  assert(0);
              }
              memcpy(tokens[nr_token].str, (const char *)substr_start, substr_len);
              break;
          default: TODO();
        }
        
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

/* 
 * Check if the expression is surrounded by a matched pair of bracket
 * and check if all brackets is vailed.
 */
bool check_parentheses(int p, int q) {
    int expr_len = q - p + 1;
    unsigned int top = 0; // top of stack
    
    assert(expr_len  >= 3);
    
    if (tokens[p].type !=TK_LBRKT && tokens[q].type != TK_RBRKT)  
        return false; // no surrounded

    p++; q--;
    expr_len -= 2;

    for (int i = p; i <= q; i++) {
        int type = tokens[i].type;
        if (TK_LBRKT == type) {
            // stack[top++] = i;
            top ++;
        } else if (TK_RBRKT == type) {
            if (top == 0) return false; // invailed brackets
            top--;
        } else { }
    }

    if (top == 0) return true;
    else return false;
}

bool priority_is_higher(int op1, int op2) {
    assert(op1 == TK_PLUS || op1 == TK_SUB || op1 == TK_MUX || op1 == TK_DIV);
    assert(op2 == TK_PLUS || op2 == TK_SUB || op2 == TK_MUX || op2 == TK_DIV);

    if ((op1 == TK_MUX || op1 == TK_DIV) && (op2 == TK_PLUS || op2 == TK_SUB)) 
        return true;
    else 
        return false;
}

    
word_t eval(int p, int q) {
    if (p > q) {
        printf("p = %d, q = %d(p > q), error\n", p, q);
        assert(0);
    } else if (p == q) {
        return (word_t)strtol(tokens[p].str, NULL, 10); // only base-10 supported
    } else if (check_parentheses(p, q) == true) {
        return eval(p + 1, q - 1);
    } else {
        /* find 主运算符 */
        int main_op_pos = 0;
        int main_op = TK_INVAILD; // poison value
        word_t val1, val2;


        for (int i = q, need_brkt = 0; i >= p; i--) {
            int type = tokens[i].type;
            if (!token_is_vaild_op(type)) continue;
            if (TK_RBRKT == type) {
                need_brkt++;
                continue;
             }
            if (need_brkt !=0) {
                if (TK_LBRKT == type) need_brkt--;
                continue;
            } else {
                /* may be main op, consider priority */
                 if (TK_INVAILD == main_op || priority_is_higher(main_op, type)) {
                    main_op = type;
                    main_op_pos = i;
                }
             } // end of else
         } // end of for

          
    printf("main op = %d, position = %d\n", main_op, main_op_pos);
    val1 = eval(p, main_op_pos - 1);
    val2 = eval(main_op_pos + 1, q);
    
    switch(main_op) {
        case TK_PLUS: return val1 + val2;
        case TK_SUB: return val1 - val2;
        case TK_MUX: return val1 * val2;
        case TK_DIV:
            assert(val2 != 0);
            return val1 / val2;
        default:
            assert(0);
            break;
    }  
     } // end of else


}  

word_t expr(char *e, bool *success) {  
    if (!make_token(e)) {
        *success = false;
        return 0;
    } 

    /* TODO: Insert codes to evaluate the expression. */
    int p, q;
    word_t res;

    p = 0;
    q = nr_token - 1;
    
    res = eval(p, q);
    *success = true;
    return res;
}
bool split_expr_rst(char *buf, char **expr, char **result) {
    bool success = true;
    char *p = NULL;

    p = strtok(buf, " ");
    if (NULL == p)  success = false;
    else {
        *result = strdup(p);
        p = strtok(NULL, "\n");
        if (NULL == p)  success = false;
        else {
            *expr = strdup(p);
        }
    }

    return success;
}

bool test_expr(void) {
    FILE *fd;
    char input_path[200];
    const char *temp = getenv("NEMU_HOME");
    char *buf = NULL;
    size_t len = 0;
    ssize_t read;
    

    if (NULL == temp) {
        Log("Get env variable error\n");
        exit(EXIT_FAILURE);
    }
    sprintf(input_path, "%s/tools/gen-expr/input", temp);
    fd = fopen(input_path, "r" );
    if (NULL == fd) {
        printf("open test input error\n");
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&buf, &len, fd)) != -1) {
        char *expr_s, *rst_s;
        uint32_t rst = 0;
        bool success = true; // expr execute flag
        uint32_t cal_rst = 0; // expr calculate result

        expr_s = (char *)malloc(read);
        rst_s = (char *)malloc(read);

//        printf("line %zu:\n", read);
//        printf("%s", buf);
        
        if (split_expr_rst(buf, &expr_s, &rst_s) == false) {
            Log("split expression error\n");
        }
        
        rst = (uint32_t)strtol(rst_s, NULL, 10);
        cal_rst = expr(expr_s, &success);
        if (true != success) {
            Log("Execute expr() error\n");
            exit(EXIT_FAILURE);
        }
        if (cal_rst != rst) {
            printf("Calculate error!\n");
            printf("expression: %s\n", expr_s);
            printf("result: %u, expr return:%u\n", rst, cal_rst);
            return false;
        }
        free(expr_s);
        free(rst_s);
    }
        printf("expression calculate test passed!\n"); 
        return true;

}

