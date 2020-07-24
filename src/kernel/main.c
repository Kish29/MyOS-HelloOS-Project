/***************************
 * jar kernel_main
 **************************/

#include "lib.h"
#include "printk.h"

void _init_kernel(void) {
	int *addr = (int *)0xffff800000a00000;	// addr 保存帧缓存的地址
											// a00000 线性地址映射到物理地址0xe00000处
	int i = 0;

	// 1280 x 768
	while (i < 1280 * 20) {
		*((char *)addr + 0) = (char)0x00; // 强转为 char 保证是一个字节（B）一个字节的操作
		*((char *)addr + 1) = (char)0x00;
		*((char *)addr + 2) = (char)0xff; // red 
		*((char *)addr + 3) = (char)0x00; // 保留
		addr++;
		i++;
	}

	i = 0;
	while (i < 1280 * 20) {
		*((char *)addr + 0) = (char)0x00;
		*((char *)addr + 1) = (char)0xff;
		*((char *)addr + 2) = (char)0x00;
		*((char *)addr + 3) = (char)0x00;
		addr++;
		i++;
	}

	i = 0;
	while (i < 1280 * 20) {
		*((char *)addr + 0) = (char)0xff;
		*((char *)addr + 1) = (char)0x00;
		*((char *)addr + 2) = (char)0x00;
		*((char *)addr + 3) = (char)0x00;
		addr++;
		i++;
	}

	i = 0;
	while (i < 1280 * 20) {
		*((char *)addr + 0) = (char)0xff;
		*((char *)addr + 1) = (char)0xff;
		*((char *)addr + 2) = (char)0xff;
		*((char *)addr + 3) = (char)0x00;
		addr++;
		i++;
	}

	i = 0;
	while (i < 1280 * 20) {
		*((char *)addr + 0) = (char)0xff;
		*((char *)addr + 1) = (char)0xaa;
		*((char *)addr + 2) = (char)0x00;
		*((char *)addr + 3) = (char)0x00;
		addr++;
		i++;
	}

	pos_info._x_resolution = 1280;
	pos_info._y_resolution = 768;

	pos_info._x_position = 0;
	pos_info._y_position = 0;

	pos_info._x_char_size = 8;
	pos_info._y_char_size = 16;

	pos_info._frame_buf_addr = (int *)0xffff800000a00000;
	// 每个像素4B
	pos_info._frame_buf_length = pos_info._x_resolution * pos_info._y_resolution * 4;

	color_printk(YELLOW, BLACK, "\bHello\tWorld!%d", 2020);

	while(1);
}
