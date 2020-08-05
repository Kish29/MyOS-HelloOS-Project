/**************************************************
 * jar entry.S 为中断和异常添加现场执行情况保存工作
 * 是各种中断和异常的入口
 *************************************************/

#include "linkage.h"

/*64位下，寄存器占用8B*/
/*按照x86-64的IA-32e模式下定义的各个寄存器或变量相对于异常栈栈顶的偏移量*/
/*
 +---+
 |   |  高地址
 |SS |  注意先压入原栈环境，再压入代码段
 |RSP|
 |EFLAGS|
 |CS |
 |RIP|
 | · |
 | · | 低地址
 |R14|
 |R15| <-rsp
 +---+
*/


R15 = 0x00
R14 = 0x08
R13 = 0x10
R12 = 0x18
R11 = 0x20
R10 = 0x28
// 这儿其实是按照参数的入栈顺序来的
R9  = 0x30
R8  = 0x38
RBX = 0x40
RCX = 0x48
RDX = 0x50
RSI = 0x58
RDI = 0x60
RBP = 0x68

DS  = 0x70
ES  = 0x78
RAX = 0x80
// fuc和errorcode必须手动弹出栈
FUNC = 0x88
ERRORCODE = 0x90
// iretq会自行判断还原过程是否涉及特权级切换过程
// 如果是, 就将OLDRSP和OLDSS弹出
RIP = 0x98
CS  = 0xa0
RFLAGS = 0xa8
OLDRSP = 0xb0
OLDSS  = 0xb8


/* 定义还原模块 */
RESTORE_ALL:
	popq	%r15
	popq	%r14
	popq	%r13 
	popq	%r12 
	popq	%r11 
	popq	%r10 
	popq	%r9 
	popq	%r8
	popq	%rbx 
	popq	%rcx 
	popq	%rdx 
	popq	%rsi 
	popq	%rdi 
	popq	%rbp 
	popq	%rax 
	movq	%rax,	%ds 
	popq	%rax 
	movq	%rax,	%es 
	popq	%rax 
	addq	$0x10,	%rsp  //手动将func和errorcode弹出栈
	iretq

ret_from_exception:
	/*以后会添加进程调度、进程信号处理等工作*/
ENTRY(ret_from_itrpt)
	jmp	RESTORE_ALL  /* need rewrite */

/*异常处理时，处理先自动压入原ss、sp，flags、cs、ip寄存器的值*/
ENTRY(divide_error)
	//除零错误没有错误码，含有错误码的异常处理器自动压栈
	pushq	$0			/* errorcode 这儿压入0占位*/
	pushq	%rax 
	leaq	do_divide_error(%rip),	%rax 
	xchgq	%rax,	(%rsp)			/*与栈顶交换数据，此举不仅压入了func的地址，还取回了rax原先的值*/

// 异常处理前的通用现场保护部分, 其他异常/中断只需要设置自己的错误码和func地址跳转到这儿执行就可以了
error_code_prefix:
	pushq	%rax 
	
	movq	%es,	%rax 
	pushq	%rax 
	movq	%ds,	%rax	
	pushq	%rax 
	xorq	%rax,	%rax 

	pushq	%rbp 
	pushq	%rdi 
	pushq	%rsi 
	pushq	%rdx 
	pushq	%rcx 
	pushq	%rbx
	pushq	%r8 
	pushq	%r9 
	pushq	%r10 
	pushq	%r11 
	pushq	%r12 
	pushq	%r13 
	pushq	%r14 
	pushq	%r15 

	cld 
	movq	ERRORCODE(%rsp),	%rsi		// 错误码传入处理函数的第二个参数
	movq	FUNC(%rsp),		%rdx 

	/* 考虑到不同特权级之间的切换，异常处理需要在内核中执行，如应用层(3)发生中断
	 * 切入到内核层中时，需要从新设置es、ds 
	 */
	movq	$0x10,	%rax 
	movq	%rax,	%es 
	movq	%rax,	%ds 

	movq	%rsp,	%rdi					// 栈顶指针传入处理函数的第一个参数 

	callq	*%rdx				// AT&T汇编中，跳转前'*'代表使用绝对地址

	jmp	ret_from_exception

ENTRY(debug_error)
	pushq	$0
	pushq	%rax 
	leaq	do_debug_error(%rip),	%rax 
	xchgq	%rax,	(%rsp)
	jmp	error_code_prefix 

// 不可屏蔽中断，一般是硬件出了问题，要特殊处理
ENTRY(nmi)
	pushq	%rax 
	cld 
	pushq	%rax 

	pushq	%rax 

	movq	%es,	%rax 
	pushq	%rax 
	movq	%ds,	%rax 
	pushq	%rax 
	xorq	%rax,	%rax

	pushq	%rbp 
	pushq	%rdi 
	pushq	%rsi 
	pushq	%rdx 
	pushq	%rcx 
	pushq	%rbx	// 漏了rbx，补上。。。。。
	pushq	%r8 
	pushq	%r9 
	pushq	%r10 
	pushq	%r11 
	pushq	%r12 
	pushq	%r13 
	pushq	%r14 
	pushq	%r15 

	movq	$0x10,	%rax 
	movq	%es,	%rax 
	movq	%ds,	%rax 

	movq	$0,		%rsi		// error_code = 0
	movq	%rsp,	%rdi 

	callq	do_nmi 

	jmp	RESTORE_ALL 

ENTRY(int3)
	pushq	$0
	pushq	%rax 
	leaq	do_int3(%rip),	%rax 
	xchgq	%rax,	(%rsp)
	
	jmp	error_code_prefix 

ENTRY(overflow)
	pushq	$0
	pushq	%rax 
	leaq	do_overflow(%rip),	%rax 
	xchgq	%rax,	(%rsp)

	jmp	error_code_prefix 

ENTRY(bounds)
	pushq	$0
	pushq	%rax 
	leaq	do_bounds(%rip),	%rax 
	xchgq	%rax,	(%rsp)

	jmp	error_code_prefix 

ENTRY(undefined_opcode)
	pushq	$0
	pushq	%rax 
	leaq	do_undefined_opcode(%rip),	%rax 
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(dev_not_availabel)
	pushq	$0
	pushq	%rax	
	leaq	do_dev_not_availabel(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

// 注意，含有错误码的异常，处理器自动将错误码压入栈中
ENTRY(double_fault)
	pushq	%rax	
	leaq	do_double_fault(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(coprocessor_segment_overrun)
	pushq	$0
	pushq	%rax	
	leaq	do_coprocessor_segment_overrun(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(invalid_TSS)
	pushq	%rax	
	leaq	do_invalid_TSS(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(segment_not_present)
	pushq	%rax	
	leaq	do_segment_not_present(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(stack_segment_fault)
	pushq	%rax	
	leaq	do_stack_segment_fault(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(general_protection)
	pushq	%rax	
	leaq	do_general_protection(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(page_fault)
	pushq	%rax	
	leaq	do_page_fault(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(x87_FPU_error)
	pushq	$0
	pushq	%rax	
	leaq	do_x87_FPU_error(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(alignment_check)
	pushq	%rax	
	leaq	do_alignment_check(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(machine_check)
	pushq	$0
	pushq	%rax	
	leaq	do_machine_check(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(SIMD_exception)
	pushq	$0
	pushq	%rax	
	leaq	do_SIMD_exception(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 

ENTRY(virtualization_exception)
	pushq	$0
	pushq	%rax	
	leaq	do_virtualization_exception(%rip),	%rax
	xchgq	%rax,	(%rsp)

	jmp error_code_prefix 
























