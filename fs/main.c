#include "type.h"
#include "config.h"
#include "const.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"


/*
功能：文件系统任务，用于处理和文件系统有关的消息
*/
PUBLIC void task_fs()
{
	printl("TASK FS begins.\n");

	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV);/*次设备号*/
	send_rec(BOTH, TASK_HD, &driver_msg);
	
	spin("FS");
}