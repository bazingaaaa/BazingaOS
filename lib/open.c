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


PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc);
PRIVATE int alloc_imap_bit(int dev);
PRIVATE struct inode* create_file(const char* path, int flags);
PRIVATE void new_directory_entry(struct inode *de, const char *filename, int inode_nr);
PRIVATE struct inode* new_inode(int dev, int inode_nr);


/*
功能：打开文件具体操作
*/
PUBLIC int do_open(MESSAGE *msg)
{
	return 0;
}


/*
功能：关闭文件操作
*/
PUBLIC int do_close(MESSAGE *msg)
{
	return 0;
}


/*
功能：打开文件（提供给外部的接口）
返回值：fd
备注：通过向文件系统发送消息完成
*/
PUBLIC int open(const char* pathname, int flags)
{
	MESSAGE msg;

	msg.type = OPEN;
	msg.PATHNAME = pathname;
	msg.FLAGS = flags;
	msg.NAME_LEN = strlen(pathname);
	send_rec(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.FD;
}


/*
功能：在指定设备上分配imap图位
输入：dev-次设备号
返回值：返回inode号(0为无效inode号)
备注：读取硬盘上的inode图，获取空闲inode图位并写入硬盘
*/
PRIVATE int alloc_imap_bit(int dev)
{
	int i, j, k;/*循环中的控制变量可以不用初始化*/
	int inode_nr = 0;/*分配的inode号*/

	/*读取硬盘中的inode图，依次读取，查找空闲位*/
	struct super_block* sb = get_super_block(dev);
	int imap_blk0_nr = 2;
	for(i = 0;i < sb->nr_imap_sects;i++)
	{
		RD_SECT(dev, imap_blk0_nr + i);
		for(j = 0;j < SECTOR_SIZE;j++)
		{
			if(fsbuf[j] == 0xff)
				continue;
			for(k = 0;k < 8;k++)
			{
				if((fsbuf[j] & (1<<k)) == 0)/*找到空闲位置k*/
				{
					fsbuf[j] |= (1<<k);
					WR_SECT(dev, imap_blk0_nr + i);/*改动之后写入硬盘*/
					return (j + i * SECTOR_SIZE) * 8 + k;
				}
			}
		}
	}

	/*no free bit in inode map*/
	panic("inode map is probably full\n");

	return 0;
}	


/*
功能：在指定设备上分配smap图位
输入：dev-次设备号
	  nr_sects_to_alloc-需要分配的扇区数
返回值：分配的扇区起始号
备注：读取硬盘上的sector图，获取空闲sector图位并写入硬盘
*/
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	int i, j, k;
	int sect_nr = 0;

	/*读取硬盘上的smap*/
	struct super_block *sb = get_super_block(dev);
	int smap_blk0_nr = 1 + 1 + sb->nr_imap_sects;
	for(i = 0;i < sb->nr_smap_sects;i++)/*查找free slot*/
	{
		RD_SECT(dev, smap_blk0_nr + i);
		for(j = 0;j < SECTOR_SIZE;j++)
		{
			if(fsbuf[j] == 0xff)
				continue;
			for(k = 0;k < 8;k++)
			{
				if((fsbuf[j] & (1<<k)) != 0)/*位置K已经被占用*/
				{
					continue;
				}
				if(!sect_nr)/*还未分配起始扇区号*/
				{
					sect_nr = (j + i * SECTOR_SIZE) * 8 + k;
				}
				if(nr_sects_to_alloc > 0)/*未分配完毕*/
				{
					fsbuf[j] |= (1<<k);
					nr_sects_to_alloc--;
				}
				else/*已经全部分配完毕*/
				{
					assert(nr_sects_to_alloc == 0);
					break;
				}
			}
		}
		if(sect_nr)/*找到空闲槽位*/
		{
			WR_SECT(dev, smap_blk0_nr + i);
		}
		if(nr_sects_to_alloc == 0)/*已经全部分配完毕*/
		{
			break;
		}
	}

	assert(nr_sects_to_alloc == 0);

	return sect_nr;
}


/*
功能：创建文件
*/
PRIVATE struct inode* create_file(const char* path, int flags)
{
	char filename[MAX_FILENAME_LEN];
	struct inode* dir_inode;
	struct inode* pNode = 0;
	int inode_nr = 0;
	int sect_nr = 0;

	/*获取目录节点*/
	if(0 != strip_path(filename, &dir_inode, path))
	{
		return pNode;
	}

	/*分配inode号*/
	inode_nr = alloc_imap_bit(dir_inode->i_dev);

	/*分配sector*/
	sect_nr = alloc_smap_bit(dir_inode->i_dev, NR_DEFAULT_FILE_SECTS);

	/*分配inode*/
	pNode = new_inode(dir_inode->i_dev, inode_nr);

	/*在根目录中创建目录项*/
	new_directory_entry(dir_inode, filename, inode_nr);

	return pNode;
}


/*
功能：在指定目录下创建新的目录项
备注：找到根目录文件，写入新的目录项,首先需要找到要给空闲槽位
*/
PRIVATE void new_directory_entry(struct inode *de_node, const char *filename, int inode_nr)
{
	int sect_nr = de_node->i_start_sect;/*根目录文件起始扇区*/
	int nr_dir = de_node->i_size / DIR_ENT_SIZE;/*需要查询的目录数目*/
	int nr_sects = de_node->i_nr_sects;
	struct dir_entry *en = 0;
	struct dir_entry *dst = 0;
	int i, j;

	for(i = 0;i < nr_sects;i++)
	{
		RD_SECT(de_node->i_dev,	sect_nr + i);
		en = (struct dir_entry*)fsbuf;
		for(j = 0;j < SECTOR_SIZE / DIR_ENT_SIZE && nr_dir > 0;j++, en++, nr_dir--)
		{
			if(0 == en->inode_nr)
			{
				//printf("j: %d\n", j);
				dst = en;
				break;
			}		
		}
		if(dst)/*找到空闲槽位*/
		{
			//printf("A %d\n", );
			break;
		}
		else if(!nr_dir)
		{
			//printf("B\n");
			dst = en;
			de_node->i_size += DIR_ENT_SIZE;
			break;
		}
	}

	/*填写目录信息*/
	dst->inode_nr = inode_nr;
	strcpy(dst->name, filename);

	WR_SECT(de_node->i_dev, i + sect_nr);
}


/*
功能：创建新的节点
*/
PRIVATE struct inode* new_inode(int dev, int inode_nr)
{
	struct inode *pNode = get_inode(dev, inode_nr);/*在缓存中查找inode*/

	pNode->i_dev = dev;
	pNode->i_num = inode_nr;
	pNode->i_cnt = 1;

	/*将inode信息写入硬盘*/
	sync_inode(pNode);
	return 0;
}