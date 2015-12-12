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
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "main.h"

static unsigned int CDDMajor = 0;
static unsigned int CDDMinor = 0;
static struct class *CDD_class;

struct cdev cdev;
dev_t devno;

static CDD_CONTEXT_T context;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	module_param(CDDMajor,int,0);
	module_param(CDDMinor,int,0);
#else
	MODULE_PARM(CDDMajor,"i");
	MODULE_PARM(CDDMinor,"i");
#endif

void
CDD_get_stats (CDD_STATS_T *stats)
{
    stats->alloc_sz = BUF_SZ;
    stats->used_sz = context.count;
}

static int
CDD_open (struct inode *inode, struct file *file)
{
    // MOD_INC_USE_COUNT;
    if (file->f_flags & O_TRUNC) {
	context.count = 0;
	context.storage[0] = '\0';
    }

    if (file->f_flags & O_APPEND) {
	file->f_pos = context.count;
    }

    file->private_data = (void *) &context;
    return 0;
}

static int
CDD_release (struct inode *inode, struct file *file)
{
    // MOD_DEC_USE_COUNT;
    
    file->private_data = NULL;
    return 0;
}

static ssize_t
CDD_read (struct file *file, char *buf, size_t count, loff_t * ppos)
{
    int len = 0, err = 0;
    CDD_CONTEXT_T *ctx = (CDD_CONTEXT_T *) file->private_data;
    char *CDD_storage = ctx->storage;

    do {

	if (ctx->count <= 0) {
	    break;
	}

	len = ctx->count - *ppos;
	if (len < 0) {
	    len = -EFAULT;
	    break;
	} else if (len == 0) {
	    break;
	} else if (len > count) {
	    len = count;
	}

	err = copy_to_user (buf, &CDD_storage[*ppos], len);

	if (err != 0) {
	    len = -EFAULT;
	    break;
	}

	*ppos += len;
    } while (0);

    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
static ssize_t
CDD_write (struct file *file, const char __user * buf, size_t count, loff_t * ppos)
#else
static ssize_t
CDD_write (struct file *file, const char *buf, size_t count, loff_t * ppos)
#endif
{
    int err;
    CDD_CONTEXT_T *ctx = (CDD_CONTEXT_T *) file->private_data;
    char *CDD_storage = ctx->storage;

    do {
        /* Won't write if it is read only */
	if (file->f_flags == O_RDONLY) {
	    count = -EFAULT;
	    break;
	}

        /* If Projected length is greater than the total buffer size */
	if ((*ppos + count) > BUF_SZ) {
	    count = BUF_SZ - *ppos;
	    if (count <= 0) {
		count = -EFAULT;
		break;
	    }
	}

	err = copy_from_user (&CDD_storage[*ppos], buf, count);
	if (err != 0) {
	    count = -EFAULT;
	    break;
	}

	ctx->count += count;
	*ppos += count;

    } while (0);

    return count;
}

static loff_t
CDD_llseek (struct file *filp, loff_t off, int whence)
{
    loff_t newpos;

    switch (whence) {
    case 0:
	newpos = off;
	break;
    case 1:
	newpos = filp->f_pos + off;
	break;
    case 2:
	newpos = context.count + off;
	break;
    default:
	return -EINVAL;
    }

    if (newpos < 0) {
	return -EINVAL;
    }

    filp->f_pos = newpos;
    return newpos;
}

static struct file_operations CDD_fops = {
    // for LINUX_VERSION_CODE 2.4.0 and later 
  owner:THIS_MODULE,		// struct module *owner
  open:CDD_open,		// open method 
  read:CDD_read,		// read method 
  write:CDD_write,		// write method 
  release:CDD_release,		// release method .. for close() system call
  llseek:CDD_llseek
};

static struct CDDdev
{
    const char *name;
    umode_t mode;
    const struct file_operations *fops;
} CDDdevlist[] = {
    [0] = {
"CDD2", 0666, &CDD_fops},};


static char *
CDD_devnode (struct device *dev, umode_t * mode)
{
    if (mode && CDDdevlist[MINOR (dev->devt)].mode)
	*mode = CDDdevlist[MINOR (dev->devt)].mode;
    return NULL;
}



int
device_init (void)
{
    int i = 0;

    do {
	memset (&context, 0, sizeof (CDD_CONTEXT_T));
	context.storage = (char *) vmalloc ((BUF_SZ + 1) * sizeof (char));
	if (context.storage == NULL) {
	    i = -EINVAL;
	    printk (KERN_ALERT "Cannot allocate memory for storage");
	    break;
	}

	if (CDDMajor) {
	    //  Step 1a of 2:  create/populate device numbers
	    devno = MKDEV (CDDMajor, CDDMinor);

	    //  Step 1b of 2:  request/reserve Major Number from Kernel
	    i = register_chrdev_region (devno, CDDNUMDEVS, CDD);
	    if (i < 0) {
		printk (KERN_ALERT "Error (%d) adding CDD", i);
		break;
	    }
	} else {
	    //  Step 1c of 2:  Request a Major Number Dynamically.
	    i = alloc_chrdev_region (&devno, CDDMinor, CDDNUMDEVS, CDD);
	    if (i < 0) {
		printk (KERN_ALERT "Error (%d) adding CDD", i);
		break;
	    }
	    CDDMajor = MAJOR (devno);
	    printk (KERN_ALERT "kernel assigned major number: %d to CDD\n", CDDMajor);

	}

	CDD_class = class_create (THIS_MODULE, "CDD");
	if (IS_ERR (CDD_class)) {
	    i = PTR_ERR (CDD_class);
	    break;
	}

	CDD_class->devnode = CDD_devnode;
	device_create (CDD_class, NULL, MKDEV (CDDMajor, 0), NULL, CDD);

	//  Step 2a of 2:  initialize cdev struct
	cdev_init (&cdev, &CDD_fops);

	//  Step 2b of 2:  register device with kernel
	cdev.owner = THIS_MODULE;
	cdev.ops = &CDD_fops;
	i = cdev_add (&cdev, devno, CDDNUMDEVS);
	if (i) {
	    printk (KERN_ALERT "Error (%d) adding CDD", i);
	    break;
	}
    } while (0);

    return i;
}

void
device_exit (void)
{
    //  Step 1 of 5:  unregister device with kernel
    cdev_del (&cdev);

    //  Step 2 of 5:  Destroy device file
    device_destroy (CDD_class, MKDEV (CDDMajor, 0));

    //  Step 3 of 5:  Destroy class
    class_destroy (CDD_class);

    //  Step 4 of 5:  Release request/reserve of Major Number from Kernel
    unregister_chrdev_region (devno, CDDNUMDEVS);

    //  Step 5 of 5: Release memory
    vfree (context.storage);

}
