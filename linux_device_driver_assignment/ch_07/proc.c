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
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include "main.h"

typedef struct _display_buf_t {
    char *buf;
    size_t size;
} DISPLAY_BUF_T;

DISPLAY_BUF_T display_buffer;

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_stats;

#define DELAYTIMEINSECS         1
static void
CDD_get_time_stats (CDD_TIME_STATS_T *time_stats)
{
    int ii;
    unsigned long long int startll, endll;
    unsigned long long int elasped_time_cycle;

    time_stats->start_jiffies = jiffies;
    time_stats->end_jiffies = jiffies + DELAYTIMEINSECS * HZ;
    rdtscll(startll);
    while (time_before(jiffies, time_stats->end_jiffies)) {}
    rdtscll(endll);

    elasped_time_cycle = endll - startll;
    time_stats->cpu = elasped_time_cycle;
    /* Convert it into Ghz */
    for (ii = 0; ii < 9; ii++ ) {
        time_stats->cpu /= 10;
    }
}

static void
prepare_display_buffer (void) 
{
    int len = 0;
    ssize_t unused_buf_sz = 0;
    CDD_STATS_T stats;
    CDD_TIME_STATS_T time_stats;

    memset(&time_stats, 0, sizeof (CDD_TIME_STATS_T));
    CDD_get_stats(&stats);
    CDD_get_time_stats(&time_stats);
    unused_buf_sz = stats.alloc_sz - stats.used_sz;
    memset(display_buffer.buf, 0, BUF_SZ);

    len += snprintf (display_buffer.buf + len, BUF_SZ,
                     "Group Number             : 0\n");

    len += snprintf (display_buffer.buf + len, BUF_SZ,
                     "Team Member              : '%s'\n", "Andrew Ma");

    len += snprintf (display_buffer.buf + len, BUF_SZ,
                     "Buffer Length - Allocated: %lu\n", stats.alloc_sz);

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Buffer Length - Used     : %lu\n", stats.used_sz);

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Buffer Length - Unused   : %lu\n", unused_buf_sz);

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "Number of open files     : %d\n", stats.num_open);

    len += snprintf (display_buffer.buf + len, BUF_SZ - len,
                     "CPU Speed as             : %uGhz\n", time_stats.cpu);
}

static ssize_t
proc_file_read (struct file *file, char __user *buf, 
                size_t len, loff_t *ppos)
{
    int copy_len = 0;
    int act_copy_len = 0;

    if (*ppos == 0) {
        prepare_display_buffer();
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
proc_file_write (struct file *file, const char __user *buf,
                 size_t count, loff_t *ppos)
{
    ssize_t retval;
    /* This function is to chew up all the input data */

    //printk(KERN_ALERT "Receive count '%lu'", count);
    retval = count;
    return retval;
}

static const struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .read  = proc_file_read,
    .write = proc_file_write,
};

int
proc_file_create (void)
{
    do {
        
        proc_dir = proc_mkdir("CDD", 0);
        proc_stats = proc_create("myCDD", 0777, proc_dir, &proc_fops);

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
    if (proc_stats) {
        remove_proc_entry("myCDD", proc_dir);
    }

    if (proc_dir) {
        remove_proc_entry("CDD", 0);
    }

    vfree(display_buffer.buf);
}


