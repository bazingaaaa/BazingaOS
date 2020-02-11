#include "const.h"
#include "type.h"


/* 
功能：将整型以16进制转换为字符串，开头为0x
备注：数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800
*/
PUBLIC char *itoa(char * str, int num)
{
	char *p = str;
	int flag = 0;
	int i;

	*p++ = '0';
	*p++ = 'x';

	if(0 == num)
	{
		*p++ = '0';
	}
	else
	{
		for(i = 28;i >= 0;i = i - 4)
		{
			int tmp = num >> i & 0xf;
			if(1 == flag || tmp != 0)/*为了跨越前面的0*/
			{
				*p = tmp + '0';
				if(tmp > 9)
				{
					*p += 7;
				}
				p++;
			}
		}
	}
	*p = 0;
	return str;
}


/*
功能：按16位整型输出到屏幕上
备注：输出的位置由disp_pos确定
*/
PUBLIC void disp_int(int input)
{
	char output[40];
	itoa(output, input);
	disp_str(output);
}


/*
功能：延时函数
*/
PUBLIC void delay(int time)
{
	int i, j, k;

	for(i = 0;i < time;i++)
	{
		for(j = 0;j < 10;j++)
		{
			for(k = 0;k < 100000;k++)
			{}
		}
	}
}


/*
功能：计算字符串长度
*/
PUBLIC int strlen(char* str)
{
	int len = 0;
	while(*str)
	{
		str++;
		len++;
	}
	return len;
}


