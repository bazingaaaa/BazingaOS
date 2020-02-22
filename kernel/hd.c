#include "const.h"
#include "type.h"
#include "hd.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "string.h"

#define	DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
			 dev / NR_PRIM_PER_DRIVE : \
			 (dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)


PRIVATE void interrupt_wait();
PRIVATE int waitfor(int mask, int val, int timeout);
PRIVATE void print_identify_info(u16 *hdinfo);
PRIVATE void hd_handler(int irq);
PRIVATE void hd_identify(int drive);
PRIVATE void init_hd();
PRIVATE void hd_cmd_out(hd_cmd  *cmd);
PRIVATE void hd_open(int device);
PRIVATE void get_part_table(struct part_ent *entry, int drive, int sec_nr);
PRIVATE void partition(int device, int style);
PRIVATE void print_hdinfo(struct hd_info *hdinfo);
PRIVATE void hd_close(int device);
PRIVATE void hd_rdwt(MESSAGE *msg);
PRIVATE void hd_ioctl(MESSAGE *msg);


PRIVATE	u8	hd_status;
PRIVATE	u8	hdbuf[SECTOR_SIZE * 2];
PRIVATE struct hd_info hd_info[1];/*目前只加载了一块硬盘*/

/*
功能：硬盘任务，用于处理和硬盘有关的消息，与硬件打交道
*/
PUBLIC void task_hd()
{
	MESSAGE msg;

	init_hd();

	while(1)
	{
		send_rec(RECEIVE, ANY, &msg);

		int src = msg.source;

		//printf("msg.type:%d\n", msg.type);

		switch(msg.type)
		{
			case DEV_OPEN:
				hd_open(msg.DEVICE);
				break;
			case DEV_CLOSE:
				hd_close(msg.DEVICE);
				break;
			case DEV_READ:
			case DEV_WRITE:
				hd_rdwt(&msg);
				break;
			case DEV_IOCTL:
				hd_ioctl(&msg);
				break;
			default:
				panic("hd unknown msg type");
				break;
		}
		send_rec(SEND, src, &msg);
	}
}


/*
功能：检查硬盘驱动，分配中断处理句柄，使能中断
*/
PRIVATE void init_hd()
{
	int i;
	u8 *pNrDrives = (u8*)0x475;
	printl("NrDrives:%d.\n", *pNrDrives);
	assert(*pNrDrives);

	put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);

	/*初始化硬盘信息*/
	for(i = 0;i < sizeof(hd_info)/sizeof(hd_info[0]);i++)
		memset(&hd_info[i], 0, sizeof(hd_info[0]));/*地址输错会导致系统出现page fault*/
	hd_info[0].open_cnt = 0;
}


/*
功能：获取并打印硬盘信息
*/
PRIVATE void hd_identify(int drive)
{
	hd_cmd cmd;
	cmd.command = ATA_IDENTIFY;
	cmd.device = MAKE_DEVICE_REG(0, drive, 0);

	/*向硬盘发送命令*/
	hd_cmd_out(&cmd);

	/*等待硬盘处理完毕中断*/
	interrupt_wait();

	/*读取硬盘返回的信息*/
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	/*输出硬盘信息*/
	print_identify_info((u16*)hdbuf);

	/*保存硬盘信息,填写第一个硬盘的信息*/
	u16 *hdinfo_buf = (u16*)hdbuf;
	hd_info[0].primary[0].base = 0;
	hd_info[0].primary[0].size = ((int)hdinfo_buf[61] << 16) + hdinfo_buf[60];
}


/*
功能：打印硬盘信息
*/
PRIVATE void print_identify_info(u16 *hdinfo)
{
	int i, k;
	char s[64];

	struct iden_info_ascii{
		int idx;
		int len;
		char *desc;
	}iinfo[] = {
		{10, 20, "HD SN"},
		{27, 40, "HD Model"}
	};

	for(k = 0;k < sizeof(iinfo) / sizeof(iinfo[0]);k++)
	{
		char *p = (char*)&hdinfo[iinfo[k].idx];

		for(i = 0;i < iinfo[k].len/2;i++)
		{
			s[i * 2 + 1] = *p++;
			s[i * 2] = *p++;
		}
		s[i * 2] = 0;
		printl("%s: %s\n", iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	printl("LBA supported: %s\n",
	       (capabilities & 0x0200) ? "Yes" : "No");

	int cmd_set_supported = hdinfo[83];
	printl("LBA48 supported: %s\n",
	       (cmd_set_supported & 0x0400) ? "Yes" : "No");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	printl("HD size: %dMB\n", sectors * 512 / 1000000);
}


/*
功能：通过端口向硬盘发送命令
*/
PRIVATE void hd_cmd_out(hd_cmd  *cmd)
{
	if(!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
		panic("hd error.");

	/*激活中断使能位*/
	out_byte(REG_DEV_CTRL, 0);

	out_byte(REG_FEATURE, cmd->features);
	out_byte(REG_SECTOR_COUNT, cmd->count);
	out_byte(REG_LBA_LOW, cmd->lba_low);
	out_byte(REG_LBA_MID, cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEV, cmd->device);
	out_byte(REG_CMD, cmd->command);
}


/*
功能：等待中断
*/
PRIVATE void interrupt_wait()
{
	MESSAGE msg;
	send_rec(RECEIVE, INTERRUPT, &msg);
}


/*
功能：等待某个状态
输入：mask-等待的掩码
	 val-等待的值
	 timeout-超时时间,单位：ms
*/
PRIVATE int waitfor(int mask, int val, int timeout)
{
	int t = get_ticks();

	while((get_ticks() - t) * 1000 / HZ < timeout)
	{
		if((in_byte(REG_STATUS) & mask) == val)
		{
			return 1;
		}
	}
	return 0;
}


/*
功能：中断处理句柄
*/
PRIVATE void hd_handler(int irq)
{
	hd_status = in_byte(REG_STATUS);

	inform_int(TASK_HD);
}


/*
功能：硬盘开启,打印硬盘信息
*/
PRIVATE void hd_open(int device)
{
	int drive = DRV_OF_DEV(device);

	//printf("hd_open device:%d   drive:%d\n", device, drive);

	/*只有一个硬盘*/
	assert(drive == 0);

	/*硬盘基本信息*/
	hd_identify(drive);

	/*第一次开启硬盘，打印硬盘信息*/
	if(hd_info[0].open_cnt++ == 0)
	{
		partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
		print_hdinfo(hd_info);
	}
}


/*
功能：查找给定硬盘上指定分区中的分区表（从硬盘中读取数据）
*/
PRIVATE void get_part_table(struct part_ent *entry, int drive, int sec_nr)
{
	hd_cmd cmd;

	cmd.features = 0;
	cmd.count = 1;
	cmd.lba_low = sec_nr & 0xFF;
	cmd.lba_mid = (sec_nr>>8) & 0xFF;
	cmd.lba_high = (sec_nr>>16) & 0xFF;	
	cmd.command = ATA_READ;
	cmd.device = MAKE_DEVICE_REG(1, drive, (sec_nr>>24)&0xF);

	/*向硬盘发送命令*/
	hd_cmd_out(&cmd);

	/*等待硬盘处理完毕中断*/
	interrupt_wait();

	/*读取硬盘返回的信息*/
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry, hdbuf + PARTITION_TABLE_OFFSET, sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}


/*
功能：读取分区信息，并填写hd_info结构体
输入：device-设备号(0-9)
	  style-硬盘风格（主分区或扩展分区）
*/
PRIVATE void partition(int device, int style)
{
	int i;
	int drive = DRV_OF_DEV(device);
	struct part_ent part_ents[NR_PART_PER_DRIVE];/*分区表*/

	if(P_PRIMARY == style)/*主分区*/
	{
		/*获取分区表信息*/
		get_part_table(part_ents, drive, drive);

		/*填写主分区信息*/
		int nr_prim_parts = 0;
		for(i = 0;i < NR_PART_PER_DRIVE;i++)
		{
			if(part_ents[i].sys_id == NO_PART)/*无效分区*/
				continue;
			int dev_nr = i + 1;
			nr_prim_parts++;
			hd_info[0].primary[dev_nr].base = part_ents[i].start_sect;
			hd_info[0].primary[dev_nr].size = part_ents[i].nr_sects;

			//printf("i:%d  sys_id:%d\n", i, part_ents[i].sys_id);
			if(part_ents[i].sys_id == EXT_PART)
				partition(dev_nr, P_EXTENDED);
		}
		assert(nr_prim_parts != 0)
	}
	else if(P_EXTENDED == style)/*扩展分区*/
	{
		int j = device % NR_PRIM_PER_DRIVE;/* 1 - 4*/
		int sec_base = hd_info[0].primary[j].base;
		int ext_start = sec_base;/*扩展扇区起始,迭代使用*/
		int nr_base = (j - 1) * NR_SUB_PER_PART;

		for(i = 0;i < NR_SUB_PER_PART;i++)
		{
			int nr_sub = nr_base + i;

			/*获取分区表信息*/
			get_part_table(part_ents, drive, ext_start);

			hd_info[0].logical[nr_sub].base = ext_start + part_ents[0].start_sect;
			hd_info[0].logical[nr_sub].size = part_ents[0].nr_sects;

			/*记录上一个扩展分区的起始扇区*/
			ext_start = sec_base + part_ents[1].start_sect;

			if(part_ents[1].sys_id == NO_PART)/*没有更深层次的扩展分区嵌套*/
				break;
		}
	}
	else/*shouldn't reach here*/
	{
		assert(0);
	}
}


/*
功能：打印硬盘信息
*/
PRIVATE void print_hdinfo(struct hd_info *hdinfo)
{
	int i;

	/*打印主分区信息*/
	for(i = 0;i < NR_PRIM_PER_DRIVE;i++)
	{
		printl("%sPart_%d: base %d(0x%x), size %d(0x%x)(in sector)\n",
			i == 0 ? " " : "     ",
			i,
			hdinfo[0].primary[i].base,
			hdinfo[0].primary[i].base,
			hdinfo[0].primary[i].size,
			hdinfo[0].primary[i].size
			);
	}

	/*打印逻辑分区信息*/
	for(i = 0;i < NR_SUB_PER_DRIVE;i++)
	{
		if(hdinfo[0].logical[i].size == 0)
			continue;
		printl("%sPart_%d: base %d(0x%x), size %d(0x%x)(in sector)\n",
			"         ",
			i,
			hdinfo[0].logical[i].base,
			hdinfo[0].logical[i].base,
			hdinfo[0].logical[i].size,
			hdinfo[0].logical[i].size
			);
	}
}


/*
功能：硬盘关闭
*/
PRIVATE void hd_close(int device)
{
	int drive = DRV_OF_DEV(device);

	//printf("hd_close device:%d   drive:%d\n", device, drive);

	/*只有一个硬盘*/
	assert(drive == 0);

	hd_info[drive].open_cnt--;
}


/*
功能：硬盘读写
备注：一次只能对硬盘的一个扇区进行读写
*/
PRIVATE void hd_rdwt(MESSAGE *msg)
{
	u64 pos = msg->POSITION;/*读/写硬盘的位置,可能大于4G*/
	int device = msg->DEVICE;/*读取*/
	int drive = DRV_OF_DEV(device);

	/*访问的地址不能超过1024G*/
	assert((pos>>SECTOR_SIZE_SHIFT) < (1<<31));

	/*读取的起始位置必须是整扇区字节处（暂定）*/
	assert((pos & 0x1ff) == 0);
	u32 sec_nr = (u32)(pos>>SECTOR_SIZE_SHIFT);
	sec_nr += device <= MAX_PRIM ? 
				hd_info[0].primary[device].base:/*主分区*/
				hd_info[0].logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE].base;/*逻辑分区*/
	int bytes_left = msg->CNT;/*读/写的字节数*/
	void *la = va2la(msg->PROC_NR, msg->BUF);

	hd_cmd cmd;

	cmd.features = 0;
	cmd.count = (msg->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lba_low = sec_nr & 0xFF;
	cmd.lba_mid = (sec_nr>>8) & 0xFF;
	cmd.lba_high = (sec_nr>>16) & 0xFF;	
	cmd.command = msg->type == DEV_READ ? ATA_READ : ATA_WRITE;
	cmd.device = MAKE_DEVICE_REG(1, drive, (sec_nr>>24)&0xF);

	hd_cmd_out(&cmd);

	while(bytes_left)
	{
		int bytes_proc = MIN(SECTOR_SIZE, bytes_left);

		if(msg->type == DEV_READ)/*读取数据*/
		{
			interrupt_wait();

			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			phys_copy(la, va2la(TASK_HD, hdbuf), bytes_proc);
		}
		else/*写入数据*/
		{
			if(!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				panic("hd writing error!");
			port_write(REG_DATA, la, bytes_proc);/*传入的是线性地址*/
			interrupt_wait();
		}
		bytes_left -= bytes_proc;
		la += bytes_proc;
	}
}


/*
功能：硬盘IO控制
备注：目前只完成了分区扇区起始和大小的读取
*/
PRIVATE void hd_ioctl(MESSAGE *msg)
{
	int device = msg->DEVICE;
	int drive = DRV_OF_DEV(device);
	struct hd_info *hdi = &hd_info[drive];

	if(DIOCTL_GET_GEO == msg->REQUEST)/*获取分区信息请求*/
	{
		void *dst = va2la(msg->PROC_NR, msg->BUF);
		void *src = va2la(TASK_HD, device <= MAX_PRIM ?
							&hdi->primary[device] : &hdi->logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE]);
		phys_copy(dst, src, sizeof(struct part_info));
	}
	else/*shouldn't reach here*/
	{
		assert(0);
	}
}