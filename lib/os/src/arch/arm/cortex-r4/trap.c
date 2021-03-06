/*
 * File      : trap.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2008-12-11     XuXinming    first version
 * 2013-05-24     Grissiom     port to RM48x50
 */

#include <os.h>
#include <rtcpu.h>

#include <sys_vim.h>

#include "armv7.h"

/**
 * @addtogroup RM48x50
 */
/*@{*/

/**
 * this function will show registers of CPU
 *
 * @param regs the registers point
 */
void os_arch_show_register (struct os_arch_exp_stack *regs)
{
    printk("Execption:\n");
    printk("r00:0x%08x r01:0x%08x r02:0x%08x r03:0x%08x\n", regs->r0, regs->r1, regs->r2, regs->r3);
    printk("r04:0x%08x r05:0x%08x r06:0x%08x r07:0x%08x\n", regs->r4, regs->r5, regs->r6, regs->r7);
    printk("r08:0x%08x r09:0x%08x r10:0x%08x\n", regs->r8, regs->r9, regs->r10);
    printk("fp :0x%08x ip :0x%08x\n", regs->fp, regs->ip);
    printk("sp :0x%08x lr :0x%08x pc :0x%08x\n", regs->sp, regs->lr, regs->pc);
    printk("cpsr:0x%08x\n", regs->cpsr);
}

/**
 * When comes across an instruction which it cannot handle,
 * it takes the undefined instruction trap.
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
void os_arch_trap_udef(struct os_arch_exp_stack *regs)
{
    printk("undefined instruction\n");
    os_arch_show_register(regs);
    if (os_task_self() != NULL)
        printk("Current Thread: %s\n", os_task_self()->name);
    os_arch_shutdown();
}

/**
 * The software interrupt instruction (SWI) is used for entering
 * Supervisor mode, usually to request a particular supervisor
 * function.
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
void os_arch_trap_svc(struct os_arch_exp_stack *regs)
{
    printk("software interrupt\n");
    os_arch_show_register(regs);
#ifdef RT_USING_FINSH
    list_task();
#endif
    os_arch_shutdown();
}

/**
 * An abort indicates that the current memory access cannot be completed,
 * which occurs during an instruction prefetch.
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
void os_arch_trap_pabt(struct os_arch_exp_stack *regs)
{
    printk("prefetch abort\n");
    os_arch_show_register(regs);
#ifdef RT_USING_FINSH
    list_task();
#endif
    os_arch_shutdown();
}

/**
 * An abort indicates that the current memory access cannot be completed,
 * which occurs during a data access.
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
void os_arch_trap_dabt(struct os_arch_exp_stack *regs)
{
    printk("Data Abort ");
    os_arch_show_register(regs);
#ifdef RT_USING_FINSH
    list_task();
#endif
    os_arch_shutdown();
}

/**
 * Normally, system will never reach here
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
void os_arch_trap_resv(struct os_arch_exp_stack *regs)
{
    printk("Reserved trap\n");
    os_arch_show_register(regs);
#ifdef RT_USING_FINSH
    list_task();
#endif
    os_arch_shutdown();
}

extern rt_isr_handler_t isr_table[];
void os_arch_trap_irq(void)
{
    int irqno;
    struct rt_irq_desc* irq;
    extern struct rt_irq_desc irq_desc[];

    irq = (struct rt_irq_desc*) vimREG->IRQVECREG;
    irqno = ((uint32_t) irq - (uint32_t) &irq_desc[0])/sizeof(struct rt_irq_desc);

    /* invoke isr */
    irq->handler(irqno, irq->param);
}

void os_arch_trap_fiq(void)
{
    printk("fast interrupt request\n");
}

/*@}*/
