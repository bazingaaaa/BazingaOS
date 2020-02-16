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


/*
功能：文件写操作
返回值：>=0 写成功的字节数  <0 写失败
*/
PUBLIC int write(int fd, const char* buf, int size)
{
	MESSAGE driver_msg;

	driver_msg.type = WRITE; 
	driver_msg.BUF = buf;
	driver_msg.CNT = size;
	driver_msg.FD = fd;
	
	send_rec(BOTH, TASK_FS, &driver_msg);

	return driver_msg.CNT;
}