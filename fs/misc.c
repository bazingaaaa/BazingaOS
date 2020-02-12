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

	if(strip_path(filename, &dir_inode, path))/*提取待查文件的目录节点和文件名*/
	{
		return inode_nr;
	}

	/*根目录文件的设备只能是ROOT_DEV*/
	assert(dir_inode->i_dev == ROOT_DEV);

	int sect_nr = dir_inode->i_start_sect;
	int nr_sect = dir_inode->i_nr_sects;
	int nr_en = dir_inode->i_size / DIR_ENT_SIZE;
	/*在根目录文件中找文件*/
	for(i = 0;i < nr_sect;i++)
	{
		RD_SECT(dir_inode->i_dev, sect_nr + i);
		struct dir_entry *en = (struct dir_entry*)fsbuf;
		for(j = 0;j < SECTOR_SIZE / DIR_ENT_SIZE && nr_en > 0;j++, en++, nr_en--)
		{
			if(0 == en->inode_nr)/*无效节点*/
			{
				continue;
			}
			if(!strcmp(filename, en->name))/*文件名相同*/
			{
				inode_nr = en->inode_nr;
				break;
			}	
		}
		if(inode_nr)
		{
			break;
		}
	}

	return inode_nr;
}