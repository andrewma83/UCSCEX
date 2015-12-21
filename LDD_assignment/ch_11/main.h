#ifndef __MAIN_H__
#define __MAIN_H__

#define CDD		"CDD"
#define CDDNUMDEVS	4	// 2.6
#define BUF_SZ          4095
#define LOCAL_BUF_SZ    512
#define QUEUE_NAME      "CDDqueue"

typedef struct _MOVEMENT_INFO_T {
    int x;
    int y;
    int z;

    uint8_t flag[4];    /* Ox60 */
} MOVEMNET_INFO_T;

typedef struct _CDD_MOUSE_INFO_T {
    /**
     * counter % 4 on different mode of information.
     * 0: button press
     * 1: X movement
     * 2: Y movement
     * 3: Z movement
     */
    uint32_t counter; 
    struct workqueue_struct *wq;
    struct work_struct wk;
    struct semaphore *mutex_lock;

    MOVEMNET_INFO_T move_info;

} CDD_MOUSE_INFO_T;

typedef struct _CDD_CONTEXT_T {
    int count;
    char *storage;
    int num_open;
    spinlock_t sp;
    unsigned poll_mask;
    struct semaphore mutex_lock;
} CDD_CONTEXT_T;

typedef struct _CDD_STATS_T {
    size_t alloc_sz;
    size_t used_sz;
    int num_open;
} CDD_STATS_T;

enum {
    BUF_16=0,
    BUF_64,
    BUF_128,
    BUF_256
};

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

int proc_file_create (void);
void proc_file_destroy (void);
int device_init (void);
int irq_init (void);
void device_exit (void);
int ct_init (void);
void ct_exit (void);
void irq_exit (void);
void CDD_get_stats (CDD_STATS_T *stats, int index);
extern int buf_type [];
extern CDD_MOUSE_INFO_T mouse_info_ctx;

#endif/*__MAIN_H__*/
