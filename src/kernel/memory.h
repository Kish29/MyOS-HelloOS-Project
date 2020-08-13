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
// '-1'的目的是考虑到地址刚好为2MB的倍数的时候，不会加1
#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

// 操作系统不直接操作物理地址
#define Virtual_To_Physic(addr)	((unsigned long)(addr) - KERNEL_START_OFFSET)
// 因为操作系统操作的是虚拟地址（线性地址）,所以要将它转化为指针类型，方便调用时使用
#define Physic_To_Virtual(addr)	((unsigned long *)((unsigned long)(addr) + KERNEL_START_OFFSET))

// 根据虚拟地址获得对应某个2MB页表
#define	Virtual_To_2M_Page(knl_addr) (memory_management_struct.pages_struct + (Virtual_To_Physic(knl_addr) >> PAGE_2M_SHIFT))
// 根据物理地址获得对应的某个2MB页表
#define Physic_To_2M_Page(knl_addr) (memory_management_struct.pages_struct + ((unsigned long)knl_addr >> PAGE_2M_SHIFT))

/*******************
 * 定义2MB page的属性
 *******************/
// bit-63 Execution Disable
// #define PAGE_XD		(unsigned long)0x8000000000000000
#define PAGE_XD		(1UL << 63)

// bit-12 Page Atrribute Table
// #define PAGE_PAT	(unsigned long)0x1000
#define PAGE_PAT	(1UL << 12)

// bit-8 Global Page->  1:global,	0:local 
// #define PAGE_Global	(unsigned long)0x0100
#define PAGE_Global	(1UL << 8)

// bit-7 Page Size->	1:big page(maybe 1GB/2MB)	0:small page 
// #define PAGE_PS		(unsigned long)0x0080
#define	PAGE_PS		(1UL << 7)

// bit-6 Page Dirty->	1:dirty,	0:clean 
// #define PAGE_Dirty	(unsigned long)0x0040 
#define PAGE_Dirty	(1UL << 6)

// bit-5 Page Accessed->	1:visited	0:unvisited
// #define PAGE_Accessed	(unsigned long)0x0020 
#define PAGE_Accessed	(1UL << 5)

// bit-4 Page Level Cache Disable->		1:not permitted		0:permitted
// #define PAGE_PCD	(unsigned long)0x0010 
#define	PAGE_PCD	(1UL << 4)

// bit-3 Page Level Write Back->		1:write-through		0:write-back
// #define	PAGE_PWT	(unsigned long)0x0008
#define	PAGE_PWT	(1UL << 3)

// bit-2 Page User/Supervisor->			1:user and supervisor	0:supervisor
// #define PAGE_U_S	(unsigned long)0x0004
#define	PAGE_U_S	(1UL << 2)

// bit-1 Page Write/Read->		1:Read/Write	0:Read
// #define	PAGE_R_W	(unsigned long)0x0002
#define	PAGE_R_W	(1UL << 1)

// bit-0 Page Present->		1:present	0:not present 
// #define	PAGE_Present	(unsigned long)0x0001
#define PAGE_Present	(1UL << 0)

// bit-0,1
#define PAGE_KERNEL_GDT		(PAGE_R_W | PAGE_Present)

// bit-0,1
#define PAGE_KERNEL_Dir		(PAGE_R_W | PAGE_Present)

// bit-0,1,7
#define PAGE_KERNEL_PAGE	(PAGE_PS | PAGE_R_W | PAGE_Present)

// bit-0,1,2
#define PAGE_USER_Dir		(PAGE_U_S | PAGE_R_W | PAGE_Present)

// bit-0,1,2,7
#define PAGE_USER_PAGE		(PAGE_PS | PAGE_U_S | PAGE_R_W | PAGE_Present)

/*定义各层级的页表描述符*/

typedef struct {
	unsigned long pml4t;
}pml4t_t;
#define mk_pml4t(addr, attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pml4t(pml4t_ptr, pml4t_val)		(*(pml4t_ptr) = (pml4t_val))

typedef struct {
	unsigned long pdpt;
}pdpt_t;
#define mk_pdpt(addr, attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdpt_ptr, pdpt_val)		(*(pdpt_ptr) = (pdpt_val))

typedef struct {
	unsigned long pdt;
}pdt_t;
#define mk_pdt(addr, attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdt_ptr, pdt_val)		(*(pdt_ptr) = (pdt_val))

typedef struct {
	unsigned long pt;
}pt_t;
#define mk_pt(addr, attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pt(pt_ptr, pt_val)		(*(pt_ptr) = (pt_val))

// CR3保存的最高层页基地址
unsigned long *Global_CR3 = NULL;

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

/* 自定义页结构体属性
 * 在分配页的时候用到
 * */
#define PG_PTable_Maped		(1 << 0)

#define PG_Kernel_Init		(1 << 1)

#define PG_Referenced		(1 << 2)

#define PG_Dirty			(1 << 3)

#define PG_Active			(1 << 4)

#define PG_Up_To_Date		(1 << 5)

#define PG_Device			(1 << 6)

#define	PG_Kernel			(1 << 7)

#define	PG_Knl_Share_To_Usr	(1 << 8)

#define	PG_Slab				(1 << 9)

/* 定义2MB物理页的结构体
 */
struct Page {
	struct Zone *zone_struct;	// 指向本页所属的内存区域结构体
	unsigned long phy_addr_start;		// 页的物理起始地址
	unsigned long attr;				// 本页的属性
	unsigned long referenced_count;			// 被引用的次数，同一物理页可被多个不同的线性地址映射，故使用referenced_count，并且used_count <= referenced_count
	unsigned long created_time;		// 本页的创建时间
};



/* 自定义内存管理结构Zone的属性
 * 再分配页的时候将用到
 * */
#define ZONE_DMA		(1 << 0)

#define ZONE_NORMAL		(1 << 1)

#define ZONE_UNMAPED	(1 << 2)

/* 区域内存管理结构体 */
struct Zone {
	struct Page *page_group;		// 指向本区域所管理的page结构体数组指针
	unsigned long pages_num;		// 本区域所管理的page数组长度

	unsigned long zone_addr_start;
	unsigned long zone_addr_end;
	unsigned long zone_length;
	unsigned long attr;

	struct Global_Memory_Descriptor *GMD_struct;	// 指向全局内存管理结构体
	
	unsigned long page_using_count;		// 使用中的pages
	unsigned long page_free_count;		// 空闲物理内存数量

	unsigned long page_total_referenced;	// 本区域物理页被引用次数总数
};

// 记录一些Zone的索引index
int ZONE_DMA_INDEX = 0;			// DMA部分的内存专供I/O设备DMA中断
// 因为DMA使用物理地址访问内存，不经过MMU，并且需要连续的缓冲区，所以需要特定预留

int ZONE_NORMAL_INDEX = 0;		// 内核可以自由使用

// bochs虚拟机最大物理内存4GB，超出这部分的需要重新映射
int ZONE_UNMAPED_INDEX = 0;

// max zone num 
#define MAX_NR_ZONES	10



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

unsigned long page_init(struct Page *page, unsigned long flags);

unsigned long page_clean(struct Page *page);

void init_memory();

struct Page *alloc_pages(int zone_select, int number, unsigned long page_flags);

// invlpg -> flush the page entry which is specified
#define	flush_tlb_one(addr)	\
	__asm__	__volatile__	("invlpg	(%0)	\n\t"::"r"(addr):"memory")

// 重新赋值cr3寄存器，刷新TLB，使新的页表项有效
#define	flush_tlb()								\
do {											\
	unsigned long tmp_reg;						\
	__asm__	__volatile__	(					\
				"movq	%%cr3,	%0		\n\t"	\
				"movq	%0,		%%cr3	\n\t"	\
				:"=r"(tmp_reg)					\
				:								\
				:"memory"						\
			);									\
}while(0)

unsigned long *Get_CR3();
inline unsigned long *Get_CR3() {
	unsigned long *bsc_addr_in_cr3_ptr;
	__asm__	__volatile__	(
				"movq	%%cr3,	%0	\n\t"
				:"=r"(bsc_addr_in_cr3_ptr)
				:
				:"memory"
			);
	return bsc_addr_in_cr3_ptr;
}

#endif
