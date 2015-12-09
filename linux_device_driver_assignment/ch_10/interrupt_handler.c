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


