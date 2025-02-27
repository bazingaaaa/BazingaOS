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
功能：抽取路径名中的文件名，并返回对应的文件夹的inode指针
返回值：0-抽取成功
*/
PUBLIC int strip_path(char* filename, struct inode **ppinode, const char* pathname)
{
	const char *src = pathname;
	char *dst = filename;

	if(0 == src)/*无效路径*/
	{
		return -1;
	}
	if(*src == '/')
	{
		src++;
	}

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
	*dst = 0;

	*ppinode = root_inode;
	return 0;
}


/*
功能：查找指定路径文件
返回值：若找到文件则返回文件对应的inodeh号，否则返回0
*/
PUBLIC int search_file(const char* path)
{
	int inode_nr = 0;
	struct inode *dir_inode = 0;
	char filename[MAX_FILENAME_LEN];
	int i, j;

	memset(filename, 0, MAX_FILENAME_LEN);
	if(0 != strip_path(filename, &dir_inode, path))/*提取待查文件的目录节点和文件名*/
	{
		return 0;
	}

	if(filename[0] == 0)
		return dir_inode->i_num;

	/*根目录文件的设备只能是ROOT_DEV*/
	assert(dir_inode->i_dev == ROOT_DEV);

	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries =
	  dir_inode->i_size / DIR_ENT_SIZE; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
	int m = 0;
	struct dir_entry * pde;
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENT_SIZE; j++,pde++) {
			if (strcmp(filename, pde->name) == 0)
			{	
				return pde->inode_nr;
			}
			if (++m > nr_dir_entries)
				break;
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}


	return inode_nr;
}


/*
功能:查询文件信息
*/
PUBLIC int do_stat(MESSAGE *msg)
{
	char path[MAX_PATH];
	char filename[MAX_PATH];
	int src = msg->source;
	assert(msg->NAME_LEN < MAX_PATH);
	phys_copy(va2la(TASK_FS, path), va2la(src, msg->PATHNAME), msg->NAME_LEN);
	path[msg->NAME_LEN] = 0;

	struct inode* pNode = 0;
	struct inode* dir_node = 0;
	int inode_nr = 0;

	if(0 == (inode_nr = search_file(path)))/*未找到该文件*/
	{
		return -1;
	}
	if(0 != strip_path(filename, &dir_node, path))/*执行到这个地方，该函数一定能成功*/
	{
		assert(0);
	}
	pNode = get_inode(dir_node->i_dev, inode_nr);

	struct stat s;
	s.st_dev = pNode->i_dev;
	s.st_ino = pNode->i_num;
	s.st_mode = pNode->i_mode;
	s.st_rdev = is_special(pNode->i_mode) ? pNode->i_start_sect : NO_DEV;/*针对特殊设备*/
	s.st_size = pNode->i_size;

	put_inode(pNode);

	phys_copy(va2la(src, msg->BUF), va2la(TASK_FS, &s), sizeof(struct stat));
	return 0;
}