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
		// end_addr别用start_addr去加，因为start_addr向上对齐了的，有可能内存出界
		end_addr = ((memory_management_struct.e820[i].addr + memory_management_struct.e820[i].len) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
		if(end_addr <= start_addr)
			continue;			// 不足一个页
		total_mem += ((end_addr - start_addr) >> PAGE_2M_SHIFT);		// 统计可用2MB物理页数量
	}

	color_printk(ONE_BLUE_LIGHT, BLACK, "OS Can Used Total Num of 2MB PAGE(S):%#010x=%#010d\n", total_mem, total_mem);

	// 将total_mem设置成物理内存的结束地址，通常是最后一个结构体的起始地址+大小
	total_mem = memory_management_struct.e820[memory_management_struct.e820_num].addr + memory_management_struct.e820[memory_management_struct.e820_num].len;
	
	/* 设置bit-map, bit-map保存在kernel程序结尾后预留4KB的内存空间处
	 * 防止一些错误的内存操作 */
	memory_management_struct.bits_map = (unsigned long *)((memory_management_struct.kernel_end_addr + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
	memory_management_struct.bits_num = total_mem >> PAGE_2M_SHIFT;
	color_printk(ONE_PURPLE, BLACK, "Total Memory Size is:%#018lx=%#018ld, And The Nums of 2MB PAGE(S) for Total:%#010x=%#010d\n", total_mem, total_mem, memory_management_struct.bits_num, memory_management_struct.bits_num);
	
	// 向上按照long型数据进行对齐
	// bits_map的数据类型是unsigned long(8B), bits_map的每一个bit位代表一个Page结构体，置位(1)代表已使用/ROM，复位(0)代表空闲
	// bits_len = 264B，1B=8bit -> 8 Pages
	memory_management_struct.bits_len = ((memory_management_struct.bits_num + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));
	// 预设置全为1，以标注内存空洞和ROM
	memset(memory_management_struct.bits_map, 0xff, memory_management_struct.bits_len);
	

	/* 初始化 Pages Struct */
	memory_management_struct.pages_struct = (struct Page *)(((unsigned long)memory_management_struct.bits_map + memory_management_struct.bits_len + PAGE_4K_SIZE - 1) & (PAGE_4K_MASK));
	memory_management_struct.pages_num = memory_management_struct.bits_num;
	memory_management_struct.pages_len = (memory_management_struct.pages_num * sizeof(struct Page) + sizeof(long) - 1) & (~(sizeof(long) - 1));
	memset(memory_management_struct.pages_struct, 0x00, memory_management_struct.pages_len);

	/* 初始化 Zone */
	memory_management_struct.zones_struct = (struct Zone *)(((unsigned long)memory_management_struct.pages_struct + memory_management_struct.pages_len + PAGE_4K_SIZE - 1) & (PAGE_4K_MASK));
	// 由于现在不知道Zone的数量，先暂时设置为0，大小暂时设置为5个Zone结构体的大小
	memory_management_struct.zones_num = 0;
	memory_management_struct.zones_len = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));
	memset(memory_management_struct.zones_struct, 0x00, memory_management_struct.zones_len);
	
	// 遍历物理内存结构体数组E820，找到RAM，并初始化各个内存管理结构体
	for(i = 0; i <= memory_management_struct.e820_num; i++) {
		unsigned long start_addr, end_addr;
		struct Page *page;
		struct Zone *zone;

		if(memory_management_struct.e820[i].type != 1)
			continue;
		start_addr = PAGE_2M_ALIGN(memory_management_struct.e820[i].addr);
		end_addr = ((memory_management_struct.e820[i].addr + memory_management_struct.e820[i].len) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;

		// 其实在屏幕的输出可以看到，第一个type=1的RAM不足2MB，但他却包含了内核程序，系统BIOS，中断向量表等等数据
		if(end_addr <= start_addr)
			continue;

		// Zone 初始化，zones_num初始为1
		zone = memory_management_struct.zones_struct + memory_management_struct.zones_num;
		memory_management_struct.zones_num++;

		zone->zone_addr_start = start_addr;
		zone->zone_addr_end = end_addr;
		zone->zone_length = zone->zone_addr_end - zone->zone_addr_start;

		zone->page_using_count = 0;
		zone->page_free_count = (end_addr - start_addr) >> PAGE_2M_SHIFT;

		zone->page_total_referenced = 0;
		zone->attr = 0;
		zone->GMD_struct = &memory_management_struct;

		zone->pages_num = (end_addr - start_addr) >> PAGE_2M_SHIFT;

		/* 这里为什么page_group的地址要加上start_addr >> PAGE_2M_SHIFT：
		 * start_addr >> PAGE_2M_SHIFT得到是当前RAM地址前面的可用2MB物理页数量
		 * 而Page管理结构体要与物理内存一一对应，所以，当前page_group地址 = page_struct地址+之前的已用2MB物理页数量
		 * */
		zone->page_group = (struct Page *)(memory_management_struct.pages_struct + (start_addr >> PAGE_2M_SHIFT));
		
		// page init 
		page = zone->page_group;
		for(j = 0; j < zone->pages_num; j++, page++) {
			page->zone_struct = zone;
			page->phy_addr_start = start_addr + PAGE_4K_SIZE * j;
			page->attr = 0;

			page->referenced_count = 0;
			page->created_time = 0;
			
			// 这儿 >> 6位，就是除以2^6=64
			// 和配置的bits_map每一项8B相对应
			// 同理，右边 -> '%64'获得余数即对应的bit(page号)，从右往左看
			// 用异或操作对相应的bit位置0，表示该区域是可用RAM
			*(memory_management_struct.bits_map + ((page->phy_addr_start >> PAGE_2M_SHIFT) >> 6)) ^= (1UL << ((page->phy_addr_start >> PAGE_2M_SHIFT) % 64));
		}
	}

	///////////////
	// 重新初始化物理页0号，因为0～2MB的物理页很特殊，包含内存空洞、系统bios、中断向量表、内核程序等
	
	memory_management_struct.pages_struct->zone_struct = memory_management_struct.zones_struct;
	memory_management_struct.pages_struct->phy_addr_start = 0UL;
	memory_management_struct.pages_struct->attr = 0;
	memory_management_struct.pages_struct->referenced_count = 0;
	memory_management_struct.pages_struct->created_time = 0;

	///////////////
	
	// 得到最终的zone_len
	memory_management_struct.zones_len = (memory_management_struct.zones_num * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));

	///////////////

	// 打印得到的所有数据
	/*
	 * bits_map
	 */
	color_printk(ONE_BLUE_LIGHT, BLACK, "bits_map:%#018lx, bits_num:%#018lx=%#018ld, bits_len:%#018lx\n", memory_management_struct.bits_map, memory_management_struct.bits_num, memory_management_struct.bits_num, memory_management_struct.bits_len);

	/* pages_struct
	 * */
	color_printk(ONE_BLUE_LIGHT, BLACK, "pages_struct:%#018lx, pages_num:%#018lx=%#018ld, pages_len:%#018lx\n", memory_management_struct.pages_struct, memory_management_struct.pages_num, memory_management_struct.pages_num, memory_management_struct.pages_len);

	/* zones_struct
	 * */
	color_printk(ONE_BLUE_LIGHT, BLACK, "zones_struct:%#018lx, zones_num:%#018lx=%#018ld, zones_len:%#018lx\n", memory_management_struct.zones_struct, memory_management_struct.zones_num, memory_management_struct.zones_num, memory_management_struct.zones_len);

	ZONE_DMA_INDEX = 0;
	ZONE_NORMAL_INDEX = 0;	// 因为暂时不知道其对应的Zone，暂时指向同一个Zone

	// 遍历所得的Zone结构体
	for(i = 0; i < memory_management_struct.zones_num; i++) {
		struct Zone *z = memory_management_struct.zones_struct + i;

		color_printk(ONE_PURPLE, BLACK, "zone%d_start_addr:%#018lx, zone%d_end_addr:%#018lx, zone%d_length:%#018lx=%#018ld\n", i, z->zone_addr_start, i, z->zone_addr_end, i, z->zone_length, z->zone_length);
		color_printk(ONE_PURPLE, BLACK, "zone%d:\npage_group:%#018lx, page_num:%#018lx=%#018ld\n", i, z->page_group, z->pages_num, z->pages_num);

		// 在之前的head.S中，已经定义了512个2MB的页表项，囊括了1GB的物理空间
		// 一直到0x100000000物理地址之前
		// 那么超出这个地址之后后的物理空间还没有经过页表的映射
		if(z->zone_addr_start == 0x100000000)
			ZONE_UNMAPED_INDEX = i;		// 记录Zone的索引
	}
	
	// 最后调整GMD全局内存管理结构的结束地址，并预留一段空间防止内存越界
	memory_management_struct.end_of_struct = (unsigned long)((unsigned long)memory_management_struct.zones_struct + memory_management_struct.zones_len + sizeof(long) * 32) & (~(sizeof(long) - 1));
	
	// 打印在main.c文件中初始化的code_start_code等符号的地址
	color_printk(ONE_GREEN, BLACK, "code_start_addr:%#018lx, code_end_addr:%#018lx, data_end_addr:%#018lx, kernel_end_addr:%#018lx",memory_management_struct.code_start_addr, memory_management_struct.code_end_addr, memory_management_struct.data_end_addr, memory_management_struct.kernel_end_addr);
	



















}
