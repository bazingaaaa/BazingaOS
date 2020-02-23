#define GLOBAL_VARIABLES_HERE

#include "const.h"
#include "type.h"
#include "protect.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "proto.h"
#include "fs.h"

PUBLIC PROCESS proc_table[NR_TASKS + NR_PROCS];
PUBLIC PROCESS *p_proc_ready;


PUBLIC TASK task_table[NR_TASKS] = {
	{task_sys, STACK_SIZE_TASKSYS, "task_sys"},
	{task_tty, STACK_SIZE_TASKTTY, "task_tty"},
	{task_hd, STACK_SIZE_TASKTHD, "task_hd"},
	{task_fs, STACK_SIZE_TASKTFS, "task_fs"},
	{task_mm, STACK_SIZE_TASKMM, "task_mm"}
};

PUBLIC TASK user_procs[NR_PROCS]={
	{Init,  STACK_SIZE_INIT,  "INIT"},
	{testA, STACK_SIZE_TESTA, "testA"}
};

PUBLIC TSS tss;

PUBLIC irq_handler irq_table[NR_IRQS];

PUBLIC system_call syscall_table[NR_SYS_CALLS] = {sys_sendrec, sys_printx};

PUBLIC int ticks;

PUBLIC TTY tty_table[NR_TTYS];
PUBLIC CONSOLE console_table[NR_TTYS];

int nr_cur_console;/*当前console*/

/*如果设备顺序改变此处需要更改*/
struct dev_drv_map dd_map[] = {
	/*设备号			  		主设备号*/
	{INVALID_DRIVER},		/*0:未使用*/
	{INVALID_DRIVER},		/*1:为软驱保留*/
	{INVALID_DRIVER}, 		/*2:为cdrom驱动保留*/
	{TASK_HD},				/*3:硬盘*/
	{TASK_TTY},				/*4:tty*/
	{INVALID_DRIVER}		/*5:为scsi硬盘驱动保留*/
};


/*FS缓冲区,位于6M~7M之间*/
PUBLIC u8 *fsbuf = (u8*)0x600000;
PUBLIC const int FSBUF_SIZE = 0x100000;

/*MM缓冲区，位于7M~8M之间*/
PUBLIC	u8 *mmbuf = (u8*)0x700000;
PUBLIC	const int MMBUF_SIZE = 0x100000;