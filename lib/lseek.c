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
功能：设置文件指针偏移
*/
PUBLIC off_t lseek(int fd, off_t offset, int whence)
{
	MESSAGE driver_msg;

	driver_msg.type = LSEEK;
	driver_msg.FD = fd;
	driver_msg.OFFSET = offset;
	driver_msg.WHENCE = whence;

	send_rec(BOTH, TASK_FS, &driver_msg);
	assert(driver_msg.type == SYSCALL_RET);

	return driver_msg.OFFSET;
}

