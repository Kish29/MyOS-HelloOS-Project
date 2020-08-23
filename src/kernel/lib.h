/****************
 * jar kernel_lib
 ****************/

#ifndef	__LIB_H__
#define	__LIB_H__ 

#define	NULL 0
#define true 1
#define false 0

/* container_of函数将根据一个成员变量的地址推算出父结构体的地址
 * 其精髓在于使用地址0使p先得到member在整个内存中的地址(第一行代码)
 * 然后再减去member相对于单个结构体的偏移就是父结构体在内存中的偏移 */
#define container_of(ptr, type, member)			\
({												\
	typeof(((type *)0)->member) * p = (ptr);		\
	(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));	\
})

/* start interupt */
#define sti()		__asm__	__volatile__	("sti	\n\t":::"memory")
/* clear interupt */
#define cli()		__asm__	__volatile__	("cli	\n\t":::"memory")
#define nop()		__asm__	__volatile__	("nop	\n\t")
#define	io_mfence()		__asm__	__volatile__	("mfence	\n\t":::"memory")
/* mfence sfence lfence 三条指令，其中mfence指令保证在对内存的操作的时候
 * 上一次的内存操作已经完整地结束，通常用于指令流水线*/

/* 链表 */
struct _list {
	struct _list *prev;
	struct _list *next;
};

/*所有的inline函数都必须先声明，再定义成inline！*/
void list_init(struct _list *list);

void list_add_to_tail(struct _list *tail, struct _list *node);

int list_delete(struct _list *del_node);

unsigned char list_is_empty(struct _list *node);

struct _list *get_node_prev(struct _list *node);

struct _list *get_node_next(struct _list *node);

void *memcpy(void *src, void *des, long num);

void *memset(void *addr, unsigned char c, long count);

int memcmp(void *_first_part, void *_second_part, long _count);

char *strcpy(char *src, char *des);

char *strncpy(char *src, char *des, long num);

char *strcat(char *src, char *des);

int strcmp(char *_first_str, char *_second_str);

int strncmp(char *_first_str, char *_second_str, long num);

int strlen(char *str);

void clear_screen(unsigned long *buf_addr, unsigned long buf_sz);

unsigned long bit_set(unsigned long *addr, unsigned long index);

unsigned long bit_get(unsigned long *addr, unsigned long index);

unsigned long bit_clean(unsigned long *addr, unsigned long index);

unsigned char io_in8(unsigned short port);

void io_out8(unsigned short port, unsigned char value);

unsigned int io_in32(unsigned short port);

void io_out32(unsigned short port, unsigned int value);

// 清空8024的读写缓冲区
void cls_8024_kybd_buf();

inline void list_init(struct _list *list) {
	list->prev = list;
	list->next = list;
}

inline void list_add_to_tail(struct _list *tail, struct _list *node) {
	if(tail->next == NULL || tail->prev == NULL || node->next == NULL || node->prev == NULL)
		return;
	tail->next->prev = node;
	node->next = tail->next;
	node->prev = tail;
	tail->next = node;
}

inline int list_delete(struct _list *del_node) {
	del_node->next->prev = del_node->prev;
	del_node->prev->next = del_node->next;
	return 0;
}

inline unsigned char list_is_empty(struct _list *node) {
	if(node->next == node && node->prev == node)
		return	true;
	else 
		return	false;
}

inline struct _list *get_node_prev(struct _list *node) {
	if(list_is_empty(node))
		return NULL;
	else 
		return node->prev;
}

inline struct _list *get_node_next(struct _list *node) {
	if(list_is_empty(node))
		return NULL;
	else 
		return node->next;
}

/* 参考本系统linux源码的版本 */
/* 注：void *占用8个字节 
 * memcpy，从src复制到des，一共num个字节 */
inline void *memcpy(void *src, void *des, long num) {
	int d0, d1, d2;
	__asm__	__volatile__	(
			"rep	\n\t"		/* rep 指令，movs?的前缀，在cx不等于0的情况下，对字符串重复进行操作*/
			"movsq	\n\t"			/* according to the b/w/d/q, mov ds:si -> es:di */
			"movq	%4, %%rcx	\n\t"	/* 按照上一个指令movsl，一次传送4个字节  */
			// 注意这儿的movq指令，因为64位机器上，long型的num变量（%4）默认8B，如果写成movl	%4, %%ecx 编译器就会报不支持mov指令错误！
			"andq	$7,	%%rcx	\n\t"	/* 和3求与，将rcx得到除以8的余数，按照ZF标志位，如果为0，则num为4的倍数，否则不为4的倍数，余数可能为1、2、3```7  */
			"jz	1f	\n\t"
			"rep	\n\t"
			"movsb	\n\t"		/* 不为4的倍数，每次传送1B将剩下的字节传送完  */
			"1:"
			:"=&c"(d0), "=&D"(d1), "=&S"(d2)	/* '&'表示输入输出不能使用相同的寄存器 */
			:"0"(num / 8), "q"(num), "1"(des), "2"(src)		/* 按照movsq 一次传送8B，那么ecx传送次数就是num/8 */
			:"memory"
			);
	return des;
}

/* 作者的版本
inline void * memcpy(void *From,void * To,long Num)
{
	int d0,d1,d2;
	__asm__ __volatile__	(	"cld	\n\t"
					"rep	\n\t"
					"movsq	\n\t"
					"testb	$4,%b4	\n\t"
					"je	1f	\n\t"
					"movsl	\n\t"
					"1:\ttestb	$2,%b4	\n\t"
					"je	2f	\n\t"
					"movsw	\n\t"
					"2:\ttestb	$1,%b4	\n\t"
					"je	3f	\n\t"
					"movsb	\n\t"
					"3:	\n\t"
					:"=&c"(d0),"=&D"(d1),"=&S"(d2)
					:"0"(Num/8),"q"(Num),"1"(To),"2"(From)
					:"memory"
				);
	return To;
} */

// set memory at addr with c, number is count 
inline void *memset(void *addr, unsigned char c, long count) {
	int d0, d1;
	// 因为设置的char c是一个字节(1B)，而我们每次要传送8B,所以乘上0x0101010101010101UL,
	// tmp的每一个字节都是对应的char c
	unsigned long tmp = c * 0x0101010101010101UL;
	__asm__	__volatile__	(
			"cld	\n\t"
			"rep	\n\t"
			"stosq	\n\t"     // store rax to es:[rdi]
			"movq	%3,	%%rcx	\n\t"
			"andq	$7,	%%rcx	\n\t"	// rcx设置为/8的余数
			"jz	1f	\n\t"
			"rep	\n\t"
			"stosb	\n\t"
			"1:	\n\t"
			:"=&c"(d0), "=&D"(d1)
			:"a"(tmp), "q"(count), "0"(count / 8), "1"(addr)
			:"memory"
			);
	return addr;
}


/*	if _first_part == _second_part		->		0
 *	else if	_first_part	> _second_part	->		1
 *	else if _first_part > _second_part	->	   -1
 *	_count is the compare times	*/
inline int memcmp(void *_first_part, void *_second_part, long _count) {
	register int __res;

	__asm__	__volatile__	(
			"cld	\n\t"
			"repe	\n\t"	/* rcx 不为0的情况下重复操作 */
			"cmpsb	\n\t"	/* ds:[si] compare es:[di]，ds:[si] - es:[di] */
			"je	1f	\n\t"	/* jmp 1f 中 f表示forward */
			"movl	$1,	%%eax	\n\t"
			"ja	1f	\n\t"	/* if ds:[si](_second_part) < es:[di](_first_part)  */
			"negl	%%eax	\n\t"	/*取负操作 */
			"1:	\n\t"
			:"=a"(__res)
			:"0"(0), "S"(_first_part), "D"(_second_part), "c"(_count)
			:
			);

	return __res;
}

/* string to strcpy num bytes */
inline char *strcpy(char *src, char *des) {
	__asm__	__volatile__	(
			"cld	\n\t"
			"1:		\n\t"
			"lodsb	\n\t"
			"stosb	\n\t"	/* stosb	将al中的值传送到es:[di] */
			"testb	%%al, %%al	\n\t"	/* 如果al为0，则表示string到末尾'\00'  */
			"jne	1b	\n\t"		/* b 表示back，向后跳转  */
			:
			:"S"(src), "D"(des)
			:
			);
	return des;
}


/* string copy num bytes */
inline char *strncpy(char *src, char *des, long num) {
	__asm__	__volatile__	(
			"cld	\n\t"
			"1:		\n\t"
			"decq	%2	\n\t"
			"js	2f	\n\t"		/* jmp if sign，检查SF标志位，如果结果为负则跳转 */
			"lodsb	\n\t"
			"stosb	\n\t"
			"testb	%%al, %%al	\n\t"
			"jne	1b	\n\t"
			"rep	\n\t"		/* ecx 等于0的时候自动停止  */
			"stosb	\n\t"
			"2:		\n\t"
			:
			:"S"(src), "D"(des), "c"(num)
			:
			);
	return des;
}

/* string cat src + des (拼接两个字符串) */
inline char *strcat(char *src, char *des) {
	__asm__	__volatile__	(
			"cld	\n\t"
			"repne	\n\t"		/* di、si自增直到，找到两个字符串不一样的位置*/
			"scasb	\n\t"		/* al compare es[di]*/
			"decq	%1	\n\t"	/* 除去二个字符串中重复的位置 */
			"1:"
			"lodsb	\n\t"		/* load byte from ds:[si] */
			"stosb	\n\t"		/* store byte from al to es:[di] */
			"testb	%%al, %%al	\n\t"
			"jne	1f	\n\t"
			:
			:"S"(src), "D"(des), "a"(0), "c"(0xffffffff)	/* cx初值设置为-1，以确保字符串的每个字符均能被操作 */
			:
			);
	return des;
}

/* two strings compare value 
 * if _first_str == _second_str			0
 * else if _first_str > _second_str		1
 * else if _first_str < _second_str	   -1  */
inline int strcmp(char *_first_str, char *_second_str) {
	register int __res;
	__asm__ __volatile__	(
			"cld	\n\t"
			"1:		\n\t"
			"lodsb	\n\t"	/*load byte from ds:[si] to al*/
			"scasb	\n\t"	/*compare al to es:[di]*/
			"jne	2f	\n\t"
			"testb	%%al, %%al	\n\t"
			"jne	1b	\n\t"
			"xorl	%%eax, %%eax	\n\t"
			"jmp	3f	\n\t"
			"2:		\n\t"
			"movl	$1, %%eax	\n\t"	/*先设置为1，再判断大小*/
			"ja		3f	\n\t"
			"negl	%%eax	\n\t"
			"3:	\n\t"
			:"=a"(__res)
			:"S"(_first_str), "D"(_second_str)
			:
			);
	return __res;
}

/* compare two string by num bytes */
inline int strncmp(char *_first_str, char *_second_str, long num) {
	register int __res;
	__asm__	__volatile__	(
			"cld	\n\t"
			"1:		\n\t"
			"decq	%3	\n\t"
			"js		2f	\n\t"		/* 用完比较次数直接返回相等 */
			"lodsb	\n\t"
			"scasb	\n\t"
			"jne	3f	\n\t"
			"testb	%%al, %%al	\n\t"		/* 判断字符串结尾符号'\000' */
			"jne	1b	\n\t"
			"2:		\n\t"
			"xorl	%%eax, %%eax	\n\t"
			"jmp	4f	\n\t"
			"3:		\n\t"
			"movl	$1, %%eax	\n\t"
			"ja	4f	\n\t"
			"negl	%%eax	\n\t"
			"4:		\n\t"
			:"=a"(__res)
			:"S"(_first_str), "D"(_second_str), "c"(num)
			:
			);
	return __res;
}

/*获取字符串的长度*/
inline int strlen(char *str) {
	register int __res;
	__asm__	__volatile__	(
			"cld	\n\t"		// 设置自增
			"repne	\n\t"		// 不相等重复下一条指令
			"scasb	\n\t"		// scasb -> [al] - [di] 测试al中的值是否与di中的值相等
			"notl	%0	\n\t"	// rcx寄存器取反
			"decl	%0	\n\t"	// rcx寄存器减1
			: "=c"(__res)	// rcx寄存器数值就是字符串长度
			: "D"(str), "a"(0), "0"(0xffffffff)	// rdi取str地址，al寄存器设为0(即为字符串结尾标志)，rcx初值设为-1
			);
	return __res;
}


inline void clear_screen(unsigned long *buf_addr, unsigned long buf_sz) {
	__asm__	__volatile__	(
				"decq	%1			\n\t"
				"js		2f			\n\t"
				"incq	%1			\n\t"
				"1:					\n\t"
				"movq	$0,	(%0)	\n\t"
				"incq	%0			\n\t"
				"decq	%1			\n\t"
				"jnz	1b			\n\t"
				"2:	"
				:
				:"D"(buf_addr), "c"(buf_sz)
				:"memory"
			);
}


inline unsigned long bit_set(unsigned long *addr, unsigned long index) {
	return *addr | (1UL << index);
}

inline unsigned long bit_get(unsigned long *addr, unsigned long index) {
	return *addr & (1UL << index);
}

inline unsigned long bit_clean(unsigned long *addr, unsigned long index) {
	return *addr & (~ (1UL << index));
}

// inb -> get value form port specified in dx
inline unsigned char io_in8(unsigned short port) {
	unsigned char ret = 0;
	__asm__ __volatile__ (
				"inb	%%dx,	%0		\n\t"
				"mfence					\n\t"
				:"=&a"(ret)
				:"d"(port)
				:"memory"
			);
	return ret;
}

inline unsigned int io_in32(unsigned short port) {
	unsigned int ret = 0;
	__asm__ __volatile__ (
				"inl	%%dx,	%0		\n\t"
				"mfence					\n\t"
				:"=&a"(ret)
				:"d"(port)
				:"memory"
			);
	return ret;
}

// write value into port specified in dx
inline void io_out8(unsigned short port, unsigned char value) {
	__asm__ __volatile__	(
				"outb	%0,		%%dx	\n\t"
				"mfence					\n\t"
				:
				:"a"(value), "d"(port)
				:"memory"
			);
}

inline void io_out32(unsigned short port, unsigned int value) {
	__asm__ __volatile__	(
				"outl	%0,		%%dx	\n\t"
				"mfence					\n\t"
				:
				:"a"(value), "d"(port)
				:"memory"
			);
}

inline void cls_8024_kybd_buf() {
	__asm__ __volatile__	(
				"1:						\n\t"
				"inb	$0x64,	%%al	\n\t"
				"and	$0x1,	%%al	\n\t"
				"jz		2f				\n\t"
				"inb	$0x60,	%%al	\n\t"
				"jmp	1b				\n\t"
				"2:"
				:
				:
				:"memory", "ax"
			);
}

// Intel Volumn 1: insw m16, dx :
//								Input word from I/O port specified in dx into
//								memory location specified in es:(r/e)di
//								cx store the repeat times
#define port_insw(port, buffer, index)					\
	__asm__ __volatile__	(							\
				"cld	\n\t"							\
				"rep	\n\t"							\
				"insw	\n\t"							\
				"mfence	\n\t"							\
				:										\
				:"d"(port),	"D"(buffer), "c"(index)		\
				:"memory"								\
			)

// outsw 
// output word specified in es:(r/e)si into dx
#define port_outsw(port, buffer, index)					\
	__asm__ __volatile__	(							\
				"cld	\n\t"							\
				"rep	\n\t"							\
				"outsw	\n\t"							\
				"mfence	\n\t"							\
				:										\
				:"d"(port),	"S"(buffer), "c"(index)		\
				:"memory"								\
			)

#endif
