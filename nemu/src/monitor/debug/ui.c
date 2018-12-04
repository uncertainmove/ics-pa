#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#define PMEM_SIZE (128 * 1024 * 1024)

CPU_state cpu;
uint8_t pmem[PMEM_SIZE];

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  // readline will print string first.
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

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_p(char *args) {
  return -1;
}
static int cmd_x(char *args);

static int cmd_w(char *args) {
  return -1;
}

static int cmd_d(char *args) {
  return -1;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Pause the program after execute N instructions in sigle step, if N is not defined, default 1", cmd_si },
  { "info", "Print status of registers or watchpoint information", cmd_info },
  { "p", "Calculate value of the expression EXPR", cmd_p},
  { "x", "Calculate value of the expression EXPR, use the result as a starting memory address, output N numbers of 4 bytes continuously in hexadecimal", cmd_x},
  { "w", "Pause the program when the value of expression EXPR change", cmd_w},
  { "d", "Delete the watchpoint with number N", cmd_d},

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

// Pause the program after execute N instructions.
static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  uint64_t step = 0;
  int i;
  
  if (arg == NULL) {
    // Default value is 1 if user do not give a parameter.
    cpu_exec(1);
  } else {
    // Check whether the input string is valid.
    for (i = 0; i < strlen(arg); i++) {
      if (arg[i] > '9' || arg[i] < '0') {
        step = 0;
        break;
      }
      else {
        // Convert arg into int64.
        step = 10 * step + arg[i] - '0';
      }
    }
    if (step == 0) {
      printf("Invalid instruction '%s'\n", arg);
    } else {
      cpu_exec(step);
      printf("Step '%ld' instructions\n", step);
      printf("TODO: Current instruction is\n");
    }
  }

  return 0;
}

// Print the status of register and watchpoints.
static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    printf("Command 'info' need a parameter 'r' or 'w'\n");
  } else if (strcmp("r", arg) == 0) {
    // Print register.
    printf("eax\t0x%x\t%d\n", cpu.eax, cpu.eax);
    printf("ecx\t0x%x\t%d\n", cpu.ecx, cpu.ecx);
    printf("edx\t0x%x\t%d\n", cpu.edx, cpu.edx);
    printf("ebx\t0x%x\t%d\n", cpu.ebx, cpu.ebx);
    printf("esp\t0x%x\t%d\n", cpu.esp, cpu.esp);
    printf("ebp\t0x%x\t%d\n", cpu.ebp, cpu.ebp);
    printf("esi\t0x%x\t%d\n", cpu.esi, cpu.esi);
    printf("edi\t0x%x\t%d\n", cpu.edi, cpu.edi);
  } else if (strcmp("w", arg)) {
    // Print watchpoints.
  } else {
    printf("Invalid command '%s'\n", arg);
  }

  return 0;
}

// Scan memory.
static int cmd_x(char *args) {
  char *arg1 = strtok(NULL, " ");
  char *arg2 = strtok(NULL, " ");
  int i, j;

  int length = atoi(arg1);
  int memory_addr = (int)strtol(arg2, NULL, 0);

  for (i = 0; i < length; i++) {
    printf("%8x:\t", memory_addr + i * 4);
    for (j = 0; j < 4; j++) {
      printf("%.2x ", pmem[memory_addr + i * 4 + j]);
    }
    printf("\n");
  }

  return 0;
}

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  } else {
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

void ui_mainloop(int is_batch_mode) {
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

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
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
