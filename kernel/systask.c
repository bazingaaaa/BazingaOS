#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"

/*
功能：系统任务，用于处理消息
*/
PUBLIC void task_sys()
{
	MESSAGE m;
	while(1)
	{
		send_rec(RECEIVE, ANY, &m);
		int src = m.source;

		switch(m.type)
		{
			case GET_TICKS:
				m.RETVAL = ticks;
				send_rec(SEND, src, &m);
				break;
			case GET_PID:
				m.PID = src;
				m.type = SYSCALL_RET;
				send_rec(SEND, src, &m);
			default:
				panic("unknown msg type");
				break;
		}
	}
}