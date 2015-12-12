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
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/cdev.h>		// 2.6
#include <linux/vmalloc.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include "main.h"

struct state_table_t {
    long state;
    const char *readable_str;
};

static unsigned long track_pid = 0;

/**
 * Decode state
 */
static void
decode_state (long state, char *buffer)
{
    int ii = 0;
    int jj = 0;
    char *p = buffer;
    long mask = 1;

    if (state == TASK_RUNNING) {
        p[jj++] = TASK_STATE_TO_CHAR_STR[0];
    } else {
        for (ii = 1; ii < 11; mask *= 2, ii++) {
            if (state & mask) {
                p[jj++] = TASK_STATE_TO_CHAR_STR[ii];
            }
        }
    }

    p[jj] = '\0';
}

/**
 * Sequence iterator functions.
 */
static void *
ct_seq_start (struct seq_file *s, loff_t *pos)
{
    loff_t *spos = NULL;
    struct task_struct *task = &init_task;
    char *buffer;
    do {
        if (s->index == 0) {
            s->private = (char *) vmalloc((BUF_SZ + 1) * sizeof (char));
            if (s->private == NULL) {
                break;
            } else {
                buffer = (char *) s->private;
                buffer[0] = '\0';
            }
            spos = (void *) task;
            *pos = 0;
        } else {
            spos = NULL;
        }
    } while(0);

    return spos;
}

static void *
ct_seq_next (struct seq_file *s, void *v, loff_t *pos)
{
    loff_t *spos = NULL;
    struct task_struct *next_task = NULL;
    struct task_struct *task = (struct task_struct *) v;
    char *buffer = (char *)s->private;

    (*pos)++;
    next_task = next_task(task);
    if (next_task == &init_task) {
        spos = NULL;
    } else {
        spos = (void *)next_task;
        buffer = (char *) s->private;
        buffer[0] = '\0';
    }

    return spos;
}

static void 
ct_seq_stop (struct seq_file *s, void *v)
{
    vfree(s->private);
    s->private = NULL;
}

/**
 * The show function
 */
static int
ct_seq_show (struct seq_file *s, void *v)
{
    struct task_struct *task = (struct task_struct *) v;
    if (task->pid == track_pid) {
        decode_state(task->state, s->private);
        seq_printf(s, "PID (%6d) State: ", task->pid);
        seq_putc(s, '(');
        seq_puts(s, (char *) s->private);
        seq_putc(s, ')');
        seq_printf(s, ",Prio: (%d)", task->prio);
        seq_printf(s, ",RT-Prio: (%d)", task->rt_priority);
        seq_printf(s, "\n");
    }
    return 0;
}

static struct seq_operations ct_seq_ops = {
    .start      = ct_seq_start,
    .next       = ct_seq_next,
    .stop       = ct_seq_stop,
    .show       = ct_seq_show
};

static int
ct_open (struct inode *inode, struct file *file)
{
    return seq_open(file, &ct_seq_ops);
}

static ssize_t
ct_write (struct file *file, const char __user *buf,
          size_t count, loff_t *ppos)
{
    ssize_t retval;
    long pid = 0;
    size_t length = (count > LOCAL_BUF_SZ) ? LOCAL_BUF_SZ : count;
    char *lbuf = (char *) vmalloc (sizeof (char) * (LOCAL_BUF_SZ + 1));
    memset(lbuf, 0, (LOCAL_BUF_SZ + 1));
    /* This function is to chew up all the input data */

    copy_from_user(lbuf, buf, length);

    if (kstrtol(lbuf, 10, &pid) == 0) {
        printk(KERN_ERR "Get pid: '%ld'", pid);
        track_pid = (unsigned long) pid;
    } else {
        printk(KERN_ERR "Invalid PID with string '%s'\n", lbuf);
    }
    vfree(lbuf);
    lbuf = NULL;
    retval = count;
    return retval;
}

static struct file_operations ct_file_ops = {
    .owner      = THIS_MODULE,
    .open       = ct_open,
    .write      = ct_write,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release
};

/**
 * Module setup and teardown
 */

int
ct_init (void)
{
    struct proc_dir_entry *entry;

    entry = proc_create("myps", 0777, NULL, &ct_file_ops);
    track_pid = 0;

    return 0;
}

void
ct_exit (void)
{
    remove_proc_entry("myps", NULL);
}


