#include "type.h"
#include "const.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"

/*
功能：输出到显示器
备注：较高层次，用户进程调度用, 默认用户进程已经打开控制台
*/
void printf(char *fmt, ...)
{
	char buf[256];
	int len;
	var_list args = (var_list)((char*)(&fmt) + 4);/*fmt的地址，而非fmt的值*/
	len = vsprintf(buf, fmt, args);
	int c = write(1, buf, len);
	assert(c == len);
}


/*
功能：输出到显示器
备注：较低层次，内核调用
*/
void printl(char *fmt, ...)
{
	char buf[256];
	int len;
	var_list args = (var_list)((char*)(&fmt) + 4);/*fmt的地址，而非fmt的值*/
	len = vsprintf(buf, fmt, args);
	buf[len] = 0;
	printx(buf);
}