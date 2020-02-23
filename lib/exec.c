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
#include "stdio.h"
#include "string.h"


/*
功能：执行文件
备注：直接接收来自命令行的参数
*/
PUBLIC int execl(const char* path, const char *arg, ...)
{
	var_list parg = (var_list)&arg;
	char **p = (char**)parg;
	return execv(path, p); 
}


/*
功能：执行文件
备注：以字符指针的方式传递命令行参数,
	  1.将参数用字符数组的方式组合起来
	  2.构建执行程序所需的参数栈空间
	  3.向MM任务发送消息完成执行文件操作
*/
PUBLIC int execv(const char* path, char *argv[])
{
	char **p = (char**)argv;
	int stack_len = 0;
	char stk[PROC_ORIGIN_STACK];

	while(*p++)	
	{
		assert(stack_len + 2 * sizeof(char*) < PROC_ORIGIN_STACK);
		stack_len += sizeof(char*);
	}

	*((int*)(&stk[stack_len])) = 0;

	//*(char**)(stk + stack_len) = 0;/*参数末尾用全0(4个字节)与数据隔开,在执行程序时以此来计算实际传入的参数个数*/
	stack_len += sizeof(char*);

	char **q = (char**)stk;	
	for(p = argv;*p != 0;p++)
	{
		*q++ = &stk[stack_len];/*填写参数地址*/
		strcpy(&stk[stack_len], *p);/*拷贝实际参数内容*/
		stack_len += strlen(*p);
		stk[stack_len] = 0;/*用0将数据分割开*/
		stack_len++;
	}

	MESSAGE msg;
	msg.type = EXEC;
	msg.PATHNAME = (char*)path;
	msg.NAME_LEN = strlen(path);
	msg.BUF = (void*)stk;
	msg.BUF_LEN = stack_len;

	send_rec(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}
