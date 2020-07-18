org 0x7c00 ; cs:0x0000 ip:0x7c00 

base_of_stack		equ 0x7c00
base_of_loader		equ 0x1000 
offset_of_loader	equ 0x0000 

sector_of_root_dir				equ	14
sector_num_of_root_dir_start	equ 19
sector_num_of_FAT1_start		equ 1
sector_balance					equ 17

; FAT12 describe table 
	jmp short BootStart
	nop 
    BS_OEMName	db	'jarsboot'
    BPB_BytesPerSec	dw	512
    BPB_SecPerClus	db	1   ;数据区的没簇占多少扇区
    BPB_RsvdSecCnt	dw	1
    BPB_NumFATs	db	2
    BPB_RootEntCnt	dw	224  ; 根目录总的文件数目，这个用来确定根目录区占用的扇区数量
    ; 因为根目录占用14个扇区，每个扇区512个字节，而且目录项每个占用32Bytes
    ; 那么一个扇区就有512/32=16个文件，14个扇区就有224个文件
    BPB_TotSec16	dw	2880
    BPB_Media	db	0xf0  ; 可移动磁盘的类型F0，不可移动F8
    BPB_FATSz16	dw	9   ; 每个fat表占用的扇区数
    BPB_SecPerTrk	dw	18  ; 每个磁道的扇区数
    BPB_NumHeads	dw	2
    BPB_HiddSec	dd	0
    BPB_TotSec32	dd	0
    BS_DrvNum	db	0
    BS_Reserved1	db	0
    BS_BootSig	db	0x29
    BS_VolID	dd	0
    BS_VolLab	db	'boot loader'  ; 用完11个字节
    BS_FileSysType	db	'FAT12   ' ; 注意这里的空格，要用完8个字节

BootStart:
	; seg register initialize 
	mov ax, cs 
	mov ds, ax 
	mov es, ax 
	mov ss, ax 
	mov sp, base_of_stack

; invoke BIOS interrupt No.10h -------> AH->(06h) clear screen 
	mov ax, 0600h 
	mov bx, 0700h 
	mov cx, 0
	mov dx, 184fh ; 这儿是最右下角坐标
	int 10h
; al=0 ->crear screen 
; (ch, cl) = 窗口左上角位置(Y, X)
; (dh, dl) = 窗口右下角位置(Y, X)

; set cursor focus AH->02h 
	mov ax, 0200h
    mov bx, 0000h ; bh=显示的页码，就是内存b800h的8个页中的第0页
	mov dx, 0000h ; (dh, dl) = (Y, X)
	int 10h

; AH->13h show boot message 
	mov ax, 1301h 
	mov bx, 0fh ; bh = page num, bl = 8-bits ---> glitter-br-bg-bb-highlith-fr-fg-fb(b->background, f->font)
	mov dx, 0000h ; dh = row num, dl = column num 
	mov cx, boot_message_length
	mov bp, boot_message ; bp = extend address of the string , es = basic address of the string 
	int 10h 

; reset floppya's trackhead
    xor ah, ah
    xor dl, dl ; bios int 13h的0号初始化磁盘、dl驱动器号
    int 13h	

;======= 文件系统是写给操作系统看的========

;=============================================================================================================
; 当我们对该磁盘写入某个文件系统的引导扇区的描述信息后，操作系统根据这些描述数据对写入到该磁盘中的文件进行相应
; 的处理，比如该软盘的第一个扇区中存在前面所写的FAT12文件的描诉信息，那么该软盘被挂载的时候，操作系统就会读取
; 这些信息，然后写入的信息也会被做出相应的操作： 比如拷贝一个loader程序到该软盘，那么操作系统还要在根目录区中
; 依据FAT12的格式，对这个loader程序写上相应的位置（起始簇号）、名称等等信息。
;	
; 所以，接下来有一个次数14的循环，在根目录所占有的14个扇区中，逐个逐个地寻找是否存有一个叫loader.bin的程序，
; 而FAT12的根目录中每个文件信息占有32Byte，一个扇区512Byte，那么14次大循环里面每次包含512/32=16次的小循环,除此
; 之外，因为要比较11字节大小的字符串，所以每16次循环里面还要包含11次的第三层循环，所以总体来看是一个3曾嵌套循环
; 如果找到了loader程序的话，就会根据保存的所在簇号位置，转换成相应的扇区，读入到内存中来；然而，还没完，还要
; 检查FAT12表中对该程序的信息记录是否占用了超过一个簇大小，如果超过了，还要继续把存放loader程序的下一个簇的数据 
; 继续读入到内存中去。所以接下来的嵌套3层循环还要包含一个检查FAT12表的函数以及一个根据逻辑山区号读相应的一个物理
; 扇区数据到内存的函数
;=============================================================================================================

;=========================================这是14次的大循环======================================================
Find_Loader_In_Root_Dir:
	; 比较循环次数
	cmp byte [read_num], 0
	je Loader_Have_Not_Found 
	dec byte [read_num]
	; 要读取的根目录扇区逻辑区号
	mov ax, 0
	mov es, ax 
	mov ax, [current_read_secter]
	mov bx, 8000h ; 缓冲区偏移地址 
	mov cx, 1 ; 读取的扇区数量 
	call Read_One_Sector
	; 将一个扇区的数据读到缓冲区中后，开始从缓冲区0:8000h处查找loader

	;================ 接下来是16次小循环
	mov di, 8000h
	cld ; 设置正向循环，因为接下来要用到lodsb指令---> 从ds：si寄存器指定的内存地址读取lodsb/lodsw/lodsd/lodsq相应的数据量
	; 到al/ax/eax/rax寄存器中，正向循环使si寄存器+1,逆向-1
	mov dx, 10h ; 每个扇区有16个文件,每个文件保存有数据区对应文件的位置信息以及名称
	
	;===============16 次循环入口
Find_In_One_File:
	cmp dx, 0
	je Find_In_Next_Sector
	dec dx 
	mov si, loader_name  ; 接下来用lodsb来传输11次字节数据,ds就是该程序的基地址，si为loader_name的偏移地址
	mov cx, 11 ; 比较11个字节的数据
	
	;==============11 次循环入口 
Compare_Loader_Name_Entry:
	cmp cx, 0
	je Show_Loader_Found ; 注意此时di仍然保存当前loader的根目录文件位置，一会儿会在FAT表解析函数中用到
	dec cx
	lodsb 
	cmp al, [es:di]
	je Go_on_compare_next_char 
	; 不相等则di加32字节，比较下一个文件中的内容 
	; 注意这个di加32个字节的算法
	and di, 0ffe0h ; 目的是清除Go_on_compare_next_char函数中对di的加1操作
	add di, 20h
	jmp short Find_In_One_File

Go_on_compare_next_char:
	inc di 
	jmp short Compare_Loader_Name_Entry 

Find_In_Next_Sector:
	add word [current_read_secter], 1
	jmp Find_Loader_In_Root_Dir 
;=======================================14 次循环结束 End ========================================================

Loader_Have_Not_Found:
	mov ax, ds 
	mov es, ax 
	mov bx, 008ch ; red and glitter string 
	mov bp, no_loader_found_message 
	mov cx, no_loader_found_message_length 
	mov dx, 0100h
	mov ax, 1301h
	int 10h
	jmp $

Show_Loader_Found:
	mov ax, ds 
	mov es, ax 
	mov bx, 0fh 
	mov bp, loader_found_message 
	mov cx, loader_found_message_length 
	mov dx, 0100h
	mov ax, 1301h
	int 10h

; 继续在FAT表中查找有关loader的簇的信息
;=========================================FAT12表信息获取部分 ========================================
;============================================================
; 当我们找到loader程序时，先从根目录中读取所在的簇号，然后转换
; 成扇区号，首先读入了loader的第一个簇的扇区内容，然后再从FAT
; 表中对loader的存储簇号信息来继续加载数据
;============================================================
Get_Clu_Num_From_Root_Dir:
	mov ax, 0 
	mov es, ax 
	and di, 0ffe0h ; 目的是清除Go_on_compare_next_char函数中对di的加1操作 
	add di, 1ah ; 偏移到保存簇号的位置 
	mov ax, [es:di] ; ax得到簇号
	push ax ;暂存ax

; 设置接下来将loader读入内存的位置
Set_Loader_Address:
	mov ax, base_of_loader 
	mov es, ax 
	mov bx, offset_of_loader 
	
	; 设置完成，返回ax的簇号信息
	pop ax 
Get_Info_From_FAT_Tab:
	; 用‘.’的显示数量来反映loader程序占用的簇大小
	push ax 
	push bx 
	mov ah, 09h 
	mov al, '.' ; 要显示的字符
	mov bx, 0fh
	mov cx, 1
	int 10h
	pop bx 
	pop ax

	; 暂存簇号,用来调用获取FAT表信息函数
	push ax 
	; 这个簇号用来读扇区,要加上root的扇区数量和平衡数
	add ax, sector_of_root_dir 
	add ax, sector_balance 
	mov cx, 1
	call Read_One_Sector ; 读入一个扇区的数据到内存中

	pop ax 
	call Get_Value_From_FAT_Tab 
	cmp ax, 0fffh ; 判断是否是最后一个簇
	je Loader_has_been_loaded 
	add bx, [BPB_BytesPerSec] ; loader缓冲区再加一个扇区的数据大小
	jmp short Get_Info_From_FAT_Tab 

Loader_has_been_loaded:
	jmp base_of_loader:offset_of_loader ; 跳转到loader所在的内存位置

;================================FAT12表信息解析函数，传入ax（簇号）返回对应簇号保存的信息==============
Get_Value_From_FAT_Tab:
	push es 
	push bx 
	push ax 

	mov ax, 0
	mov es, ax 

	pop ax ; 簇号信息 
	mov byte [FAT12_index_odd_or_even], 0
	; 注意FAT12的表是每12个字节为一个表项 
	mov bx, 3
	mul bx ; ax = ax * bx 
	mov bx, 2 
	div bx ; ax = ax / bx ------>  相当于ax乘了1.5
	; 16位的除数，dx保存余数，根据余数来判断数据应该是与0fffh来求与还是右移4位
	cmp dx, 0
	je Get_Value ; 余数为偶数，跳过置1
	mov byte [FAT12_index_odd_or_even], 1 

Get_Value:
	mov bx, [BPB_BytesPerSec]
	xor dx, dx ; 注意这里一定要对dx置零，因为下一条除法指令是16位的，默认dx是高16位，ax是低16位，而上面的运算完成后，如果dx=1，那么接下来的除法就会出错
	div bx 
	; 除以每扇区的字节数，余数是相对一个扇区的字节偏移位置，商是FAT12表的第几个扇区
	add ax, sector_num_of_FAT1_start ; ax 保存商 
	push dx 
	mov bx, 8000h
	mov cx, 2 ; 读2个扇区，解决表项跨扇区问题
	call Read_One_Sector 
	pop dx 
	add bx, dx 
	mov ax, [es:bx] ; 获取FAT12表中的信息
	cmp byte [FAT12_index_odd_or_even], 0
	je Index_Even 
	shr ax, 4

Index_Even:
	and ax,0fffh 
	pop bx 
	pop es 
	ret 

;=====================================FAT12表信息解析函数 END ==========================================
;========================================FAT12表信息获取部分 END ===========================================

;=============================================读扇区函数 Start Func==============================================
; read one sector's data to memory 
; 外接的调用函数一般写在最后面
;===================================================================
; 因为我们传递给读扇区的函数的参数是逻辑扇区号，也就是按照FAT12划
; 分的从0～2879号进行的标号（LBA格式），从物理磁盘中读取时，调用BIOS
; 的int 13h中断，需要的参数是CHS格式的，所以在这个函数中还要进行扇区
; 格式的转换
;===================================================================

;===================================================================
; int 13h, ah=02h, 读取磁盘扇区、
; al=读取扇区数（非零）
; ch=磁道号的低8位
; cl=扇区号（0～5bit），磁道号的高2位（6～7bit）
; dh=磁头号
; dl=驱动器号 
; es:bs => 要将数据放入的内存缓冲区起始位置
;===================================================================

Read_One_Sector:
	; 我们传入的参数是 ： ax = 逻辑扇区号，cl = 读取的扇区数量 es：bx= 缓冲区起始位置
	; === LBA格式 转换为 CHS格式====
	; 计算公式为 LBA扇区号/ 每磁道扇区数 = Q(商)--R(余数)
	; Q / 2 = 磁道号（磁道号从0开始，除以2是因为该软盘有两个磁头，两面都有存储介质）
	; Q & 1 = 磁头号（奇数商在第1磁头，偶数商在第0磁头）
	; R + 1 = 该磁道下的起始扇区号（逻辑扇区从0开始标号，物理扇区从1开始标号）
	push bp ; 保存栈底指针
	mov bp, sp 
	sub sp, 4
	mov byte [bp-2], cl ; 保存读取的扇区数量 
	mov word [bp-4], bx ; 缓冲区偏移地址
	mov bl, byte [BPB_SecPerTrk] ; bl保存每磁道扇区数量， 中括号取对应的内容，不加中括号取地址
	div bl ; 被除数在ax中, 16位除法指令，ah保存余数，al保存商
	inc ah 
	mov cl, ah ; cl获得扇区号
	mov dh, al ; ==各自获得数据 ==
	mov ch, al ; ==同上 ==
	shr ch, 1 ; 获得磁道号
	and dh, 1 ; 获得磁头号，立即数会自动拓展位（计算机组成原理）‘
	mov dl, [BS_DrvNum] ; 驱动器号 
	mov bx, [bp-4]
	mov al, [bp-2] ; al拿到读取扇区数量
; 调用13h的02h中断
Keep_Reading:
	mov ah, 02h
	int 13h
	jc Keep_Reading ; 读取失败进位标识符会置位
	add sp, 4
	pop bp 
	ret 
;======================================读扇区函数结束 End Func=========================================

;====================================变量数据区========================================================
current_read_secter:				dw		 sector_num_of_root_dir_start
;注意这里的dw字数据，一定要是字数据，如果是db数据，那么要在在传参时对ah置零

read_num:							db		 sector_of_root_dir ; 读取次数

loader_name:						db		 'LOADER  BIN' ; 一共11个字节，最后三个字节是文件拓展名
FAT12_index_odd_or_even:			db		 0 ;初始化为0,表示为偶数

;====================================变量数据区结束=================================================

;=====================================display messages START =========================================
; display messages
boot_message:						db		'Start Booting'
boot_message_length					equ		($ - boot_message)

no_loader_found_message:			db		'ERROR: NO LOADER FOUND!'
no_loader_found_message_length		equ		($ - no_loader_found_message) 

loader_found_message:				db		'Reading Loader'
loader_found_message_length			equ		($ - loader_found_message) 

;=====================================display messages END ============================================

	times	(510 - ($ - $$))	db	 0
	; fill blank part
	db 0x55 
	db 0xaa  ; Little Endian -> 0xaa55, Large Endian -> 0x55aa
