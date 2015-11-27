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
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "main.h"

MODULE_LICENSE ("GPL");		//  Kernel isn't tainted .. but doesn't 
				//  it doesn't matter for SUSE anyways :-(  

static int
CDD_init (void)
{
    int i = 0;

    do {
        i = device_init();
        if (i) break;

    } while (0);
    
    return i;
}

static void
CDD_exit (void)
{
    device_exit();
}

module_init (CDD_init);
module_exit (CDD_exit);
