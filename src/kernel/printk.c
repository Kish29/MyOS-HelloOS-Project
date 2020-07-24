/*********************
 * jar kernel_printk
 ********************/

#include <stdarg.h>
#include "printk.h"
#include "lib.h"
#include "linkage.h"

/* skip_atoi函数将*s指向的地址每次增长
 * 并取数字字符作为精度
 */
int skip_atoi(const char **s) {
	int i = 0;

	// 注意*(s++)仍然是先取s内容，然后再++
	while(is_digit(**s))
		i = i*10 + (*((*s)++) - '0');

	return i;
}

void putchar(unsigned int *_frame_buf_addr, int _x_resolution, int _x_position, int _y_position, unsigned int front_color, unsigned int back_color, unsigned char font) {
	int i = 0, j = 0;
	unsigned int *addr = NULL;
	unsigned char *font_ptr = NULL;

	int test_val = 0;

	// 拿到对应的字符矩阵数据
	font_ptr = font_ascii[font];

	for(i = 0; i < pos_info._y_char_size; i++) {
		// 对应的行
		addr = _frame_buf_addr + _x_resolution * (_y_position + i) + _x_position;
		test_val = 0x100;
		for(j = 0; j < pos_info._x_char_size; j++) {
			test_val >>= 1;
			// 根据相&后的结果判断是否显示字体色
			if (*font_ptr & test_val)
				*addr = front_color;
			else // 否则显示背景色
				*addr = back_color;
			addr++;
		}
		font_ptr++;
	}
}

/* number函数通过传入的flag参数，将传入的数字格式化
 * base是进制，str传入当前字符串地址
 */
static char *number(char *str, long num, int base, int size, int precision, int type) {
	char c, sign, tmp[50];	// tmp处理符号或者特殊字符后面的数据，最后与str整合
	// 注意const char *指地址无法改变，但是可以赋值改变
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;	// tmp的递增索引

	if(type & SMALL)	// 如果使用小写x表示16进制用小写的表示
		digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if(type & LEFT)	// 如果类型中含有左对齐，剔除零填充标志
		type &= ~ZEROPAD;
	if(base < 2 || base > 36)	// 进制范围 2 ～ 32 进制
		return 0;
	c = (type & ZEROPAD) ? '0': ' ';	// 填充字符类型
	sign = 0;	// 默认不显示
	// 如果是有符号数据，并且是负数
	if((type & SIGN) && num < 0) {
		sign = '-';
		num = -num;
	} else 
		sign = (type & PLUS) ? '+': ((type & SPACE) ? ' ': 0);
	if(sign)
		size--;	//留出一个区域显示符号
	if(type & SPECIAL)
		if(base == 16)
			size -= 2;	// 显示0x或者0X
		else if(base == 8)
			size--;
	i = 0;
	if(num == 0)
		tmp[i++] = '0';
	else 
		while(num!=0)
			tmp[i++] = digits[do_div(num, base)];	// 每次从digits中选择要显示的字符，字符是除以进制后的余数，所以接下来在整合的时候要从后往前赋值
	if(i > precision)	// 如果显示的数据大于了精度，将精度扩大
		precision = i;
	size -= precision;	// 获取剩下的显示区域宽度
	if(!(type&(ZEROPAD + LEFT)))	// 不是左对齐或者零填充，前面填充空格
		*str++ = ' ';
	if(sign)
		*str++ = sign;
	if(type & SPECIAL)
		if(base == 8)	// 8进制
			*str++ = '0';
		else if(base == 16) {
			*str++ = '0';
			*str++ = digits[33]; // digits已经根据SMALL标志进行了变化，所以只需要取第33位就行了
		}
	// 如果类型里面没有左对其标志，填充‘0’或者‘c’
	if(!type & LEFT)
		while(size-- > 0)
			*str++ = c;

	while(i < precision--)	// 如果数据长度不够精度，填充0，如1.20000
		*str++ = '0';
	while(i-- > 0)
		*str++ = tmp[i]; // 从后往前复制
	while(size-- > 0) // 如果仍然有剩余宽度，填充空格
		*str++ = ' ';
	return str;
}




/* 最重要的函数就是vsprintf函数，它将解析格式化字符串
 * 设置flag，将解析后的字符串放到字符缓冲区buf中
 * 并返回格式化字符串后的长度（通过计算偏移量）
 */
int vsprintf(char *buf, const char *fmt, va_list args) {
	char *str, *s; // str获取buf地址（副本），s获取当格式化字符串有%s时的字符串
	int flags; // 格式化标志,如左对齐、空格对齐
	int field_width; // 数据宽度
	int precision; // 数据精度 
	int len, i; // len是含字符串参数时长度，i是循环变量

	int qualifier; // 数据规格，如long、hex、decimal等等

	// *fmt写在判断时，相当于 *fmt != '\000'
	for(str = buf; *fmt; fmt++) {
		if(*fmt != '%') {
			*str++ = *fmt;
			continue;
		}
		// 根据%格式化参数
		flags = 0;
repeat:
		fmt++;
		switch(*fmt) {
			case '-':	flags |= LEFT;
			goto repeat;
			case '+':	flags |= PLUS;
			goto repeat;
			case ' ':	flags |= SPACE;
			goto repeat;
			case '#':	flags |= SPECIAL;
			goto repeat;
			case '0':	flags |= SMALL;
			goto repeat;
		}

		// 获取数据宽度
		field_width = -1;
		if(is_digit(*fmt)) { // 如果后面给出了宽度
			field_width = skip_atoi(&fmt);
		} else if(*fmt == '*') {// 如果宽度由可变参数给出
			field_width = va_arg(args, int);
			if(field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
			fmt++;
		}

		// 获取数据精度
		precision = -1;
		if(*fmt == '.') {
			// 下一个为精度
			fmt++;
			if(is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if(*fmt == '*') {
				precision = va_arg(args, int);
				fmt++;
			}
			if(precision < 0)
				precision = 0;
		}

		// 获取规格(如果给出了规格)
		qualifier = -1;
		if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z') {
			qualifier = *fmt;
			fmt++;
		}

		// 获取类型
		switch(*fmt) {
			case 'c':
				if(!(flags & LEFT)) // 没有左对齐
					while(--field_width > 0)
						*str++ = ' ';	// 空格填充 
				// 注意，va_arg不支持char、float 型，如果传入char 会被自动转为int、double 
				*str++ = (unsigned char)va_arg(args, int);
				while(--field_width > 0)
					*str++ = ' ';
				break;
			case 's':
				// 获取字符串
				s = va_arg(args, char *);
				if(!s)
					s = '\0';
				len = strlen(s);
				if(precision < 0)
					precision = len;
				else if(precision < len)
					len = precision;

				if(!(flags & LEFT))
					while(--field_width > 0)
						*str++ = ' ';
				for(i = 0; i < len; i++)
					*str++ = *s++;
				while(len < field_width)
					*str++ = ' '; // 不足宽度部分用空格填充
				break;
			case 'o':	// 数字类型, 八进制
				if(qualifier == 'l')
					str = number(str, va_arg(args, unsigned long), 8, field_width, precision, flags);
				else 
					str = number(str, va_arg(args, unsigned int), 8, field_width, precision, flags);
				break;
			case 'p':	// 打印地址
				if(field_width == -1) {
					field_width = 2 * sizeof(void *);
					flags |= ZEROPAD;	// 用零填充的类型
				}
				str = number(str, (unsigned long)va_arg(args, void *), 16, field_width, precision, flags);
				break;
			case 'x':	// 16进制
				flags |= SMALL;
			case 'X':
				if(qualifier == 'l')
					str = number(str, va_arg(args, unsigned long), 16, field_width, precision, flags);
				else 
					str = number(str, va_arg(args, unsigned int), 16, field_width, precision, flags);
				break;
			case 'd': // 10进制
			case 'i': // 有符号
				flags |= SIGN;
			case 'u':
				if(qualifier == 'l')
					str = number(str, va_arg(args, unsigned long), 10, field_width, precision, flags);
				else 
					str = number(str, va_arg(args, unsigned int), 10, field_width, precision, flags);
				break;
			case 'n':	// 返回目前为止格式化了的字符串长度给相应的可变参数
				if(qualifier == 'l') {
					long *ip = va_arg(args, long *);		// 得到可变参数的地址
					*ip = str - buf;	// 赋值（相当于返回）
				} else {
					int *ip = va_arg(args, int *);
					*ip = str - buf;
				}
				break;
			case '%':
				*str++ = '%';
				break;
			default:
				*str++ = '%';
				if(*fmt)
					*str++ = *fmt;
				else 
					fmt--;	// 为了能让循环停止需要--，因为for循环末尾的fmt++
							// 可能导致不可预测的值出现
				break;
		}
	}
	// 循环结束，末尾添加'\0'
	*str = '\0';
	return str - buf;	// 返回处理好的字符串长度
}



/* 可变参数'...'，前面的参数用来获得可变参数的离定位参数最近的参数的地址
 * 注意，虽然是可变参数，但是具体的参数数量未知
 * 按照约定，如果定位参数是int，那么调用者应当给定位参数赋值参数个数
 * 如果是const char* 或者 char *，按照C语言的实验，则需要查找定位参数中的'%s、%d'等等
 */
int color_printk(unsigned int front_color, unsigned int back_color, const char *fmt, ...) {
	// str_len是vsprintf函数格式化后的字符串长度
	int str_len = 0;
	int count = 0;

	// line 是当前光标位置距离下一个制表位需要填充的空格符数量
	int line = 0;

	// 定义tab的大小
	int tab_val = 4;
	
	// 关键字：可变参数
	va_list args;

	/* va_start(args, fmt)获取离fmt（format）最近的可变参数的地址
	 * va_arg(args, type) 根据type将保存可变参数的地址中的值取出
	 * 并自动地址向上增长1（这个通常由type决定，一般是+4B）
	 */
	va_start(args, fmt);

	// 通过vsprintf函数，获得
	str_len = vsprintf(buf, fmt, args);

	va_end(args);

	for(count = 0; count < str_len || line; count++) {
		// 如果还有继续打印的tab空格符
		if(line > 0) {
			count--;
			goto prt_tab;
		}
		
		// 首先检测换行符
		if((unsigned char)*(buf + count) == '\n') {
			pos_info._x_position = 0;
			pos_info._y_position += pos_info._y_char_size;
		} else if((unsigned char)*(buf + count) == '\b') { // 如果是退格符，打印空格覆盖前面的，光标回退一格
			// 如果此时已经是屏幕(0, 0)处，则不操作
			if(pos_info._x_position == 0 && pos_info._y_position == 0) 
				continue;
			pos_info._x_position -= pos_info._x_char_size;
			if(pos_info._x_position < 0) {
				// 回退到上一行的最后一个字符位置
				pos_info._x_position = (pos_info._x_resolution / pos_info._x_char_size - 1) * pos_info._x_char_size;
				// 回退到上一行
				pos_info._y_position -= pos_info._y_char_size;
				putchar(pos_info._frame_buf_addr, pos_info._x_resolution, pos_info._x_position, pos_info._y_position, front_color, back_color, ' ');
				// 光标x、y位置不用变化
			}
		} else if((unsigned char)*(buf + count) == '\t') {
			/* (pos_info._x_position + tab_val) &~ (tab_val - 1))
			 * 向下取tab_val的整数倍，line等于离下一个tab符的还需要多少个空格
			 */
			line = ((pos_info._x_position + tab_val) &~ (tab_val - 1)) - pos_info._x_position;
prt_tab:
			line--;
			putchar(pos_info._frame_buf_addr, pos_info._x_resolution, pos_info._x_position, pos_info._y_position, front_color, back_color, ' ');
			pos_info._x_position += pos_info._x_char_size;
		} else {
			putchar(pos_info._frame_buf_addr, pos_info._x_resolution, pos_info._x_position, pos_info._y_position, front_color, back_color, (unsigned char)*(buf + count));
			pos_info._x_position += pos_info._x_char_size;
		}

		// 检查当前坐标的范围
		// 注意顺序，先检查x轴
		if(pos_info._x_position >= pos_info._x_resolution) {
			pos_info._x_position = 0;
			pos_info._y_position += pos_info._y_char_size;
		}
		if(pos_info._y_position >= pos_info._y_resolution) {
			pos_info._y_position = 0;
			// x轴不动
			// 也可以回到原点
		}
	}
	return str_len;
}
