/***********************************
 * jar definition of traps/interrput
 **********************************/


#ifndef __TRAP_H__
#define __TRAP_H__ 

#include "linkage.h"
#include "printk.h"
#include "lib.h"

/* 异常向量表
 * 事实上，这些函数是在entry.S中进行声明的
 * */
// 除零错误
void divide_error();
void debug_error();
void nmi();		//不可屏蔽的硬件异常
void int3();	//断点异常
void overflow();
void bounds();	//越界错误
void undefined_opcode();		//未定义/无效的机器码
void dev_not_availabel();	//设备异常
void double_fault();		//浮点指令异常
void coprocessor_segment_overrun();		//协处理器段越界(保留)
void invalid_TSS();			//无效的TSS段
void segment_not_present();		//段不存在
void stack_segment_fault();		// SS段错误
void general_protection();		//通用保护异常
void page_fault();		//页错误
void x87_FPU_error();	// x87 FPU错误
void alignment_check();		//对齐检测
void machine_check();		//机器检测(错误即停止运行，一般是CPU)
void SIMD_exception();		//SIMD浮点异常
void virtualization_exception();		//虚拟化异常


/*初始化系统异常向量表*/
void sys_idt_vector_init();

#endif 
