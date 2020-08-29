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

unsigned long system_call_function(struct pt_regs *regs) {
	return system_call_table[regs->rax](regs);			// 根据entry.S中rdi传进来的参数就是pt_regs结构体起始地址-
}


void user_level_function() {
	// color_printk(ONE_GREEN, ONE_RED, "user_level_function is running...\n");
	long ret = 0;
	char string[] = "This function is in USER privilege, and I'll say: Hello World!\n";

	// 调用sys_printf函数
	__asm__	__volatile__	(
				"leaq	sysexit_return_address(%%rip),	%%rdx	\n\t"
				"movq	%%rsp,	%%rcx	\n\t"
				"sysenter	\n\t"
				"sysexit_return_address:"
				:"=a"(ret)
				:"0"(1), "D"(string)
				:"memory"
			);

	while(1);
}

unsigned long do_execve(struct pt_regs * regs) {
	// 传入的regs参数指针就是当前进程的栈指针
	// 设置rdx(装入rip)和rcx(装入rsp)
	regs->rdx = 0x800000;		// rdx->rip
	regs->rcx = 0xa00000;	// rcx->rsp		// 2MB物理页
	// 返回值
	regs->rax = 1;
	regs->es = 0x30;
	regs->ds = 0x30;		// 用户数据段

	color_printk(ONE_GREEN, BLACK, "do_execve task is running...\n");

	memcpy((void *)user_level_function, (void *)0x800000, 1024);		// 1KB 

	return 0;
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

	if(!(tsk->flags & PF_KTHREAD))	// 如果是应用层，应当从系统调用处返回
		thrd->rip = regs->rip = (unsigned long)ret_from_system_call;
	
	tsk->state = TASK_RUNNING;

	return 0;
}


// 该函数的作用是把即将切换的进程的执行现场恢复到寄存器中
// 因为在do_fork函数中，fork出的进程的栈空间进行了regs的复制，rsp此时指向栈顶向下偏移pt_regs大小的地方
extern void kernel_thread_func(void);
__asm__ (
	".global kernel_thread_func	\n\t"		// 这儿必须加上.global伪描述符让编译器做好全局化处理
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


// 修改init进程，使其跳转到应用层(使用ret_from_system_call/sysexit)
unsigned long init(unsigned long arg) {

	struct pt_regs *regs;

	color_printk(ONE_RED, BLACK, "init task is running, arg:%#018lx\n", arg);
	
	// 设置init函数的rip执行代码地址以及rsp
	current->thread->rip = (unsigned long)ret_from_system_call;
	current->thread->rsp = (unsigned long)current + STACK_SIZE - sizeof(struct pt_regs);	// 此时rsp就是pt_regs的起始地址

	// regs绑定当前进程的pt_regs结构体起始地址
	regs = (struct pt_regs *)current->thread->rsp;

	__asm__	__volatile__	(
				"movq	%1,	%%rsp	\n\t"
				"pushq	%2	\n\t"			// 压入ret_from_system_call的执行地址
				"jmp	do_execve	\n\t"
				:
				:"D"(regs), "m"(current->thread->rsp), "m"(current->thread->rip)
				:"memory"
			);

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

	// 为系统调用的两个指令sysenter/sysexit设置MSR寄存器组
	wrmsr(0x174, KERNEL_CS);		// 代码段选择子和栈段选择子
	wrmsr(0x175, current->thread->rsp);			// rsp
	wrmsr(0x176, (unsigned long)system_call);	// rip


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

	show_rsp();

	switch_to(current, p);
}


