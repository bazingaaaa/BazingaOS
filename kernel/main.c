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


PRIVATE void untar(const char *path);

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
		//milli_delay(200);
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
		//milli_delay(200);
	}
}


/*
功能：内核切换至任务
备注：和进程有关的内容一共有四个，1进程体，2进程表，3ldt，4tss
*/
PUBLIC void kernel_main()
{
	disp_str("------\"kernel_main\" begins------\n");
	int i, j, prio;
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
			prio = 15;
		}
		else/*用户进程*/
		{
			p_task = &user_procs[i - NR_TASKS];
			eflags = 0x202;
			rpl = SA_RPL3;
			privilege = PRIVILEGE_USER;
			prio = 5;
		}
		
		//if(strcmp(p_proc->p_name, "INIT") == 0)/*INIT进程的内存分布*/
		if(i == NR_TASKS)/*INIT进程的内存分布*/
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

		p_proc->ticks = p_proc->priority = prio;

		p_proc->p_parent = NO_TASK;
		p_proc->p_flags = 0;/*一定要进行初始化，不然会被阻塞*/
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;/*发送消息至当前进程的进程队列*/
		p_proc->next_sending = 0;/*进程队列中的下一个进程*/
		p_proc->p_msg = 0;

		p_stack = p_stack - p_task->stacksize;

		/*初始化文件描述符*/
		for (j = 0;j < NR_FILES;j++)
			p_proc->filp[j] = 0;

		//seletor_ldt += 8;
		p_proc++;	
	}

	/*设置时钟中断相关*/
	ticks = 0;

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
	/*首先打开终端文件（fd为0代表输入 fd为1代表输出）*/
	char tty_name[] = "/dev_tty1";
	int fd_stdin = open(tty_name, O_RDWR);
	int fd_stdout = open(tty_name, O_RDWR);
	int pid;

	printf("Init() is running ...\n");

	untar("/cmd.tar");

	pid = fork();
	if(pid != 0)
	{
		printf("this is parent proc, pid of child is %d\n", pid);
		int s;
		int child;
		if(0 <=	(child = wait(&s)))
		{
			printf("child:%d exit with status:%d\n", child, s);
		}
	}
	else
	{
		printf("this is child proc\n");
		exit(123);
	}

	while(1)
	{
	}
}


/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*
功能：解压cmd.tar文件
*/
PRIVATE void untar(const char *path)
{
	char buf[SECTOR_SIZE * 16];
	int chunk = SECTOR_SIZE * 16;
	printf("[extrat file:%s\n", path);
	int fd_tar = open(path, O_RDWR);
	assert(fd_tar >= 0);

	while(1)
	{
		read(fd_tar, buf, SECTOR_SIZE);
		if(buf[0] == 0)
		{
			break;
		}
		struct posix_tar_header *pth = (struct posix_tar_header*)buf;
		int fd = open(pth->name, O_CREAT | O_RDWR);
		if(fd < 0)
		{
			printf("   fail to extract file:%s\n", pth->name);
			printf(" aborted]\n");
			return;
		}
		/*解析文件大小*/
		int len = 0;
		char *q = pth->size;
		while(*q)
		{
			len = (len * 8 + *q - '0');
			q++;
		}
		int byte_left = len;
		printf("   extract file:%s (%d byte)\n", pth->name, len);
		while(byte_left > 0)
		{
			int byte_read = MIN(byte_left, chunk);
			read(fd_tar, buf, ((byte_read - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			write(fd, buf, byte_read);
			byte_left -= byte_read;
		}
		close(fd);
	}
	close(fd_tar);
	printf(" done]\n");
}


