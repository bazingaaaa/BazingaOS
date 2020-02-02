#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "global.h"


PUBLIC void clock_handler(int iqr)
{
	//disp_str("#");
	ticks++;
	p_proc_ready->ticks--;

	if(0 != k_reenter)
	{
		//disp_str("!");
		return ;
	}
	schedule();
}


PUBLIC void milli_delay(int milli_sec)
{
	int ticks = get_ticks();
	while((get_ticks() - ticks) * 1000 / HZ < milli_sec) 
	{
		;
	}
}

PUBLIC void init_clock()
{
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ/HZ));
	out_byte(TIMER0, (u8)((TIMER_FREQ/HZ) >> 8));

	put_irq_handler(CLOCK_INT_NO, clock_handler);
	enable_irq(CLOCK_INT_NO);
}