#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"


/*
功能：断言失败
*/
PUBLIC void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	printl("%c assert(%s) failed:file:%s,base_file:%s,ln:%d"
		, MAG_CH_ASSERT, exp, file, base_file, line);

	spin("assertion_failure()");

	/*shouldn't reach here*/
	__asm__ __volatile__("ud2");
}


/*
功能：自锁
*/
PUBLIC void spin(char *func_name)
{
	printf("\nspinning in %s......\n", func_name);
	while(1){}
}


/*
功能：内存比较
返回值：0-内存完全相同
*/
PUBLIC int memcmp(const void *s1, const void *s2, int n)
{
	int i;
	const char *p1 = (const char *)s1;
	const char *p2 = (const char *)s2;

	if(s1 == 0 || s2 == 0)/*s1或s2为无效指针*/
	{
		return s1 - s2;
	}

	for(i = 0;i < n;i++, p1++, p2++)
	{
		if(*p1 != *p2)
		{
			return *p1 - *p2;
		}
	}
	return 0;

}