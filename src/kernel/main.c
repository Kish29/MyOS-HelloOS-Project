/***************************
 * jar kernel_main
 **************************/

#include "lib.h"
#include "printk.h"
#include "trap.h"
#include "gate.h"

void _init_kernel(void) {
	// int *addr = (int *)0xffff800000a00000;	// addr 保存帧缓存的地址
	// 										// a00000 线性地址映射到物理地址0xe00000处
	// int i = 0;

	// // 1280 x 768
	// while (i < 1280 * 20) {
	// 	*((char *)addr + 0) = (char)0x00; // 强转为 char 保证是一个字节（B）一个字节的操作
	// 	*((char *)addr + 1) = (char)0x00;
	// 	*((char *)addr + 2) = (char)0xff; // red 
	// 	*((char *)addr + 3) = (char)0x00; // 保留
	// 	addr++;
	// 	i++;
	// }

	// i = 0;
	// while (i < 1280 * 20) {
	// 	*((char *)addr + 0) = (char)0x00;
	// 	*((char *)addr + 1) = (char)0xff;
	// 	*((char *)addr + 2) = (char)0x00;
	// 	*((char *)addr + 3) = (char)0x00;
	// 	addr++;
	// 	i++;
	// }

	// i = 0;
	// while (i < 1280 * 20) {
	// 	*((char *)addr + 0) = (char)0xff;
	// 	*((char *)addr + 1) = (char)0x00;
	// 	*((char *)addr + 2) = (char)0x00;
	// 	*((char *)addr + 3) = (char)0x00;
	// 	addr++;
	// 	i++;
	// }

	// i = 0;
	// while (i < 1280 * 20) {
	// 	*((char *)addr + 0) = (char)0xff;
	// 	*((char *)addr + 1) = (char)0xff;
	// 	*((char *)addr + 2) = (char)0xff;
	// 	*((char *)addr + 3) = (char)0x00;
	// 	addr++;
	// 	i++;
	// }

	// i = 0;
	// while (i < 1280 * 20) {
	// 	*((char *)addr + 0) = (char)0xff;
	// 	*((char *)addr + 1) = (char)0xaa;
	// 	*((char *)addr + 2) = (char)0x00;
	// 	*((char *)addr + 3) = (char)0x00;
	// 	addr++;
	// 	i++;
	// }

	pos_info._x_resolution = 1280;
	pos_info._y_resolution = 768;

	pos_info._x_position = 0;
	pos_info._y_position = 0;

	pos_info._x_char_size = 8;
	pos_info._y_char_size = 16;

	pos_info._frame_buf_addr = (int *)0xffff800000a00000;
	// 每个像素4B
	pos_info._frame_buf_length = pos_info._x_resolution * pos_info._y_resolution * 4;

	// color_printk(YELLOW, BLACK, "HelloOS v0.0.5\t\tToday is 2020-07-26\n\n");

	// color_printk(WHITE, BLACK, "hellos>");

	// char *str1 = "abcde";
	// char *str2 = "abcde";

	// char *memcpy1 = "test memcpy";
	// char *memcpy2 = (char *)0xffff800000a00000;

	// memcpy(memcpy1, memcpy2, 10);

	// int b = memcmp(str1, str2, 5);

	// color_printk(PURPLE, BLACK, "The value is %d\n", b);

	// // sys_idt_vector_init();
	// while(1);
	// int i;
	// i = 1 / 0;

	// color_printk(YELLOW, BLACK, "After interrupt.\n");
	
	load_TR(9);

	unsigned long tss_bean = 0xffff800000007c00;
	// 由于使用的是2MB物理页，低21-bit作为offset，所以就是使用内存0x7c00处作为栈顶
	set_TSS64(tss_bean, tss_bean, tss_bean, tss_bean, tss_bean, tss_bean, tss_bean, tss_bean, tss_bean, tss_bean);

	sys_idt_vector_init();

	int i;
	i = *(int *)0xffff80000aa00000;
	// i = 1 / 0;
	while(1);
}
