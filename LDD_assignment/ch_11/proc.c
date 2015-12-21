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
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "main.h"

typedef struct _display_buf_t {
    char *buf;
    size_t size;
} DISPLAY_BUF_T;

DISPLAY_BUF_T display_buffer;

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_stats[CDDNUMDEVS];

static void
prepare_display_buffer (int type) 
{
    int len = 0;
    ssize_t unused_buf_sz = 0;
    CDD_STATS_T stats;
    u16 value = 128;
    u16 change_val;
    char test_char;

    CDD_get_stats(&stats, type);
    unused_buf_sz = stats.alloc_sz - stats.used_sz;
    memset(display_buffer.buf, 0, BUF_SZ);

    len += snprintf (display_buffer.buf + len, BUF_SZ,
                     "Group Number                             : 0\n");

    len += snprintf (display_buffer.buf + len, BUF_SZ,
                     "Team Member                              : '%s'\n", "Andrew Ma");

    len += snprintf (display_buffer.buf + len, BUF_SZ,
                     "Buffer Length - Allocated                : %lu\n", stats.alloc_sz);

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Buffer Length - Used                     : %lu\n", stats.used_sz);

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Buffer Length - Unused                   : %lu\n", unused_buf_sz);

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Number of open files                     : %d\n", stats.num_open);

    change_val = cpu_to_le16(value);   
    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "CPU Edianness                            : %s endian\n",
                     (value == change_val) ? "Little" : "Big");

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Size of u8 u16 u32 u64                   : %lu %lu %lu %lu (in bytes)\n",
                     sizeof (u8), sizeof (u16), sizeof (u32), sizeof (u64));

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Size of s8 s16 s32 s64                   : %lu %lu %lu %lu (in bytes)\n",
                     sizeof (s8), sizeof (s16), sizeof (s32), sizeof (s64));

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Size of int8_t int16_t int32_t int64_t   : %lu %lu %lu %lu (in bytes)\n",
                     sizeof (int8_t), sizeof (int16_t), sizeof (int32_t), sizeof (int64_t));


    test_char = -1;
    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Char is                                  : %s\n",
                     (test_char < 0) ? "Signed" : "Unsigned)");
}

static ssize_t
proc_file_read_core (struct file *file, char __user *buf, 
                     size_t len, loff_t *ppos, int type)
{
    int copy_len = 0;
    int act_copy_len = 0;

    if (*ppos == 0) {
        prepare_display_buffer(type);
    }

    copy_len = strlen(display_buffer.buf + *ppos);
    if (copy_len) {
        if (len >= copy_len) {
            act_copy_len = copy_len;
        } else if (len < copy_len){
            act_copy_len = len;
        }

        copy_to_user(buf, (display_buffer.buf + *ppos), act_copy_len);
        //memcpy(buf, (display_buffer.buf + *ppos), act_copy_len);
        *ppos += act_copy_len;
    }

    return act_copy_len;
}

static ssize_t
proc_file_read_16 (struct file *file, char __user *buf, 
                   size_t len, loff_t *ppos)
{
    return proc_file_read_core(file, buf, len, ppos, BUF_16);
}

static ssize_t
proc_file_read_64 (struct file *file, char __user *buf, 
                   size_t len, loff_t *ppos)
{
    return proc_file_read_core(file, buf, len, ppos, BUF_64);
}

static ssize_t
proc_file_read_128 (struct file *file, char __user *buf, 
                    size_t len, loff_t *ppos)
{
    return proc_file_read_core(file, buf, len, ppos, BUF_128);
}

static ssize_t
proc_file_read_256 (struct file *file, char __user *buf, 
                    size_t len, loff_t *ppos)
{
    return proc_file_read_core(file, buf, len, ppos, BUF_256);
}

static ssize_t
proc_file_write (struct file *file, const char __user *buf,
                 size_t count, loff_t *ppos)
{
    ssize_t retval;
    /* This function is to chew up all the input data */

    //printk(KERN_ALERT "Receive count '%lu'", count);
    retval = count;
    return retval;
}

static const struct file_operations proc_fops[] = {
    {
        .owner = THIS_MODULE,
        .read  = proc_file_read_16,
        .write = proc_file_write,
    },
    {
        .owner = THIS_MODULE,
        .read  = proc_file_read_64,
        .write = proc_file_write,
    },
    {
        .owner = THIS_MODULE,
        .read  = proc_file_read_128,
        .write = proc_file_write,
    },
    {
        .owner = THIS_MODULE,
        .read  = proc_file_read_256,
        .write = proc_file_write,
    }
};

int
proc_file_create (void)
{
    int ii = 0;
    char device_name[LOCAL_BUF_SZ + 1];

    do {
        proc_dir = proc_mkdir("CDD", 0);
        for (ii = 0; ii < CDDNUMDEVS; ii++) {
            snprintf(device_name, LOCAL_BUF_SZ, "CDD%d", buf_type[ii]);
            proc_stats[ii] = proc_create(device_name, 0777, proc_dir, &proc_fops[ii]);
        }

        display_buffer.buf = vmalloc ((BUF_SZ + 1) * sizeof (char));
        if (display_buffer.buf == NULL) {
            printk(KERN_ALERT "Cannot allocate memory !!!");
            break;
        } else {
            display_buffer.size = BUF_SZ;
        }

    } while (0);
    return 0;
}

void
proc_file_destroy (void)
{
    int ii = 0;
    char device_name[LOCAL_BUF_SZ + 1];

    for (ii = 0; ii < CDDNUMDEVS; ii++) {
        snprintf(device_name, LOCAL_BUF_SZ, "CDD%d", buf_type[ii]);
        if (proc_stats[ii]) {
            remove_proc_entry(device_name, proc_dir);
        }
    }

    if (proc_dir) {
        remove_proc_entry("CDD", 0);
    }

    vfree(display_buffer.buf);
}


