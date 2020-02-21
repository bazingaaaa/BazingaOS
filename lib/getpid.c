#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"


/*
功能：获取当前所在进程的pid
*/
PUBLIC int getpid()
{
	MESSAGE msg;

	msg.type = GET_PID;
	send_rec(BOTH, TASK_SYS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.PID;
}
