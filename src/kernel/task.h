#ifndef __TASK_H__
#define __TASK_H__ 

#include "memory.h"
#include "lib.h"
#include "cpu.h"
#include "ptrace.h"
#include "gate.h"

#define KERNEL_CS	(0x08)
#define KERNEL_DS	(0x10)

#define USER_CS	(0x18)
#define	USER_DS	(0x20)

// 定义初始化状态信息
#define CLONE_FS		(1 << 0)
#define CLONE_FILES		(1 << 1)
#define CLONE_SIGNAL	(1 << 2)

// 每个进程的栈大小, 32KB
#define STACK_SIZE 32768

// Kerenl.lds中的变量符号
extern char _text;
extern char _etext;
extern char _data;
extern char _edata;
extern char _rodata;		// 只读数据部分
extern char _erodata;
extern char _bss;
extern char _ebss;
extern char _end;

extern unsigned long _stack_start;

// 使用 entry.S 中的现场返回
extern void ret_from_itrpt();

// 定义进程的状态标志
#define TASK_RUNNING			(1 << 0) 
#define TASK_INTERRUPTIBLE		(1 << 1)
#define TASK_UNINTERRUPTIBLE	(1 << 2)
#define TASK_ZOMBIE				(1 << 3)
#define TASK_STOPPED			(1 << 4)

struct mm_struct {
	pml4t_t *pgd;		// 保存着cr3的页目录基地址

	unsigned long start_code, end_code;
	unsigned long start_data, end_data;
	unsigned long start_rodata, end_rodata;
	unsigned long start_brk, end_brk;		// 进程动态内存分配区域(堆区域)
	unsigned long start_stack;
};

// 进程切换时要保存的信息
struct thread_struct {
	unsigned long rsp0;		// 保存在TSS中的内核层栈基地址
	
	unsigned long rip;		// 内核层代码指针
	unsigned long rsp;		// 内核层当前栈指针

	unsigned long fs;
	unsigned long gs;

	unsigned long cr2;		// 页表错误基地址
	unsigned long trap_nr;		// 异常号
	unsigned long error_code;	// 错误码
};

// 定义进程的属性：内核进程、应用层进程
#define PF_KTHREAD	(1 << 0)


// define the  PCB -> process control block 
struct task_struct {
	struct _list list;			// 以链表的方式链接各个进程控制体
	volatile long state;		// 用volatile修饰，表示该变量可能会在某些地方被修改，所以要让编译器编译不能对改变量进行优化，让处理器每次重新读取，而不要用寄存器中的备份值
	unsigned long flags;	// 进程标志：进程、线程、内核线程

	struct mm_struct *mm;	// 内存空间分布结构体，记录页表和程序段信息
	struct thread_struct *thread;	// 进程切换时的保留信息

	unsigned long addr_lmt;
	/* 0x0000_0000_0000_0000 -> 0x0000_7fff_ffff_ffff	USER 
	 * 0xffff_8000_0000_0000 -> 0xffff_ffff_ffff_ffff	KERNEL 
	 */
	
	long pid;		// 进程ID号

	long counter;	// 进程的时间片

	long signal;	// 进程持有的信号

	long priority;	// 进程的优先级
};

// 将task_struct和每个进程的栈空间组成联合体，联合体的大小是该union中最大的数据变量
union task_union {
	struct task_struct task;
	unsigned long stack[STACK_SIZE / sizeof(unsigned long)];	// 按照unsigned long的大小进行分割
}__attribute__((aligned(8)));


struct mm_struct init_mm;

struct thread_struct init_thread;
// 初始化内核 init 进程，pid为1
#define INIT_TASK(tsk) {			\
	.state = TASK_UNINTERRUPTIBLE,	\
	.flags = PF_KTHREAD,			\
	.mm = &init_mm,					\
	.thread = &init_thread,			\
	.addr_lmt = 0xffff800000000000,	\
	.pid = 0,						\
	.counter = 1,					\
	.signal = 0,					\
	.priority = 0					\
}


////////////////////////////////
/* 内核 init 进程
 * 该进程特殊放在.data.init_task处
 * */ 
union task_union init_task_union __attribute__ ((__section__ (".data.init_task"))) = {INIT_TASK(init_task_union.task)};	// 连接到Kernel.lds中的data.init_task符号处

struct task_struct *init_task[NR_CPUS] = {&init_task_union.task, 0};
/////////////////////////////////

struct mm_struct init_mm = {0};

// 内核init_thread
struct thread_struct init_thread = {
	// union的stack变量的增减按照类型进行增减，也就是long型数据
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	// 栈顶指针
	.rsp = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	// 栈顶指针
	.fs = KERNEL_DS,
	.gs = KERNEL_DS,
	.cr2 = 0,
	.trap_nr = 0,
	.error_code = 0
};

// 定义进程TSS表
struct tss_struct {
	unsigned int reserved0;
	unsigned long rsp0;
	unsigned long rsp1;
	unsigned long rsp2;
	unsigned long reserved1;
	unsigned long ist1;
	unsigned long ist2;
	unsigned long ist3;
	unsigned long ist4;
	unsigned long ist5;
	unsigned long ist6;
	unsigned long ist7;
	unsigned long reserved2;
	unsigned short reserved3;
	unsigned short iomap_base_addr;
}__attribute__((packed));

#define INTI_TSS	{		\
	.reserved0 = 0,			\
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp1 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp2 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.reserved1 = 0,	\
	.ist1 = 0xffff800000007c00,	\
	.ist2 = 0xffff800000007c00,	\
	.ist3 = 0xffff800000007c00,	\
	.ist4 = 0xffff800000007c00,	\
	.ist5 = 0xffff800000007c00,	\
	.ist6 = 0xffff800000007c00,	\
	.ist7 = 0xffff800000007c00,	\
	.reserved2 = 0, \
	.reserved3 = 0,	\
	.iomap_base_addr = 0, \
}

// 为处理器的每个核设置一个tss表
struct tss_struct init_tss[NR_CPUS] = { [0 ... NR_CPUS - 1] = INTI_TSS};

struct task_struct *get_current();

// 获得当前进程的
inline struct task_struct *get_current() {
	struct task_struct *current = NULL;
	__asm__	__volatile__	("andq	%%rsp,	%0	\n\t":"=r"(current):"0"(~32767UL):);
	return current;
}

#define current get_current()

// -32768的补码 = ~32767的补码
#define GET_CURRENT			\
	"movq	%rsp,	%rbx	\n\t"		\
	"andq	$-32768,	%rbx	\n\t"


// 传入task_struct
// #define switch_to(prev, next)				\
// do {										\
// 	__asm__	__volatile__	(				\
// 			"pushq	%%rbp				\n\t"	\
// 			"pushq	%%rax				\n\t"	\
// 			"movq	%%rsp,	%0			\n\t"	\
// 			"movq	%2,	%%rsp			\n\t"	\
// 			"leaq	1f(%%rip),	%%rax	\n\t"	\
// 			"movq	%%rax,	%1			\n\t"	\
// 			"pushq	%3					\n\t"	\
// 			"jmp	__switch_to			\n\t"	\
// 			"1:							\n\t"	\
// 			"popq	%%rax				\n\t"	\
// 			"popq	%%rbp				\n\t"	\
// 			:"=m"(prev->thread->rsp), "=m"(prev->thread->rip)		\
// 			:"m"(next->thread->rsp), "m"(next->thread->rip), "D"(prev), "S"(next)		\
// 			:"memory"			\
// 		);				\
// }while(0)


void switch_to(struct task_struct *prev, struct task_struct *next);

inline void switch_to(struct task_struct *prev, struct task_struct *next) {

	// color_printk(ONE_RED, BLACK, "prev->thread->rsp:%#018lx\n", prev->thread->rsp);
	// color_printk(ONE_RED, BLACK, "next->thread->rsp:%#018lx\n", next->thread->rsp);

	__asm__ __volatile__ (
			"pushq	%%rbp				\n\t"
			"pushq	%%rax				\n\t"
			"movq	%%rsp,	%0			\n\t"
			"movq	%2,	%%rsp			\n\t"
			"leaq	1f(%%rip),	%%rax	\n\t"
			"movq	%%rax,	%1			\n\t"
			"pushq	%3					\n\t"
			// __switch_to函数的ret返回时，相当于popq	%rip 
			// 所以要提前将next进程的函数入口kernel_thread_func压入栈中
			"jmp	__switch_to			\n\t"
			"1:							\n\t"
			"popq	%%rax				\n\t"
			"popq	%%rbp				\n\t"
			:"=m"(prev->thread->rsp), "=m"(prev->thread->rip)
			:"m"(next->thread->rsp), "m"(next->thread->rip), "D"(prev), "S"(next)
			:"memory", "ax"		// 这儿必须声明对ax寄存器的修改！！！！
			);

	// color_printk(ONE_RED, BLACK, "prev->thread->rsp:%#018lx\n", prev->thread->rsp);
	// color_printk(ONE_RED, BLACK, "next->thread->rsp:%#018lx\n", next->thread->rsp);

	// color_printk(ONE_RED, BLACK, "Here is while(1) circle\n");

	// while(1);
}


// 进程切换工作
void __switch_to(struct task_struct *prev, struct task_struct *next);

// __switch_to函数最好定义为inline或者是extern
inline void __switch_to(struct task_struct *prev, struct task_struct *next) {
	init_tss[0].rsp0 = next->thread->rsp0;

	set_TSS64(init_tss[0].rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	__asm__ __volatile__ ("movq	%%fs,	%0	\n\t":"=a"(prev->thread->fs));
	__asm__ __volatile__ ("movq	%%gs,	%0	\n\t":"=a"(prev->thread->gs));

	__asm__ __volatile__ ("movq	%0,	%%fs	\n\t"::"a"(next->thread->fs));
	__asm__ __volatile__ ("movq	%0,	%%gs	\n\t"::"a"(next->thread->gs));

	color_printk(WHITE, BLACK, "next->thread->rip:%#018lx\n", next->thread->rip);
	color_printk(WHITE, BLACK, "prev->thread->rsp0:%#018lx\n", prev->thread->rsp0);
	color_printk(WHITE, BLACK, "next->thread->rsp0:%#018lx\n", next->thread->rsp0);
	color_printk(WHITE, BLACK, "prev->thread->rsp:%#018lx\n", prev->thread->rsp);
	color_printk(WHITE, BLACK, "next->thread->rsp:%#018lx\n", next->thread->rsp);

	// color_printk(ONE_BLUE, BLACK, "Here is while(1) circle in __switch_to method\n");
	// while(1);
}


unsigned long do_fork(struct pt_regs *regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size);

void task_init();

#endif
