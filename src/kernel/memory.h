#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"

// 定义页表项数量
#define PTRS_PER_PAGE	512

#define KERNEL_START_OFFSET	((unsigned long)0xffff800000000000)

#define PAGE_PML4_SHIFT	39
#define PAGE_1G_SHIFT	30
#define	PAGE_2M_SHIFT	21
#define	PAGE_4K_SHIFT	12

#define	PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define	PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

// 将传入的物理页起始地址按照2MB/4KB对齐
#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE) & PAGE_4K_MASK)

// 操作系统不直接操作物理地址
#define Virtual_To_Physic(addr)	((unsigned long)(addr) - KERNEL_START_OFFSET)
// 因为操作系统操作的是虚拟地址（线性地址）,所以要将它转化为指针类型，方便调用时使用
#define Physic_To_Virtual(addr)	((unsigned long *)((unsigned long)(addr) + KERNEL_START_OFFSET))

// 为对齐数据，全部采用int类型
// struct Memory_E820_Format {
// 	unsigned int addr_low;
// 	unsigned int addr_high;
// 	unsigned int len_low;
// 	unsigned int len_high;
// 	unsigned int type;
// };

struct E820 {
	unsigned long addr;
	unsigned long len;
	unsigned int type;
}__attribute__((packed));	// packed修饰该结构体不生成对齐空间，改用紧凑格式，保证每个结构体20B

/* 定义2MB物理页的结构体
 */
struct Page {
	struct Zone *zone_struct;	// 指向本页所属的内存区域结构体
	unsigned long phy_addr_start;		// 页的物理起始地址
	unsigned long attr;				// 本页的属性
	unsigned long referenced_count;			// 被引用的次数，同一物理页可被多个不同的线性地址映射，故使用referenced_count，并且used_count <= referenced_count
	unsigned long created_time;		// 本页的创建时间
};

/* 区域内存管理结构体 */
struct Zone {
	struct Page *page_group;		// 指向本区域所管理的page结构体数组指针
	unsigned long page_nums;		// 本区域所管理的page数组长度

	unsigned long zone_addr_start;
	unsigned long zone_addr_end;
	unsigned long zone_length;
	unsigned long attr;

	struct Global_Memory_Descriptor *GMD_struct;	// 指向全局内存管理结构体
	
	unsigned long page_using_count;		// 使用中的pages
	unsigned long page_free_count;		// 空闲物理内存数量

	unsigned long page_total_referenced;	// 本区域物理页被引用次数总数
};

/* 全局内存管理结构体 */
struct Global_Memory_Descriptor {
	struct E820 e820[32];
	unsigned long e820_num;

	unsigned long *bits_map;		// 空间页的映射位图，与页一一对应,用于检索空闲和使用中的物理页
	unsigned long bits_num;
	unsigned long bits_len;

	struct Page *pages_struct;		// 页指针
	unsigned long pages_num;
	unsigned long pages_len;

	struct Zone *zones_struct;		// 区域指针
	unsigned long zones_num;
	unsigned long zones_len;

	unsigned long code_start_addr, code_end_addr, data_end_addr, kernel_end_addr;		//这些属性从Kernel.lds链接脚本中获得

	unsigned long end_of_struct;	// 内存页管理结构的结束地址
};

extern struct Global_Memory_Descriptor memory_management_struct;

void init_memory();

#endif
