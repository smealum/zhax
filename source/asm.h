#ifndef ASM_H
#define ASM_H

void flush_dcache();
void _rfe(void*);
u32 _cpsr();
void return_to_usermode();

#endif
