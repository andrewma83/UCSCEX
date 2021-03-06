#ifndef __MAIN_H__
#define __MAIN_H__

#define CDD		"CDDAMA"
#define CDDNUMDEVS	6	// 2.6
#define BUF_SZ          4095

typedef struct _CDD_CONTEXT_T {
    int count;
    char *storage;
} CDD_CONTEXT_T;

typedef struct _CDD_STATS_T {
    size_t alloc_sz;
    size_t used_sz;

} CDD_STATS_T;

int device_init (void);
void device_exit (void);
void CDD_get_stats (CDD_STATS_T *stats);

#endif/*__MAIN_H__*/
