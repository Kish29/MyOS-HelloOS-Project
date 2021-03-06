#include "lib.h"
#include "interrupt.h"
#include "linkage.h"
#include "printk.h"
#include "gate.h"
#include "memory.h"
#include "kybd.h"

const char *key_code = "\0~1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`^\\zxcvbnm,./^\0\0 \0";

// 保存现场，由于没有错误码，所以不使用entry.S中的error_code_prefix
#define SAVE_ALL						\
	"cld					\n\t"		\
	"pushq	%rax			\n\t"		\
	"pushq	%rax			\n\t"		\
	"movq	%es,	%rax	\n\t"		\
	"pushq	%rax			\n\t"		\
	"movq	%ds,	%rax	\n\t"		\
	"pushq	%rax			\n\t"		\
	"xorq	%rax,	%rax	\n\t"		\
	"pushq	%rbp			\n\t"		\
	"pushq	%rdi			\n\t"		\
	"pushq	%rsi			\n\t"		\
	"pushq	%rdx			\n\t"		\
	"pushq	%rcx			\n\t"		\
	"pushq	%rbx			\n\t"		\
	"pushq	%r8				\n\t"		\
	"pushq	%r9				\n\t"		\
	"pushq	%r10			\n\t"		\
	"pushq	%r11			\n\t"		\
	"pushq	%r12			\n\t"		\
	"pushq	%r13			\n\t"		\
	"pushq	%r14			\n\t"		\
	"pushq	%r15			\n\t"		\
	"movq	$0x10,	%rdx	\n\t"		\
	"movq	%rdx,	%es		\n\t"		\
	"movq	%rdx,	%ds		\n\t"


#define IRQ_NAME2(nr)	nr##_interrupt(void)		// 拼接字符串
#define IRQ_NAME(nr)	IRQ_NAME2(IRQ##nr)

// 定义中断入口，跳转到do_IRQ函数处运行
// SYMBOL_NAME_STR定义在linkage.h头文件中
// pushq	$0x00是错误码
#define Build_IRQ(nr)										\
void IRQ_NAME(nr);											\
__asm__	(													\
			SYMBOL_NAME_STR(IRQ)#nr"_interrupt:		\n\t"	\
			"pushq	$0x00							\n\t"	\
			SAVE_ALL										\
			"movq	%rsp,	%rdi					\n\t"	\
			"leaq	ret_from_itrpt(%rip),	%rax	\n\t"	\
			"pushq	%rax							\n\t"	\
			"movq	$"#nr",	%rsi					\n\t"	\
			"jmp	do_IRQ							\n\t"	\
		);
// 手写汇编jmp需要手动设置返回地址
// 不要省略了nr前面的'#'


// 定义0x20~0x37的24个中断
Build_IRQ(0x20)
Build_IRQ(0x21)
Build_IRQ(0x22)
Build_IRQ(0x23)
Build_IRQ(0x24)
Build_IRQ(0x25)
Build_IRQ(0x26)
Build_IRQ(0x27)
Build_IRQ(0x28)
Build_IRQ(0x29)
Build_IRQ(0x2a)
Build_IRQ(0x2b)
Build_IRQ(0x2c)
Build_IRQ(0x2d)
Build_IRQ(0x2e)
Build_IRQ(0x2f)
Build_IRQ(0x30)
Build_IRQ(0x31)
Build_IRQ(0x32)
Build_IRQ(0x33)
Build_IRQ(0x34)
Build_IRQ(0x35)
Build_IRQ(0x36)
Build_IRQ(0x37)

// 定义24个中断函数的函数指针
void (*interrupt[24]) (void) = {
	IRQ0x20_interrupt,
	IRQ0x21_interrupt,
	IRQ0x22_interrupt,
	IRQ0x23_interrupt,
	IRQ0x24_interrupt,
	IRQ0x25_interrupt,
	IRQ0x26_interrupt,
	IRQ0x27_interrupt,
	IRQ0x28_interrupt,
	IRQ0x29_interrupt,
	IRQ0x2a_interrupt,
	IRQ0x2b_interrupt,
	IRQ0x2c_interrupt,
	IRQ0x2d_interrupt,
	IRQ0x2e_interrupt,
	IRQ0x2f_interrupt,
	IRQ0x30_interrupt,
	IRQ0x31_interrupt,
	IRQ0x32_interrupt,
	IRQ0x33_interrupt,
	IRQ0x34_interrupt,
	IRQ0x35_interrupt,
	IRQ0x36_interrupt,
	IRQ0x37_interrupt,
};


void init_interrupt() {
	int i;
	// 在IDT中设置这24个中断
	// 32号及其之后的中断供用户自定义
	for(i = 32; i < 56; i++) {
		// 使用 IST2 号的栈切换机制
		set_itrpt_gate(i, 2, interrupt[i - 32]);
	}

	color_printk(ONE_PURPLE, BLACK, "8259A init\n");
	
	// 8259A-master		ICW1-4
	io_out8(0x20, 0x11);		// constant -> 0x11 
	io_out8(0x21, 0x20);		// start at number 0x20 to 0x27 
	io_out8(0x21, 0x04);		// IRQ2 级联从芯片
	io_out8(0x21, 0x01);		// standard

	// 8259A-slava		ICW1-4
	io_out8(0xa0, 0x11);
	io_out8(0xa1, 0x28);		// start at number 0x28 to 0x30
	io_out8(0xa1, 0x02);		// IRQ1 重定向主芯片
	io_out8(0xa1, 0x01);

	// 8259-m/s			OCW1(IMR)
	io_out8(0x21, 0xfd);			// 只允许键盘的中断 
	io_out8(0xa1, 0xff);

	sti();
}


void do_IRQ(unsigned long rsp, unsigned long index) {
	if(index == 0x21) {
		kybd_itrpt();
		return;
	}
	// unsigned char x;
	color_printk(ONE_BLUE_LIGHT, BLACK, "do_IRQ:%#08x\t", index);
	// 读取0x60处的键盘缓冲区字符
	// x = io_in8(0x60);
	// color_printk(ONE_BLUE_LIGHT, BLACK, "key code:%#08x\t", x);
	// 向OCW2号寄存器bit-5发送置位信息
	io_out8(0x20, 0x20);
}


inline void kybd_itrpt() {
	unsigned char k = io_in8(0x60);
	if( 2 <= k && k <= 0x3a) {
		unsigned char c = key_code[k];
		if(c != '\0')
			color_printk(WHITE, BLACK, "%c", c);
	}// 向OCW2号寄存器bit-5发送置位信息
	io_out8(0x20, 0x20);
}






