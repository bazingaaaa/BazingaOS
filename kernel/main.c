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
	int fd;
	int i, n;

	char filename[MAX_FILENAME_LEN+1] = "/blah";
	const char bufw[] = "abcde";
	const int rd_bytes = 3;
	char bufr[rd_bytes];

	assert(rd_bytes <= strlen(bufw));

	/* create */
	fd = open(filename, O_CREAT | O_RDWR);
	assert(fd != -1);
	printf("File created: %s (fd %d)\n", filename, fd);

	/* write */
	n = write(fd, bufw, strlen(bufw));
	assert(n == strlen(bufw));

	/* close */
	close(fd);

	/* open */
	fd = open(filename, O_RDWR);
	assert(fd != -1);
	printf("File opened. fd: %d\n", fd);

	/* read */
	n = read(fd, bufr, rd_bytes);
	assert(n == rd_bytes);
	bufr[n] = 0;
	printf("%d bytes read: %s\n", n, bufr);

	/* close */
	close(fd);

	char * filenames[] = {"/foo", "/bar", "/baz"};

	/* create files */
	for (i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++) {
		fd = open(filenames[i], O_CREAT | O_RDWR);
		assert(fd != -1);
		printf("File created: %s (fd %d)\n", filenames[i], fd);
		close(fd);
	}

	char * rfilenames[] = {"/bar", "/foo", "/baz", "/dev_tty0"};

	/* remove files */
	for (i = 0; i < sizeof(rfilenames) / sizeof(rfilenames[0]); i++) {
		if (unlink(rfilenames[i]) == 0)
			printf("File removed: %s\n", rfilenames[i]);
		else
			printf("Failed to remove file: %s\n", rfilenames[i]);
	}

	spin("TestA");
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
	int i;
	PROCESS *p_proc = proc_table;
	TASK *p_task = task_table;
	u16 seletor_ldt = SELECTOR_LDT_FIRST;
	char* p_stack = task_stack + STACK_SIZE_TOTAL;/*指向栈顶*/
	u32 eflags;
	u32 rpl;
	u32 privilege;

	for(i = 0;i < NR_TASKS + NR_PROCS;i++)
	{
		if(i < NR_TASKS)/*首先是系统任务*/
		{
			p_task = &task_table[i];
			eflags = 0x1202;
			rpl = SA_RPL1;
			privilege = PRIVILEGE_TASK;
		}
		else
		{
			p_task = &user_procs[i - NR_TASKS];
			eflags = 0x202;
			rpl = SA_RPL3;
			privilege = PRIVILEGE_USER;
		}
		strcpy(p_proc->p_name, p_task->name);
		/*初始化进程表中的局部描述符信息,ldt中的第0和第1个段描述符*/
		p_proc->ldt_sel = seletor_ldt;
		p_proc->pid = i;
		memcpy((char*)&p_proc->ldts[0], (char*)&gdt[SELECTOR_KERNEL_CS>>3], sizeof(DESCRIPTOR));
		p_proc->ldts[0].attrl = DA_C | privilege<<5;
		memcpy((char*)&p_proc->ldts[1], (char*)&gdt[SELECTOR_KERNEL_DS>>3], sizeof(DESCRIPTOR));
		p_proc->ldts[1].attrl = DA_DRW | privilege<<5;
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

		p_proc->p_flags = 0;/*一定要进行初始化，不然会被阻塞*/
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;/*发送消息至当前进程的进程队列*/
		p_proc->next_sending = 0;/*进程队列中的下一个进程*/
		p_proc->p_msg = 0;

		p_stack = p_stack - p_task->stacksize;
		seletor_ldt += 8;
		p_proc++;
		//p_task++;
		
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

	proc_table[4].priority = 5;
	proc_table[4].ticks = 5;
	
	proc_table[5].priority = 5;
	proc_table[5].ticks = 5;

	proc_table[6].priority = 5;
	proc_table[6].ticks = 5;

	proc_table[0].nr_tty = 0;
	proc_table[1].nr_tty = 0;
	proc_table[2].nr_tty = 0;
	proc_table[3].nr_tty = 0;
	proc_table[4].nr_tty = 1;
	proc_table[5].nr_tty = 2;
	proc_table[6].nr_tty = 2;

	/*清屏*/
	// disp_pos = 0;
	// for(i = 0;i < 80 * 25;i++)
	// {
	// 	disp_str(" ");
	// }
	// disp_pos = 0;

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


