#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/random.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static int buf_index = 0;
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";


/*
 * Generate random number less than n
 */
/*
uint32_t choose(uint32_t n) {
    uint32_t rand_num = rand() % n;

    return rand_num;
} */

#define MAX_INDEX   65536
#define choose(n) (uint32_t) (rand() % n)


static inline bool gen(char c) {
    if (buf_index >= MAX_INDEX - 1) 
        return false;

    buf[buf_index++] = c;
    return true;
}

static inline bool gen_num(void ) {
    uint32_t ran_num = choose(1000);  
    char ran_num_str[12];
    int len = 0;
   
    sprintf(ran_num_str, "%dU", ran_num); // 'U' make sure always unsigned calculation
    len = strlen(ran_num_str);
    if (buf_index + len >= MAX_INDEX -1 ) 
        return false;

    memcpy(buf + buf_index, ran_num_str, len);
    buf_index += len;
    return true;
}
static inline bool gen_rand_op(void ) {
    if (buf_index >= MAX_INDEX - 1)
        return false;

    switch(choose(4)) {
        case 0: buf[buf_index++] = '+'; break;
        case 1: buf[buf_index++] = '-'; break;
        case 2: buf[buf_index++] = '*'; break;
        case 3: buf[buf_index++] = '/'; break;
        default: assert(0);
    }
    return true;
}

static bool gen_rand_expr() {
  switch(choose(4)) {
      case 0: 
          return gen_num();
          break;
      case 1: 
          if (false == gen('('))  return false;
          if (false == gen_rand_expr()) return false;
          if (false == gen(')'))  return false;
          break;
      case 2:
          if (false == gen_rand_expr())  return false;
          if (false == gen_rand_op()) return false;
          if (false == gen_rand_expr()) return false;
          break;
      default: // insert space
          if (false == gen(' ')) return false;
          if (false == gen_rand_expr()) return false;
          break;
  }

  return true;

}


int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  
//    srandom(time(NULL));
  for (i = 0; i < loop; i ++) {
    while(gen_rand_expr() == false) {
        printf("false occurs\n");
        buf_index = 0;
        memset(buf, 0, 65535 * sizeof(char));
    }

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -Werror=div-by-zero /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) { 
        buf_index = 0;
        memset(buf, 0 , 65535 * sizeof(char));
        continue;
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);
   

    int result;
    if (fscanf(fp, "%d", &result) == 0) {
        printf("fscanf error\n");
    }
    pclose(fp);

    printf("%u %s\n", result, buf);
    buf_index = 0;
    memset(buf, 0 , 65535 * sizeof(char));
  }
  return 0;
}
