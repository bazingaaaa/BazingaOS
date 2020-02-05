#include "type.h"
#include "const.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"


/*
功能：把值按基底转换为字符串
备注：在转换完后，指针需要指向最后一个元素，所以需要传递指针的指针
*/
PRIVATE char* i2a(int val, int base, char ** ps)
{
	int m = val % base;
	int q = val / base;
	if (q) {
		i2a(q, base, ps);
	}
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');

	return *ps;
}


/*
功能：格式化输出
备注：目前只能解析%x，%s，%c
*/
int vsprintf(char* buf, const char* fmt, var_list args)
{
	char *p;
	char temp[256];
	var_list p_next_arg = args;
	int m;

	for(p = buf;*fmt;fmt++)
	{
		if(*fmt != '%')
		{
			*p++ = *fmt;
			continue;
		}

		fmt++;
		switch(*fmt)
		{
			case 'x':/*16进制*/
				m = *((int*)p_next_arg);/*此处不能用strlen处理，会出现异常情况*/
				i2a(m, 16, &p);
				p_next_arg += 4;
				break;
			case 's':/*字符串,注意此处的p_next_arg是指向参数的地址，当传入字符串指针时，p_next_arg是指向
					 该地址的地址*/
				strcpy(p, *(char**)p_next_arg);
				p += strlen(*(char**)p_next_arg);
				p_next_arg += 4;
				break;
			case 'c':/*字符*/
				*p = *(char*)p_next_arg;
				p++;
				p_next_arg += 4;
				break;
			case 'd':/*整型*/
				m = *((int*)p_next_arg);
				if (m < 0) {
					m = m * (-1);
					*p++ = '-';
				}
				i2a(m, 10, &p);
				p_next_arg += 4;
			default:
				break;
		}		
	}
	return p - buf;
}