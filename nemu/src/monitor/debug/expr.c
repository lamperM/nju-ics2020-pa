#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h>
#include <limits.h>
enum {
  TK_NOTYPE = 256,

  /* TODO: Add more token types */
  TK_OP_START, // operator token start...
  TK_PLUS, TK_SUB, TK_MUX, TK_DIV,
  TK_LBRKT, TK_RBRKT,
  TK_EQ,      // ==
  TK_NEQ,     // !=
  TK_AND,     // &&
  TK_OP_END,  // operator token end...

  TK_REG,     // $<reg>
  TK_DEREF,   // *
  TK_NUM,     // dec number
  TK_HEX_NUM, // hex number

  TK_INVAILD, // poison value
};

static struct rule {
  char *regex;
  int token_type;
  unsigned char precedence;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  /* Add operator precedence */

  {" +", TK_NOTYPE, UCHAR_MAX},     // spaces
  /* operation token start */
  {"\\+", TK_PLUS, 4},      // plus
  {"\\-", TK_SUB, 4},       // sub
  {"\\*", TK_MUX, 3},       // mux or dereference
  {"\\/", TK_DIV, 3},       // div
  {"\\(", TK_LBRKT, 1},     // (
  {"\\)", TK_RBRKT, 1},     // )
  {"==", TK_EQ, 7},         // equal
  {"!=", TK_NEQ, 7},        // not equal
  {"&&", TK_AND, 11},        // and
  {"\\$[A-Za-z]+", TK_REG, UCHAR_MAX}, // register name prefix
  /* operation token end */
  {"0[xX][0-9]+U?", TK_HEX_NUM, UCHAR_MAX}, 
  {"[0-9]+U?", TK_NUM, UCHAR_MAX}, // dec number (also '[[:digit:]]+U?' in POSIX)

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

#define TOKENS_MAX_SIZE (65535 + 1)
static Token tokens[TOKENS_MAX_SIZE] __attribute__((used)) = {};
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

        // 匹配已经测试ok
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        memset(tokens[nr_token].str, 0, TOKENS_MAX_SIZE);
        switch (rules[i].token_type) {
          case TK_NOTYPE:
              nr_token--;
              break;
          case TK_PLUS:
              tokens[nr_token].type = TK_PLUS;
              break;
          case TK_SUB:
              tokens[nr_token].type = TK_SUB;
              break;
          case TK_MUX:
              /* '*' may be TK_MUX or TK_DEREF */
              if (nr_token == 0 ) tokens[nr_token].type = TK_DEREF;
              else {
                  int type = tokens[nr_token - 1].type;
                  if (type == TK_RBRKT || type == TK_NUM || type == TK_HEX_NUM)
                      tokens[nr_token].type = TK_MUX;
                  else
                      tokens[nr_token].type = TK_DEREF;
              }
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
          case TK_EQ:
              tokens[nr_token].type = TK_EQ;
              break;
          case TK_NEQ:
              tokens[nr_token].type = TK_NEQ;
              break;
          case TK_AND:
              tokens[nr_token].type = TK_AND;
              break;
          case TK_REG:
              tokens[nr_token].type = TK_REG;
              // Jump '$'. Store register name ONLY
              memcpy(tokens[nr_token].str, (const char *)substr_start + 1, substr_len - 1);
              break;
          case TK_NUM:
              tokens[nr_token].type = TK_NUM;
              if (substr_len >= 32) {
                  printf("number is longer than 32\n");
                  assert(0);
              }
              // 'substr_len - 1' is to delete 'U'
              if (substr_start[substr_len - 1] == 'U')
                  memcpy(tokens[nr_token].str, (const char *)substr_start, substr_len - 1); 
              else 
                  memcpy(tokens[nr_token].str, (const char *)substr_start, substr_len); 
              break;
          case TK_HEX_NUM:
              if (substr_len >= 10) {
                  printf("Hex number is longer than 0xFFFFFFFF \n");
                  assert(0);
              }
              // Consider dereference
              if (nr_token != 0 && tokens[nr_token - 1].type == TK_DEREF) {
                  memcpy(tokens[--nr_token].str, (const char *)substr_start, substr_len);
              } else {
                  tokens[nr_token].type = TK_HEX_NUM;
                  memcpy(tokens[nr_token].str, (const char *)substr_start, substr_len);
              }
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
static bool check_parentheses(int p, int q) {
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

static uint8_t get_precedence(int type) {
    for (int i = 0; i < NR_REGEX; i++) {
        if (rules[i].token_type == type) 
            return rules[i].precedence;
    }
    return -1; // never be here normally
               // '-1' means UCHAR_MAX
}
/*
 * Precedence of op2 is higher than op1?
 */
bool priority_is_higher(int op1, int op2) {
    // assert(op1 == TK_PLUS || op1 == TK_SUB || op1 == TK_MUX || op1 == TK_DIV);
    // assert(op2 == TK_PLUS || op2 == TK_SUB || op2 == TK_MUX || op2 == TK_DIV);
    assert(op1 > TK_OP_START && op1 < TK_OP_END);
    assert(op2 > TK_OP_START && op2 < TK_OP_END);

    uint8_t op1_prec = get_precedence(op1);
    uint8_t op2_prec = get_precedence(op2);

    return op1_prec > op2_prec ? true : false; 
}

extern void* guest_to_host(paddr_t addr);
word_t eval(int p, int q) {
    if (p > q) {
        printf("p = %d, q = %d(p > q), error\n", p, q);
        assert(0);
    } else if (p == q) {
        int type = tokens[p].type;
        char str[32];
        word_t ret = 0;
        
        memset(str, 0 ,sizeof(str));
        strcpy(str, tokens[p].str);
        switch (type) {
            case TK_NUM:
                ret = (word_t)strtol(str, NULL, 10); 
                break;
            case TK_HEX_NUM:
                ret = (word_t)strtol(str, NULL, 16);
                break;
            case TK_REG:
                ; // Introduce null statement, otherwise compile error
                bool success = false;
                ret = isa_reg_str2val(str, &success);
                if (false == success) {
                    printf("%s: Invailed register name\n", str);
                    // TODO: Should continue ui
                    assert(0);
                }
                break;
            case TK_DEREF:
                ;
                word_t *host_addr = (word_t *)guest_to_host(strtol(str, NULL, 16));
                ret = *(host_addr);
                break;
            default:
                printf("Unsupported number\n");
                assert(0);
        }
        return ret;
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

    if (main_op == TK_INVAILD) {
        printf("Missing main operator!\n");
        assert(0); 
    }
//    printf("main op = %d, position = %d\n", main_op, main_op_pos);
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
    } // end of switch 
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

static bool split_expr_rst(char *buf, char **expr, char **result) {
    bool success = true;
    char *p = NULL;

    p = strtok(buf, " ");
    if (NULL == p)  success = false;
    else {
        *result = strdup(p);
        p = strtok(NULL, "\n");
        if (NULL == p)  success = false;
        else {
            *expr = strdup(p); // Will add NULL terminator
        }
    }

    return success;
}

/*
 * Test expr() with casefile 'nemu/tool/gen-expr/input'.
 */
bool test_expr(void) {
    FILE *fd;
    char input_path[200];
    const char *temp = getenv("NEMU_HOME"); // compatibility
    char *buf = NULL;  // A line
    size_t len = 0; 
    ssize_t read_size;
    

    if (NULL == temp) {
        printf("Get env variable error\n");
        return false;
    }
    sprintf(input_path, "%s/tools/gen-expr/input", temp);
    fd = fopen(input_path, "r" );
    if (NULL == fd) {
        printf("open case file:%s  error\n", input_path);
        return false;
    }

    while ((read_size = getline(&buf, &len, fd)) != -1) {
        char *expr_s, *rst_s;
        uint32_t rst = 0;
        bool success = true; // expr() execute flag
        uint32_t cal_rst = 0; // expr() calculate result

        expr_s = (char *)malloc(read_size);
        rst_s = (char *)malloc(read_size);

//        printf("line %zu:\n", read);
//        printf("%s", buf);
        if (split_expr_rst(buf, &expr_s, &rst_s) == false) {
            printf("Split expression error\n");
            free(expr_s); free(rst_s);
            goto failed;
        }
        
        rst = (uint32_t)strtol(rst_s, NULL, 10);
        cal_rst = expr(expr_s, &success);
        if (true != success) {
            printf("Execute expr() error\n");
            free(expr_s); free(rst_s);
            goto failed;
        }
        if (cal_rst != rst) {
            printf("Calculate error!\n");
            printf("expression: %s\n", expr_s);
            printf("result: %u, expr return:%u\n", rst, cal_rst);
            free(expr_s); free(rst_s);
            goto failed;
        }
        free(expr_s); free(rst_s);
    } // end of while

        printf("Expression calculate test passed!\n"); 
        free(buf);
        pclose(fd);
        return true;
failed:
        printf("Expression calculate test failed! Let's fix bugs\n");
        free(buf);
        pclose(fd);
        return false;
}

