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
功能：抽取路径名中的文件名，并返回对应的文件夹的inode指针
返回值：0-抽取成功
*/
PUBLIC int strip_path(char* filename, struct inode **ppinode, const char* pathname)
{
	const char *src = pathname;
	char *dst = filename;

	if('/' != src[0])/*无效路径*/
	{
		return -1;
	}
	src++;
	/*拷贝文件名*/
	while(*src)
	{
		if('/' == *src)/*无效文件名*/
		{
			return -1;
		}
		*dst = *src;
		src++;
		dst++;
		if(dst - filename >= MAX_FILENAME_LEN)
		{
			break;
		}
	}

	*ppinode = root_inode;
	return 0;
}