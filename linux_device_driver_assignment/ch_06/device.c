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
#include <asm/uaccess.h>
#include "main.h"

static unsigned int CDDMajor = 0;
static unsigned int CDDMinor = 0;
static struct class *CDD_class;

struct cdev cdev;
dev_t devno;

static CDD_CONTEXT_T context[CDDNUMDEVS];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	module_param(CDDMajor,int,0);
	module_param(CDDMinor,int,0);
#else
	MODULE_PARM(CDDMajor,"i");
	MODULE_PARM(CDDMinor,"i");
#endif

void
CDD_get_stats (CDD_STATS_T *stats, int index)
{
    stats->alloc_sz = buf_type[index];
    stats->used_sz = context[index].count;
    spin_lock(&context[index].sp);
    stats->num_open = context[index].num_open;
    spin_unlock(&context[index].sp);

}

static int
CDD_open (struct inode *inode, struct file *file)
{
    int index= iminor(file->f_path.dentry->d_inode);
    // MOD_INC_USE_COUNT;
    if (file->f_flags & O_TRUNC) {
	context[index].count = 0;
	context[index].storage[0] = '\0';
    }

    if (file->f_flags & O_APPEND) {
	file->f_pos = context[index].count;
    }

    file->private_data = (void *) &context[index];
    spin_lock(&context[index].sp);
    context[index].num_open++;
    spin_unlock(&context[index].sp);
    return 0;
}

static int
CDD_release (struct inode *inode, struct file *file)
{
    // MOD_DEC_USE_COUNT;
    
    file->private_data = NULL;
#if 0
    spin_lock(&context.sp);
    context.num_open--;
    spin_unlock(&context.sp);
#endif
    return 0;
}

static ssize_t
CDD_read (struct file *file, char *buf, size_t count, loff_t * ppos)
{
    int len = 0, err = 0;
    CDD_CONTEXT_T *ctx = (CDD_CONTEXT_T *) file->private_data;
    char *CDD_storage = ctx->storage;
    int index = iminor(file->f_path.dentry->d_inode);

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

        printk(KERN_ALERT "CDD_read ready for down\n");
        down(&context[index].mutex_lock);
        printk(KERN_ALERT "CDD_read down\n");
	err = copy_to_user (buf, &CDD_storage[*ppos], len);
        printk(KERN_ALERT "CDD_read ready for up\n");
        up(&context[index].mutex_lock);
        printk(KERN_ALERT "CDD_read up\n");

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
    int index = iminor(file->f_path.dentry->d_inode);

    do {
        /* Won't write if it is read only */
	if (file->f_flags == O_RDONLY) {
	    count = -EFAULT;
	    break;
	}

        /* If Projected length is greater than the total buffer size */
	if ((*ppos + count) > buf_type[index]) {
	    count = BUF_SZ - *ppos;
	    if (count <= 0) {
		count = -EFAULT;
		break;
	    }
	}

        printk(KERN_ALERT "CDD_write ready for down\n");
        down(&context[index].mutex_lock);
        printk(KERN_ALERT "CDD_write down\n");
	err = copy_from_user (&CDD_storage[*ppos], buf, count);
        printk(KERN_ALERT "CDD_write ready for up\n");
        up(&context[index].mutex_lock);
        printk(KERN_ALERT "CDD_write up\n");
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
CDD_llseek (struct file *file, loff_t off, int whence)
{
    loff_t newpos;
    int index = iminor(file->f_path.dentry->d_inode);

    switch (whence) {
    case 0:
	newpos = off;
	break;
    case 1:
	newpos = file->f_pos + off;
	break;
    case 2:
	newpos = context[index].count + off;
	break;
    default:
	return -EINVAL;
    }

    if (newpos < 0) {
	return -EINVAL;
    }

    file->f_pos = newpos;
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

static struct CDDdev {
    const char *name;
    umode_t mode;
    const struct file_operations *fops;
} CDDdevlist[] = {
    [0] = {
            "CDD",
            0666,
            &CDD_fops
    },
    [1] = {
            "CDD",
            0666,
            &CDD_fops
    },
    [2] = {
            "CDD",
            0666,
            &CDD_fops
    },
    [3] = {
            "CDD",
            0666,
            &CDD_fops
    },
};

int buf_type [] = {
    16,
    64,
    128,
    256
};

static char *
CDD_devnode (struct device *dev, umode_t * mode)
{
    if (mode && CDDdevlist[MINOR (dev->devt)].mode) {
	*mode = CDDdevlist[MINOR (dev->devt)].mode;
    }
    return NULL;
}



int
device_init (void)
{
    int i = 0;
    int ii = 0;
    char device_name[LOCAL_BUF_SZ + 1];
    int success = 0;

    do {
	memset (&context, 0, sizeof (CDD_CONTEXT_T));
        for (ii = 0, success = 1; ii < CDDNUMDEVS; ii++) {
	    context[ii].storage = (char *) vmalloc (buf_type[ii] * sizeof (char));
            if (context[ii].storage == NULL) {
                success = 0;
                break;
            }

            sema_init(&context[ii].mutex_lock, 1);
            spin_lock_init(&context[ii].sp);
        }

        if (!success) {
            if (context[ii].storage == NULL) {
                i = -EINVAL;
                printk (KERN_ALERT "Cannot allocate memory for storage");
                break;
            }           
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
	    i = alloc_chrdev_region(&devno, CDDMinor, CDDNUMDEVS, CDD);
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
        for (ii = 0; ii < CDDNUMDEVS; ii++) {
            snprintf(device_name, LOCAL_BUF_SZ, "CDD/CDD%d", buf_type[ii]);
            device_create (CDD_class, NULL,
                           MKDEV (CDDMajor, CDDMinor + ii),
                           NULL, device_name);

        }

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
    int ii = 0;
    //  Step 1 of 5:  unregister device with kernel
    cdev_del (&cdev);

    //  Step 2 of 5:  Destroy device file
    for (ii = 0; ii < CDDNUMDEVS; ii++) {
        device_destroy (CDD_class, MKDEV (CDDMajor, CDDMinor + ii));
    }

    //  Step 3 of 5:  Destroy class
    class_destroy (CDD_class);

    //  Step 4 of 5:  Release request/reserve of Major Number from Kernel
    unregister_chrdev_region (devno, CDDNUMDEVS);

    //  Step 5 of 5: Release memory
    for (ii = 0; ii < CDDNUMDEVS; ii++) {
        vfree (context[ii].storage);
    }
}
