/*******************************
*	kernel执行头head.S
*	2020-07-22
*******************************/ 

/* 本程序会用链接脚本将内核的执行地址设置成
* Canonical的高位地址0xffff800000010000处
*/ 
// .section 伪指令声明代码段 .text

.section .text

.global _start // _start 指定程序起始位置

_start:
	// 初始化数据段
	mov	$0x10,	%ax
	mov %ax,	%ds
	mov %ax,	%es 
	mov %ax,	%fs 
	mov %ax,	%ss 
	mov $0x7e00,	%esp 

/**************为IA-32e模式重新加载新的GDT、IDT等等***********/

// ============== load GDTR 

	lgdt	GDT_POINTER(%rip)
	// displacement(%rip) 

// ============== load IDTR 
	
	lidt	IDT_POINTER(%rip)

// ================ 需要重新为段寄存器赋值=========
	
	mov $0x10,	%ax
	mov %ax,	%ds
	mov %ax,	%es 
	mov %ax,	%fs 
	mov %ax,	%gs
	mov %ax,	%ss 

	movq	 $0x7e00,	%rsp 

// ================ 为cr3控制寄存器加载页目录(PML4)基地址
	
	movq	$0x101000,	%rax	/* 编译器会根据链接脚本设置的地址将cr3设置成 */
	movq	%rax,		%cr3	/*	0xffff800000101000 */

	/**********************************************************************************
	*	这儿将程序使用的线性地址切换为高位Canonical 0xffff800000000000处
	*	因为GAS编译器暂不支持远跳指令lcall和ljmp，所以我们用lretq模拟跳转
	*	lretq 远返回，相当于  /* pop %rip 
				即相当于	/* movq	(%rsp), %rip  第一个为偏移量
							/* add	$0x8,	%rsp  栈指针加8B（8B对齐）
							/* movq (%rsp), %cs	  第二个为基地址(IA-32e模式下为段选择子)
							/* add	$0x8,	%rsp 
							/* movq (%rsp),	%rbp  获得斩基地址
	*	具体实现的原理：
	*	loader跳转后，程序运行在  物理地址  0x100000处，在跳转前，程序要使用的数据段和
	*	代码段将默认为0x100000    线性地址  处，但是在编译链接kernel.bin的时候，链接脚本
	*	会将代码段和数据段描述成  线性地址	0xffff800000100000处（那么其中的表也会在这个
	*	线性地址  的基础由编译器设置地址，设置成 0xffff800000100000 + 0x1000 
	*										   = 0xffff800000101000 处)
	*	，跳转时，程序的跳转地址也会变成编译器编译出的
	*	0xffff800000100000为基地址的	  线性地址  处，但要记住，kernel程序始终运行在真实
	*	的物理地址	0x100000	处！改变的仅仅是IA-32e模式下的线性地址寻址范围
	***********************************************************************************/
	movq	switch_seg(%rip),	%rax 
	pushq	$0x8 // 代码段的选择子
	pushq	%rax 
	lretq

// ================ new 64-bit mode code 

switch_seg:
	.quad	entry64

entry64:
	movq	$0x10,	%rax 
	movq	%rax,	%ds 
	movq	%rax,	%es 
	movq	%rax,	%gs 
	movq	%rax,	%ss	
	movq	$0xffff800000007e00,	%rsp	/* rsp address */
	
	movq	go_to_kernel(%rip),		%rax	/* base of 0xffff800000000000 code address */
	pushq	$0x8 
	pushq	%rax 
	lretq

go_to_kernel:
	.quad	_init_kernel

/********************** 接下来是页的4层不同描述符 ***********************/

.align	8	// 8B 对齐

// 顶层页
.org	0x1000  /* 编译器会根据链接脚本设置的地址将cr3设置成 */
				/*	0xffff800000101000 */
				/* 对照页描述，其实还是在物理地址 0x101000处 */
__PML4E:
	
	.quad	0x102007	// 7 -> 用户模式、可读写、存在
	.fill	255, 8, 0 
	.quad	0x102007 
	.fill	255, 8, 0

// 二级页
.org	0x2000 
__PDPTE:
	
	.quad	0x103003	// 3 ->	超级模式、可读写、已存在
	.fill	511, 8, 0

// 三级页
.org	0x3000
__PDE:
	
	// 每张表占用2MB物理也，按2递增
	// 前 5 x 2 = 10MB 物理内存的页描述
	.quad	0x000083	//表0	// 83 -> 1000_0011 2MB物理页：超级模式、可读写、存在
	.quad	0x200083	//表1	// 那么按照2MB物理也，指令的底21位将用来作为偏移量
	.quad	0x400083	//表2	// 那么按照PML4E的描述符
	.quad	0x600083	//表3	// 实际上 0x000083 描述符，将物理地址0x0 映射到	Canonical线性地址 的
	.quad	0x800083	//表4	// 0x0和0xffff800000000000处


	/* 后 8 x 2 = 16MB 物理内存的页描述
	 * 0xe0000083 将物理地址 0xe0000000 映射到线性地址 0xa00000处（0xa00000的底21位作为偏移量）
	 * 注意 0xe00000 物理地址是VBE模式下的平坦帧缓存（线性帧，64bit mode下的平坦Canonical线性地址）
	 * 0xe00000 地址可在loader中通过ax = 4f02h 获取，在返回缓冲区偏移40B处
	 */
	.quad	0xe0000083
	.quad	0xe0200083
	.quad	0xe0400083
	.quad	0xe0600083
	.quad	0xe0800083
	.quad	0xe0a00083
	.quad	0xe0c00083
	.quad	0xe0e00083
	.fill	499, 8, 0	// 剩余 499 个页描述符号

// 数据段，包括GDT表等
.section .data 

/* ========== GDT table ============*/
.global GDT_Table 

GDT_Table:
	.quad	0x0000000000000000			/*0 null descriptor					0x00 */
	.quad	0x0020980000000000			/*1 kernel code		64-bit	segment 0x08 */
	.quad	0x0000920000000000			/*2 kernel data		64-bit	segment 0x10 */
	.quad	0x0020f80000000000			/*3 user   code		64-bit	segment 0x18 */
	.quad	0x0000f20000000000			/*4 user   data		64-bit	segment 0x20 */
	.quad	0x00cf9a000000ffff			/*5 kernel code		32-bit	segment 0x28 */
	.quad	0x00cf92000000ffff			/*6 kernel data		32-bit	segment 0x30 */
	.fill	10, 8, 0					/*	这儿是TSS任务状态段描述符，16B一个，这儿定义了40个*/
	// .fill repeat, {size}, {value}
GDT_END:

// 64bit模式下，伪描述符为10B
GDT_POINTER:
GDT_LIMIT:	.word	GDT_END - GDT_Table - 1
GDT_BASE:	.quad	GDT_Table
			

/* ========== IDT table ============*/
.global IDT_Table 

IDT_Table:
	.fill	512, 8, 0
IDT_END:

IDT_POINTER:
IDT_LIMIT:	.word	IDT_END - IDT_Table - 1
IDT_BASE:	.quad	IDT_Table

/* ========== TSS64 table ============*/
.global TSS64_Table

TSS64_Table:
	.fill	13, 8, 0 
TSS64_END:

TSS64_POINTER:
TSS64_LIMIT:	.word	TSS64_END - TSS64_Table - 1
TSS64_BASE:		.quad	TSS64_Table
