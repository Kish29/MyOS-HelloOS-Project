#include "task.h"
#include "ptrace.h"
#include "memory.h"
#include "printk.h"
#include "lib.h"
#include "linkage.h"
#include "gate.h"

// 创建进程的入口，为即将创建的进程布置标志和相应的寄存器值
unsigned long kernel_thread(unsigned long (*fn)(unsigned long), unsigned long arg, unsigned long flags);

unsigned long init(unsigned long arg);

// 进程退出部分
unsigned long do_exit(unsigned long arg) {
	color_printk(ONE_RED, BLACK, "exit task is running, arg:%#018lx\n", arg);
	while(1);
}



unsigned long do_fork(struct pt_regs *regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size) {
	struct task_struct *tsk = NULL;			// PCB
	struct thread_struct *thrd = NULL;		// thread 保存切换信息
	struct Page *p = NULL;					// 进程所在页

	color_printk(WHITE, BLACK, "alloc_pages, bit_map:%#018lx\n", *memory_management_struct.bits_map);

	// 申请一个2MB物理页
	p = alloc_pages(ZONE_NORMAL, 1, PG_PTable_Maped | PG_Active | PG_Kernel_Init);

	color_printk(WHITE, BLACK, "alloc_pages, bit_map:%#018lx\n", *memory_management_struct.bits_map);

	// tsk保存物理页起始地址
	tsk = (struct task_struct *)Physic_To_Virtual(p->phy_addr_start);
	color_printk(WHITE, BLACK, "struct task_struct address:%#018lx\n", (unsigned long)tsk);

	// c 语言中的结构体变量名始终表示的是整个集合体本身，不是第一个变量，和数组不一样
	memset(tsk, 0, sizeof(*tsk));
	*tsk = *current;		// *tsk获取整个结构体内容，这儿是把当前的进程PCB整个复制到当前物理页的PCB

	list_init(&tsk->list);
	list_add_to_tail(&init_task_union.task.list, &tsk->list);	// 加入到PCB链表中
	tsk->pid++;			// 由于是从父进程fork，pid为父进程+1
	tsk->state = TASK_UNINTERRUPTIBLE;

	thrd = (struct thread_struct *)(tsk + 1);			// thread结构体紧凑PCB结构体
	tsk->thread = thrd;

	// 重要！复制pt_regs参数到新进程的栈空间中，注意顺序也是吻合的
	memcpy(regs, (void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs)), sizeof(struct pt_regs));
	// tsk注意要先强制转换
	thrd->rsp0 = (unsigned long)tsk + STACK_SIZE;
	thrd->rip = regs->rip;
	thrd->rsp = (unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs);

	if(!(tsk->flags & PF_KTHREAD))	// 如果是应用层，应当从中断处返回
		thrd->rip = regs->rip = (unsigned long)ret_from_itrpt;
	
	tsk->state = TASK_RUNNING;

	return 0;
}


// 该函数的作用是把即将切换的进程的执行现场恢复到寄存器中
// 因为在do_fork函数中，fork出的进程的栈空间进行了regs的复制，rsp此时指向栈顶向下偏移pt_regs大小的地方
extern void kernel_thread_func(void);
__asm__ (
	"kernel_thread_func:		\n\t"
	"popq	%r15				\n\t"
	"popq	%r14				\n\t"
	"popq	%r13				\n\t"
	"popq	%r12				\n\t"
	"popq	%r11				\n\t"
	"popq	%r10				\n\t"
	"popq	%r9					\n\t"
	"popq	%r8					\n\t"
	"popq	%rbx				\n\t"
	"popq	%rcx				\n\t"
	"popq	%rdx				\n\t"
	"popq	%rsi				\n\t"
	"popq	%rdi				\n\t"
	"popq	%rbp				\n\t"
	"popq	%rax				\n\t"
	"movq	%rax,	%ds			\n\t"
	"popq	%rax				\n\t"
	"movq	%rax,	%es			\n\t"
	"popq	%rax				\n\t"
	"addq	$0x38,	%rsp		\n\t"		// rsp增长56B，也就是56/8 = 7，跳过func->old_ss这7个变量，回到栈顶
////////////////////////////////////////
// 调用进程函数
	"movq	%rdx,	%rdi		\n\t"
	"callq	*%rbx				\n\t"
	"movq	%rax,	%rdi		\n\t"		// 返回值保存在rax中
	"callq	do_exit				\n\t"
);


unsigned long init(unsigned long arg) {
	color_printk(ONE_RED, BLACK, "init task is running, arg:%#018lx\n", arg);
	/* need complete
	 */
	return 1;
}

/* 该函数传入一个函数（进程）的执行地址，并为其设置属性，并通过当前进程调用do_fork函数，为其创建成进程 
 */
inline unsigned long kernel_thread(unsigned long (*fn)(unsigned long), unsigned long arg, unsigned long flags) {
	// 为将要创建的进程分配新的寄存器的值
	struct pt_regs regs;
	// 初始化为0
	memset(&regs, 0, sizeof(regs));

	// 保存函数地址和参数
	regs.rbx = (unsigned long)fn;
	regs.rdx = (unsigned long)arg;

	regs.ds = KERNEL_DS;
	regs.es = KERNEL_DS;
	regs.cs = KERNEL_CS;
	regs.ss = KERNEL_DS;

	regs.rflags = (1 << 9);			// 置位IF标志位(响应外部中断)
	regs.rip = (unsigned long)kernel_thread_func;			// 如果该进程是应用层，则需要转换为ret_from_itrpt

	color_printk(WHITE, BLACK, "regs.rip:%#018lx\n", regs.rip);

	// 从当前进程fork出新的进程
	return do_fork(&regs, flags, 0, 0);
}



void task_init() {
	// new task_struct ---
	struct task_struct *p = NULL;

	// initialize init_mm's parameters 
	init_mm.pgd = (pml4t_t *)Global_CR3; // 使得pgd指向PML4页表的第一项

	// init进程就是内核进程
	init_mm.start_code = memory_management_struct.code_start_addr;
	init_mm.end_code = memory_management_struct.code_end_addr;

	init_mm.start_data = (unsigned long)&_data;		// _data在task.h中声明为extern
	init_mm.end_data = memory_management_struct.data_end_addr;

	init_mm.start_rodata = (unsigned long)&_rodata;
	init_mm.end_rodata = (unsigned long)&_erodata;

	init_mm.start_brk = 0;
	init_mm.end_brk = memory_management_struct.kernel_end_addr;

	init_mm.start_stack = _stack_start;		// defined in head.S

	// 初始化为init进程的tss表
	set_TSS64(init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);
	
	init_tss[0].rsp0 = init_thread.rsp0;

	// 初始化PCB链表
	list_init(&init_task_union.task.list);

	// kernel_thread创建 init 进程, init就是一个函数
	kernel_thread(init, 10, CLONE_FS | CLONE_FILES | CLONE_SIGNAL); 

	init_task_union.task.state = TASK_RUNNING;

	// 获得init进程的task_struct起始地址
	p = container_of(get_node_next(&current->list), struct task_struct, list);

	color_printk(ONE_RED, BLACK, "p->thread->rip:%#018lx\n", p->thread->rip);
	switch_to(current, p);
}


