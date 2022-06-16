#include "cc.h"

static inline def_EHelper(test) {
  TODO();
  print_asm_template2(test);
}

static inline def_EHelper(and) {
  TODO();
  print_asm_template2(and);
}

static inline def_EHelper(xor) {
    uint32_t rm_type;
    
    printf("dest optype: %d, r:%d\n", s->dest.type, s->dest.reg);
    printf("src1 optype: %d, r:%d\n", s->src1.type, s->src1.reg);
  
  rm_type = s->src1.type;
  if (rm_type == OP_TYPE_REG) {
      rtl_xor(s, ddest, ddest, dsrc1);
  } else if (rm_type == OP_TYPE_IMM) { /* 是否应该这样判断? */
      /* 如何区别simm和imm? decode时会存入哪里? */
      rtl_xori(s, ddest, ddest, s->src1.simm);
  }
  print_asm_template2(xor);
}

static inline def_EHelper(or) {
  TODO();
  print_asm_template2(or);
}

static inline def_EHelper(not) {
  TODO();
  print_asm_template1(not);
}

static inline def_EHelper(sar) {
  TODO();
  // unnecessary to update CF and OF in NEMU
  print_asm_template2(sar);
}

static inline def_EHelper(shl) {
  TODO();
  // unnecessary to update CF and OF in NEMU
  print_asm_template2(shl);
}

static inline def_EHelper(shr) {
  TODO();
  // unnecessary to update CF and OF in NEMU
  print_asm_template2(shr);
}


static inline def_EHelper(setcc) {
  uint32_t cc = s->opcode & 0xf;
  rtl_setcc(s, ddest, cc);
  operand_write(s, id_dest, ddest);

  print_asm("set%s %s", get_cc_name(cc), id_dest->str);
}
