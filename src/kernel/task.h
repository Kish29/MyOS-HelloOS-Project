#ifndef __TASK_H__
#define __TASK_H__ 

#include "memory.h"
#include "lib.h"

#define KERNEL_CS	(0x08)
#define KERNEL_DS	(0x10)

#define USER_CS	(0x18)
#define	USER_DS	(0x20)


// 每个进程的栈大小, 32KB
#define STACK_SIZE 32768

// Kerenl.lds中的变量符号
extern char _text;
extern char _etext;
extent char _data;
extern char _edata;
extern char _rodata;		// 只读数据部分
extern char _erodata;
extern char _bss;
extern char _ebss;
extern char _end;

extern unsigned long _stack_start;

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

union task_union init_task_union __attribute__ ((__section__ (".data.init_task"))) = {INIT_TASK(init_task_union.task)};	// 连接到Kernel.lds中的data.init_task符号处

struct task_struct *init_task[NR_CPUS] = {&init_task_union.task, 0};

struct mm_struct init_mm = {0};

// 内核init_thread
struct thread_struct init_thread = {
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	// 栈顶指针
	.rsp = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	// 栈顶指针
	.fs = KERNEL_DS,
	.gs = KERNEL_DS,
	.cr2 = 0,
	.trap_nr = 0,
	.error_code = 0
};








#endif
