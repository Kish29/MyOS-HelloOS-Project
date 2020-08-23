/***************************
 * jar kernel_main
 **************************/

#include "lib.h"
#include "printk.h"
#include "trap.h"
#include "gate.h"
#include "memory.h"
#include "interrupt.h"
#include "task.h"

/* 全局化Kernel.lds中的标示符
 * 在main.c中声明后，在连接的过程中
 * 这些标示符就会和Kernel.lds中的相同的标示符进行统一
 * 链接到指定的地址处
 * 然后获取这些标示符的地址即为代码段、数据段的地址
 * */
extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

struct Global_Memory_Descriptor memory_management_struct = {{0}, 0};

void _init_kernel(void) {

	int i = 0;


	/* initialize screen information
	 * */
	pos_info._x_resolution = 1280;
	pos_info._y_resolution = 768;

	pos_info._x_position = 0;
	pos_info._y_position = 0;

	pos_info._x_char_size = 8;
	pos_info._y_char_size = 16;

	pos_info._frame_buf_addr = (int *)0xffff800000a00000;  // 保存帧缓存的地址 a00000 线性地址映射到物理地址0xe00000处

	// 每个像素4B
	pos_info._frame_buf_length = pos_info._x_resolution * pos_info._y_resolution * 4;

	// TSS表的索引在GDT表的第9项
	load_TR(9);

	unsigned long tss_bean = 0xffff800000007c00;
	// 由于使用的是2MB物理页，低21-bit作为offset，所以就是使用内存0x7c00处作为栈顶
	color_printk(ONE_PURPLE, BLACK, "_stack_start:%#018lx\n", _stack_start);
	set_TSS64(_stack_start, _stack_start, _stack_start, tss_bean, tss_bean, tss_bean, tss_bean, tss_bean, tss_bean, tss_bean);

	sys_idt_vector_init();

	/* 获取标示符所对应的地址 */
	memory_management_struct.code_start_addr = (unsigned long) &_text;	// 获得代码区域的起始地址
	memory_management_struct.code_end_addr = (unsigned long) &_etext;
	memory_management_struct.data_end_addr = (unsigned long) &_edata;
	memory_management_struct.kernel_end_addr = (unsigned long) &_end;

	color_printk(ONE_PURPLE, BLACK, "Memory Init \n");
	init_memory();

	color_printk(ONE_PURPLE, BLACK, "interrupt init\n");
	// 清空8024键盘控制寄存器缓冲区
	cls_8024_kybd_buf();
	init_interrupt();

	color_printk(ONE_PURPLE, BLACK, "task init\n");
	task_init();

	while(1);
}
