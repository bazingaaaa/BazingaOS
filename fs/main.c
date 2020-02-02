#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"

PUBLIC void task_fs()
{
	printl("TASK FS begins.\n");

	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	send_rec(BOTH, TASK_HD, &driver_msg);
	
	spin("FS");
}