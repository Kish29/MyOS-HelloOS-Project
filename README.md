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
	2020-07-19

	上传了两个python脚本，实现对当前目录下的所有代码源文件中的代码保存到数据库当中
	支持的文件cpp, c, java, asm以及makefile等等
	因为昨天一次不小心的git操作，把链接点删除了，辛苦写了一天的代码消失了，还不小心用rm -rf 命令删除了git仓库中的一个文件，当时是真的绝望
	试了各种办法没找回来，不过最后还是找回来了哈哈哈
	所以吸取了这次教训，自己写个脚本
	如果github的提交操作出现问题或者异常，先把文件保存到数据库中，这样就可以大胆的操作了

1. 实现loader读取内核程序到物理地址1MB处
2. 添加loader过程中出现任何异常的关机提示处理 
3. 实现读取物理地址的内存结构

### HelloOS v0.0.3 <font size=2><u>2020.7.20</u></font>
1. 实现读取VBE信息，设置屏幕分辨率
2. 优化书上所表示的VBE模式的表示
	>将不同模式的分辨率显示在后面(仅支持VBE1.2版本及以上)

3. 跳转到临时IA-32保护模式

### HelloOS v0.0.4  <font size=2><u>2020.7.20</u></font>
1. 处理器切换到IA-32e模式
2. 处理器控制器转交给内核程序

### HelloOS v0.0.5  <font size=2><u>2020.7.22</u></font>
1. 写完内核头程序head.S，重新定义了GDT、IDT、64位页表等
2. 排除了存在于loader中的惊天BUG，该BUG导致kernel程序无法完整地加载到指定内存

### HelloOS v0.0.5  <font size=2><u>2020.7.24</u></font>
1. 写完了打印彩色字符串的函数，可以自由调用color_printk函数打印字符串了
	> 时间花了这么久主要原因是刚接触内核层面，好多库函数要自己些，只能看着作者的源码写，作者参考的是linux2.几的源码，期间我参考了linux-0.11的源码，发现二者是一模一样的，作者在此基础上写了color_printk函数，不过这个函数有个错误，已经向作者发了邮件等待回复
	>
	> 作者回复已在图灵社区进行了勘误
	>
	> 这期间被阅读源码、指针(什么char *s, char **s, int *强制转换)、可变参数(这个最恶心)、&、|操作以及内嵌汇编，inline链接所折磨，所以花了很长时间去学习这些东西
2. 不过现在没搞懂的是为什么.h中声明一个inline函数链接器会报undefined reference错误，把inline删掉就过了(虽然知道.h中的函数实现会被默认定义为inline)
	>作者回复我说由于我的编译器版本过高（9.3.0）
