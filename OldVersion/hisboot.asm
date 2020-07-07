;/***************************************************
;		版权声明
;
;	本操作系统名为：MINE
;	该操作系统未经授权不得以盈利或非盈利为目的进行开发，
;	只允许个人学习以及公开交流使用
;
;	代码最终所有权及解释权归田宇所有；
;
;	本模块作者：	田宇
;	EMail:		345538255@qq.com
;
;
;***************************************************/

	org	0x7c00	

BaseOfStack	equ	0x7c00

BaseOfLoader	equ	0x1000
OffsetOfLoader	equ	0x00

RootDirSectors	equ	14
SectorNumOfRootDirStart	equ	19
SectorNumOfFAT1Start	equ	1
SectorBalance	equ	17	

	jmp	short Label_Start
	nop
	BS_OEMName	db	'MINEboot'
	BPB_BytesPerSec	dw	512
	BPB_SecPerClus	db	1
	BPB_RsvdSecCnt	dw	1
	BPB_NumFATs	db	2
	BPB_RootEntCnt	dw	224
	BPB_TotSec16	dw	2880
	BPB_Media	db	0xf0
	BPB_FATSz16	dw	9
	BPB_SecPerTrk	dw	18
	BPB_NumHeads	dw	2
	BPB_HiddSec	dd	0
	BPB_TotSec32	dd	0
	BS_DrvNum	db	0
	BS_Reserved1	db	0
	BS_BootSig	db	0x29
	BS_VolID	dd	0
	BS_VolLab	db	'boot loader'
	BS_FileSysType	db	'FAT12   '

Label_Start:

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack

;=======	clear screen

	mov	ax,	0600h
	mov	bx,	0700h
	mov	cx,	0
	mov	dx,	0184fh
	int	10h

;=======	set focus

	mov	ax,	0200h
	mov	bx,	0000h
	mov	dx,	0000h
	int	10h

;=======	display on screen : Start Booting......

	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0000h
	mov	cx,	10
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartBootMessage
	int	10h

;=======	reset floppy

	xor	ah,	ah
	xor	dl,	dl
	int	13h

;=======	search loader.bin
	mov	word	[SectorNo],	SectorNumOfRootDirStart ; save SectorNumOf..

Lable_Search_In_Root_Dir_Begin:

	cmp	word	[RootDirSizeForLoop],	0 ; 循环14次
	jz	Label_No_LoaderBin	; 如果14个扇区都没有loader则转到没有loader的函数中
	dec	word	[RootDirSizeForLoop]	; 循环一次减一
	mov	ax,	00h
	mov	es,	ax
	mov	bx,	8000h	; 这里是设置缓冲区位置
	mov	ax,	[SectorNo]	; 设置读取的扇区号
	mov	cl,	1 ; 读取的扇区数量
	call	Func_ReadOneSector ; 从扇区中读出数据到缓冲区（0:8000h）中
	mov	si,	LoaderFileName	; 因为马上要用到lodsb指令，该指令从ds:si中读一个字节的数据到al中，si中保存了要查找的文件名，在FAT12中，文件名在目录项的开头，且通过大写字母来指定唯一标识
	; 这里的si保存的是loaderfilename的偏移地址
	mov	di,	8000h 	; di设置缓冲区的偏移地址，方便后面的比较，8000h=1000 0000 0000 0000b
	cld	; 设置DF标志位向上增长
	mov	dx,	10h	; dx保存查找的文件数量，因为根目录区，每个文件的信息占32个字节(B)，一个扇区512个字节，所以一个扇区共有512 / 32 = 16个文件
	;（这也是为什么根目录一共有 16 * 14 = 224个文件）
	;文件名占用11个字节(B)的目录项，且在文件项的开头
	
Label_Search_For_LoaderBin:

	cmp	dx,	0 ; 检查每一个扇区的16个文件是否检查完成
	jz	Label_Goto_Next_Sector_In_Root_Dir ; 如果dx=0，说明这一个扇区的16个文件全检查完了，并且没有找到loader程序
	dec	dx	; 检查一个文件减一
	mov	cx,	11  ; cx被用来后面配合lodsb来循环11次检查al中的数据是否和loader的文件名相同

Label_Cmp_FileName:

	cmp	cx,	0 ; 循环11次
	jz	Label_FileName_Found ; 这里有相等的才会cx - 1，所以只要发现不相等的字符，就会跳到另一个函数处,所以这里找到了以后直接跳到loader找到的函数入口处执行
	; cx=0说明这11个字符都是和缓冲区es：di中的前11个字节的数据相同，说明找到了loader程序
	dec	cx ; 一定要记住cx-1
	lodsb	
	cmp	al,	byte [es:di] ; 比较
	jz	Label_Go_On ; 相等继续判断下一个
	jmp	Label_Different ; 不相等跳到different函数处执行下一个文件的读取指令

Label_Go_On:
	
	inc	di ; es：di指向缓冲区的下一个字节的数据
	jmp	Label_Cmp_FileName	; 跳到比较的函数处

Label_Different:
	; 如果不相等，那么就要判断下一个文件项
	and	di,	0ffe0h ; ffe0h = 1111 1111 1110 0000b，让低5位置零，其它位不变
	add	di,	20h ; 20h = 0010 0000b = 32d，即每个文件占用32个字节，转向下一个文件项
	mov	si,	LoaderFileName ; 重新获取文件名
	jmp	Label_Search_For_LoaderBin	; 机选检查下一个文件中存放的描述数据区的数据

Label_Goto_Next_Sector_In_Root_Dir:
	
	add	word	[SectorNo],	1 ; 扇区数目加1
	jmp	Lable_Search_In_Root_Dir_Begin ; 检查下一个扇区内的16个文件中的数据
	
;=======	display on screen : ERROR:No LOADER Found

Label_No_LoaderBin: ; 这个函数只有在14个扇区读取完都没有发现loader的情况下才会执行
	; 这个函数的主要功能是调用bios的int 10h的13h号中断在屏幕上显示没有找到的字符串信息
	mov	ax,	1301h
	mov	bx,	008ch
	mov	dx,	0100h
	mov	cx,	21
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$

;=======	found loader.bin name in root director struct

Label_FileName_Found:	;这个函数会在找打loader程序后被立马跳转到此处执行代码

	mov	ax,	RootDirSectors ; ax保存根目录的扇区数目（14个）
	and	di,	0ffe0h ; 低5位置零，其它位不变
	add	di,	01ah	; 这里加上16将要判断这个文件中对loader程序的保存的起始簇号（0x1A处）,此时es:di指向缓冲区中暂存的该文件保存的起始簇号信息
	mov	cx,	word	[es:di] ; cx保存起始簇号，簇号占两个字节
	push	cx	; 先暂存一下簇号
	add	cx,	ax	; 注意，这里开始以下代码的说明：
	; 在FAT12文件中，数据区是在根目录区的后面，那么开始的扇区号按照逻辑扇区来说的话，就是第 【1（保留扇区） + 2*9 （两个FAT表占用的扇区）+14】= 33个扇区(注意LAB(逻辑扇区地址)从0开始)
	; 但是FAT12会对数据区按照簇的单位来标记，这里的一簇等于一个扇区，此时ax保存根目录占用扇区数目，加上ax等于加上根目录的偏移量
	add	cx,	SectorBalance ; 这里的balance=17，因为有效的簇号是从2号开始的，1号和0号不能读取，所以文件项中保存的最小簇号是2，0号和1号簇保存在FAT表的第一个表中
	; 这里举个例子来理解一下：
	; 如果这个loader程序的簇号被说明是第4号簇（在本fat12格式中，一簇等于一个扇区），即04h号，那么它的逻辑扇区号是17 + 4 + 14 = 35号逻辑扇区
	; 对比一下第2簇号，也就是17 + 2 + 14 = 33 号扇区，这样应该就能明白17这个数字的作用了
	; cx得到文件项中保存的起始簇号后，计算其扇区的位置，要不然cx - 2，要不然（19 - 2 =17），不然19 + 2 + 14 = 35扇区并不是用户的有效扇区的第一个扇区
	mov	ax,	BaseOfLoader	; 	准备将loader数据通过Func_ReadOneSector函数读到内存0x1000:0中去
	mov	es,	ax 	; 段地址
	mov	bx,	OffsetOfLoader 	; 偏移地址
	mov	ax,	cx	; 得到要读的扇区序号

Label_Go_On_Loading_File:
	push	ax 	; 保存扇区序号
	push	bx 	; 保存目的缓冲区(0x1000:0)
	mov	ah,	0eh ; bios 的10h中断的 0eh号中断在屏幕上显示一个字符（光标前移），就是显示一个点后光标移到后面，al字符，bh页码，bl属性
	mov	al,	'.' ; 每一个簇显示一个点
	mov	bl,	0fh
	int	10h
	pop	bx
	pop	ax

	mov	cl,	1	; 读取的扇区数量
	call	Func_ReadOneSector	; 将含有loader程序的扇区数据移到内存0x1000:0处
	pop	ax	; ax得到loader所在的簇号（FAT12表项号），173行代码处，push cx
	call	Func_GetFATEntry	; 得到与loader对应的FAT表中的值
	cmp	ax,	0fffh	; 0fffh表示最后一个簇，以簇号2为例，即这个loader的数据只占用1个簇的大小，如果不止一个簇的大小，ax中的数据会是0003h，即它还占用了第三个簇
	jz	Label_File_Loaded	; 如果是一个簇的大小，跳转
	push	ax	; 如果不是，那么通过下面三行代码，又得到新的簇号
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance ; 此时的ax得到新的扇区号，再读一个扇区进去
	add	bx,	[BPB_BytesPerSec] ; 内存中要存放的loader程序还要再加512个字节
	jmp	Label_Go_On_Loading_File	; 继续读取下一个簇在FAT12中保存的信息，检查是否还占有簇号

Label_File_Loaded:
	
	jmp	BaseOfLoader:OffsetOfLoader ; 设置cs：ip寄存器，跳到loader处执行指令

;=======	read one sector from floppy

Func_ReadOneSector:
	
	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl
	push	bx
	mov	bl,	[BPB_SecPerTrk]
	div	bl
	inc	ah
	mov	cl,	ah
	mov	dh,	al
	shr	al,	1
	mov	ch,	al
	and	dh,	1
	pop	bx
	mov	dl,	[BS_DrvNum]
Label_Go_On_Reading:
	mov	ah,	2
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
	ret

;=======	get FAT Entry

Func_GetFATEntry:

	push	es
	push	bx
	push	ax
	mov	ax,	00
	mov	es,	ax ; 设置段地址为0
	pop	ax ; 取回簇号,簇号也就是FAT表的表项的序号
	mov	byte	[Odd],	0 ; 奇偶标志先置0，odd 是奇数， even 是偶数
	mov	bx,	3
	mul	bx	; ax = ax * 3
	mov	bx,	2 
	div	bx 	; ax = ax / 2
	; 这里是让 ax * 1.5，因为FAT12表中每个表项的数据占用1.5个字节，扩大1.5背后，可以从余数和商来获取该表项里的数据
	; 比如，ax里读到存放loader的簇号(表项号)是3，那么3*1.5=4.5，那么相应的余数也就是1，ax是4，也就是说，FAT12表项排列中
	; 我们要得到FAT[3]表项的数据，应该从FAT表的第4个字节的高4位读取，然后再加上第五个字节，也就是1.5个字节了
	cmp	dx,	0	; 判断簇号的余数
	jz	Label_Even	; 等于0说明是偶数，odd不需要赋值
	mov	byte	[Odd],	1

Label_Even:

	xor	dx,	dx 	; dx置0
	mov	bx,	[BPB_BytesPerSec]  ; bx = 512，值是每个扇区的字节数
	div	bx	; 前面知道，当ax*1.5后，ax中存放该表项的开始字节，也就是相对于FAT表的0号字节开始的偏移字节量
	; 用它除以bx=512（FAT12表的一个扇区占512个字节），在ax中得到的商就是相对于FAT12表偏移的扇区数，
	; 在dx中得到的余数就是相对于一个从0字节开始的扇区的偏移字节
	push	dx	; 高位保存余数
	mov	bx,	8000h	; 置es:bx = 0:8000h为缓冲区
	add	ax,	SectorNumOfFAT1Start	; ax加上FAT12开始的扇区序号，就是要读到缓冲区的起始逻辑扇区号
	mov	cl,	2 ; 读取两个连续的扇区，将包含有loader程序信息的FAT12表中的数据放到缓冲区中
	call	Func_ReadOneSector
	; 当拿到缓冲区中的相应的FAT12表中的数据后
	pop	dx
	add	bx,	dx	; 余数是偏移字节，加上8000h后，由于缓冲区中的数据和表项中的数据排列一一对应，所以这里也是获取N号表项对应的M号字节开头处的数据
	mov	ax,	[es:bx]
	cmp	byte	[Odd],	1 	; 判断奇偶
	jnz	Label_Even_2	; 以上面的3号表项来说，应该从FAT表的第4个字节的高4位读取，然后再加上第五个字节，也就是1.5个字节了
	shr	ax,	4  ; 不要第四个字节的低四位，逻辑右移4位即可

Label_Even_2:	; 如果是偶数，以表项2得到的偏移字节3来说，它的数据就是第三个字节加上第4个字节的低4位
	and	ax,	0fffh ; 将最高4位置0
	pop	bx
	pop	es
	ret

;=======	tmp variable

RootDirSizeForLoop	dw	RootDirSectors
SectorNo		dw	0
Odd			db	0

;=======	display messages

StartBootMessage:	db	"Start Boot"
NoLoaderMessage:	db	"ERROR:No LOADER Found"
LoaderFileName:		db	"LOADER  BIN",0

;=======	fill zero until whole sector

	times	510 - ($ - $$)	db	0
	dw	0xaa55

