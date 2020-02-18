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
功能：创建新的进程（对外接口）
返回值：>=0 新进程的pid 否则创建失败
*/
PUBLIC int fork()
{
	MESSAGE msg;

	msg.type = FORK;
	send_rec(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
	assert(msg.RETVAL == 0);

	return msg.PID;
}