/* 内核栈反向跟踪，打印异常处地址
 * 与相应的函数名称
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_map();
int read_symbol();
void write_src();

struct symbol_entry {
	unsigned long address;
	char type;
	char *symbol;
	int symbol_length;
};

// table保存所有的标示符
struct symbol_entry *table;
int size, count;
unsigned long _text, _etext;

int main(int argc, char **argv) {
	read_map(stdin);	// 读取标准输入流stdin中数据并解析
	write_src();		// 组建数据列表
	return 0;
}

// 读取标准输入流stdin中数据并解析
void read_map(FILE *fp) {
	int i;

	while(!feof(fp)) {
		if(count >= size) {
	
		}
	}
}
