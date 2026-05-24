#include <ykernel.h>
#include "validate.h"
#include "scheduler.h"

int validate_user_buffer(const void *buf, int len, int prot) {
  if (len < 0)
    return ERROR; 

  if (len == 0)
    return SUCCESS; 

  if (buf == NULL)
    return ERROR; 
  
  unsigned int start = (unsigned int) buf;
  unsigned int end = start + (unsigned int)len; 

  if (end < start) {
    return ERROR;
  }

  /* Buffer must live within Region 1 */
  if (start < VMEM_1_BASE || end > VMEM_1_LIMIT) {
    TracePrintf(1, "validate_user_buffer: [%p, %p) outside Region 1\n", (void *)start, (void *)end);
    return ERROR; 
  }

  /* Walk every page for the buffer */
  unsigned int first_vpn = start >> PAGESHIFT;
  unsigned int last_vpn = (end - 1) >> PAGESHIFT;
  unsigned int r1_base = VMEM_1_BASE >> PAGESHIFT;

  pte_t *r1 = g_current_process->region_1;

  for (unsigned int vpn = first_vpn; vpn <= last_vpn; vpn++) {
    unsigned int index = vpn - r1_base; 

    if (r1[index].valid == 0){
      TracePrintf(1, "validate_user_buffer: pid=%d vpn=%u not mapped\n", g_current_process->pid, vpn); 
      return ERROR; 
    }

    if ((r1[index].prot & prot) != (unsigned int) prot) {
      TracePrintf(1, "validate_user_buffer: pid=%d vpn=%u page_prot=0x%x required=0x%x\n",
                  g_current_process->pid, vpn, r1[index].prot, prot);

      return ERROR; 
    }
  }

  return SUCCESS; 
}