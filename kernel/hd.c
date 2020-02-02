#include "const.h"
#include "type.h"
#include "hd.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"

PRIVATE void interrupt_wait();
PRIVATE int waitfor(int mask, int val, int timeout);
PRIVATE void print_identify_info(u16 *hdinfo);
PRIVATE void hd_handler(int irq);
PRIVATE void hd_identify(int drive);
PRIVATE void init_hd();
PRIVATE void hd_cmd_out(hd_cmd  *cmd);

PRIVATE	u8	hd_status;
PRIVATE	u8	hdbuf[SECTOR_SIZE * 2];

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

		printf("msg.type:%d\n", msg.type);

		switch(msg.type)
		{
			case DEV_OPEN:
				hd_identify(0);
				break;
			default:
				panic("unknown msg type");
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
	u8 *pNrDrives = (u8*)0x475;
	printl("NrDrives:%d.\n", *pNrDrives);
	assert(*pNrDrives);

	put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);
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
