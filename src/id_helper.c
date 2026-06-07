#include "id_helper.h"

#define FIRST_SYNC_ID 1000

static int next_id = FIRST_SYNC_ID;

int helper_new_id(void) {
  return next_id++;
}
void helper_retire_id(int) {
  // Assumption: ids won't wrap around 
  return;
}