#ifndef _yalnix_kernel_h
#define _yalnix_kernel_h

#include <hardware.h>

#define KSTACK_NPG (KERNEL_STACK_MAXSIZE / PAGESIZE)
#define MAX_ARGS 64
#define MAX_USER_STRING_LEN 256

extern pte_t *g_region_0_pt;
extern int g_kernel_brk_page;
extern int g_vm_enabled;

#endif

