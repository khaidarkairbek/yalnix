#ifndef _yalnix_kernel_h
#define _yalnix_kernel_h

#include <hardware.h>
#include "pcb.h"

#define KSTACK_NPG (KERNEL_STACK_LIMIT / PAGESIZE)

extern pte_t *g_region_0_pt;
extern int g_kernel_brk_page;
extern int g_vm_enabled;

#endif

