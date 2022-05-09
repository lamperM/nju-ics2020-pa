# ICS2020 Exprimental Eeport of PA1

## RTFM: 尝试查阅ISA手册
### EFLAGS寄存器中的CF位是什么意思?
CF: Carry Flag. 进位标志.  

### ModR/M字节是什么?
ModR/M字节是X86指令集中指令编码的重要部分.  
X86指令集中操作码通常是一个字节，ModR/M字节跟在操作码的后面指明该指令执行的一些额外信息.  
[ModeR/m](https://wiki.osdev.org/X86-64_Instruction_Encoding#ModR.2FM) 

### mov指令的具体格式是怎么样的?
mov指令可以方便地在以下路径中交换数据:
1. 内存到寄存器
2. 寄存器到内存
3. 通用寄存器之间
4. 为寄存器赋值立即数
5. 为内存赋值立即数  

mov指令**不支持**从内存到内存(可能要使用movs)，或者从段寄存器到段寄存器.  
[mov指令格式](https://nju-projectn.github.io/i386-manual/MOV.htm)

## shell命令 - 统计代码行数
统计当前目录下的所有.c和.h文件的行数，且去除空行，不显示文件名:
```shell
(find ./ -name '*.[c|h]' -print0 | xargs -0 cat) | sed '/^\s*$$/d' | wc -l 
```
在PA0分支下运行，结果是4728.  
在PA1分支下运行，结果是5373. :)  

## RTFM - '-Wall -Werror'
* -Werror: Make all warnings into errors.  
* -Wall: Turn on all warning flags.  

这样做能够保证代码不因为某些Warning造成潜在的错误.



