#ifndef __MAIN_H__
#define __MAIN_H__

#define CDD		"CDDAMA"
#define CDDNUMDEVS	6	// 2.6
#define BUF_SZ          4095
#define LOCAL_BUF_SZ    1041

typedef struct _CDD_CONTEXT_T {
    int count;
    char *storage;
    int num_open;
    spinlock_t sp;
    struct semaphore mutex_lock;
} CDD_CONTEXT_T;

typedef struct _CDD_STATS_T {
    size_t alloc_sz;
    size_t used_sz;
    int num_open;
} CDD_STATS_T;

typedef struct _CDD_TIME_STATS_T {
    unsigned long int start_jiffies;
    unsigned long long int start_timetick;
    unsigned long int end_jiffies;
    unsigned long long int end_timetick;
    unsigned int cpu;
} CDD_TIME_STATS_T;

int proc_file_create (void);
void proc_file_destroy (void);
int device_init (void);
void device_exit (void);
int ct_init (void);
void ct_exit (void);
void CDD_get_stats (CDD_STATS_T *stats);

#endif/*__MAIN_H__*/
