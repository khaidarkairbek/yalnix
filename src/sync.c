#include "sync.h"
#include "id_helper.h"
#include <ykernel.h>
#include "scheduler.h"

static lock_t *g_locks = NULL;

static lock_t *lock_find(int id) {
  lock_t *lock = g_locks; 
  while (lock != NULL && lock->id != id) {
    lock = lock->next; 
  }

  return lock; 
}

static void waiter_enqueue(pcb_t **head, pcb_t *p) {
  p->next = NULL;
  if (*head == NULL) { 
    *head = p; 
    return; 
  }
  pcb_t *tail = *head;
  while (tail->next != NULL) {
    tail = tail->next;
  }
  tail->next = p;
}

static pcb_t *waiter_dequeue(pcb_t **head) {
  pcb_t *p = *head; 
  if (p != NULL) { 
    *head = p->next; 
    p->next = NULL; 
  }

  return p; 
}

static int waiter_remove(pcb_t **head, pcb_t *p) {
  if (*head == p) {
    *head = p->next;
    p->next = NULL;
    return 1;
  }
  pcb_t *prev = *head;
  while (prev != NULL && prev->next != p) {
    prev = prev->next;
  }
  if (prev != NULL) {
    prev->next = p->next;
    p->next = NULL;
    return 1;
  }
  return 0;
}

int lock_init(void) {
  lock_t *lock = malloc(sizeof(lock_t)); 
  if (lock == NULL) return ERROR;

  lock->owner = NULL;
  lock->id = helper_new_id(); 
  lock->next = NULL;
  lock->waiters = NULL;

  lock->next = g_locks;
  g_locks = lock;

  return lock->id; 
}
int lock_acquire(int lock_id) {
  lock_t *lock = lock_find(lock_id); 
  if (lock == NULL)
    return ERROR; 

  /* Already own the lock, reject */
  if (lock->owner == g_current_process) {
    TracePrintf(0, "kernel_Acquire: pid=%d already owns lock %d\n", g_current_process->pid, lock_id);
    return ERROR;
  }

  /* Free, take it */
  if (lock->owner == NULL) {
    lock->owner = g_current_process;
    TracePrintf(3, "kernel_Acquire: pid=%d took a lock %d (free)\n", g_current_process->pid, lock_id);
    return SUCCESS;
  }

  /* Held by someone else */
  TracePrintf(3, "kernel_Acquire: pid=%d blocking on lock %d (owner=%d)\n", g_current_process->pid, lock_id, lock->owner->pid);

  g_current_process->state = BLOCKED;
  g_current_process->waiting_on = WAIT_LOCK;
  g_current_process->wait_arg = lock_id;
  waiter_enqueue(&lock->waiters, g_current_process); 
  schedule();

  lock = lock_find(lock_id); 
  if (lock == NULL || lock->owner != g_current_process) {
    return ERROR; 
  } 

  TracePrintf(3, "kernel_Acquire: pid=%d woke as owner of lock %d\n", g_current_process->pid, lock_id);

  return SUCCESS; 
}
int lock_release(int lock_id) {
  lock_t *lock = lock_find(lock_id); 
  if (lock == NULL)
    return ERROR; 

  /* only the owner can release the lock */
  if (lock->owner != g_current_process) {
    TracePrintf(0, "kernel_Release: pid=%d does not own lock %d (owner=%d)\n", g_current_process->pid, lock_id, lock->owner ? lock->owner->pid : -1);
    return ERROR;
  }

  /* Handoff to first waiter */
  pcb_t *waiter; 
  if ((waiter = waiter_dequeue(&lock->waiters)) != NULL) {
    lock->owner = waiter;
    waiter->state = RUNNABLE;
    waiter->waiting_on = WAIT_NONE;

    ready_enqueue(waiter); 
    TracePrintf(3, "kernel_Release: pid=%d handed lock %d to pid=%d\n", g_current_process->pid, lock_id, waiter->pid);

  } else {
    lock->owner = NULL;
    TracePrintf(3, "kernel_Release: pid=%d freed lock %d (no waiters)\n", g_current_process->pid, lock_id); 
  }

  return SUCCESS; 
}
int lock_destroy(int lock_id) {
  lock_t *lock = lock_find(lock_id); 
  if (lock == NULL)
    return ERROR;

  pcb_t *waiter; 
  while ((waiter = waiter_dequeue(&lock->waiters)) != NULL) {
    waiter->uctx.regs[0] = ERROR;
    waiter->state = RUNNABLE;
    waiter->waiting_on = WAIT_NONE;

    ready_enqueue(waiter); 
  }

  /* Remove lock from list */
  if (g_locks == lock) {
    g_locks = lock->next;
    lock->next = NULL; 
  } else {
    lock_t *prev = g_locks; 
    while (prev != NULL && prev->next != lock) {
      prev = prev->next; 
    }
    if (prev != NULL) {
      prev->next = lock->next; 
      lock->next = NULL; 
    }
  }

  helper_retire_id(lock_id); 
  free(lock);
  return SUCCESS; 
}

void lock_remove_waiter(pcb_t *p) {
  /* Used by kernel_Exit */
  lock_t *lock = g_locks; 
  while (lock != NULL) {
    if (waiter_remove(&lock->waiters, p)) return;
    lock = lock->next;
  }
}