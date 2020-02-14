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
功能：打开文件（提供给外部的接口）
返回值：fd
备注：通过向文件系统发送消息完成
*/
PUBLIC int unlink(const char* pathname)
{
	MESSAGE msg;

	msg.type = UNLINK;
	msg.PATHNAME = pathname;
	msg.NAME_LEN = strlen(pathname);
	send_rec(BOTH, TASK_FS, &msg);

	return msg.RETVAL;
}