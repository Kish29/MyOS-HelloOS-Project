#ifndef __INTERRUP_H__
#define __INTERRUP_H__ 

void init_interrupt();

// rdi-> rsp, rsi-> index
void do_IQR(unsigned long rsp, unsigned long index);

#endif
