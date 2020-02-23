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
	while(1)
	{}
}


void testB()
{
	while(1)
	{
	}
}


void testC()
{
	while(1)
	{
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

	for(i = 0;i < NR_TASKS + NR_PROCS;i++, p_proc++)
	{		
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
		strcpy(p_proc->p_name, p_task->name);
		
		//if(strcmp(p_proc->p_name, "INIT") == 0)/*INIT进程的内存分布*/
		if(i == NR_TASKS)/*INIT进程的内存分布*/
		{
			int base, limit;
			int ret = get_kernel_map(&base, &limit);
			if(0 != ret)
			{
				return;
			}
			init_descriptor(&p_proc->ldts[INDEX_LDT_C], 0, (base + limit)>>LIMIT_4K_SHIFT, DA_32 | DA_C | (privilege<<5) | DA_LIMIT_4K);
			init_descriptor(&p_proc->ldts[INDEX_LDT_RW], 0, (base + limit)>>LIMIT_4K_SHIFT, DA_32 | DA_DRW | (privilege<<5) | DA_LIMIT_4K);
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
	int fd_stdin = open("/dev_tty0", O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);
	char *tty_list[] = {"/dev_tty1", "/dev_tty2"};
	int i;

	printf("Init() is running ...\n");
	
	untar("/cmd.tar");

	// /*创建两个shell分别连接终端1和终端2*/
	for(i = 0;i < sizeof(tty_list)/sizeof(tty_list[0]);i++)
	{
		int pid = fork();
		if(pid != 0)/*主进程*/
		{
			printf("create child proc:%d\n", pid);
		}
		else/*子进程，创建shell*/
		{
			close(0);
			close(1);

			shabby_shell(tty_list[i]);
		}
	}

	while(1)
	{
		int s;
		int child = wait(&s);
		printf("child:%d exit with status:%d\n", child, s);
	}
}


/*
功能：一个简陋的shell
输入：终端名称
备注：1.开启终端文件作为输入/输出
*/
PUBLIC void shabby_shell(const char* tty_name)
{
	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);
	char rdbuf[70];/*输入缓冲区*/

	while(1)
	{
		write(fd_stdout, "$ ", 2);
		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		int word = 0;
		char *p = rdbuf;
		char *s;
		int argc = 0;/*参数个数*/
		char *argv[PROC_ORIGIN_STACK];/*参数列表*/
		char c;

		/*对输入字符进行解析*/
		do
		{
			c = *p;
			if(!word && *p != ' ' && *p != 0)/*单词开始*/
			{
				word = 1;
				s = p;
			}
			if(word && (*p == ' ' || *p == 0))/*单词结束*/
			{
				argv[argc] = s;
				argc++;
				word = 0;
				*p = 0;/*字符串结尾，将空格替换为0*/
			}
			p++;
		}while(c);
		argv[argc] = 0;/*标志参数数组结尾*/

		/*判断是否是有效命令*/
		//printf("open file:%s\n", argv[0]);
		int fd = open(argv[0], O_RDWR);
		//printf("fd:%d\n", fd);
		if(fd < 0)/*无效命令*/
		{
			if(rdbuf[0])
			{
				write(fd_stdout, "{", 1);
				write(fd_stdout, rdbuf, r);
				write(fd_stdout, "}\n", 2);
			}
		}
		else/*有效命令*/
		{
			close(fd);
			int pid = fork();
			if(pid != 0)
			{
				int s;
				wait(&s);
			}
			else
			{
				//printf("path:%s argc:%d r:%d\n", argv[0], argc, r);
				//exit(0);
				//execl("/echo", "echo", "hello", "world!", 0);
				execv(argv[0], argv);
			}
		}
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


/*
功能：输出文件信息
*/
void dump_file_stat(char *path)
{
	struct stat s;
	if(0 != stat(path, &s))
	{
		return;
	}
	printf("dev:0x%x\ninode_nr:%d\ni_mode:0x%x\nsdev:0x%x\nsize:%d\n", s.st_dev, s.st_ino, s.st_mode,
			s.st_rdev, s.st_size);
}