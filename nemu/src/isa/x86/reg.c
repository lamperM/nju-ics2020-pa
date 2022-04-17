#include <isa.h>
#include <stdlib.h>
#include <time.h>
#include "local-include/reg.h"
#include <ctype.h>

const char *regsl[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
const char *regsw[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char *regsb[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

void reg_test() {
  srand(time(0));
  word_t sample[8];
  word_t pc_sample = rand();
  cpu.pc = pc_sample;

  int i;
  for (i = R_EAX; i <= R_EDI; i ++) {
    sample[i] = rand();
    reg_l(i) = sample[i];
    assert(reg_w(i) == (sample[i] & 0xffff));
  }

  assert(reg_b(R_AL) == (sample[R_EAX] & 0xff));
  assert(reg_b(R_AH) == ((sample[R_EAX] >> 8) & 0xff));
  assert(reg_b(R_BL) == (sample[R_EBX] & 0xff));
  assert(reg_b(R_BH) == ((sample[R_EBX] >> 8) & 0xff));
  assert(reg_b(R_CL) == (sample[R_ECX] & 0xff));
  assert(reg_b(R_CH) == ((sample[R_ECX] >> 8) & 0xff));
  assert(reg_b(R_DL) == (sample[R_EDX] & 0xff));
  assert(reg_b(R_DH) == ((sample[R_EDX] >> 8) & 0xff));

  assert(sample[R_EAX] == cpu.eax);
  assert(sample[R_ECX] == cpu.ecx);
  assert(sample[R_EDX] == cpu.edx);
  assert(sample[R_EBX] == cpu.ebx);
  assert(sample[R_ESP] == cpu.esp);
  assert(sample[R_EBP] == cpu.ebp);
  assert(sample[R_ESI] == cpu.esi);
  assert(sample[R_EDI] == cpu.edi);

  assert(pc_sample == cpu.pc);
}

word_t isa_reg_str2val(const char *s, bool *success);
void isa_reg_display() {
    printf("register display\n");

    for (int i = 0; i < 8; i++) {
        printf("  %s: 0x%08X\n", regsl[i], cpu.gpr[i]._32);
    }
    printf("\n");

    //`for (int i = 0; i < 8; i++) {
    //    printf("  %s: 0x%X\n", regsw[i], cpu.gpr[i]._16);
    //}
    //printf("\n");

    printf("pc: 0x%08X\n",cpu.pc);

    bool success;
    for (int i = 0; i < 8; i++) 
        printf("%s: 0x%08x\n", regsl[i], isa_reg_str2val(regsl[i], &success));
    for (int i = 0; i < 8; i++) 
        printf("%s: 0x%08x\n", regsw[i], isa_reg_str2val(regsw[i], &success));
    for (int i = 0; i < 4; i++) {
        printf("%s: 0x%08x  %s: 0x%08x\n", regsb[i], isa_reg_str2val(regsb[i], &success), 
                                           regsb[i + 4], isa_reg_str2val(regsb[i + 4], &success));
    }

    printf("pc: 0x%08x\n",isa_reg_str2val("pc", &success)); 
    
}

word_t isa_reg_str2val(const char *s, bool *success) {
    int len = strlen(s);
    char s_dup[len + 1]; // c99

    *success = true;
    memset(s_dup, 0, len);
    strcpy(s_dup, s);
    for (int i = 0; i < len; i++) 
        s_dup[i] = tolower(s_dup[i]);

    /* pc is special, check first */
    if (strcmp(s_dup, "pc") == 0) {
        return cpu.pc;
    }
    for (int i = 0; i < 8; i++) {
        if (strcmp(s_dup, regsl[i]) == 0) 
            return cpu.gpr[i]._32;
        if (strcmp(s_dup, regsw[i]) == 0)
            return cpu.gpr[i]._16;
        if (strcmp(s_dup, regsb[i]) == 0) {
            int base = i % 4;
            int offset = i / 4;
            return cpu.gpr[base]._8[offset];
        }
    }

    *success = false;
    return 0;
}
