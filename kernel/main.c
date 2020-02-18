#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"
#include "stdio.h"


int k_reenter;
u8 task_stack[STACK_SIZE_TOTAL];

void testA()
{
	int base, limit;
	char tty_name[] = "/dev_tty1";
	int fd_stdin = open(tty_name, O_RDWR);
	//printf("fd_stdin:%d\n", fd_stdin);
	//assert(0 == fd_stdin);
	int fd_stdout = open(tty_name, O_RDWR);
	//assert(1 == fd_stdout);
	char rdbuf[128];

	//get_kernel_map(&base, &limit);
	//printf("base:0x%x limit:0x%x\n", base, limit);
	while(1)
	{
		write(fd_stdout, "$ ", 2);
		int n = read(fd_stdin, rdbuf, 70);
		rdbuf[n] = 0;

		if(strcmp(rdbuf, "hello") == 0)
		{
			printf("hello world!\n");
		}
		else
		{
			if(rdbuf[0])
			{
				printf("{%s}\n", rdbuf);
			}
		}
	}
}


void testB()
{
	int i = 0x1000;
	while(1)
	{
		//disp_color_str("B", BRIGHT | MAKE_COLOR(BLACK, RED));
		//(get_ticks());
		//printf("B");
		milli_delay(200);
	}
}


void testC()
{
	// int i = 0x2000;
	while(1)
	{
		//disp_int(11111);
		//disp_color_str("C", BRIGHT | MAKE_COLOR(BLACK, RED));
		//disp_int(get_ticks());
		//printf("C");
		milli_delay(200);
	}
}


/*
功能：内核切换至任务
备注：和进程有关的内容一共有四个，1进程体，2进程表，3ldt，4tss
*/
PUBLIC void kernel_main()
{
	disp_str("------\"kernel_main\" begins------\n");
	int i, j;
	PROCESS *p_proc = proc_table;
	TASK *p_task = task_table;
	//u16 seletor_ldt = SELECTOR_LDT_FIRST;
	char* p_stack = task_stack + STACK_SIZE_TOTAL;/*指向栈顶*/
	u32 eflags;
	u32 rpl;
	u32 privilege;

	for(i = 0;i < NR_TASKS + NR_PROCS;i++)
	{
		strcpy(p_proc->p_name, p_task->name);
		/*初始化进程表中的局部描述符信息,ldt中的第0和第1个段描述符*/
		//p_proc->ldt_sel = seletor_ldt;/*该进程的ldt在gdt中的位置，切换时加载用*/
		p_proc->pid = i;

		if(i >= NR_TASKS + NR_NATIVE_PROCS)/*空闲进程表项*/
		{
			p_proc->p_flags = FREE_SLOT;
			continue;
		}
		else if(i < NR_TASKS)/*首先是系统任务*/
		{
			p_task = &task_table[i];
			eflags = 0x1202;
			rpl = SA_RPL1;
			privilege = PRIVILEGE_TASK;
		}
		else/*用户进程*/
		{
			p_task = &user_procs[i - NR_TASKS];
			eflags = 0x202;
			rpl = SA_RPL3;
			privilege = PRIVILEGE_USER;
		}
		
		if(strcmp(p_proc->p_name, "INIT") == 0)/*INIT进程的内存分布*/
		{
			int base, limit;
			int ret = get_kernel_map(&base, &limit);
			if(0 != ret)
			{
				return;
			}
			init_descriptor(&p_proc->ldts[INDEX_LDT_C], 0, (base + limit)>>LIMIT_4K_SHIFT, DA_32 | DA_C | privilege<<5 | DA_LIMIT_4K);
			init_descriptor(&p_proc->ldts[INDEX_LDT_RW], 0, (base + limit)>>LIMIT_4K_SHIFT, DA_32 | DA_DRW | privilege<<5 | DA_LIMIT_4K);
		}
		else/*普通进程的内存分布*/
		{
			memcpy((char*)&p_proc->ldts[0], (char*)&gdt[SELECTOR_KERNEL_CS>>3], sizeof(DESCRIPTOR));/*为进程分配内存空间*/
			p_proc->ldts[0].attrl = DA_C | privilege<<5;
			memcpy((char*)&p_proc->ldts[1], (char*)&gdt[SELECTOR_KERNEL_DS>>3], sizeof(DESCRIPTOR));/*为进程分配内存空间*/
			p_proc->ldts[1].attrl = DA_DRW | privilege<<5;	
		}
		
		p_proc->regs.gs = SELECTOR_KERNEL_GS & SA_RPL_MASK | rpl;/*位于gdt中*/
		p_proc->regs.cs = rpl | SA_TIL;
		p_proc->regs.fs = 8 | rpl | SA_TIL;
		p_proc->regs.es = 8 | rpl | SA_TIL;
		p_proc->regs.ds = 8 | rpl | SA_TIL;
		p_proc->regs.ss = 8 | rpl | SA_TIL;   

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_stack;/*指向栈顶*/
		//p_proc->regs.esp = (u32)task_stack[i];/*指向栈顶*/
		p_proc->regs.eflags = eflags;
		p_proc->p_parent = NO_TASK;

		p_proc->p_flags = 0;/*一定要进行初始化，不然会被阻塞*/
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;/*发送消息至当前进程的进程队列*/
		p_proc->next_sending = 0;/*进程队列中的下一个进程*/
		p_proc->p_msg = 0;

		p_stack = p_stack - p_task->stacksize;
		//seletor_ldt += 8;
		p_proc++;

		/*初始化文件描述符*/
		memset(p_proc->filp, 0, sizeof(struct file_desc*) * NR_FILES);
	}

	/*设置时钟中断相关*/
	ticks = 0;

	/*为每个任务设置priority,新增任务或进程时这里必须设置优先级和tick数*/
	proc_table[0].priority = 15;
	proc_table[0].ticks = 15;

	proc_table[1].priority = 15;
	proc_table[1].ticks = 15;

	proc_table[2].priority = 15;
	proc_table[2].ticks = 15;

	proc_table[3].priority = 15;
	proc_table[3].ticks = 15;

	proc_table[4].priority = 15;
	proc_table[4].ticks = 15;
	
	proc_table[5].priority = 5;
	proc_table[5].ticks = 5;

	proc_table[6].priority = 5;
	proc_table[6].ticks = 5;

	proc_table[7].priority = 5;
	proc_table[7].ticks = 5;
	
	/*清屏*/
	disp_pos = 0;
	for(i = 0;i < 80 * 25;i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;

	init_clock();
	
	init_keyboard();	

	k_reenter = 0;

	p_proc_ready = proc_table;
	restart();/*这块儿会让k_reenter减1，故k_reenter初始化需设置为1*/

	while(1)
	{
		;
	}
}


/*
功能：发生严重故障
*/
PUBLIC void panic(char *fmt, ...)
{
	char buf[256];
	int len;
	var_list args = (var_list)((char*)(&fmt) + 4);/*fmt的地址，而非fmt的值*/
	len = vsprintf(buf, fmt, args);
	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);
}


/*
功能：获取时钟tick数的系统调用
*/
PUBLIC int get_ticks()
{
	MESSAGE m;
	reset_msg(&m);
	m.type = GET_TICKS;
	send_rec(BOTH, TASK_SYS, &m);
	return m.RETVAL;
}


/*
功能：初始进程
*/
PUBLIC void Init()
{
	while(1)
	{

	}
}