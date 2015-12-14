// Example# 3.1b:  Simple Char Driver with Statically allocated Major#  
//                works only in 2.6  (Similar .. to Example# 3.1a)


//      using dev_t, struct cdev (2.6)

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
#include <generated/utsrelease.h>
#else
#include <linux/utsrelease.h>
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>		// 2.6
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/poll.h>
#include <asm/uaccess.h>

#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include "main.h"
#include "ioctl_code.h"

#define MOUSE_PORT              0x60
#define BUTN_MASK               0x08
#define RIGHT_BTN_PRESS         (BUTN_MASK | 0x02)
#define LEFT_BTN_PRESS          (BUTN_MASK | 0x01)

static int wq_init = 0;
int dev_id = 128;

CDD_MOUSE_INFO_T mouse_info_ctx;

#define PREPARE_WORK(_work, _func) \
    do {\
        (_work)->func = (_func);\
    } while(0)

#define PREPARE_DELAYED_WORK(_work, _func) \
    PREPARE_WORK(&(_work)->work, (_func))

#define CDD_MOUSE_IRQ           12
static void
movement_work (struct work_struct *wk)
{
   printk(KERN_ALERT "Get some movemnet");
}

static irqreturn_t
irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{

    printk(KERN_ALERT "Get some movemnet");
    
    if (!wq_init) {
        INIT_WORK(&mouse_info_ctx.wk, movement_work);
        wq_init = 1;
    } else {
        PREPARE_WORK(&mouse_info_ctx.wk, movement_work);
    }


    queue_work(mouse_info_ctx.wq, &mouse_info_ctx.wk);

    return IRQ_HANDLED;
}

int
irq_init (void)
{
    int retval = 0;
    unsigned long irqflags = 0;

    memset (&mouse_info_ctx, 0, sizeof (CDD_MOUSE_INFO_T));
    do {

        mouse_info_ctx.mutex_lock = (struct semaphore *) kmalloc(sizeof (struct semaphore), GFP_ATOMIC);
        if (mouse_info_ctx.mutex_lock == NULL) {
            break;
        }

        mouse_info_ctx.wq = create_workqueue(QUEUE_NAME);
        if (mouse_info_ctx.wq == NULL) {
            break;
        }

        irqflags = IRQF_SHARED;

#if 0
        if (!can_request_irq(CDD_MOUSE_IRQ, irqflags)) {
            /* Clean up if cannot requset the IRQ */
            free_irq(CDD_MOUSE_IRQ, (void *) &dev_id);
        } 
#endif

        retval = request_irq(CDD_MOUSE_IRQ,
                             (irq_handler_t) irq_handler,
                             irqflags,
                             (char *) "CDD Mouse IRQ handler",
                             (void *) &dev_id);

    } while(0);

    return retval;
}

void
irq_exit (void)
{
    unsigned long irqflags = 0;

    irqflags = IRQF_SHARED;
    /* Clean up if cannot requset the IRQ */
    free_irq(CDD_MOUSE_IRQ, (void *) &dev_id);

    kfree(mouse_info_ctx.mutex_lock);
    destroy_workqueue(mouse_info_ctx.wq);
}
