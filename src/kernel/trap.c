#include "gate.h"
#include "trap.h"
#include "printk.h"

void divide_error() {
	color_printk(RED, YELLOW, "You divided the zero!");
	while(1);
}

void debug_error() {
	return;
}

void nmi() {
	return;
}

void int3() {
	return;
}

void overflow() {
	return;
}

void bounds() {
	return;
}

void undefined_opcode() {
	return;
}

void dev_not_availabel() {
	return;
}

void double_fault() {
	return;
}

void coprocessor_segment_overrun() {
	return;
}

void invalid_TSS() {
	return;
}

void segment_not_present() {
	return;
}

void stack_segment_fault() {
	return;
}

void general_protection() {
	return;
}

void page_fault() {
	return;
}

void x87_FPU_error() {
	return;
}

void alignment_check() {
	return;
}

void machine_check() {
	return;
}

void SIMD_exception() {
	return;
}

void virtualization_exception() {
	return;
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
