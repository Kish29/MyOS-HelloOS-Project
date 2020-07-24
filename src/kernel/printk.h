#ifndef __PRINT_H__
#define	__PRINT_H__ 

// GNU C编译环境自带头文件，因为要用到可变参数，该头文件定义了可变参数
#include <stdarg.h>
#include "font.h"
#include "linkage.h"

// 定义格式化字符串的标志变量
#define	ZEROPAD	1		/* 用0填充 */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4
#define SPACE	8
#define	LEFT	16
#define	SPECIAL	32
#define	SMALL	64

#define is_digit(c)	((c) >= '0' && (c) <= '9')

#define WHITE	0x00ffffff
#define BLACK	0x00000000
#define RED		0x00ff0000
#define GREEN	0x0000ff00
#define BLUE	0x000000ff
#define YELLOW	0x00ffff00
#define ORANGE	0x00ff8000
#define	INDIGO	0x0000ffff
#define	PURPLE	0x008000ff

// 使用font.h中的ascii字符像素矩阵
extern unsigned char font_ascii[256][16];

/* 显示字符缓冲区
 */ 
char buf[4096] = {0};

/* 该结构体保存当前屏幕分辨率、字符光标所在位置、字符像素矩阵尺寸
 * 帧缓存区起始地址，以及帧缓存区容量的大小
 */ 
struct position_info {
	int _x_resolution;	// x轴分辨率
	int _y_resolution;

	int _x_position;
	int _y_position;

	int _x_char_size;
	int _y_char_size;

	unsigned int *_frame_buf_addr;
	unsigned long _frame_buf_length;
}pos_info;

/* 获取余数的宏定义
 */
#define do_div(n, base) ({ \
int __res; \
__asm__("divq %%rcx": "=a"(n), "=d"(__res): "0"(n), "1"(0), "c"(base)); \
__res; })

/* putchar函数通过检测字体循环
 * 将对应的字体矩阵信息
 * 放到帧缓存地址中
 */

void putchar(unsigned int *_frame_buf_addr, int _x_resolution, int _x_position, int _y_position, unsigned int front_color, unsigned int back_color, unsigned char font);

/* skip_atoi函数将*s指向的地址每次增长
 * 并取数字字符作为精度
 */
int skip_atoi(const char **s);


/* number函数通过传入的flag参数，将传入的数字格式化
 * base是进制，str传入当前字符串地址
 */
static char *number(char *str, long num, int base, int size, int precision, int type);

/* 最重要的函数就是vsprintf函数，它将解析格式化字符串
 * 设置flag，将解析后的字符串放到字符缓冲区buf中
 * 并返回格式化字符串后的长度（通过计算偏移量）
 */
int vsprintf(char *buf, const char *fmt, va_list args);


/* 可变参数'...'，前面的参数用来获得可变参数的离定位参数最近的参数的地址
 * 注意，虽然是可变参数，但是具体的参数数量未知
 * 按照约定，如果定位参数是int，那么调用者应当给定位参数赋值参数个数
 * 如果是const char* 或者 char *，按照C语言的实验，则需要查找定位参数中的'%s、%d'等等
 */
int color_printk(unsigned int front_color, unsigned int back_color, const char *fmt, ...);

#endif 
