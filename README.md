# MyOS-HelloOS-Project
## HelloOS
	记录从零开始编写操作系统，之前的版本算是废掉了，因为是好几个月前写的了，现在几乎忘得一干二净，现在从新开始学习操作系统的知识。
	现在也对当时一些理解不能的地方有一个初步的了解。

>本项目的学习基于图书《一个64位操作系统的设计与实现》来进行研究于学习。需要有一定的汇编代码基础以及相应的硬件基础、C语言能力。废话不多说，这里将会记录学习的日期以及进度等等。

### HelloOS v0.0.1  <font size=2><u>2020.7.6~2020.7.7</u></font>
  1. bximage 创建虚拟磁盘，选择软盘
  2. nasm 编译boot.asm为bin文件
  3. dd 拷贝boot.bin到第一个扇区 
  4. mount 挂载boot.img文件，并加上-o loop参数，使其描述为磁盘格式
  5. cp loader.bin 到boot.img 中，操作系统会为根据这个文件的文件系统格式进行相应的信息录入
  6. bochs -f .bochsrc 启动bochs虚拟机（记得提前配置好.bochsrc）

写完boot.asm程序，从FAT12的文件系统格式从读入loader程序到内存的指定位置

### HelloOS v0.0.2 <font size=2><u>2020.7.18</u></font>
	2020-07-19

	上传了两个python脚本，实现对当前目录下的所有代码源文件中的代码保存到数据库当中
	支持的文件cpp, c, java, asm以及makefile等等
	因为昨天一次不小心的git操作，把链接点删除了，辛苦写了一天的代码消失了，还不小心用rm -rf 命令删除了git仓库中的一个文件，当时是真的绝望
	试了各种办法没找回来，不过最后还是找回来了哈哈哈
	所以吸取了这次教训，自己写个脚本
	如果github的提交操作出现问题或者异常，先把文件保存到数据库中，这样就可以大胆的操作了

1. 实现loader读取内核程序到物理地址1MB处
2. 添加loader过程中出现任何异常的关机提示处理 
3. 实现读取物理地址的内存结构

### HelloOS v0.0.3 <font size=2><u>2020.7.20</u></font>
1. 实现读取VBE信息，设置屏幕分辨率
2. 优化书上所表示的VBE模式的表示
	>将不同模式的分辨率显示在后面(仅支持VBE1.2版本及以上)

3. 跳转到临时IA-32保护模式

### HelloOS v0.0.4  <font size=2><u>2020.7.20</u></font>
1. 处理器切换到IA-32e模式
2. 处理器控制器转交给内核程序

### HelloOS v0.0.5  <font size=2><u>2020.7.22</u></font>
1. 写完内核头程序head.S，重新定义了GDT、IDT、64位页表等
2. 排除了存在于loader中的惊天BUG，该BUG导致kernel程序无法完整地加载到指定内存

### HelloOS v0.0.6  <font size=2><u>2020.7.24</u></font>
1. 写完了打印彩色字符串的函数，可以自由调用color_printk函数打印字符串了
	> 时间花了这么久主要原因是刚接触内核层面，好多库函数要自己些，只能看着作者的源码写，作者参考的是linux2.几的源码，期间我参考了linux-0.11的源码，发现二者是一模一样的，作者在此基础上写了color_printk函数，不过这个函数有个错误，已经向作者发了邮件等待回复
	>
	> 作者回复已在图灵社区进行了勘误
	>
	> 这期间被阅读源码、指针(什么char *s, char **s, int *强制转换)、可变参数(这个最恶心)、&、|操作以及内嵌汇编，inline链接所折磨，所以花了很长时间去学习这些东西
2. 不过现在没搞懂的是为什么.h中声明一个inline函数链接器会报undefined reference错误，把inline删掉就过了(虽然知道.h中的函数实现会被默认定义为inline)
	> 请教了老师后发现，在我版本的gcc中，需要在编译阶段加上-O参数进行优化，强制在编译过程把inline函数展开到调用处，这样就能正确通过了

### HelloOS v0.0.7  <font size=2><u>2020.7.27</u></font>
1. 这一个星期将开启原始人模式<del>(因为去了外婆家，乡下没网)<del> 
2. 这两天一直在帮着干活，有点累，只能万忙之中抽空出来敲
3. 写完了lib.h中的一些常用函数，虽然没有完全理解，以后花时间去理解
4. 重写了printk.c中的一个分支，之前作者的分支导致负数的补码直接显示而没能正确显示
5. 作者的lib.h中的条件都是反的，自己修改了一下

### HelloOS v0.0.8  <font size=2><u>2020.8.1</u></font>
1. 写完IDT的简单初始化，TSS写入GDT表中，IDT中有一个简单的异常处理程序
2. 发现一个问题，在上次的问题处理gcc加-O优化处理的时候，把代码中很明确发生异常的地方（如除0错误）进行了优化，在编译阶段将系统的异常处理加了进去而没有成功调用我自己写的异常程序，明天再请教一下老师吧。
	> 这个问题得到了最终的解决：原来是我写inline函数的定义的时候，没有事先在.h文件中进行声明，所以连接的时候先去找声明，再找定义，-O优化强制把只有定义的inine函数进行了展开。所以，所有的inline函数必须先声明，再定义成inline，这样才能在调用的地方展开
	>
	>像这样:
	> ```c
	> int foo(void *);
	> inline int foo(void *) {
	>	// functions
	> }
	>```

### HelloOS v0.0.9  <font size=2><u>2020.8.5</u></font>
1. 简单写完了处理器自带的前21个异常处理程序，其中比较重要的还是异常/中断的现场保护和还原工作(entry.S)
2. bug目前是修复完了，调试了挺久的，简单做一个记录吧
```
a. 之前最开始的时候，在gate.h中写完set_gate的宏以及其他的设置描述符的函数后，我直接写了一个divide_error的函数定义，
并且在主函数中调用了sys_idt_init()，结果bochs直接异常了，现在想来，原因是这个：
	所有的异常/中断处理，要涉及到栈的使用(并且不同特权级，栈还需要切换)。
	因为在sys_idt_init函数内，对相应的异常/中断进行设置，调用的是set_gate的宏定义，
	参数中有IST并且我们确实传入了1表示要用1号的IST栈切换机制(要用到相应的已设置的rsp的值)，但是，ist设置的值保存在TSS表中，而当时有没有对TSS表进行初始化，所以就异常了。
	而为什么ignore_int没有异常呢，其实去head.S中看一下就会发现，当时写的时候，IST设置的值是0，表示不使用IST，使用原来的栈切换机制

b. 第二个bug就是在设置TSS表的时候，把传入的RSP的设置值写成了0xffff8000007c00(也就是0x00ffff8000007c00)，很明显这个地址在页表中是没有对应的物理地址映射的。

c. 中断/异常的现场保护工作入口的地方，少写了个pushq	%rbx。。。
```
### HelloOS v0.0.10  <font size=2><u>2020.8.12</u></font>
1. 总于完成了内存的读取和分配工作，也算是对内存管理有了个好的理解
2. 修复了两个bug，一个是memory.c中按照zone区域对bits_map进行映射时，计算索引的时候，写成了PAGA_4K_SHIFT，另外一个就是printk.c中，计算tab的时候问题，具体在注视部分

### HelloOS v0.0.10  <font size=2><u>2020.8.16</u></font>
1. 完成外设的中断（基于8259中断控制器）
2. 完成键盘中断，注意，由于8024键盘中断控制器的特性，如果0x60读写缓冲区的数据没有被系统取走，8024将不会接受新的键盘中断，而我在loader中写了个等待用户敲击任意键继续的代码，所以导致后续内核初始化中断后键盘缓冲区数据没有被取走从而不能读取键盘中断，有两个解决方法
	> 1. 在main函数中执行一次读取0x60缓冲区的方法
	> 2. 写一个清空缓冲区的方法

我选择了后者，cls_8042_kybd_buf()定义在lib.h中，原理非常简单（参考了8042官方文档对该控制器的说明）
### HelloOS v0.0.10  <font size=2><u>2020.8.20</u></font>
1. 回到学校的第一天，nice！！！！
2. 修改了打印函数color_printk的开头和结尾部分，让字符的后面显示一个白色矩形块来指示下一个字符的位置
3. 修改了键盘的中断部分，完善常用的字符XT扫描码，使得键盘按下字符能在屏幕显示，不过现在只能显示这些字符的小写
	>ps. 回到学校有点舒爽

### HelloOS v0.0.10  <font size=2><u>2020.8.24</u></font>
1. 简单的进程算是写完了，但是出现了一个bug，switch_to函数的内嵌asm中，给rsp赋值的操作直接导致了页错误异常，现在还没有弄明白究竟是什么原因，明天问问老师
> 最近挺忙的，开学了，还有小学期得上，进程这一块儿也比较难啃，加油奥利给！


### HelloOS v0.0.10  <font size=2><u>2020.8.27</u></font>
接上面的bug修复部分，问题主要出现在task.h和task.c中，
一共有3处faltal error，

#### 自己的测试部分
#### 1. 在task.h中，有如下的代码:

上次我为了测试rsp赋值的情况，写了如下的asm内嵌汇编代码
```asm
	pushq	%%rbp
	pushq	%%rax
	pushq	%%rsp,	%0
	pushq	%2，	%%rsp
	popq	%%rax
	popq	%%rbp
```
问题出现在后面从栈中弹出rax和rbp值的时候，由于之前压入rax和rbp的时候，rsp在另一个栈中

而当我re-assign rsp后，栈基地址的发生了改变，所以当执行popq	%%rax和popq	%%rbp的时候

是找不到原先的rax和rbp的值的


所以当然会出现页错误


#### 真正的错误之处（2个）：
#### 2. 在对prev->thread->rip进行赋值的时候：

我将popq	%%rax处的地址保存在了rax寄存器中
>代码  leaq	1f(%%rip),	%%rax

<font color=red>而没有在内嵌汇编的损坏部分声明对ax寄存器的修改</font>导致抛出了异常

所以必须得在损坏部分加上ax寄存器，让编译器提前做好ax寄存器的保存工作，使得能够正常编译

```asm
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
```

#### 3. 如上代码，进入__switch_to函数是通过段间跳转指令jmp __switch_to实现的，<font color=red>而并不是使用callq进行的跳转，</font>所以为了能够在__switch_to函数返回时，能正确的回到下一个进程的函数入口
<font color=red>将next->thread->rip压入了栈的返回地址处</font>

>代码	pushq	%3<font color=red>将下一个进程的rip放在了__switch_to函数ret(ret指令相当于popq	%rip)取值处</font>

而next->thread->rip是通过task.c的kernel_thread函数进行赋值的：
```c
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
```
可以看到代码<font color=red>**regs.rip = (unsigned long)kernel_thread_func**</font> 为下一个进程的入口进行赋值，而kernel_thread_func是这样在task.c中进行定义的：
```c

extern void kernel_thread_func(void);
__asm__ (
	"kernel_thread_func:		\n\t"
	"popq	%r15				\n\t"
	"popq	%r14				\n\t"
```


而我反复进行调试的时候发现，next->thread->rip的值总是不正确的(比如负数或者特别大的数)，所以考虑可能是编译器在进行编译的时候，并没有对这个函数进行全局化的处理，所以在kernel_thread_fun前面加上.global伪描述符
```c
extern void kernel_thread_func(void);
__asm__ (
	".global kernel_thread_func	\n\t"
	"kernel_thread_func:		\n\t"
	"popq	%r15				\n\t"
	"popq	%r14				\n\t"
```





让编译器进行函数的全局化处理

修复这2个bug后，kenel终于能正确运行了！！！！
> 无数次的debug...和无数次的情况尝试....

### HelloOS v0.1.0  <font size=2><u>2020.8.29</u></font>
1. 跟着作者一步一步把代码写到了应用层，实现了一个最最最基本的操作系统雏形，但是还没有实现一些复杂的内存管理和设备文件管理以及驱动、进程等等，但是经过这俩月的学习，自己对操作系统的认识还是认识了不少，也很感谢作者！期间自己遇到了很多问题和bug，因为gcc版本或者其他的原因，可能作者能正常运行的程序到我这儿就失败了，当然，最后也自己解决了（<del>锻炼了自己debug的能力</del>）
2. 系统调用这块儿，0x800000处的函数是由memcpy把代码复制过去的，由于gcc版本的原因，编译后的调用color_printk代码块使用的不是color_printk的绝对地址，而是经过编译器优化过后计算的方式进行的调用，所以单纯的memcpy代码块再直接调用color_printk在一些高版本的gcc是行不通的(目前暂不知道函数调用的编译选项)
3. 旅途才刚刚开始，接下来征战物理平台。。。

