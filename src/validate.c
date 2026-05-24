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

int validate_user_string(const char *str) {
  if (str == NULL)
    return ERROR;

  unsigned int addr = (unsigned int)str; 
  if (addr < VMEM_1_BASE || addr > VMEM_1_LIMIT) {
    TracePrintf(1, "validate_user_string: %p outside Region 1\n", str);
    return ERROR; 
  }

  pte_t *r1 = g_current_process->region_1;
  unsigned int r1_base = VMEM_1_BASE >> PAGESHIFT; 

  /* Track which page we've most recently validated */
  unsigned int current_vpn = (unsigned int)-1; 
  for (int i = 0; i < MAX_USER_STRING_LEN; i++) {
    unsigned int byte_addr = addr + i; 

    if (byte_addr >= VMEM_1_LIMIT) {
      TracePrintf(1, "validate_user_string: walked past Region 1\n");
      return ERROR; 
    }

    unsigned int vpn = byte_addr >> PAGESHIFT; 

    if (vpn != current_vpn) {
      unsigned int idx = vpn - r1_base;
      if (r1[idx].valid == 0) {
        TracePrintf(1, "validate_user_string: pid=%d vpn=%u not mapped\n", g_current_process->pid, vpn);
        return ERROR;
      }

      if (!(r1[idx].prot & PROT_READ)) {
        TracePrintf(1, "validate_user_string: pid=%d vpn=%u not readable (prot=0x%x)\n", g_current_process->pid, vpn, r1[idx].prot);
        return ERROR;
      }

      current_vpn = vpn; 
    }

    /* Safe to dereference: the page is mapped and readable. */
    if (((const char *)byte_addr)[0] == '\0') {
      return SUCCESS;
    }
  }

  /* We are past MAX_STRING_LEN */
  TracePrintf(1, "validate_user_string: no '\\0' within %d bytes\n", MAX_USER_STRING_LEN);
  return ERROR;
}