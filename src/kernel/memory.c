#include "lib.h"
#include "memory.h"

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
	// bits_map的数据类型是unsigned long(8B), bits_map的每一个bit位代表一个Page结构体
	// 而每一行bits_map(8B)能够代表64个物理页
	// 置位(1)代表已使用/ROM，复位(0)代表空闲
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
			page->phy_addr_start = start_addr + PAGE_2M_SIZE * j;		// 不要写成了PAGE_4K_SHIFT，不然下面bits_map置位的时候由于偏移量沒统一导致重复置位同一bit导致异常
			page->attr = 0;

			page->referenced_count = 0;
			page->created_time = 0;

			// 这儿 >> 6位，就是除以2^6=64
			// 和配置的bits_map每一项8B(每一项64个物理页)相对应
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

		// bochs虚拟机最大物理内存4GB，超出这部分的需要重新映射
		if(z->zone_addr_start == 0x100000000)
			ZONE_UNMAPED_INDEX = i;		// 记录Zone的索引
	}
	
	// 最后调整GMD全局内存管理结构的结束地址，并预留一段空间防止内存越界
	memory_management_struct.end_of_struct = (unsigned long)((unsigned long)memory_management_struct.zones_struct + memory_management_struct.zones_len + sizeof(long) * 32) & (~(sizeof(long) - 1));
	
	// 打印在main.c文件中初始化的code_start_code等符号的地址
	color_printk(ONE_GREEN, BLACK, "code_start_addr:%#018lx, code_end_addr:%#018lx, data_end_addr:%#018lx\nkernel_end_addr:%#018lx, end_of_struct:%#018lx\n",memory_management_struct.code_start_addr, memory_management_struct.code_end_addr, memory_management_struct.data_end_addr, memory_management_struct.kernel_end_addr, memory_management_struct.end_of_struct);
	
	// 将内核于内存管理结构所占用的物理页一个一个初始化(所占用的物理页到end_of_struct结束)
	i = Virtual_To_Physic(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT; // 得到2MB物理页数量
	// 初始化
	for(j = 0; j < i; j++) {
		page_init(memory_management_struct.pages_struct + j, PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel);
	}

	Global_CR3 = Get_CR3();
	// 打印验证cr3
	color_printk(ONE_INDIGO, BLACK, "Global_CR3:\t\t%#018lx\n", Global_CR3);
	// head.S中"movq $0x10100, cr3"保存的是物理地址，
	// 而在head.S中定义的低层页表项的地址却是线性地址，所以要转化成线性地址
	// 虽然这些线性地址映射的物理地址和直接*Global_CR3是一样的，但是按照逻辑，这样处理更好
	// 然后根据IA-32e模式的寻址模式，'*'取内容得到该线性地址的保存值
	// 这儿与'0xff'求与是剔除掉低位的属性，防止寻址出错
	color_printk(ONE_INDIGO, BLACK, "*Global_CR3:\t%#018lx\n", *Physic_To_Virtual(Global_CR3) & (~ 0xff));
	// color_printk(ONE_INDIGO, BLACK, "*Global_CR3: \t%#018lx\n", *Global_CR3 & (~0xff));
	// color_printk(ONE_INDIGO, BLACK, "**Global_CR3:\t%#018lx\n", *((unsigned long *)(*Global_CR3 & (~ 0xff))) & (~ 0xff));
	color_printk(ONE_INDIGO, BLACK, "**Global_CR3:\t%#018lx\n", *Physic_To_Virtual(*Physic_To_Virtual(Global_CR3) & (~ 0xff)) & (~ 0xff));


	// 消除一致性页表映射，先清零PML4的前10个物理页表
	// 因为内核所使用的线性地址是0xffff80... -> 0xffffff...，那么按照页表映射，去除0xffff的符号，0x8...选择PML4的第256项及其之后的项
	// 在head.S中，第256项和第0项都被定义成0x102007
	// 而用户所使用的线性地址为0x0000000 -> 0x00007fffff...，那么按照也表映射，去除0x0000的符号，0x0.. -> 0x7选择PML4的第0项到255项
	// 所以，为了防止用户以及应用程序访问到内核的页表，需要将第一个置0，起保护作用
	// for(i = 0; i< 10; i++)
	// 	*(Physic_To_Virtual(Global_CR3 + i)) = 0UL;

	// 刷新TLB使其生效
	flush_tlb();

	// color_printk(ONE_BLUE, BLACK, "After setting CR3 and flushing TLB\n");
	// color_printk(ONE_INDIGO, BLACK, "*0xffff800000101800:\t%#018lx\n", (*((unsigned long *)0xffff800000101800) & (~ 0xff)));
	// color_printk(ONE_INDIGO, BLACK, "*0xffff800000102000:\t%#018lx\n", (*((unsigned long *)0xffff800000102000) & (~ 0xff)));
	// color_printk(ONE_INDIGO, BLACK, "*0xffff800000103000:\t%#018lx\n", (*((unsigned long *)0xffff800000103000) & (~ 0xff)));
	// color_printk(ONE_INDIGO, BLACK, "*0xffff800000103028:\t%#018lx\n", (*((unsigned long *)0xffff800000103028) & (~ 0xff)));
}


unsigned long page_init(struct Page *page, unsigned long flags) {
	// 如果是新的未初始化的页
	if(page->attr == 0) {
		// 置位
		*(memory_management_struct.bits_map + ((page->phy_addr_start >> PAGE_2M_SHIFT) >> 6)) |= (1UL << ((page->phy_addr_start >> PAGE_2M_SHIFT) % 64));
		page->attr = flags;
		page->referenced_count++;
		page->zone_struct->page_using_count++;
		page->zone_struct->page_free_count--;
		page->zone_struct->page_total_referenced++;
	} else if((page->attr & PG_Referenced) || (page->attr & PG_Knl_Share_To_Usr) || (flags & PG_Referenced) || (flags & PG_Knl_Share_To_Usr)) {
		// 如果该页被使用或者是内核用户共用页
		page->attr |= flags;
		page->referenced_count++;
		page->zone_struct->page_total_referenced++;
	} else {
		// 置位
		*(memory_management_struct.bits_map + ((page->phy_addr_start >> PAGE_2M_SHIFT) >> 6)) |= (1UL << ((page->phy_addr_start >> PAGE_2M_SHIFT) % 64));
		page->attr |= flags;
	}
	return 0;
}


unsigned long page_clean(struct Page *page) {
	if(page->attr == 0)
		return 0;
	// 如果该页已经被引用或者具有共享属性
	else if((page->attr & PG_Referenced) || (page->attr & PG_Knl_Share_To_Usr)) {
		page->referenced_count--;
		page->zone_struct->page_total_referenced--;
		// 如果此时该页没有了引用
		if(page->referenced_count == 0) {
			page->attr = 0;
			page->zone_struct->page_using_count++;
			page->zone_struct->page_free_count++;
		}
	} else {		// 其他情况释放
		// 复位
		*(memory_management_struct.bits_map + ((page->phy_addr_start >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << ((page->phy_addr_start >> PAGE_2M_SHIFT) % 64));
		
		page->attr = 0;
		page->referenced_count = 0;
		page->zone_struct->page_free_count++;
		page->zone_struct->page_using_count--;
		page->zone_struct->page_total_referenced--;
	}

	return 0;
}


/*	number <= 64
 *	zone_select: select from DMA, mapped in pagetable, unmapped in pagetable
 *	page_flags: struct Page attribute
 * */
struct Page *alloc_pages(int zone_select, int number, unsigned long page_flags) {
	int i;
	// page 记录找到的连续number个空闲物理页的偏移
	unsigned long page = 0;

	int zone_start = 0;
	int zone_end = 0;

	// 选择相应的zone
	switch(zone_select) {
		case ZONE_DMA:		// 1
			zone_start = 0;
			zone_end = ZONE_DMA_INDEX;
			break;

		case ZONE_NORMAL:	// 2
			zone_start = ZONE_DMA_INDEX;
			zone_end = ZONE_NORMAL_INDEX;
			break;

		case ZONE_UNMAPED:	// 4
			zone_start = ZONE_UNMAPED_INDEX;
			zone_end = memory_management_struct.zones_num - 1;
			break;

		default:
			color_printk(RED, BLACK, "alloc_pages error by zone_select index\n");
			return NULL;
			break;
	}

	// 遍历该部分的所有的Zone区域，寻找符合申请条件的struct page 结构体数组
	for(i = zone_start; i <= zone_end; i++) {
		struct Zone *z;
		unsigned long j;
		unsigned long start, end, len;
		unsigned long tmp;

		if((memory_management_struct.zones_struct + i)->page_free_count < number)
			continue;

		z = memory_management_struct.zones_struct + i;
		start = z->zone_addr_start >> PAGE_2M_SHIFT;	// page 起始索引
		end = z->zone_addr_end >> PAGE_2M_SHIFT;		// page 结束索引
		len = z->zone_length >> PAGE_2M_SHIFT;			// 该区域page数量

		// 这部分画图容易理解，tmp是离下一个bits_map的差值
		tmp = 64 - (start % 64);

		// j变量设置成unsigned long的原因是为了数据对齐
		// 该循环第一次只能检索bits_map中tmp个bit位
		// 之后检测64个bit位
		for(j = start; j <= end; j += (j % 64? tmp: 64)) {		//自增部分是为了让第二次及以后每次从
			// bits_map的第0位开始检测64个bit位
			// 找到bits_map数组索引项
			unsigned long *pg_in_bitsmap_ptr = memory_management_struct.bits_map + (j >> 6);
			unsigned long shift = j % 64;		// bit偏移位置
			unsigned long k;

			// num是根据申请内存数量在8B的数据上进行了置位操作
			// 如：number = 3 -> num: ...00011b
			unsigned long num = (1UL << number) - 1;
			for(k = shift; k < 64; k++) {
				// 判断内存页是否是空闲状态
				// 为了检索出连续的number个(<=64)空闲物理页
				// (k ? ((*p >> k) | (*(p + 1) << (64 - k)) 这段代码的作用是
				// 将当前高（64-k）位和下一组底k位组合成一个新的64bit数据和num求与，从而判断出一个连续的number个物理页 
				// 示意图：
				// 当前的检索状态          k
				// +-----------------------+----+
				// |***********************|    |   <- p
				// +-----------------------+----+
				// |                       |xxxx|   <- p + 1
				// +-----------------------+----+
				// 整合后的判断值:
				//       k                 k
				// +-----+-----------------+----+
				// |xxxxx|**********************|
				// +-----+-----------------+----+
				if(((k ? ((*pg_in_bitsmap_ptr >> k) | (*(pg_in_bitsmap_ptr + 1) << (64 - k))) : *pg_in_bitsmap_ptr) & num ) == 0) {	// k = 0说明在起始bit处
					// 如果找到连续number个空闲
					unsigned long l;
					// page 记录找到的连续number个空闲物理页的偏移
					page = j + k - 1;
					// 分配number个物理页
					for(l = 0; l < (unsigned long)number; l++) {
						struct Page *x = memory_management_struct.pages_struct + page + l;
						page_init(x, page_flags);
					}
					goto alloc_pages_succeed;
				}
			}
		}
	}

	// 如果申请失败
	return NULL;

alloc_pages_succeed:
	
	return (struct Page *)(memory_management_struct.pages_struct + page);
}
