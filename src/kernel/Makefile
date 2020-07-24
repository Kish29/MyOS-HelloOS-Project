CFLAGS := -mcmodel=large -fno-builtin -m64 -O
ASFLAGS := --64

# 不要缩进注释，否则make认为是一条指令
all: system
	objcopy	-I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin 
# objcopy [options] input [output]
# -I 指定输入文件格式
# -S 移除所有symbol和relocation信息
# -R “string” 从输出文件中移除名为string的程序段
# -O 指定输出文件格式
# binary 以二进制格式将处理好的input->system输出到output->kernel.bin中
# 使用objcopy工具的原因是因为kernel是内核程序，不是应用程序，所以要转换从二进制格式，并且移除冗余数据

system: head.o main.o printk.o
	ld -b elf64-x86-64 -z muldefs -o system head.o main.o printk.o -T Kernel.lds 
# -b 指定输入的文件格式
#  -T 链接过程提供脚本文件

main.o:	main.c
	gcc $(CFLAGS) -c main.c 
# -mcmodel限制程序访问的内存空间，large可访问整个虚拟内存，其他不行
# -fno-builtin 编译器在编译过程中，不识别系统的内建函数(保证自己的函数不被编译器修改从系统的)
# -m64 生成64位代码
# -c 执行预处理，编译、汇编过程，不执行链接过程

head.o:	head.S
	gcc -E head.S > head.s
# -E 选项 只执行预处理，不执行编译、汇编、链接
# -C 不删除注释信息
	as $(ASFLAGS) -o head.o head.s
# 生成二进制代码段

printk.o: printk.c
	gcc $(CFLAGS) -c printk.c

.PHONY: clean 

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system Makefile~ Kernel.lds~ kernel.bin
