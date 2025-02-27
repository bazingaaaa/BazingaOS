#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "const.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"


void init_8259A()
{
	out_byte(INT_M_CTL, 0x11);
	out_byte(INT_S_CTL, 0x11);

	out_byte(INT_M_CTLMASK, INT_VEC_IRQ0);
	out_byte(INT_S_CTLMASK, INT_VEC_IRQ8);

	out_byte(INT_M_CTLMASK, 0x4);
	out_byte(INT_S_CTLMASK, 0x2);

	out_byte(INT_M_CTLMASK, 0x1);
	out_byte(INT_S_CTLMASK, 0x1);

	out_byte(INT_M_CTLMASK, 0b11111111);/*屏蔽所有中断*/
	out_byte(INT_S_CTLMASK, 0b11111111);/*屏蔽所有中断*/

	int i;
	for(i = 0;i < NR_IRQS;i++)
	{
		irq_table[i] = spurious_irq;
	}
}


PUBLIC void put_irq_handler(int irq, irq_handler handler)
{
	disable_irq(irq);
	irq_table[irq] = handler;
}

PUBLIC void spurious_irq(int irq)
{
	disp_str("spurious_irq: ");
	disp_int(irq);
	disp_str("\n");
}
