#include "type.h"
#include "config.h"
#include "const.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"
#include "string.h"
#include "elf.h"
#include "stdio.h"


/*
功能：执行文件操作
备注：1.读取可执行文件到缓存中
     2.构建执行程序的栈空间
     3.解析elf文件，将文件写入到进程内存映像中
     4.计算参数个数，更改ax和cx寄存器（执行程序的输入）
     4.更改指令寄存器和栈指针寄存器指向
*/
PUBLIC int do_exec(MESSAGE *msg)
{
	char path[MAX_PATH];
	int src = msg->source;
	int buf_len = msg->BUF_LEN;/*参数栈空间大小*/
	int argc = 0;/*参数个数*/
	int i, n;
	int name_len = msg->NAME_LEN;
	assert(msg->NAME_LEN < MAX_PATH);
	phys_copy((void*)va2la(TASK_MM, path), (void*)va2la(src, msg->PATHNAME), name_len);
	path[name_len] = 0;/*字符串结尾*/

	struct stat s;
	if(0 != stat(path, &s))
	{
		printl("do_exec:stat fail\n");
		return -1;
	}

	int fd = open(path, O_RDWR);
	if(fd < 0)/*文件不存在*/
	{
		return -1;
	}

	

	assert(s.st_size <= MMBUF_SIZE);
	read(fd, mmbuf, s.st_size);/*将文件读取到mm任务的缓存区*/
	close(fd);

	/*解析elf文件并装载内存映像*/
	Elf32_Ehdr *pEh = (Elf32_Ehdr*)mmbuf;
	for(i = 0;i < pEh->e_phnum;i++)
	{
		Elf32_Phdr *pPh = (Elf32_Phdr*)(mmbuf + pEh->e_phoff + i * pEh->e_phentsize);
		if(pPh->p_type == PT_LOAD)/*可加载段，需要装入到内存映像中*/
		{
			phys_copy(va2la(src, pPh->p_vaddr), va2la(TASK_MM, mmbuf + pPh->p_offset), pPh->p_filesz);
			assert(pPh->p_vaddr < PROC_DEFAULT_IMG_SIZE);
		}
	}
	printl("0x%x 0x%x 0x%x 0x%x\n", mmbuf[0], mmbuf[1], mmbuf[2], mmbuf[3]);
	printl("e_phnum:%d \n", pEh->e_phnum);



	char stk[PROC_ORIGIN_STACK];

	phys_copy(va2la(TASK_MM, stk), va2la(src, msg->BUF), buf_len);/*拷贝参数栈空间*/
	/*由于参数栈空间的位置需要调整，参数栈空间里的地址需要相应更改
		参数栈空间存放于进程内存映像的末尾*/
	int stk_pos = PROC_DEFAULT_IMG_SIZE - PROC_ORIGIN_STACK;
	int delta = stk_pos - (int)msg->BUF;
	char **p = (char**)stk;

	while(*p)/*地址需要加上偏移（因为整体栈空间挪动了）*/
	{
		*p += delta;
		p++;
		argc++;
	}
	phys_copy(va2la(src, stk_pos), va2la(TASK_MM, stk), buf_len);/*拷贝参数栈空间*/

	/*更改寄存器*/
	proc_table[src].regs.eax = stk_pos;
	proc_table[src].regs.ecx = argc;

	proc_table[src].regs.eip = pEh->e_entry;
	proc_table[src].regs.esp = stk_pos;

	strcpy(proc_table[src].p_name, path);

	return 0;
}


