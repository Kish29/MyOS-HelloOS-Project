# MyOS-HelloOS-Project
## HelloOS
	记录从零开始编写操作系统，之前的版本算是废掉了，因为是好几个月前写的了，现在几乎忘得一干二净，现在从新开始学习操作系统的知识。
	现在也对当时一些理解不能的地方有一个初步的了解。

>本项目的学习基于图书《一个64位操作系统的设计与实现》来进行研究于学习。需要有一定的汇编代码基础以及相应的硬件基础、C语言能力。废话不多说，这里将会记录学习的日期以及进度等等。

### HelloOS v0.0.1  <font size=2><u>2020.7.6~2020.7.7</u></font>
  1. bximage 创建虚拟磁盘，选择软盘
  2. nasm 编译boot.asm为bin文件
  3. dd 拷贝boot.bin到第一个扇区 
  4. mount 挂载boot.img文件，并加上-o loop参数，使其描述为磁盘格式
  5. cp loader.bin 到boot.img 中，操作系统会为根据这个文件的文件系统格式进行相应的信息录入
  6. bochs -f .bochsrc 启动bochs虚拟机（记得提前配置好.bochsrc）

写完boot.asm程序，从FAT12的文件系统格式从读入loader程序到内存的指定位置

### HelloOS v0.0.2 <font size=2><u>2020.7.18</u></font>
1. 暂时把loader的小功能实现了一下，明天继续更新

