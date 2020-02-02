#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"

PUBLIC void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	printl("%c assert(%s) failed:file:%s,base_file:%s,ln:%d"
		, MAG_CH_ASSERT, exp, file, base_file, line);

	spin("assertion_failure()");

	/*shouldn't reach here*/
	__asm__ __volatile__("ud2");
}


PUBLIC void spin(char *func_name)
{
	printf("\nspinning in %s......\n", func_name);
	while(1){}
}