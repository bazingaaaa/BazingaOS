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

EXTERN int key_pressed; /**
			      * used for clock_handler
			      * to wake up TASK_TTY when
			      * a key is pressed
			      */
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

EXTERN struct dev_drv_map dd_map[];/*只是声明，不用填写长度*/

EXTERN u8 *fsbuf;
EXTERN const int FSBUF_SIZE;

EXTERN struct super_block super_block[NR_SUPER_BLOCK];
EXTERN struct inode inode_table[NR_INODE];
EXTERN struct file_desc fd_table[NR_FILE_DESC]; 
EXTERN struct inode *root_inode;
EXTERN PROCESS *pcaller;

#endif