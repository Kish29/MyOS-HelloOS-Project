/*************************
 * jar definition of gate
 * gate.h 用来宏定义描述符
 * 实现的定义
 ************************/

#ifndef __GATE_H__
#define __GATE_H__ 

/* GDT描述符，代码段和数据段描述符一个占8B
 * 系统段(IDT、TSS等)一个占16B */ 
struct seg_desc_struct {
	unsigned char desc[8];
};

// 系统段的门描述符结构，一个占16B
struct gate_desc_struct {
	unsigned char desc[16];
};

/* 使用head.S中的GDT_Table和IDT_Table、TSS64_Table
 * 并将他们声明为结构体数组*/
extern struct seg_desc_struct GDT_Table[];
extern struct gate_desc_struct IDT_Table[];
/*TSS 表总长103B*/
extern unsigned int TSS64_Table[26];	// 4 x 26 = 104B

/* IDT门描述符:
 * 127		   96 95         64 63     48   34 32 31       16         0
 * +-------------+-------------+--------+----+---+--------+----------+
   |             | 偏移地址    |偏移地址|属性|IST|段选择子|  偏移地址|
   +-----------------------------------------------------------------+ */

// 一个表项16B，__d0是低8B，__d1是高8B，"i"表示传入立即数
// "3"表示向第3个约束所用的寄存器/变量传入值
// "2"传入的是段选择子
// set gate macro IDT表索引项         属性  IST(IST栈切换机制) 中断代码地址 
#define _set_gate(gate_selector_addr, attr, IST, code_addr)				\
do {																	\
	unsigned long __d0, __d1;											\
	__asm__ __volatile__	(											\
				"movw	%%dx,	%%ax	\n\t"							\
				"andq	$0x7,	%%rcx	\n\t"							\
				"addq	%4,		%%rcx	\n\t"							\
				"shlq	$32,	%%rcx	\n\t"							\
				"addq	%%rcx,	%%rax	\n\t"							\
				"xorq	%%rcx,	%%rcx	\n\t"							\
				"movl	%%edx,	%%ecx	\n\t"							\
				"shrq	$16,	%%rcx	\n\t"							\
				"shlq	$48,	%%rcx	\n\t"							\
				"addq	%%rcx,	%%rax	\n\t"							\
				"shrq	$32,	%%rdx	\n\t"							\
				"movq	%%rax,	%0		\n\t"							\
				"movq	%%rdx,	%1		\n\t"							\
				:"=m"(*((unsigned long *)(gate_selector_addr))),		\
				 "=m"(*(1 + (unsigned long *)(gate_selector_addr))),	\
				 "=&a"(__d0),											\
				 "=&d"(__d1)											\
				:"i"(attr << 8),										\
				 "3"((unsigned long *)(code_addr)),						\
				 "2"(0x08 << 16),										\
				 "c"(IST)												\
				:"memory"												\
			);															\
}while(0)

// 注：所有的GDTR、IDTR、TR、LDTR寄存器均为内存中的一段伪描述符
/* TR寄存器结构32/64-bit，注意：不可见部分均由段选择子从GDT中加载到TR寄存器中
 * 32-bit:  12B
 * +-----------------不可见(hidden portion)----------+-----可见-----------+
 * +-----------------+-----------------+-------------+------------|TI|RPL|+
 * |32bit-TSS表基地址|32bit-TSS表段限长|16bit-TSS属性|    16bit-段选择子  |
 * +----------------------------------------------------------------------+
 *
 * 64-bit:  16B
 * +-----------------不可见(hidden portion)----------+-----可见-----------+
 * +-----------------+-----------------+-------------+------------|TI|RPL|+
 * |64bit-TSS表基地址|32bit-TSS表段限长|16bit-TSS属性|    16bit-段选择子  |
 * +----------------------------------------------------------------------+
 */
/*这儿传入的n是序号，不是偏移字节数*/
#define load_TR(n)				\
do {							\
	__asm__ __volatile__	(	\
			"ltr	%%ax"		\
			:					\
			:"a"(n << 3)		\
			:"memory"			\
			);					\
}while(0)

void set_itrpt_gate(unsigned int index, unsigned char IST, void *code_addr);
void set_system_itrpt_gate(unsigned int index, unsigned char IST, void *code_addr);
void set_trap_gate(unsigned int index, unsigned char IST, void *code_addr);
void set_system_trap_gate(unsigned int index, unsigned char IST, void *code_addr);
/* e:	64bit中断门描述符      f:	64bit陷阱门描述符
 * 8：	0特权级				   e:	3特权级       DPL
 * */

// 3 特权级表示可以交给应用层面进行调用的接口API
inline void set_itrpt_gate(unsigned int index, unsigned char IST, void *code_addr) {
	_set_gate(IDT_Table + index, 0x8e, IST, code_addr);		//P,DPL=0,TYPE=E
}

inline void set_system_itrpt_gate(unsigned int index, unsigned char IST, void *code_addr) {
	_set_gate(IDT_Table + index, 0xee, IST, code_addr);		//P,DPL=3,TYPE=E
}

inline void set_trap_gate(unsigned int index, unsigned char IST, void *code_addr) {
	_set_gate(IDT_Table + index, 0x8f, IST, code_addr);		//P,DPL=0,TYPE=F
}

inline void set_system_trap_gate(unsigned int index, unsigned char IST, void *code_addr) {
	_set_gate(IDT_Table + index, 0xef, IST, code_addr);		//P,DPL=3,TYPE=F
}

#endif
