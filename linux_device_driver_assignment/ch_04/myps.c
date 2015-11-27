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

struct flag_table_t {
    unsigned long flags;
    const char *readable_str;
    const char *flag_str;
};

struct flag_table_t flag_table [] = {
    { PF_EXITING,        "PF_EXITING"    , "X" },
    { PF_EXITPIDONE,	 "PF_EXITPIDONE" , "XD"},
    { PF_VCPU,		 "PFVCPU"        , "V"},
    { PF_WQ_WORKER,      "PF_WQ_WORKER"  , "WQ"},
    { PF_FORKNOEXEC,	 "PF_FORKNOEXEC" , "FNO"},
    { PF_MCE_PROCESS,    "PF_MCE_PROCESS", "MP"},
    { PF_SUPERPRIV,	 "PF_SUPERPRIV"  , "SP"},
    { PF_DUMPCORE,	 "PF_DUMPCORE"   , "DC"  },
    { PF_SIGNALED,       "PF_SIGNALED"   , "SIG"},
    { PF_MEMALLOC,       "PF_MEMALLOC"   , "MEM"},
    { PF_NPROC_EXCEEDED, "PF_NPROC_EXCEEDED", "NPX"},
    { PF_USED_MATH,      "PF_USED_MATH"  , "MATH"},
    { PF_USED_ASYNC,     "PF_USED_ASYNC" , "ASYNC"},
    { PF_NOFREEZE,       "PF_NOFREEZE"   , "NFREZ"},
    { PF_FROZEN,         "PF_FROZEN"     , "FROZ"},
    { PF_FSTRANS,        "PF_FSTRANS"    , "FSX"},
    { PF_KSWAPD,         "PF_KSWAPD"     , "KSWP"},
    { PF_MEMALLOC_NOIO,  "PF_MEMALLOC_NOIO", "MEMNO"},
    { PF_LESS_THROTTLE,  "PF_LESS_THROTTLE", "LTHRO"},
    { PF_KTHREAD,        "PF_KTHREAD"    , "KTRED"},
    { PF_RANDOMIZE,      "PF_RANDOMIZE"  , "RND" },
    { PF_SWAPWRITE,      "PF_SWAPWRITE"  , "SWAPW"},
    { PF_NO_SETAFFINITY, "PF_NO_SETAFFINITY", "NSAF"},
    { PF_MCE_EARLY,      "PF_MCE_EARLY"  , "MCEAR"},
    { PF_MUTEX_TESTER,   "PF_MUTEX_TESTER", "MUTST"},
    { PF_FREEZER_SKIP,   "PF_FREEZER_SKIP", "FRESP"},
    { PF_SUSPEND_TASK,   "PF_SUSPEND_TASK", "SUSD" },
    { 0,                 NULL             }
};

/**
 * Decode flag
 */
static void
decode_flag (unsigned int flags, char *buffer)
{
    int ii = 0;
    struct flag_table_t *flag_item = NULL;
    char *p = buffer;

    for (flag_item = flag_table, ii = 0;
         flag_item->flags;
         flag_item = &flag_table[++ii]) {
        if (flags & flag_item->flags) {
            p = buffer +  strlen(buffer);
            snprintf(p, BUF_SZ - strlen(buffer), "%s,",flag_item->flag_str);
        }
    }

    buffer[strlen(buffer) - 1] = '\0';
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
                decode_flag(task->flags, buffer);
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
        decode_flag(task->flags, buffer);
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
    seq_printf(s, "PID:%5d:", task->pid);
    seq_puts(s, (char *) s->private);
    seq_printf(s, "\n");
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

static struct file_operations ct_file_ops = {
    .owner      = THIS_MODULE,
    .open       = ct_open,
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

    return 0;
}

void
ct_exit (void)
{
    remove_proc_entry("myps", NULL);
}


