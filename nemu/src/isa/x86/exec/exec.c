#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "all-instr.h"

static inline void set_width(DecodeExecState *s, int width) {
  if (width == -1) return;
  if (width == 0) {
    width = s->isa.is_operand_size_16 ? 2 : 4;
  }
  s->src1.width = s->dest.width = s->src2.width = width;
}

/* 0x80, 0x81, 0x83 */
static inline def_EHelper(gp1) {
  switch (s->isa.ext_opcode) {
      case 0:
          // ADD
          rtl_addi(s, ddest, ddest, id_src1->simm);
          print_asm_template2(add);
          break;
      case 4: 
          // AND
          rtl_andi(s, ddest, ddest, s->src1.simm);
          print_asm_template2(and);
          break;
      case 5:
          // SUB
          rtl_subi(s, ddest, ddest, s->src1.simm);
          rtl_update_ZFSF(s, ddest, s->dest.width); 
          print_asm_template2(sub);
          break;
    EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(6)
      case 7:
          exec_cmp(s);
          break;
          

  }
}

/* 0xc0, 0xc1, 0xd0, 0xd1, 0xd2, 0xd3 */
static inline def_EHelper(gp2) {
  switch (s->isa.ext_opcode) {
      case 5:
          exec_shr(s);
          break;
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(6) EMPTY(7)
  }
}

/* 0xf6, 0xf7 */
static inline def_EHelper(gp3) {
  switch (s->isa.ext_opcode) {
      case 3: // NEG
          rtl_neg(s, ddest, ddest);
          break;
    EMPTY(0) EMPTY(1) EMPTY(2) 
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
  }
}

/* 0xfe */
static inline def_EHelper(gp4) {
  switch (s->isa.ext_opcode) {
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
  }
}

/* 0xff */
static inline def_EHelper(gp5) {
  switch (s->isa.ext_opcode) {
      case 6:
          // PUSHL
          rtl_push(s, ddest);
          print_asm_template1(push);
          break;
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(7)
  }
}

/* 0x0f 0x01*/
static inline def_EHelper(gp7) {
  switch (s->isa.ext_opcode) {
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
  }
}


static inline def_EHelper(2byte_esc) {
  uint8_t opcode = instr_fetch(&s->seq_pc, 1);
  s->opcode = opcode;
  switch (opcode) {
  /* TODO: Add more instructions!!! */
    IDEX (0x01, gp7_E, gp7)
    IDEX (0x94, setcc_E, setcc)
    IDEX (0x95, setcc_E, setcc)
    IDEX (0xb6, mov_E2G, movzx) // 不能打印al 和 eax, 因为 set_width() 中统一设置的 
    IDEX (0xbe, mov_E2G, movsx); // add by wanglu 7.7
    default: exec_inv(s);
  }
}

static inline void fetch_decode_exec(DecodeExecState *s) {
  uint8_t opcode;
again:
  opcode = instr_fetch(&s->seq_pc, 1);
  s->opcode = opcode;
  switch (opcode) {
    EX   (0x0f, 2byte_esc)
    IDEX (0x29, G2E, sub)
    IDEX (0x31, G2E, xor) // xor(31)
    IDEXW(0x38, G2E, cmp, 1)
    IDEX (0x40, r, inc)
    IDEX (0x41, r, inc)
    IDEX (0x42, r, inc)
    IDEX (0x43, r, inc)
    IDEX (0x44, r, inc)
    IDEX (0x45, r, inc)
    IDEX (0x46, r, inc)
    IDEX (0x50, r, push) /* push(EAX) */
    IDEX (0x51, r, push) /* push(ECX) */
    IDEX (0x52, r, push) /* push(EDX) */
    IDEX (0x53, r, push) /* push(EBX) */
    IDEX (0x54, r, push) /* push(ESP) */
    IDEX (0x55, r, push) /* push(EBP) */
    IDEX (0x56, r, push) /* push(ESI) */
    IDEX (0x57, r, push) /* push(EDI) */
    IDEX (0x58, r, pop)  /* pop(EAX) */
    IDEX (0x59, r, pop)  /* pop(ECX) */
    IDEX (0x5a, r, pop)  /* pop(EDX) */
    IDEX (0x5b, r, pop)  /* pop(EBX) */
    IDEX (0x5c, r, pop)  /* pop(ESP) */
    IDEX (0x5d, r, pop)  /* pop(EBP) */
    IDEX (0x5e, r, pop)  /* pop(ESI) */
    IDEX (0x68, I, push) /* push(imm32) */
    IDEXW(0x6a, I, push, 1) /* push(imm8) */
    IDEXW(0x74, J, jcc, 1) /* jcc */
    IDEXW(0x75, J, jcc, 1) /* jcc */
    IDEXW(0x80, I2E, gp1, 1)
    IDEX (0x81, I2E, gp1)
    IDEX (0x83, SI2E, gp1) // sub, and
    IDEXW(0x84, G2E, test, 1);
    IDEX (0x85, G2E, test) // test
    IDEXW(0x88, mov_G2E, mov, 1)
    IDEX (0x89, mov_G2E, mov)
    IDEXW(0x8a, mov_E2G, mov, 1)
    IDEX (0x8b, mov_E2G, mov)
    IDEX (0x8d, lea_M2G, lea)
    IDEXW(0xa0, O2a, mov, 1)
    IDEX (0xa1, O2a, mov)
    IDEXW(0xa2, a2O, mov, 1)
    IDEX (0xa3, a2O, mov)
    IDEXW(0xb0, mov_I2r, mov, 1)
    IDEXW(0xb1, mov_I2r, mov, 1)
    IDEXW(0xb2, mov_I2r, mov, 1)
    IDEXW(0xb3, mov_I2r, mov, 1)
    IDEXW(0xb4, mov_I2r, mov, 1)
    IDEXW(0xb5, mov_I2r, mov, 1)
    IDEXW(0xb6, mov_I2r, mov, 1)
    IDEXW(0xb7, mov_I2r, mov, 1)
    IDEX (0xb8, mov_I2r, mov)
    IDEX (0xb9, mov_I2r, mov)
    IDEX (0xba, mov_I2r, mov)
    IDEX (0xbb, mov_I2r, mov)
    IDEX (0xbc, mov_I2r, mov)
    IDEX (0xbd, mov_I2r, mov)
    IDEX (0xbe, mov_I2r, mov)
    IDEX (0xbf, mov_I2r, mov)
    IDEXW(0xc0, gp2_Ib2E, gp2, 1)
    IDEX (0xc1, gp2_Ib2E, gp2)
    EX   (0xc3, ret)
    IDEXW(0xc6, mov_I2E, mov, 1)
    IDEX (0xc7, mov_I2E, mov)
    EX   (0xc9, leave) // leave 
    IDEXW(0xd0, gp2_1_E, gp2, 1)
    IDEX (0xd1, gp2_1_E, gp2)
    IDEXW(0xd2, gp2_cl2E, gp2, 1)
    IDEX (0xd3, gp2_cl2E, gp2)
    EX   (0xd6, nemu_trap)
    IDEX (0xe8, A, call) /* call procedure */
    IDEXW(0xf6, E, gp3, 1)
    IDEX (0xf7, E, gp3)
    IDEXW(0xfe, E, gp4, 1)
    IDEX (0xff, E, gp5)
  case 0x66: s->isa.is_operand_size_16 = true; goto again;
  default: exec_inv(s);
  }
}

vaddr_t isa_exec_once() {
  DecodeExecState s;
  s.is_jmp = 0;
  s.isa = (ISADecodeInfo) { 0 };
  s.seq_pc = cpu.pc;

  fetch_decode_exec(&s);
  update_pc(&s);

  return s.seq_pc;
}
