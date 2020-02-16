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
功能：读/写操作
返回值：实际读/写的字节数
*/
PUBLIC int do_rdwt(MESSAGE *msg)
{
	int fd = msg->FD;
	int cnt = msg->CNT;
	struct inode *pNode = 0;
	struct file_desc *pFd = 0;
	int pos = 0;
	int i_mode = 0;
	int i;

	if(fd < 0 || fd >= NR_FILES)/*无效fd*/
	{
		return -1;
	}

	assert(pcaller->filp[fd] >= &fd_table[0] && pcaller->filp[fd] < &fd_table[NR_FILE_DESC]);
	assert(0 != pcaller->filp[fd]->fd_inode);

	pFd = pcaller->filp[fd];
	pNode = pFd->fd_inode;

	if(!(pFd->fd_mode & O_RDWR))/*无读写操作*/
	{
		return -2;
	}

	/*获取文件描述符在文件中的位置*/
	pos = pFd->fd_pos;

	/*inode模式*/
	i_mode = I_TYPE_MASK & pNode->i_mode;

	if(I_CHAR_SPECIAL == i_mode)/*字符文件,特殊处理，需要向相应的设备驱动程序发送消息*/
	{
		msg->type = msg->type == READ ? DEV_READ : DEV_WRITE; 
		int dev = pNode->i_start_sect;
		msg->DEVICE = MINOR(dev);/*次设备号*/
		//msg->BUF = msg->BUF;
		//msg->CNT = msg->CNT;
		msg->PROC_NR = msg->source;/*此處需要告訴TTY終端是哪個進程需要讀/寫操作*/
		assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
		send_rec(BOTH, dd_map[MAJOR(dev)].driver_nr, msg);

		return msg->CNT;
	}
	else/*非字符文件（普通文件）*/
	{
		assert(I_DIRECTORY == i_mode || I_REGULAR == i_mode);

		int pos_end = pos + cnt;/*相对于文件起始*/
		if(READ == msg->type)
		{
			pos_end = MIN(pNode->i_size, pos_end);
		}
		else
		{	
			pos_end = MIN(pNode->i_nr_sects * SECTOR_SIZE, pos_end);
		}
		
		int off = pos % SECTOR_SIZE;/*读/写位置在一个扇区内的偏移*/
		int sect_beg = pNode->i_start_sect + (pos>>SECTOR_SIZE_SHIFT);/*扇区绝对起始（相对于分区）*/
		int sect_end = pNode->i_start_sect + (pos_end>>SECTOR_SIZE_SHIFT);/*扇区绝对终止（相对于分区）*/
		int chunk = MIN(sect_end - sect_beg + 1, FSBUF_SIZE>>SECTOR_SIZE_SHIFT);/*一次读取的扇区数（不能超过文件系统的缓存大小）*/
		int bytes_proc = 0;/*已经处理的字节数*/
		int bytes_left = cnt;/*待处理字节数*/
		int bytes = 0;

		/*遍历读取文件内容*/
		for(i = sect_beg;i <= sect_end;i += chunk)
		{
			/*读取文件*/
			rw_sector(DEV_READ, pNode->i_dev, i * SECTOR_SIZE, chunk * SECTOR_SIZE, TASK_FS, fsbuf);
			bytes = MIN(chunk * SECTOR_SIZE, bytes_left);

			if(READ == msg->type)/*读取*/
			{
				if(pos_end <= pFd->fd_pos)
				{
					break;
				}
				bytes = MIN(pos_end - pFd->fd_pos, bytes);
				phys_copy(va2la(msg->source, msg->BUF + bytes_proc), va2la(TASK_FS, fsbuf + off), bytes);
			}
			else/*写入*/
			{
				assert(WRITE == msg->type);
				phys_copy(va2la(TASK_FS, fsbuf + off), va2la(msg->source, msg->BUF + bytes_proc), bytes);
				rw_sector(DEV_WRITE, pNode->i_dev, i * SECTOR_SIZE, chunk * SECTOR_SIZE, TASK_FS, fsbuf);
			}
			off = 0;/*只有第一次有扇区内偏移*/
			bytes_left -= bytes;
			bytes_proc += bytes;
			pFd->fd_pos += bytes;
		}

		if(pFd->fd_pos > pNode->i_size)/*文件大小发生改变*/
		{
			pNode->i_size = pFd->fd_pos;

			/*同步到硬盘上*/
			sync_inode(pNode);
		}
		return bytes_proc;
	}
}