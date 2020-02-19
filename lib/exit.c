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
功能：进程退出
输入：status-进程退出状态
*/
PUBLIC void exit(int status)
{
	MESSAGE msg;

	msg.type = EXIT;
	msg.STATUS = status;
	send_rec(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
}
