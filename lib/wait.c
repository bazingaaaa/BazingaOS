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
功能：等待子进程退出
返回值：退出的子进程pid
输出：status-子进程退出的状态
*/
PUBLIC int wait(int *status)
{
	MESSAGE msg;

	msg.type = WAIT;
	send_rec(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
	*status = msg.STATUS;

	return msg.PID;
}