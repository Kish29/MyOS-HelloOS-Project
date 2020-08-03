/*********************
 * jar kernel_linkage
 ********************/


#ifndef	_LINKAGE_H_
#define	_LINKAGE_H_ 


#define	L1_CACHE_BYTES	32

/* __attribute__ 可以指定函数或结构体，用圆括弧将参数扩起来
 * 可用参数有packed、aligned、regparm等等,
 * */
#define	asmlinkage	__attribute__((regparm(0)))		/*regparm指定最多可以用n个寄存器传递参数*/

#define	____cacheline_aligned __attribute__((__aligned__(L1_CACHE_BYTES)))

#define	SYMBOL_NAME(X)	X

#define	SYMBOL_NAME_STR(X)	#X	/*将参数进行字符串化*/

#define	SYMBOL_NAME_LABEL(X)	X##:

/* '##'可以将字符串进行拼接 如#define foo（para1, para2） (para1##para2)
 * foo(12, 34) -> 1234
 * */

#define	ENTRY(name)		\
.global	SYMBOL_NAME(name);	\
SYMBOL_NAME_LABEL(name)

#endif
