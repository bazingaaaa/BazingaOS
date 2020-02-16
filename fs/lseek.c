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
功能：seek实现
返回值：文件当前指针
*/
PUBLIC int do_lseek(MESSAGE *msg)
{
	int fd = msg->FD;
	int whence = msg->WHENCE;
	int offset = msg->OFFSET;

	if(fd < 0 || fd >= NR_FILES)/*无效fd*/
	{
		return -1;
	}
	assert(pcaller->filp[fd] >= fd_table && pcaller->filp[fd] < fd_table + NR_FILE_DESC);
	assert(pcaller->filp[fd]->fd_inode);

	struct file_desc *pFd = pcaller->filp[fd];
	struct inode *pNode = pcaller->filp[fd]->fd_inode;
	int file_size = pNode->i_size;
	int pos_end = pNode->i_nr_sects * SECTOR_SIZE - 1;

	if(I_REGULAR != pNode->i_mode)/*暂时只支持对普通文件的操作*/
	{
		return -1;
	}

	switch(whence)
	{
		case SEEK_SET:
			pFd->fd_pos = offset;
			break;
		case SEEK_CUR:
			pFd->fd_pos += offset;
			break;
		case SEEK_END:
			pFd->fd_pos = file_size + offset;
			break;
		default:
			assert(0);
	}
	SET_RANGE(pFd->fd_pos, 0, pos_end);
	return pFd->fd_pos;
}