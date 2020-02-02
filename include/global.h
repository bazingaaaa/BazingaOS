#ifndef _GLOBAL_H_
#define _GLOBAL_H_


#ifdef GLOBAL_VARS_HERE
#undef EXTERN
#define EXTERN
#endif

EXTERN int disp_pos;
EXTERN u8 gdt_ptr[6];
EXTERN DESCRIPTOR gdt[GDT_SIZE];


EXTERN u8 idt_ptr[6];
EXTERN GATE idt[IDT_SIZE];

EXTERN int disp_pos;

EXTERN int ticks;

EXTERN TSS tss;

EXTERN TASK user_procs[NR_PROCS];
EXTERN TASK task_table[NR_TASKS];
EXTERN PROCESS proc_table[NR_TASKS + NR_PROCS];
EXTERN PROCESS *p_proc_ready;

EXTERN int k_reenter;

EXTERN irq_handler irq_table[];

EXTERN PUBLIC TTY tty_table[NR_TTYS];
EXTERN PUBLIC CONSOLE console_table[NR_TTYS];

EXTERN int nr_cur_console;/*当前console*/

#endif