/***********************
* 该链接脚本让应用程序的
* 代码及数据放在
* 0xffff800000000000中
***********************/

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS {

	. = 0xffff800000000000 + 0x100000;	/* '.' 表示程序运行的起始地址*/
	.text : {
		_text = .; /*起始线性地址*/
		*(.text)
		/*正则表达式，输出文件.text程序段保存输入文件所有.text程序段*/
		_etext = .; /*结尾线性地址*/
	}

	. = ALIGN(8); /*8B 对齐*/
	.data : {
		_data = .;  /*这里的'.'表示获取'.'的当前地址*/
		*(.data)

		_edata = .; /*同理*/
	}

	.rodata : {
		_rodata = .;
		*(.rodata)
		_erodata = .;
	}

	. = ALIGN(32768);
	.data.init_task : { *(data.init_task) }

	.bss : {
		_bss = .;
		*(.bss)

		_ebss = .;
	}

	_end = .;
}
