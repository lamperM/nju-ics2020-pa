#include <isa.h>
#include "expr.h"
#include "watchpoint.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
int is_batch_mode();

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
  return -1;
}

static int cmd_si(char *args) {
    uint32_t nsteps;

    if (args == NULL) {
        nsteps = 1;
    } else {
        nsteps = (uint32_t)atoi(args);
    }    

    cpu_exec(nsteps); 

    return 0;
}
static int cmd_x(char *args);
static int cmd_help(char *args);
static int cmd_info(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "single step execution", cmd_si },
  { "info", "Display information about following arguements", cmd_info },
  { "x", "Display memory", cmd_x },
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

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

static int cmd_x(char *args) {
    extern void* guest_to_host(paddr_t addr);
    char *arg = strtok(NULL, " ");
    
    if (arg == NULL) {
        printf("invailed arguments\n");
        return 0;
    } else {
        int byte_num = atoi(arg);
        uint64_t g_addr = 0; // guest addr
        int *h_addr = 0; // host addr
        arg = strtok(NULL, " ");
        if (arg == NULL) {
            printf("invailed argument\n");
            return 0;
        } else {
            g_addr = (uint64_t)strtol(arg, NULL, 16);
            printf("addr: 0x%0lx", g_addr);
            for (int i = 0; i < byte_num; i++) {
                h_addr = (int *)guest_to_host(g_addr);
                printf(" 0x%02x", *h_addr);
            }
        }

    }

    return 0;
}
static int cmd_info(char *args) {
    char *arg = strtok(NULL, " ");

    if (arg == NULL) {
        printf("invailed arguments\n");
        return 0;
    }
    
    if (strcmp(arg, "r") == 0) {
        isa_reg_display();
    }
    return 0;
}


void ui_mainloop() {
  if (is_batch_mode()) {
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

#ifdef HAS_IOE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
