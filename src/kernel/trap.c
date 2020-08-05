#include "gate.h"
#include "trap.h"
#include "printk.h"

// 按照entry.S中的参数传递，第一个rdi传入的是rsp, 第二个是错误码
void do_divide_error(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_divide_error(0), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_debug_error(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_debug_error(1), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_nmi(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_nmi(2), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_int3(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_int3(3), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_overflow(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_overflow(4), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}


void do_bounds(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_bounds(5), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_undefined_opcode(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_undefined_opcode(6), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_dev_not_availabel(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_dev_not_availabel(7), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_double_fault(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_double_fault(8), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_coprocessor_segment_overrun(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_coprocessor_segment_overrun(9), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}


// 对含有错误码，且比较重要的函数，要打印出具体的异常信息
void do_invalid_TSS(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_invalid_TSS(10), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);

	if(error_code & 0x1)
		color_printk(RED, BLACK, "The exception occured during delivery of an event external to the program, such as an interrupt or an ealier exception.\n");

	if(error_code & 0x2)
		color_printk(RED, BLACK, "Refers to a gate descriptor in the IDT.\n");
	else {
		color_printk(RED, BLACK, "Refers to a descriptor in GDT or the current LDT;\n");

		if(error_code & 0x4)
			color_printk(RED, BLACK, "Refers to a segment or gate descriptor in the current LDT.\n");
		else
			color_printk(RED, BLACK, "Refers to a descriptor in GDT.\n");
	}

	color_printk(RED, BLACK, "Segment Selector Index(offset of bytes):%#010x\n", error_code & 0xfff8); // 此处不除以8的原因是：代码段/数据段和系统段所占用的字节数不同，只能用偏移字节数表示

	while(1);
}

void do_segment_not_present(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_segment_not_present(11), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);

	if(error_code & 0x1)
		color_printk(RED, BLACK, "The exception occured during delivery of an event external to the program, such as an interrupt or an ealier exception.\n");

	if(error_code & 0x2)
		color_printk(RED, BLACK, "Refers to a gate descriptor in the IDT.\n");
	else {
		color_printk(RED, BLACK, "Refers to a descriptor in GDT or the current LDT;\n");

		if(error_code & 0x4)
			color_printk(RED, BLACK, "Refers to a segment or gate descriptor in the current LDT.\n");
		else
			color_printk(RED, BLACK, "Refers to a descriptor in GDT.\n");
	}

	color_printk(RED, BLACK, "Segment Selector Index(offset of bytes):%#010x\n", error_code & 0xfff8); // 此处不除以8的原因是：代码段/数据段和系统段所占用的字节数不同，只能用偏移字节数表示

	while(1);
}

void do_stack_segment_fault(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_stack_segment_fault(12), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);

	if(error_code & 0x1)
		color_printk(RED, BLACK, "The exception occured during delivery of an event external to the program, such as an interrupt or an ealier exception.\n");

	if(error_code & 0x2)
		color_printk(RED, BLACK, "Refers to a gate descriptor in the IDT.\n");
	else {
		color_printk(RED, BLACK, "Refers to a descriptor in GDT or the current LDT;\n");

		if(error_code & 0x4)
			color_printk(RED, BLACK, "Refers to a segment or gate descriptor in the current LDT.\n");
		else
			color_printk(RED, BLACK, "Refers to a descriptor in GDT.\n");
	}

	color_printk(RED, BLACK, "Segment Selector Index(offset of bytes):%#010x\n", error_code & 0xfff8); // 此处不除以8的原因是：代码段/数据段和系统段所占用的字节数不同，只能用偏移字节数表示

	while(1);
}

void do_general_protection(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_general_protection(13), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);

	if(error_code & 0x1)
		color_printk(RED, BLACK, "The exception occured during delivery of an event external to the program, such as an interrupt or an ealier exception.\n");

	if(error_code & 0x2)
		color_printk(RED, BLACK, "Refers to a gate descriptor in the IDT.\n");
	else {
		color_printk(RED, BLACK, "Refers to a descriptor in GDT or the current LDT;\n");

		if(error_code & 0x4)
			color_printk(RED, BLACK, "Refers to a segment or gate descriptor in the current LDT.\n");
		else
			color_printk(RED, BLACK, "Refers to a descriptor in GDT.\n");
	}

	color_printk(RED, BLACK, "Segment Selector Index(offset of bytes):%#010x\n", error_code & 0xfff8); // 此处不除以8的原因是：代码段/数据段和系统段所占用的字节数不同，只能用偏移字节数表示

	while(1);
}


/* 页错误十分重要！
 * 所以处理器给页错误的错误码也与其他普通的错误码不同(错误码16/32位，2/4B)
 * 0～4为5个标志位
 * */
void do_page_fault(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	
	// 页错误发生时，cr2寄存器保存页错误的 **线性地址**
	// 页错误异常程序应当保存当前的引发页错误的线性地址，防止重复在此处引发异常
	unsigned long cr2 = 0;
	__asm__	__volatile__	("movq	%%cr2,	%0":"=r"(cr2)::"memory");	// 获取出错的页线性地址

	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_page_fault(14), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);

	// bit-1: P
	if((error_code & 0x1) == 0)
		color_printk(RED, BLACK, "Page Not-Present,\t");
	else 
		color_printk(RED, BLACK, "Page Protect-Exception:\t");

	// bit-2: W/R
	if(error_code & 0x2)
		color_printk(RED, BLACK, "Write Cause Fault,\t");
	else 
		color_printk(RED, BLACK, "Read Cause Fault,\t");

	// bit-3: U/S
	if(error_code & 0x4)
		color_printk(RED, BLACK, "Fault in user privilege(3),\t");
	else 
		color_printk(RED, BLACK, "Fault in supervisor privilege(0,1,2).");

	// bit-4: PSVD
	if(error_code & 0x8)
		color_printk(RED, BLACK, "\tUse reserved-bit cause Fault.");

	// bit-5: I/D (当fetch指令发生错误时，该位会被置位)
	if(error_code & 0x10)
		color_printk(RED, BLACK, "\tInstruction fetch cause Fault.");
	
	color_printk(RED, BLACK, "\n");

	color_printk(RED, BLACK, "CR2:%018lx\n", cr2);

	while(1);
}

/* 15 号 Intel保留
 * */

void do_x87_FPU_error(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_x87_FPU_error(16), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_alignment_check(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_alignment_check(17), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_machine_check(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_machine_check(18), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_SIMD_exception(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_SIMD_exception(19), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void do_virtualization_exception(unsigned long rsp, unsigned long error_code) {
	unsigned long *p = NULL;
	p = (unsigned long *)(rsp + 0x98);			// 按照entry.S中的定义，rsp向上偏移0x98个字节是rip的值
	color_printk(RED, BLACK, "do_virtualization_exception(20), ERROR_CODE:%#018lx, rsp:%#018lx, rip:%#018lx\n", error_code, rsp, *p);
	while(1);
}

void sys_idt_vector_init() {
	// 带有'system'的函数表示可由应用层处理的异常
	set_trap_gate(0, 1, divide_error);
	set_trap_gate(1, 1, debug_error);
	set_itrpt_gate(2, 1, nmi);
	set_system_trap_gate(3, 1, int3);
	set_system_trap_gate(4, 1, overflow);
	set_system_trap_gate(5, 1, bounds);
	set_trap_gate(6, 1, undefined_opcode);
	set_trap_gate(7, 1, dev_not_availabel);
	set_trap_gate(8, 1, double_fault);
	set_trap_gate(9, 1, coprocessor_segment_overrun);
	set_trap_gate(10, 1, invalid_TSS);
	set_trap_gate(11, 1, segment_not_present);
	set_trap_gate(12, 1, stack_segment_fault);
	set_trap_gate(13, 1, general_protection);
	set_trap_gate(14, 1, page_fault);
	// index-15 Intel 保留，不能使用
	set_trap_gate(16, 1, x87_FPU_error);
	set_trap_gate(17, 1, alignment_check);
	set_trap_gate(18, 1, machine_check);
	set_trap_gate(19, 1, SIMD_exception);
	set_trap_gate(20, 1, virtualization_exception);
}
