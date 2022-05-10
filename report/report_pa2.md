## Big Endian and Little Endian

Q: Motorola 68k系列的处理器都是大端架构的. 现在问题来了, 考虑以下两种情况:
1. 假设我们需要将NEMU运行在Motorola 68k的机器上(把NEMU的源代码编译成Motorola 68k的机器码)
2. 假设我们需要把Motorola 68k作为一个新的ISA加入到NEMU中

A: 需要考虑两种情况下**数据存储的位置在哪**，又是**谁将定义数据的代码转化为内存中的值** 
1. NEMU中的定义常数存储在真正的物理内存中。所以将这一行代码转化为内存操作的指令是NEMU的**编译器**完成的，故只要改变NEMU的编译器为适配Motorola 68k的即可，我们编写的代码**不需要任何改动**。
2. 若NEMU上运行Motorola 68k的target，此时target中定义的常数表面上是保存到了NEMU模拟的内存，也就是定义的一个大数组当中（虽然最终都是存到物理内存上）。但是很重要的区别是：**从定义常量的代码到指令写入模拟内存的部分是NEMU实现的**，也就是需要我们进行大小端的正确识别。也就是说在模拟内存的层面上已经转换为正确的格式了，模拟内存映射到物理内存这一步到底怎么样是由编译器、由NEMU运行在何种架构来决定的。



## Set up
### Decode
```c
// decode.h
#define def_DHelper(name) void concat(decode_, name) (DecodeExecState *s)
// local-include/decode.h
#define def_DopHelper(name) void concat(decode_op_, name) (DecodeExecState *s, Operand *op, bool load_val)

// local-include/decode.h
static inline def_DHelper(A) {
    // todo
    decode_op_SI(s, id_dest, false);
    s->jmp_pc = id_dest->simm + s->seq_pc;
}
static inline def_DHelper(push_SI) {
  decode_op_SI(s, id_dest, true);
}
/* sign immediate */
static inline def_DopHelper(SI) {
  assert(op->width == 1 || op->width == 4);

  /* TODO: Use instr_fetch() to read `op->width' bytes of memory
   * pointed by 's->seq_pc'. Interpret the result as a signed immediate,
   * and call `operand_imm()` as following.
   *
   operand_imm(s, op, load_val, ???, op->width);
   */
    sword_t simm = instr_fetch(&s->seq_pc, op->width);
    operand_imm(s, op, load_val, simm, op->width);
}
static inline void operand_imm(DecodeExecState *s, Operand *op, bool load_val, word_t imm, int width) {
  op->type = OP_TYPE_IMM;
  op->imm = imm;
  if (load_val) {
    rtl_li(s, &op->val, imm);
    op->preg = &op->val;
  }
  print_Dop(op->str, OP_STR_SIZE, "$0x%x", imm);
}

// pseudo.h
static inline def_rtl(li, rtlreg_t* dest, const rtlreg_t imm) {
  rtl_addi(s, dest, rz, imm);
}
```

### Execute
```c
//rtl.h
#define def_rtl(name, ...) void concat(rtl_, name)(DecodeExecState *s, __VA_ARGS__)

// rtl-basic.h
static inline def_rtl(j, vaddr_t target) {
  s->jmp_pc = target;
  s->is_jmp = true;
}

static inline def_EHelper(jmp) {
  // the target address is calculated at the decode stage
  rtl_j(s, s->jmp_pc);

  print_asm("jmp %x", s->jmp_pc);
}
```
