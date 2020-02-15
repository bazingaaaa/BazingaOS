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
功能：文件删除操作
返回值：0-删除成功 否则删除失败
备注：删除文件分为4步 1.清除inode图中对应的位 
					2.清除sect图中对应的位 
					3.清除inode数组中对应的inode 
					4.清除对应的目录项
*/
PUBLIC int do_unlink(MESSAGE *msg)
{
	char path[MAX_PATH];
	int str_len = msg->NAME_LEN;

	assert(str_len < MAX_PATH);
	phys_copy(va2la(TASK_FS, path), va2la(proc2pid(pcaller), msg->PATHNAME), str_len);
	path[str_len] = 0;/*此处是必要的，拷贝的时候末尾没有'\0'*/

	if(0 == strcmp(path, "/"))/*不能删除根目录文件*/
	{
		return -1;
	}

	int inode_nr = search_file(path);
	if(inode_nr == INVALID_INODE)/*文件不存在*/
	{
		return -1;
	}

	char filename[MAX_PATH];
	struct inode *dir_node = 0;
	if(0 != strip_path(filename, &dir_node, path))/*提取文件名*/
	{
		return -1;
	}

	assert(strlen(filename) < MAX_PATH);
	int dev = dir_node->i_dev;
	struct inode *pNode = get_inode(dev, inode_nr);
	if(I_REGULAR != pNode->i_mode)/*文件非普通文件*/
	{
		return -1;
	}
	if(1 < pNode->i_cnt)/*有多个文件描述符指向该inode*/
	{
		return -1;
	}
	
	/*清除inode图中对应的位，inode位图只占用了一个扇区*/
	int inode_blk0_nr = 1 + 1;
	RD_SECT(dev, inode_blk0_nr);
	int byte_idx = inode_nr / 8;
	int bit_idx = inode_nr % 8;
	assert(byte_idx < SECTOR_SIZE);
	assert(fsbuf[byte_idx % SECTOR_SIZE] & (1<<bit_idx));
	fsbuf[byte_idx % SECTOR_SIZE] &= ~(1<<bit_idx);
	WR_SECT(dev, inode_blk0_nr);

	/*清除sect图中对应的位*/
	struct super_block *sb = get_super_block(dev);
	bit_idx = pNode->i_start_sect - sb->n_1st_sect + 1;/*n_1st_sect对应的扇区索引是1（0为保留）*/
	int bits_left = pNode->i_nr_sects;/*需要处理的扇区数*/
	byte_idx = bit_idx / 8;
	int byte_cnt = (bits_left - (8 - bit_idx % 8)) / 8;/*总字节数，除第一个字节和最后一个字节*/
	int i, j, k;
	int sect_beg = 1 + 1 + 1 + byte_idx / SECTOR_SIZE;/*起始处理的扇区号*/

	/*处理第一个字节*/
	RD_SECT(dev, sect_beg);
	for(i = bit_idx % 8;i < 8 && bits_left > 0;i++, bits_left--)
	{
		assert(fsbuf[byte_idx % SECTOR_SIZE] & (1<<i));
		fsbuf[byte_idx % SECTOR_SIZE] &= ~(1<<i);
	}
	/*处理中间字节*/
	i = byte_idx % SECTOR_SIZE + 1;/*移动至下一个字节*/
	for(j = 0;j < byte_cnt && bits_left > 0;j++, bits_left -= 8, i++)
	{
		if(i == SECTOR_SIZE)/*字节数跨越扇区*/
		{
			i = 0;
			WR_SECT(dev, sect_beg);
			RD_SECT(dev, ++sect_beg);
		}
		assert(fsbuf[i] == 0xFF);
		fsbuf[i] = 0; 
	}
	/*处理最后一个字节*/
	if(i == SECTOR_SIZE)/*字节数跨越扇区*/
	{
		i = 0;
		WR_SECT(dev, sect_beg);
		RD_SECT(dev, ++sect_beg);
	}
	assert(bits_left < 8);
	u8 mask = ~((~(u8)0)<<bits_left);
	assert(fsbuf[i] & mask == mask);
	fsbuf[i] &= ((~0)<<bits_left);
	WR_SECT(dev, sect_beg);

	/*清除inode数组中对应的inode*/
	pNode->i_mode = 0;/*不初始化也可以*/
	pNode->i_size = 0;
	pNode->i_start_sect = 0;
	pNode->i_nr_sects = 0;
	put_inode(pNode);/*主要是处理缓存中的inode引用计数，避免inode位图释放后还能查找到对应的缓存*/
	sync_inode(pNode);/*写入硬盘*/

	/*清除对应的目录项*/
	int flag = 0;
	int dir_size = 0;
	int sect_nr = dir_node->i_start_sect;/*根目录文件起始扇区*/
	int nr_dir = dir_node->i_size / DIR_ENT_SIZE;/*需要查询的目录数目*/
	int nr_sects = dir_node->i_nr_sects;
	int m = 0;

	for(i = 0;i < nr_sects;i++)
	{
		RD_SECT(dev, sect_nr + i);
		struct dir_entry* en = (struct dir_entry*)fsbuf;
		for(j = 0;j < SECTOR_SIZE / DIR_ENT_SIZE && m < nr_dir;j++, en++)
		{
			m++;/*必须在此处m++，不然break后m不会自增*/
			if(inode_nr == en->inode_nr)
			{
				en->inode_nr = INVALID_INODE;
				memset(en->name, 0, MAX_FILENAME_LEN);
				WR_SECT(dev, sect_nr + i);
				flag = 1;
				break;
			}
			if(INVALID_INODE != en->inode_nr)
			{
				dir_size += DIR_ENT_SIZE;
			}	
		}
		if(flag)/*目录已删除*/
		{
			break;
		}
	}
	assert(flag);
	if(m == nr_dir)
	{
		dir_node->i_size = dir_size;
		sync_inode(dir_node);
	}

	return 0;
}