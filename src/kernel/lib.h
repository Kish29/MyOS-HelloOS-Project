/****************
 * jar kernel_lib
 ****************/

#ifndef	__LIB_H__
#define	__LIB_H__ 

#define	NULL 0
// 
// #define container_of(ptr, type, member)			\
// ({												\
// 	typeof(((type *)0)->member) * p = ptr;		\

int strlen(char *str) {
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


#endif
