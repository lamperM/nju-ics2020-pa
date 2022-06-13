# Big Endian and Little Endian

Q: Motorola 68k系列的处理器都是大端架构的. 现在问题来了, 考虑以下两种情况:
1. 假设我们需要将NEMU运行在Motorola 68k的机器上(把NEMU的源代码编译成Motorola 68k的机器码)
2. 假设我们需要把Motorola 68k作为一个新的ISA加入到NEMU中

A: 需要考虑两种情况下**数据存储的位置在哪**，又是**谁将定义数据的代码转化为内存中的值** 
1. NEMU中的定义常数存储在真正的物理内存中。所以将这一行代码转化为内存操作的指令是NEMU的**编译器**完成的，故只要改变NEMU的编译器为适配Motorola 68k的即可，我们编写的代码**不需要任何改动**。
2. 若NEMU上运行Motorola 68k的target，此时target中定义的常数表面上是保存到了NEMU模拟的内存，也就是定义的一个大数组当中（虽然最终都是存到物理内存上）。但是很重要的区别是：**从定义常量的代码到指令写入模拟内存的部分是NEMU实现的**，也就是需要我们进行大小端的正确识别。也就是说在模拟内存的层面上已经转换为正确的格式了，模拟内存映射到物理内存这一步到底怎么样是由编译器、由NEMU运行在何种架构来决定的。


# 实现命令


## RTL(Register Transfer Language)

### RTL指令
在NEMU中, RTL指令分为两种
#### 1. RTL基本指令
定义在`nemu/src/engine/interpreter/rtl-basic.h`中,

RTL基本指令的特点是**不需要使用临时寄存器**. 是ISA无关的代码

#### 2. RTL伪指令
RTL伪指令即包含ISA无关的, 也有ISA相关的

ISA无关的伪指令定义在 `nemu/include/rtl/pseudo.h`  
ISA相关的伪指令定义在 `nemu/src/isa/x86/local-include/rtl.h`

RTL伪指令是需要使用临时寄存器的, 所以我们约定在实现RTL伪指令时, 就尽量不要使用临时寄存器.
例如, 如果下面的`rtl_sext`的实现中使用了`t0`, 那么最终的结果就会改变
```c
rtl_mv(s, t0, s1);
rtl_sext(s, s1, s0, 1);
rtl_add(s, s0, t0, s2);
```

所以, 我们约定:
* `dest`: 实现 RTL伪指令 尽量优先使用
* `t0`,`t1`: only for RTL伪指令 implementation
* `s0`,`s1`: only for RTL伪指令 invoking parameter



## 执行命令的流程

`cpu_exec()`模拟了CPU的工作方式: 不断的执行指令.

`isa_exec_once()`(在`nemu/src/isa/$ISA/exec/exec.c`中定义) 让CPU执行当前PC指向的一条指令, 然后更新PC
### Set up
#### Decode
```c
// decode.h
#define def_DHelper(name) void concat(decode_, name) (DecodeExecState *s)
// local-include/decode.h
#define def_DopHelper(name) void concat(decode_op_, name) (DecodeExecState *s, Operand *op, bool load_val)

// local-include/decode.h
static inline void operand_reg(DecodeExecState *s, Operand *op, bool load_val, int r, int width) {
  op->type = OP_TYPE_REG;
  op->reg = r;

  if (width == 4) {
    op->preg = &reg_l(r);
  } else {
    assert(width == 1 || width == 2);
    op->preg = &op->val;
    if (load_val) rtl_lr(s, &op->val, r, width);
  }

  print_Dop(op->str, OP_STR_SIZE, "%%%s", reg_name(r, width));
}

// local-include/decode.h
static inline void operand_imm(DecodeExecState *s, Operand *op, bool load_val, word_t imm, int width) {
  op->type = OP_TYPE_IMM;
  op->imm = imm;
  if (load_val) {
    rtl_li(s, &op->val, imm);
    op->preg = &op->val;
  }
  print_Dop(op->str, OP_STR_SIZE, "$0x%x", imm);
}
```

#### Execute
```c
//rtl.h
#define def_rtl(name, ...) void concat(rtl_, name)(DecodeExecState *s, __VA_ARGS__)

// rtl-basic.h
static inline def_rtl(j, vaddr_t target) {
  s->jmp_pc = target;
  s->is_jmp = true;
}

//x86/exec/control.h
static inline def_EHelper(jmp) {
  // the target address is calculated at the decode stage
  rtl_j(s, s->jmp_pc);

  print_asm("jmp %x", s->jmp_pc);
}
```

&nbsp;
## call 
### Decode  

### Execute
```c
// control.h
static inline def_EHelper(call) {
  // push return address to stack
  rtl_li(s, ddest, s->seq_pc);
  rtl_push(s, ddest);

  // the target address is calculated at the decode stage
  rtl_j(s, s->jmp_pc);
  
  print_asm("call %x", s->jmp_pc);
}
// pseduo.h - mov imm to reg
static inline def_rtl(li, rtlreg_t* dest, const rtlreg_t imm) {
  rtl_addi(s, dest, rz, imm);
}

//local-include/rtl.h
static inline def_rtl(push, const rtlreg_t* src1) {
  // esp <- esp - 4
  cpu.esp -= 4;
  // M[esp] <- src1
  rtl_sm(s, &(cpu.esp), 0, src1, s->dest.width);
}

// engine/interpreter/rtl-basic.h
static inline def_rtl(j, vaddr_t target) {
  s->jmp_pc = target;
  s->is_jmp = true;
}

```
## push
#### Decode(imm32)
```c
// local-include/decode.h
/* Ib, Iv */
static inline def_DopHelper(I) {
  /* pc here is pointing to the immediate */
  word_t imm = instr_fetch(&s->seq_pc, op->width);
  operand_imm(s, op, load_val, imm, op->width);
}

static inline def_DHelper(I) {
  decode_op_I(s, id_dest, true);
}
```
#### Decode(reg32)
```c
// local-include/decode.h
static inline def_DopHelper(r) {
  operand_reg(s, op, load_val, s->opcode & 0x7, op->width);
}

static inline def_DHelper(r) {
  decode_op_r(s, id_dest, true);
}
```
#### Execute
```c
// x86/exec/data-mov.h
static inline def_EHelper(push) {
  // target data is stored in s->dest at the decode stage
  rtl_push(s, ddest);
  print_asm_template1(push);
}

// control.h
static inline def_rtl(push, const rtlreg_t* src1) {
  // esp <- esp - 4
  cpu.esp -= 4;
  // M[esp] <- src1
  rtl_sm(s, &(cpu.esp), 0, src1, s->dest.width);
}
```

&nbsp;
## sub
### Decode 
```c
/* Eb, Ew, Ev
 * Gb, Gv
 * Cd,
 * M
 * Rd
 * Sw
 */
// nemu/src/isa/x86/local-include/decode.h 
static inline void operand_rm(DecodeExecState *s, Operand *rm, bool load_rm_val, Operand *reg, bool load_reg_val) {
  read_ModR_M(s, rm, load_rm_val, reg, load_reg_val);
}

```

