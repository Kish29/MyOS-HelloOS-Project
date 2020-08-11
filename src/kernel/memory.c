#include "memory.h"
#include "lib.h"

void init_memory() {
	int i, j;
	unsigned long total_mem = 0;
	struct E820 *p = NULL;

	color_printk(ONE_BLUE, BLACK, "Display Physics Address Map, Type(1:RAM, 2:ROM or Reserved, 3:ACPI Reclaim Memory, 4:ACPI Memory, Others:Undefined)\n");

	p = (struct E820 *)0xffff800000007e00;

	for(i = 0; i < 32; i++) {		// 一般来说，结构体数量不会超过32个
		color_printk(ONE_GREEN, BLACK, "Address:%#018lx\tLength:%018lx\tType:%#010x\n", p->addr, p->len, p->type);

		if(p->type == 1)		// RAM可用内存
			total_mem += p->len;

		memory_management_struct.e820[i].addr = p->addr;
		memory_management_struct.e820[i].len = p->len;
		memory_management_struct.e820[i].type = p->type;
		memory_management_struct.e820_num = i;

		p++;		// p = p + 20B
		if(p->type > 4 || p->type < 1 || p->len == 0)	// 截断脏数据，只读取到前面都是有效数据的部分
			break;
	}

	/* convert 16-scale to MB/GB/TB
	 * */
	unsigned long total_mem_decimal = 0;

	unsigned long tmp = total_mem;
	int val = 1;
	while(tmp) {
		total_mem_decimal += (tmp & 0xf) * val;
		val <<= 4;
		tmp >>= 4;
	}

	char *suffix = "B";

	if(1024 < total_mem_decimal) {
		total_mem_decimal >>= 10;		// KB
		suffix = "KB";
		if(1024 < total_mem_decimal) {
			total_mem_decimal >>= 10;	//MB
			suffix = "MB";
		}
	}

	color_printk(ONE_AQUA, BLACK, "OS Can Used Total RAM:%#018lx\t%d %s\n", total_mem, total_mem_decimal, suffix);

	// 将得到的物理内存按照2MB页大小进行地址的对其
	total_mem = 0;
	for(i = 0; i <= memory_management_struct.e820_num; i++) {
		unsigned long start_addr, end_addr;
		if(memory_management_struct.e820[i].type != 1)
			continue;
		start_addr = PAGE_2M_ALIGN(memory_management_struct.e820[i].addr);
		// 先右移21位，再左移回21位是为end处地址留出不足2MB的物理内存，即向下对齐
		// 如(假设4K页大小)：start_addr = 0x1000, len = 0x2555 -> end_addr = 0x3000，留出0x555的地址偏移
		end_addr = ((start_addr + memory_management_struct.e820[i].len) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
		if(end_addr <= start_addr)
			continue;			// 不足一个页
		total_mem += ((end_addr - start_addr) >> PAGE_2M_SHIFT);		// 统计可用2MB物理页数量
	}

	color_printk(ONE_BLUE_LIGHT, BLACK, "OS Can Used Total Num of 2MB PAGE(S):%#010x=%#010d\n", total_mem, total_mem);

	// 将total_mem设置成物理内存的结束地址，通常是最后一个结构体的起始地址+大小
	total_mem = memory_management_struct.e820[memory_management_struct.e820_num].addr + memory_management_struct.e820[memory_management_struct.e820_num].len;
	
	/* 设置bit-map, bit-map保存在kernel程序结尾后预留4KB的内存空间处
	 * 防止一些错误的内存操作 */
	memory_management_struct.bits_map = (unsigned long *)((memory_management_struct.kernel_end_addr + PAGE_4K_SIZE) & PAGE_4K_MASK);
	memory_management_struct.bits_num = total_mem >> PAGE_2M_SHIFT;
	color_printk(ONE_PURPLE, BLACK, "Total Memory Size is:%#018lx=%#018ld, And The Nums of 2MB PAGE(S) for Total:%#010x=%#010d\n", total_mem, total_mem, memory_management_struct.bits_num, memory_management_struct.bits_num);
}
