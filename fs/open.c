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

PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc);
PRIVATE int alloc_imap_bit(int dev);
PRIVATE struct inode* create_file(const char* path, int flags);
PRIVATE void new_directory_entry(struct inode *de, const char *filename, int inode_nr);
PRIVATE struct inode* new_inode(int dev, int inode_nr, int start_sect);


/*
功能：打开文件具体操作
*/
PUBLIC int do_open(MESSAGE *msg)
{
	char path[MAX_PATH];
	char filename[MAX_FILENAME_LEN];
	int flags = msg->FLAGS;
	int fd = -1;
	struct inode *dir_inode;
	struct inode *pNode = 0;
	int inode_nr = 0;
	int i, j;
	int imode = 0;

	/*获取文件路径*/
	phys_copy(va2la(TASK_FS, path), va2la(proc2pid(pcaller), msg->PATHNAME), msg->NAME_LEN);
	path[msg->NAME_LEN] = 0; 


	/*查看是否有空闲fd*/
	for(i = 0;i < NR_FILES;i++)
	{
		if(0 == pcaller->filp[i])
		{
			fd = i;
			break;
		}
	}
	if(fd < 0 || fd >= NR_FILES)
	{
		panic("fd slot is full");
	}
	


	/*查看fd_table中是否有空闲槽位*/
	for(i = 0;i < NR_FILE_DESC;i++)
	{
		if(0 == fd_table[i].fd_inode)/*空闲槽位*/
		{
			break;
		}
	}
	if(i >= NR_FILE_DESC)
	{
		panic("fd table is full");
	}

	inode_nr = search_file(path);
	if(INVALID_INODE == inode_nr)
	{
		if(flags & O_CREAT)
		{
			pNode = create_file(path, flags);
			inode_nr = pNode->i_num;
		}
		else
		{
			printl("file exist\n");
			return -1;
		}
	}
	else if(flags & O_RDWR)
	{
		if((flags & O_CREAT) && !(flags & O_TRUNC))
		{
			return -1;
		}
		assert((flags == O_RDWR ||
				flags == O_RDWR | O_TRUNC) ||
				flags == O_RDWR | O_TRUNC | O_CREAT);
		strip_path(filename, &dir_inode, path);
		pNode = get_inode(dir_inode->i_dev, inode_nr);
	}
	else
	{
		return -1;
	}
	if(flags & O_TRUNC)
	{
		pNode->i_size = 0;
		sync_inode(pNode);
	}

	assert(0 != inode_nr && 0 != pNode);

	/*将进程的fd绑定到fd_table中的某个元素上*/
	pcaller->filp[fd] = &fd_table[i];

	/*将文件描述符和inode绑定*/
	fd_table[i].fd_inode = pNode;

	fd_table[i].fd_cnt = 1;
	fd_table[i].fd_mode = flags;
	fd_table[i].fd_pos = 0;

	imode = pNode->i_mode;


	if(I_CHAR_SPECIAL == imode)/*特殊字符文件*/
	{
		MESSAGE driver_msg;
		driver_msg.type = DEV_OPEN;
		driver_msg.DEVICE = MINOR(pNode->i_start_sect);/*次设备号*/
		assert(dd_map[MAJOR(pNode->i_start_sect)].driver_nr != INVALID_DRIVER);
		send_rec(BOTH, dd_map[MAJOR(pNode->i_start_sect)].driver_nr, &driver_msg);
	}
	else if(I_DIRECTORY == imode)/*目录文件*/
	{
		assert(pNode->i_num == ROOT_INODE);
	}
	else
	{
		assert(I_REGULAR == imode);
	}
	
	return fd;
}


/*
功能：关闭文件操作
*/
PUBLIC int do_close(MESSAGE *msg)
{
	int fd = msg->FD;

	if(fd < 0 || fd >= NR_FILES)/*无效fd*/
	{
		return -1;
	}
	put_inode(pcaller->filp[fd]->fd_inode);
	if(--pcaller->filp[fd]->fd_cnt == 0)
		pcaller->filp[fd]->fd_inode = 0;
	pcaller->filp[fd] = 0;

	return 0;
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
					sect_nr = (j + i * SECTOR_SIZE) * 8 + k - 1 + sb->n_1st_sect;/*第一个扇区预留出来了*/
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
	pNode = new_inode(dir_inode->i_dev, inode_nr, sect_nr);

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
				dst = en;
				break;
			}		
		}
		if(dst)/*找到空闲槽位*/
		{
			break;
		}
		else if(!nr_dir)
		{
			dst = en;
			de_node->i_size += DIR_ENT_SIZE;
			break;
		}
	}

	/*填写目录信息*/
	dst->inode_nr = inode_nr;
	strcpy(dst->name, filename);

	/*将更新之后的根目录文件写入硬盘*/
	WR_SECT(de_node->i_dev, i + sect_nr);

	/*更新de_node*/
	sync_inode(de_node);
}


/*
功能：创建新的节点
*/
PRIVATE struct inode* new_inode(int dev, int inode_nr, int start_sect)
{
	struct inode *pNode = get_inode(dev, inode_nr);/*在缓存中查找inode,若没有则申请一个新的缓存*/

	pNode->i_mode = I_REGULAR;
	pNode->i_size = 0;
	pNode->i_start_sect = start_sect;
	pNode->i_nr_sects = NR_DEFAULT_FILE_SECTS;

	pNode->i_dev = dev;
	pNode->i_num = inode_nr;
	pNode->i_cnt = 1;

	/*将inode信息写入硬盘*/
	sync_inode(pNode);

	return pNode;
}