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
功能：文件读操作
返回值：>=0 读成功的字节数  <0 读失败
*/
PUBLIC int read(int fd, char* buf, int size)
{
	MESSAGE driver_msg;

	driver_msg.type = READ; 
	driver_msg.BUF = buf;
	driver_msg.CNT = size;
	driver_msg.FD = fd;
	
	send_rec(BOTH, TASK_FS, &driver_msg);

	return driver_msg.CNT;
}