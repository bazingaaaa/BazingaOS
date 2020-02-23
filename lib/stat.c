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


/*
功能：查询文件信息
输入：path-文件路径
输出：s-存放文件信息
返回值：0-查询成功 否则失败
*/
PUBLIC int stat(char* path, struct stat *s)
{
	MESSAGE msg;
	msg.type = STAT;
	msg.PATHNAME = path;
	msg.NAME_LEN = strlen(path);
	msg.BUF = (void*)s;

	send_rec(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	
	return msg.RETVAL;
}