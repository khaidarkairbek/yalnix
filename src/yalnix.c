#include <hardware.h>
#include <ykernel.h>
#include <ylib.h>
#include "kernel.h"
#include "frame.h"
#include "trap_handler.h"
#include "scheduler.h"


pte_t *g_region_0_pt = NULL;
int g_vm_enabled = 0;
int g_kernel_brk_page = 0;

static void setup_region_0(void);
static pcb_t *setup_idle(UserContext *uctxt);
static void idle_function(void);

void KernelStart(char* cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
  TracePrintf(0, "KernelStart: pmem_size=0x%x\n", pmem_size);

  /* Initialize frame allocator */
  frame_init(pmem_size);

  /* Initialize trap vector table */
  trap_init();

  /* Build Region 0 Page Table */
  setup_region_0();

  /* Set up the current and idle process from uctxt*/
  g_idle_process = setup_idle(uctxt);
  g_current_process = g_idle_process;

  /* Set up the Region 0 and 1 Base and Limit Registers*/
  WriteRegister(REG_PTBR0, (unsigned int) g_region_0_pt); 
  WriteRegister(REG_PTLR0, MAX_PT_LEN); 
  WriteRegister(REG_PTBR1, (unsigned int) g_idle_process->region_1);
  WriteRegister(REG_PTLR1, MAX_PT_LEN);
  
  /* Enable Virtual Memory */
  WriteRegister(REG_VM_ENABLE, 1);
  g_vm_enabled = 1; 
  TracePrintf(0, "KernelStart: VM enabled\n");

  *uctxt = g_idle_process->uctx; 

  TracePrintf(0, "KernelStart: returning into user space (pc=%p sp=%p)\n", uctxt->pc, uctxt->sp);
}

static void setup_region_0(void) {
  /* This might not be true if SetKernelBrk has been called before now. */
  if (g_kernel_brk_page < _orig_kernel_brk_page) {
    g_kernel_brk_page = _orig_kernel_brk_page; 
  }

  g_region_0_pt = (pte_t *) malloc(MAX_PT_LEN * sizeof(pte_t)); 
  if (g_region_0_pt == NULL) {
    TracePrintf(0, "setup_region_0: malloc() failed\n");
    Halt(); 
  }

  /* Initialize all PTEs to invalid */
  for (int i = 0; i < MAX_PT_LEN; i++) {
    g_region_0_pt[i].valid = 0; 
    g_region_0_pt[i].prot = 0;
    g_region_0_pt[i].pfn = 0;
  }

  /* Initialize kernel text: Read and Exec */
  for (int i = _first_kernel_text_page; i < _first_kernel_data_page; i++) {
    g_region_0_pt[i].valid = 1;
    g_region_0_pt[i].prot = PROT_READ | PROT_EXEC;
    g_region_0_pt[i].pfn = i;
    frame_reserve(i); 
  }

  /* Initialize kernel data: Read and Write */
  for (int i = _first_kernel_data_page; i < g_kernel_brk_page; i++) {
    g_region_0_pt[i].valid = 1;
    g_region_0_pt[i].prot = PROT_READ | PROT_WRITE;
    g_region_0_pt[i].pfn = i;
    frame_reserve(i); 
  }

  /* Initialize kernel stack: Read and Write*/
  for (int i = KERNEL_STACK_BASE >> PAGESHIFT; i < KERNEL_STACK_LIMIT >> PAGESHIFT; i++) {
    g_region_0_pt[i].valid = 1;
    g_region_0_pt[i].prot = PROT_READ | PROT_WRITE;
    g_region_0_pt[i].pfn = i;
    frame_reserve(i);
  }

  TracePrintf(1, "setup_region_0: kernel text [%d..%d), data/heap [%d..%d), "
    "kstack [%d..%d)\n",
    _first_kernel_text_page, _first_kernel_data_page,
    _first_kernel_data_page, g_kernel_brk_page,
    KERNEL_STACK_BASE >> PAGESHIFT, KERNEL_STACK_LIMIT >> PAGESHIFT);
}

static pcb_t *setup_idle(UserContext *uctxt) {
  /**
   * Creates Idle Process, which runs idle_function. 
   */

  pcb_t *p = pcb_create(); 
  if (p == NULL) {
    TracePrintf(0, "setup_idle: pcb_create failed\n");
    Halt();
  }

  /* Set up Idle's stack */
  int stack_frame = frame_alloc(); 
  if (stack_frame == -1) {
    TracePrintf(0, "setup_idle: frame_alloc failed for idle stack\n");
    Halt();
  }

  int top_stack_vpn = MAX_PT_LEN - 1; 
  p->region_1[top_stack_vpn].valid = 1;
  p->region_1[top_stack_vpn].prot = PROT_READ | PROT_WRITE;
  p->region_1[top_stack_vpn].pfn = stack_frame;

  /* Set up Idle's UserContext */
  p->uctx = *uctxt;
  p->uctx.pc = (void *)idle_function;
  p->uctx.sp = (void *)(VMEM_1_LIMIT - sizeof(int));

  p->ustack_low = VMEM_1_LIMIT - PAGESIZE;
  p->ubrk = VMEM_1_BASE;   /* no heap */
  p->udata_end = VMEM_1_BASE;

  TracePrintf(1, "setup_idle: pid=%d pc=%p sp=%p stack_frame=%d\n", 
    p->pid, p->uctx.pc, p->uctx.sp, stack_frame);

  return p; 
}

static void idle_function(void) {
  while (1) {
    TracePrintf(1, "Idle\n");
    Pause();
  }
}

int SetKernelBrk(void *addr) {
  unsigned int new_brk = UP_TO_PAGE((unsigned int)addr);
  unsigned int new_brk_page = new_brk >> PAGESHIFT; 

  TracePrintf(2, "SetKernelBrk: addr=%d, new_brk=%d, new_brk_page=%d (was %d, vm_enabled=%d)\n", 
    addr, new_brk, new_brk_page, g_kernel_brk_page, g_vm_enabled); 

  /* Before VM enabled, page table was not built yet, so just update global. */
  if (g_vm_enabled == 0) {
    if (new_brk_page > g_kernel_brk_page) {
      g_kernel_brk_page = new_brk_page; 
    }

    return 0; 
  }

  if (new_brk >= KERNEL_STACK_BASE){
    TracePrintf(0, "SetKernelBrk: collision with kernel stack\n");
    return -1; 
  }
  /* Update region 0 page tables baed on grow or shrink */
  if (new_brk_page > g_kernel_brk_page) {
    /* Grow */
    for (int vpn = g_kernel_brk_page; vpn < new_brk_page; vpn++) {
      int frame = frame_alloc(); 
      if (frame == -1) {
        for (int v = g_kernel_brk_page; v < vpn; v++) {
          frame_free(g_region_0_pt[v].pfn); 
          g_region_0_pt[v].valid = 0;
        }

        return -1; 
      }

      g_region_0_pt[vpn].valid = 1;
      g_region_0_pt[vpn].prot = PROT_READ | PROT_WRITE;
      g_region_0_pt[vpn].pfn = frame; 
    }
  } else if (new_brk_page < g_kernel_brk_page) {
    /* Shrink */
    for (int vpn = new_brk_page; vpn < g_kernel_brk_page; vpn++) {
      if (g_region_0_pt[vpn].valid) {
        frame_free(g_region_0_pt[vpn].pfn); 
        g_region_0_pt[vpn].valid = 0; 
      }
    }

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0); 
  }

  g_kernel_brk_page = new_brk_page; 
  return 0;
}

