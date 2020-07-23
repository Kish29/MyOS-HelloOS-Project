org 0x10000
	jmp Load_Start 

; 包含fat12结构信息的文件
%include	"fat12.inc"

base_of_kernel							equ		0x00 
offset_of_kernel						equ		0x100000 ; 物理地址0:100000处

; 当读取到kernel时，先将kernel放在0x7e00处的缓冲区，然后逐字节地将数据复制到1MB的物理地址处
; 因为实模式下只支持1MB以内的寻址空间
base_of_temp_kernel_file				 equ	0x00 
offset_of_temp_kernel_file				 equ	0x7e00 

; 读取完kernel后，0x7e00作为物理内存结构信息的缓冲保存区域
memory_struct_buffer_addr				 equ	0x7e00 

;===========================32位全局描述符表GDT_32==================
; 伪指令，定义一个名字为gdt的数据段 
; 全局描述符表
[SECTION gdt_32]

; 每个描述符64位
LABEL_GDT_32:				dd	0, 0 ; 起始处为NULL
LABEL_DESC_CODE32:			dd	0x0000FFFF, 0x00CF9A00 
LABEL_DESC_DATA32:			dd	0x0000FFFF, 0x00CF9200 

gdt_length_32				equ	$ - LABEL_GDT_32 
; gdt的48位伪描述符指针
gdt_pointer_32				dw	gdt_length_32 - 1 ; 16位长度
							dd	LABEL_GDT_32 ; 32位基地址

; 段选择子index
selector_code_32			equ LABEL_DESC_CODE32 - LABEL_GDT_32 
selector_data_32			equ LABEL_DESC_DATA32 - LABEL_GDT_32 
;===================================================================


;===========================64位全局描述符表GDT_64==================
[SECTION gdt_64]
LABEL_GDT_64:				dq	0
LABEL_DESC_CODE64:			dq	0x0020980000000000 ; 非一致性、应用程序不可读（只能执行）、未访问
LABEL_DESC_DATA64:			dq	0x0000920000000000

gdt_length_64				equ $ - LABEL_GDT_64
gdt_pointer_64				dw	gdt_length_64 - 1
							dd	LABEL_GDT_64 

selector_code_64			equ	LABEL_DESC_CODE64 - LABEL_GDT_64 
selector_data_64			equ	LABEL_DESC_DATA64 - LABEL_GDT_64
;====================================================================

[SECTION .s16]
; 伪指令，告诉编译器该段的代码运行在16位的处理器上
[BITS 16]

Load_Start:
; ====== initial segment registers
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ax, 0h
    mov ss, ax
    mov sp, 7c00h

; ====== 显示开始执行loader程序
	; 先置位光标到下一行
	call Set_Message_Row 

    mov ax, 1301h
    mov bx, 0007h
    mov cx, start_load_message_length
    mov bp, start_load_message
    int 10h
    
; ====== query A20 port 
;	mov ax, 2402h
;	xor bx, bx 
;	int 15h
;
;	add byte [query_res], bl 
;
;    mov ax, 1301h
;    mov bx, 000bh
;    mov dx, 031fh
;    mov cx, 1
;    mov bp,	query_res 
;    int 10h
;
;	jmp $

; query_res: db 97

; ====== 通过置位0x92端口的第二位开启A20功能，使得寻址范围超过1MB,达到4GB
	push ax

	in	al, 92h ; 读取0x92端口的信息
	or	al, 00000010b ; 置位第二位，开启A20
	; 注意不要置位第一位，置位第一位系统重启
	out 92h, al 	

	pop ax 

	cli ; 关闭外中断

	db	  0x66 ; 使用32位数据指令前加前缀0x66 
	lgdt  [gdt_pointer_32] ; 将数据加载到GDTR寄存器中
	
	; 置位CR0.PE位，开启保护模式
	mov eax, cr0 
	or	eax, 1
	mov cr0, eax 

	mov ax, selector_data_32 ; 注意这里是数据段！！！！
	; 如果不小心写成了code段，根据code段描述符，将会是非一致性、可读、未访问
	; 将会失去写入数据的能力!!!(别问我为什么知道（调试了一晚上。。。。）)
	mov fs, ax ; fs保存代码段描述符的段选择子
	; 注意，当fs寄存器获得4GB的寻址能力后，不要再对其进行赋值，否则它将失去4GB的物理寻址能力，要重新开启(BOCHS平台不对其进行检查)
	mov eax, cr0 
	and al, 11111110b ; 关闭保护模式
	mov cr0, eax

	sti ; 开启外中断

; ====== reset floppy 
	xor ah, ah 
	xor dl, dl
	int 13h

;=========================================这是14次的大循环======================================================
Find_Loader_In_Root_Dir:
	; 比较循环次数
	cmp byte [read_num], 0
	je Kernel_Not_Found 
	dec byte [read_num]
	; 要读取的根目录扇区逻辑区号
	mov ax, 0
	mov es, ax 
	mov ax, [current_read_secter]
	mov bx, 8000h ; 缓冲区偏移地址 
	mov cx, 1 ; 读取的扇区数量 
	call Read_One_Sector
	; 将一个扇区的数据读到缓冲区中后，开始从缓冲区0:8000h处查找kernel

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
	mov si, kernel_name  ; 接下来用lodsb来传输11次字节数据,ds就是该程序的基地址，si为kernel_name的偏移地址
	mov cx, 11 ; 比较11个字节的数据
	
	;==============11 次循环入口 
Compare_Loader_Name_Entry:
	cmp cx, 0
	je Found_Kernel ; 注意此时di仍然保存当前kernel的根目录文件位置，一会儿会在FAT表解析函数中用到
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

; ====== 没有找到则退出kernel程序，关机
Kernel_Not_Found:
	push es 
	mov ax, 0x1000 
	mov es, ax 

	call Set_Message_Row

	mov ax, 1301h
    mov bx, 000ch
    mov cx, no_kernel_found_mesaage_length 
    mov bp, no_kernel_found_mesaage
    int 10h

	pop es

;====提示关机模块
Power_off_When_Erro_Occur:
	; 获取光标所在行
	call Set_Message_Row 
	
	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h
	mov bx, 000ch 
	mov cx, power_off_meassge_length
	mov bp, power_off_meassge 
	int 10h

	; 按任意键关机
	mov ax, 0
	int 16h
	
	; 显示一下用户的输入
	; 显示的字符串在al中
	mov ah, 0eh 
	xor bx, bx 
	; 字符属性
	mov bl, 0ch
	int 10h

	; 调用15h 5307h功能号设置电源状态
	mov ax, 5307h
	mov bx, 0001h ; 00系统BIOS,01表示整个系统，cx=3(关机)不支持APM v1.0的01h号（整个系统）
	mov cx, 0003h ; poweroff
	int 15h

Found_Kernel:
	nop
; 继续在FAT表中查找有关kernel的簇的信息
;=========================================FAT12表信息获取部分 ========================================
;============================================================
; 当我们找到kernel程序时，先从根目录中读取所在的簇号，然后转换
; 成扇区号，首先读入了kernel的第一个簇的扇区内容，然后再从FAT
; 表中对kernel的存储簇号信息来继续加载数据
;============================================================
Get_Clu_Num_From_Root_Dir:
	mov ax, 0 
	mov es, ax 
	and di, 0ffe0h ; 目的是清除Go_on_compare_next_char函数中对di的加1操作 
	add di, 1ah ; 偏移到保存簇号的位置 
	mov ax, [es:di] ; ax得到簇号
	push ax ;暂存ax

; 设置接下来将kernel读入内存的临时缓冲位置
Set_Kernel_Address:
	mov ax, base_of_temp_kernel_file 
	mov es, ax 
	mov bx, offset_of_temp_kernel_file 
	
	; 设置完成，返回ax的簇号信息
	pop ax 
Get_Info_From_FAT_Tab:
	; 用‘.’的显示数量来反映kernel程序占用的簇大小
	push ax 
	push bx 
	mov ah, 0eh 
	mov al, '.' ; 要显示的字符
	mov bx, 000fh
	int 10h
	pop bx 
	pop ax

	; 暂存簇号,用来调用获取FAT表信息函数
	push ax 
	; 这个簇号用来读扇区,要加上root的扇区数量和平衡数
	add ax, sector_of_root_dir 
	add ax, sector_balance 
	mov cx, 1
	call Read_One_Sector ; 读入一个扇区的数据到内核缓冲区中

	pop ax 

; ====== 利用fs段寄存器的4GB寻址能力，将kernel数据复制到0x100000处============
	push cx 
	push eax 
	push fs
	push edi 
	push ds 
	push esi 
	
	; 复制kernel文件还是一个字节一个字节的复制，防止数据丢失
	; 因为是一个字节一个字节的复制过去，循环512次
	mov cx, 200h ; 512次
	mov ax, base_of_kernel 
	mov fs, ax
	mov edi, dword [offset_of_kernel_count]

	mov ax, base_of_temp_kernel_file 
	mov ds, ax 
	mov esi, offset_of_temp_kernel_file
	
Move_Kernel:
	mov al,	byte [ds:esi]
	mov byte [fs:edi], al

	inc esi 
	inc edi 

	loop Move_Kernel 

	mov ax, 0x1000 ; 该程序的基地址 
	mov ds, ax ; 数据段缺省基地址段寄存器
	mov dword [offset_of_kernel_count], edi 

	pop esi 
	pop ds 
	pop edi 
	pop fs
	pop eax 
	pop cx 
; =================================== END =====================================

	call Get_Value_From_FAT_Tab 
	cmp ax, 0fffh ; 判断是否是最后一个簇
	je Kernel_has_been_loaded 
	; add bx, [BPB_BytesPerSec] ; kernel缓冲区再加一个扇区的数据大小
;!!!!!!!!!!!!!!!!!!! 这句话必须注释掉，因为每次esi都是从offset_of_temp_kernel_file处拿数据!!!!!!!!!!!!!
;!!!!!!!!!!!!!!!!!!! 而offset_of_temp_kernel_file这个地址没有加512B操作，没有变!!!!!!!!!!!!!!!!!!!!!!!!
;!!!!!!!!!!!!!!!!!!! 如果bx加512B的话，将拿不到kernel的数据!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;!!!!!!!!!!!!!!!!!!! 妈的，调试了一晚上，以为是kernel出现了问题，没想到是loader!!!!!!!!!!!!!!!!!!!!!!!!
	jmp short Get_Info_From_FAT_Tab 

Kernel_has_been_loaded:
	push es 
	mov ax, 0x1000 
	mov es, ax 

	; 清空屏幕, 重置光标位置
	push dx 

	; 清屏 
	mov ax, 0600h
	mov bx, 0700h ; 注意，bh是清屏后屏幕显示字符的属性，相当于全局范围的定义，在一些没有定义字符属性的中断现实中，这个起到了作用
	; 7 -> 0111 白色，不高亮
	mov cx, 0
	mov dx, 184fh 
	int 10h
	
	; 重置光标 
	mov ax, 0200h
	mov bx, 0 
	mov dx, 0
	int 10h 

	pop dx
	

	mov ax, 1301h
	mov bx, 0007h 
	mov dx, 0000h
	mov cx, kernel_load_complete_message_length 
	mov bp, kernel_load_complete_message 
	int 10h
	
	pop es 

; 因为kernel已经加载完成，所以可以关闭软驱马达
Kill_Motor:
	push dx 
	
	mov dx, 03f2h
	mov al, 0
	out dx, al ; 向03f2h端口选择0号驱动盘（A盘），复位，关闭软驱马达
	
	pop dx 

;===========获取物理地址空间信息===============
	mov ax, 0x1000 
	push es 
	mov es, ax 

	call Set_Message_Row

	mov ax, 1301h
	mov bx, 0007h 
	mov cx, get_memory_struct_message_length 
	mov bp, get_memory_struct_message 
	int 10h 

	pop es

Pre_Get_Memory_Struct:
	; 调用BIOS的新增15h e820h功能，获取物理地址空间信息 
	mov ax, 0
	; es:di是结果的缓冲区地址
	mov es, ax 
	mov di, memory_struct_buffer_addr 
	mov ebx, 0 ; 起始的结构映射体序号，通常为0

Get_Memory_Struct:
	mov eax, 0xe820
	mov ecx, 20 ; 保存预设的返回的结构体结果大小（>= 20 bytes）
	mov edx, 0x534d4150 ; 字符串"SMAP" 
	int 15h

	; 获取成功CF标志位会复位
	jc Get_Memory_Struct_Fail
	add di, 20 ; di每一次增加20Bytes 
	cmp ebx, 0 ; ebx 返回值为0表示检测结束，其他值为后续映射结构体序号
	jne Get_Memory_Struct 
	jmp short Get_Memory_Struct_Succeed 

Get_Memory_Struct_Fail:
	
	call Set_Message_Row

	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h
	mov bx, 000ch 
	mov cx, get_memory_struct_fail_length 
	mov bp, get_memory_struct_fail 
	int 10h
	
	jmp Power_off_When_Erro_Occur 

Get_Memory_Struct_Succeed:
	call Set_Message_Row 

	push es 
	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h
	mov bx, 0007h 
	mov cx,	get_memory_struct_succeed_length 
	mov bp, get_memory_struct_succeed 
	int 10h

	pop es

;=========获取VBE 控制器信息=========
; VBE 提供统一的(S)VGA接口
	call Set_Message_Row 
	
	push es 
	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h
	mov bx, 0007h 
	mov cx, get_svga_info_message_length 
	mov bp, get_svga_info_message 
	int 10h

	pop es 
	
	; 设置缓冲区 
	; 4f00号查询功能返回的信息将保存在这个缓冲区当中
	; 通过00号功能获取VedioModeList指针
	mov ax, 0
	mov es, ax 
	mov di, 0x8000 
	; ah=4f是VBE统一的主功能号
	; al=00获取VBE控制器信息
	mov ax, 4f00h
	int 10h
	; 返回值 
	; al = 4fh 支持4f00h中断(返回值就是调用的主功能号)
	; ah = 00h(成功)，01（失败）
	cmp ax, 004fh 
	jz Get_SVGA_Info_Succeed

; =========读取失败==========
Get_SVGA_Info_Failed:
	call Set_Message_Row 

	push es 
	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h
	mov bx, 0007h 
	mov cx, get_svga_info__fail_length 
	mov bp, get_svga_info_fail 
	int 10h

	pop es 
	
	jmp Power_off_When_Erro_Occur

Get_SVGA_Info_Succeed:
	call Set_Message_Row 

	push es 
	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h 
	mov bx, 0007h 
	mov cx, get_svga_info_succeed_length 
	mov bp, get_svga_info_succeed 
	int 10h

	pop es 

;=======显示resolution提示字符串在屏幕的第12行=========
Show_Resolution_Hint_Message:
	push gs 
	push di 
	push si 
	push cx 

	mov ax, 0xb800 
	mov gs, ax 
	
	mov si, resolution_hint_message 
	mov di, 0x780 ; 屏幕的第13行（以12行为索引计算）

	mov cx, resolution_hint_message_length
.show_hint_message:
	mov ah, 0xf 
	mov al, [ds:si] ; 缺省时，默认ds寄存器
	mov word [gs:di], ax
	inc si 
	; 注意di是加2，不是自增！
	add di, 2 
	
	loop .show_hint_message
	
	pop cx 
	pop si 
	pop di 
	pop gs

; 获取VBE模式信息
;======显示开始获取 VBE 模式信息=========
	call Set_Message_Row 

	push es 
	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h
	mov bx, 0007h 
	mov cx, get_svga_mode_message_length 
	mov bp, get_svga_mode_message 
	int 10h 

	pop es 

;=========获取查询的VBE缓冲区中偏移14个字节处的VedioModeList指针
;=========并预先设置4f01h功能好查询模式的缓冲区地址
	mov ax, 0 
	mov es, ax 
	mov si, 0x800e 

	mov esi, dword [es:si] 
	; 预先设置4f01号功能的ModeInfoBlock缓冲区
	; 01号每次查询结果数据块大小256B，与下面的0x100也是对应的
	mov edi, 0x8200 

Get_SVGA_Mode:
;=========首先显示模式索引============
	; 通过VedioModeList指针,获取VBE模式的信息，并打印在屏幕上
	mov cx, word [es:esi]

;==========显示SVGA的信息=======

	push ax 
	
	mov ax, 0
	; 先显示高位在屏幕上（按照逻辑也是如此）
	mov al, ch 
	call Display_AL 

	mov ax, 0 
	mov al, cl 
	call Display_AL 

	pop ax 

;=======如果cx当前值是ffffh，表示是最后一个模式或者没有模式支持，其他值表示存在可能还有下一个模式
;=======那么esi偏移2B获得下一个显示模式
	cmp cx, 0xffff 
	jz Get_SVGA_Mode_Finish 

;=========然后显示分辨率=======
Show_resolution:
	;=====先显示一个“:”符号
	mov al, ':'
	call Display_AL_Char

	; 先查询当前显示模式保存的信息
	mov ax, 4f01h
	int 10h

	cmp ax, 004fh 
	jnz Get_SVGA_Mode_Failed

	; svga模式加1
	; add byte [svga_mode_num], 1

	; 显示分辨率 =====注意只有VBE 1.2以上的版本才有分辨率=====
	; 分辨率大小各占2B
	; 18B处的x轴分辨率, 20B处的y轴分辨率
;=====X轴：
	; 注意，小段法cpu低位数据在低地址！
	xor ax, ax 
	mov al, byte [es:edi+19]
	call Display_AL 

	xor ax, ax 
	mov al, byte [es:edi+18]
	call Display_AL 

;=====显示 ‘x’ 号
	mov al, 'x'
	call Display_AL_Char 

;=====Y轴：
	xor ax, ax 
	mov al, byte [es:edi+21]
	call Display_AL

	xor ax, ax 
	mov al, byte [es:edi+20]
	call Display_AL

;=====显示 '-' 号
	mov al, '-'
	call Display_AL_Char

	add esi, 2
	add edi, 0x100 

	jmp Get_SVGA_Mode

Get_SVGA_Mode_Failed:
	call Set_Message_Row 

	push es 
	mov ax, 0x1000  
	mov es, ax 

	mov ax, 1301h 
	mov bx, 000ch 
	mov cx, get_svga_mode_fail_length 
	mov bp, get_svga_mode_fail 
	int 10h

	pop es

	jmp Power_off_When_Erro_Occur


;======获取VBE模式信息结束=======
Get_SVGA_Mode_Finish:
	call Set_Message_Row 

	push es 
	mov ax, 0x1000  
	mov es, ax 

	mov ax, 1301h 
	mov bx, 0007h 
	mov cx, get_svga_mode_succeed_length 
	mov bp, get_svga_mode_succeed 
	int 10h

	pop es

;==================          ;======根据用户选择设置svga模式=======
;==================          	; 获取用户输入
;==================          	;mov ax, 0
;==================          	;int 16h
;==================          
;==================          	;cmp al, 0
;==================          	;jl Power_off_When_Erro_Occur 
;==================          	;cmp al, byte [svga_mode_num]
;==================          	;ja Power_off_When_Erro_Occur
;==================          
;==================          ;====== 设置SVGA模式
;==================          ;====== 通过功能号4f02h实现
;==================          
;==================          ;获得用户选择的功能号
;==================          	; 高位先置零
;==================          	; and ax, 0x00ff  
;根据用户的选择模式现在实现还比较困难，以后再说
	

;=======按任意键继续======
	call Set_Message_Row 

	push es 
	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h 
	mov bx, 0007h 
	mov cx, press_key_to_continue_length 
	mov bp, press_key_to_continue 
	int 10h 

	pop es

	mov ax, 0
	int 16h

;=========设置显示分辨率=====
	mov ax, 4f02h 
	mov bx, 0x4177 ; 1280x768 4B像素
	int 10h ; 设置显示模式

	cmp ax, 004fh 
	jnz Set_SVGA_Mode_Failed

;========注意，设置VBE显示模式后，使用线性地址的帧缓存，那么必须在32位或者63位运行模式下进行字符串的输出 调用BIOS的10h中断不再管用=====
;========           ;=======显示成功信息=====
;========           Show_Set_SVGA_Succeed:
;========           	push es 
;========           	mov ax, 0x1000 
;========           	mov es, ax 
;========           
;========           	mov ax, 1301h 
;========           	mov bx, 000fh 
;========           	mov dx, 0000h
;========           	mov cx, set_svga_mode_succeed_length 
;========           	mov bp, set_svga_mode_succeed 
;========           	int 10h 
;========           
;========           	pop es 
	jmp short Init_GDT_32Mode

;设置SVGA模式失败
Set_SVGA_Mode_Failed:
	call Set_Message_Row 
	
	push es 
	mov ax, 0x1000 
	mov es, ax 

	mov ax, 1301h 
	mov bx, 000ch 
	mov cx, set_svga_mode_fail_length 
	mov bp, set_svga_mode_fail
	int 10h

	pop es 

	jmp Power_off_When_Erro_Occur





;===============接下来是进入32位保护模式=================
Init_GDT_32Mode:
	;开启保护模式!
	cli			;====关闭外中断 

	db 0x66 
	lgdt [gdt_pointer_32]

;========控制寄存器只能在0特权级下操作===================
	mov eax, cr0 
	mov eax, 1
	mov cr0, eax 

	; 注意这里要使用跳转指令跳转过来，而不是直接往下运行
;================段选择子index:断类偏移======== 
	jmp dword selector_code_32:GO_TO_TMP_32_Protect
;====执行完这条远跳转指令后，处理器进入了32位的保护模式==========


;====接下来马上准备进入64位的IA-32e模式（也叫长模式）=============
;================接下来是在32位保护模式运行下的32位宽代码部分 ~^_^~ ===========
[SECTION .s32]
[BITS 32]
GO_TO_TMP_32_Protect:
;===============要准备进入临时的长模式=============

;===============设置32位模式下的数据段选择子===========
	mov ax, selector_data_32 
	mov ds, ax 
	mov fs, ax 
	mov es, ax 
	mov ss, ax 
	mov esp, 0x7e00 ;0x7e00向上保存了物理地址内存的空间结构

	;查询是否支持长模式
	call Query_Support_Long_Mode
	test ax, ax 

	jz	Power_off_When_Erro_Occur


	;查询是否支持MSR寄存器组
	call Query_Support_MSR

	test ax, ax 
	jz Power_off_When_Erro_Occur
;=============注意这两个查询函数要在对数据段寄存器ds赋上IA-32e模式的64位数据段描诉符号前前进行查询========
;=============因为在没有进入IA-32e，却又对段寄存器赋值了IA-32e的描述符后，32位保护模式下，发现===========
;=============段描述符的长度均为0，所以会抛出异常=======================

;==============在物理地址0x90000处初始化页表项=============
;============用户模式页表项============
	mov dword	[0x90000], 0x91007 ;7-> 用户模式、可读写、存在
	mov dword	[0x90004], 0x00000 ;页表项0

	mov dword	[0x90800], 0x91007 
	mov dword	[0x90804], 0x00000 ;页表项1

	mov dword	[0x91000], 0x92007 
	mov dword	[0x91004], 0x00000 ;页表项2

;============超级模式页表项============
	mov dword	[0x92000], 0x00083 ; 83-> 超级模式、可读写、存在
	mov dword	[0x92004], 0x00000 ;页表项3

	mov dword	[0x92008], 0x20083 
	mov dword	[0x9200c], 0x00000 ;页表项4

	mov dword	[0x92010], 0x40083 
	mov dword	[0x92014], 0x00000 ;页表项5

	mov dword	[0x92018], 0x60083 
	mov dword	[0x9201c], 0x00000 ;页表项6

	mov dword	[0x92020], 0x80083 
	mov dword	[0x92024], 0x00000 ;页表项7

	mov dword	[0x92028], 0xa0083 
	mov dword	[0x9202c], 0x00000 ;页表项8

;===============加载64位GDTD表=========
	lgdt	[gdt_pointer_64]

;=====初始化64位数据段=====
	mov ax, selector_data_64
	mov ds, ax 
	mov es, ax 
	mov fs, ax 
	mov gs, ax 
	mov ss, ax 

	mov esp, 0x7e00 


;====标准步骤进入IA-32e模式====
;1.复位cr0.PG, 关闭分页管理，以加载64位的页表项（之前未开启，跳过这一步）

;2.置位cr4.PAE，开启物理地址拓展
	mov eax, cr4 
	bts eax, 5	; bit test and set 现将eax第5位复制给CF寄存器,然后eax第5位置1
	mov cr4, eax 

;3.加载64位页目录(顶层页表PML4)的物理基地址在cr3中(这里是临时的页目录)
	mov eax, 0x90000 
	mov cr3, eax 

;4.置位MSR寄存器组的IA32_EFER寄存器第8位LME，开启IA-32e模式 
;=====MSR寄存器组通过地址访问，使用rdmsr置零访问由ecx指定的寄存器====
	mov ecx, 0xc0000080 
	rdmsr 
	;读入数据到edx:eax 

;=====访问成功后，被访问的MSR寄存器会将它的高32位->edx的底32位中
;==========================================底32位->eax的底32位中
	bts eax, 8
	wrmsr
	;将edx:eax数据写入到被访问的MSR寄存器中

;5.置位cr0.PG开启分页管理，此时处理器自动置位IA32_EFER的第10位(LMA位：IA32e激活状态指示位)
	mov eax, cr0 
	bts eax, 31 
	mov cr0, eax

;6.远跳转指令，真正进入IA-32e模式，处理权交给内核
	jmp dword selector_code_64:offset_of_kernel
;;===========处理器运行在IA-32e模式下的三个条件（前提是已经进入了保护模式）:
;;========== 1-> cr0.PG 置位 
;;========== 2-> IA32-EFER.LME 置位 
;;========== 3-> cr4.PAE 置位 
;;====在IA-32e模式下，当试图改变其中任意一个标志，都不会通过处理器的一致性检测，抛出异常====

Query_Support_Long_Mode:
	;======查询功能号======
	mov eax, 0x80000000 
	cpuid 
	
	cmp eax, 0x80000001 
	setnb	al ; 如果eax大于等于0x80000001则al置为1，否则为0
	jb	.query_finish
	
	mov eax, 0x80000001 
	cpuid 
	bt	edx, 29	;将edx的第29位复制到CF标志位
	setc	al	;如果CF为1，设置al为1，否则为0

.query_finish:
	movzx eax, al 
	ret 

Query_Support_MSR:
	mov ax, 1
	cpuid 

	bt edx, 5
	setc	al
	
	ret


[SECTION .s16lib]
[BITS 16]
;================================fat12表信息解析函数，传入ax（簇号）返回对应簇号保存的信息==============
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

; ======== 传入ax参数，显示al的数据到屏幕上
Display_AL:
	push ecx 
	push edx 
	push edi
	push gs

	push ax 
	;push bx 

	mov ax, 0xb800 
	mov gs, ax 
	
	; ====显示第几号模式====
	; mov ah, 0xe 
	; mov al, [svga_mode_index]
	; xor bx, bx 
	; mov bl, 0xf ;白色
	; int 10h

	; ;序号加1
	; add byte [svga_mode_index], 1

	; pop bx 
	pop ax 

	mov edi, [display_position]
	mov ah, 0xf 
	mov dl, al 
	shr al, 4
	mov ecx, 2 ; 循环次数 

.begin:
	and al, 0x0f 
	cmp al, 9
	ja .1
	add al, '0'
	jmp .2

.1:
	sub al, 0xa 
	add al, 'A'

.2:
	mov [gs:edi], ax 
	add edi, 2

	mov al, dl 
	loop .begin 

	mov dword [display_position], edi 
	
	pop gs 
	pop edi 
	pop edx 
	pop ecx 

	ret 

;显示al的特殊字符
Display_AL_Char:
	push edi
	push gs 
	
	push ax 
	mov ax, 0xb800 
	mov gs, ax 
	pop ax 

	mov ah, 0xf 
	mov edi, dword [display_position]
	mov word [gs:edi], ax 
	add edi, 2 
	mov dword [display_position], edi

	pop gs 
	pop edi 

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

Set_Message_Row:
	push ax 
	push bx
	push cx 

	mov ax, 0300h
	mov bx, 0000h
	int 10h

	xor dl, dl 
	add dh, 1 ; 移至下一行显示
	
	pop cx 
	pop bx 
	pop ax 
	ret 



;======================================读扇区函数结束 End Func=========================================

;====================================变量数据区========================================================
current_read_secter:				dw		 sector_num_of_root_dir_start
;注意这里的dw字数据，一定要是字数据，如果是db数据，那么要在在传参时对ah置零

read_num:							db		 sector_of_root_dir ; 读取次数

kernel_name:						db		'KERNEL  BIN' ; 11个字节
FAT12_index_odd_or_even:			db		 0 ;初始化为0,表示为偶数

; 该变量保存kernel的读取偏移位置
offset_of_kernel_count:				dd		 offset_of_kernel 

display_position:					dd		 0x820 ; 显示在屏幕的第14行(索引为13行换算得来)

; svga_mode_index:					db		 '0'

; 保存svga模式的总数
; svga_mode_num:						db		 0
;======================================================================================================


;======================================显示字符区======================================================
start_load_message:						db		'Loading Kernel'
start_load_message_length				equ		$ - start_load_message

power_off_meassge:						db		'Press any key to shutdown->'
power_off_meassge_length				equ		$ - power_off_meassge 

no_kernel_found_mesaage:				db		'No OS_SYSTEM found!'
no_kernel_found_mesaage_length:			equ		$ - no_kernel_found_mesaage

kernel_load_complete_message:			db		'Loading Kernel Complete!'
kernel_load_complete_message_length		equ		$ - kernel_load_complete_message

get_memory_struct_message:				db		'Getting memory struct...'
get_memory_struct_message_length		equ		$ - get_memory_struct_message

get_memory_struct_fail:					db		'Get memory struct failed!'
get_memory_struct_fail_length			equ		$ - get_memory_struct_fail 

get_memory_struct_succeed:				db		'Get momory struct successful!'
get_memory_struct_succeed_length		equ		$ - get_memory_struct_succeed 

get_svga_info_message:					db		'Getting SVGA VBE information...'
get_svga_info_message_length			equ		$ - get_svga_info_message 
    
get_svga_info_fail:						db		'Get SVGA VBE information failed!'
get_svga_info__fail_length				equ		$ - get_svga_info_fail 
    
get_svga_info_succeed:					db		'Get SVGA VBE information successful!'
get_svga_info_succeed_length			equ		$ - get_svga_info_succeed 
    
get_svga_mode_message:					db		'Getting SVGA VBE mode information...'
get_svga_mode_message_length			equ		$ - get_svga_mode_message

get_svga_mode_fail:						db		'Get SVGA VBE mode information failed!'
get_svga_mode_fail_length				equ		$ - get_svga_mode_fail 

get_svga_mode_succeed:					db		'Get SVGA VBE mode information successful!'
get_svga_mode_succeed_length			equ		$ - get_svga_mode_succeed 

; set_svga_mode_succeed:					db		'Setting SVGA mode... OK!'
; set_svga_mode_succeed_length			equ		$ - set_svga_mode_succeed

set_svga_mode_fail:						db		'Set SVGA mode failed!'
set_svga_mode_fail_length				equ		$ - set_svga_mode_fail

press_key_to_continue:					db		'Basic initialize finished! Press any key to continue->'
press_key_to_continue_length			equ		$ - press_key_to_continue

resolution_hint_message:				db		'All resolutions HERE:'
resolution_hint_message_length			equ		$ - resolution_hint_message 

