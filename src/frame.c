#include "frame.h"
#include <ykernel.h>

static int total_frames = 0;
static int free_count = 0; 
static unsigned char *frame_bitmap = NULL; 

// Bitmap helpers
#define BIT_GET(bm, i)  ((bm[(i) / 8] >>  ((i) % 8)) & 1)
#define BIT_SET(bm, i)  (bm[(i) / 8] |=  (1 << ((i) % 8)))
#define BIT_CLR(bm, i)  (bm[(i) / 8] &= ~(1 << ((i) % 8)))

void frame_init(unsigned int pmem_size) {
  total_frames = pmem_size / PAGESIZE; 
  int bytes = (total_frames + 7) / 8;

  frame_bitmap = malloc(bytes); 
  if (frame_bitmap == NULL) {
    Halt(); 
  }

  for (int i = 0; i < bytes; i++) {
    frame_bitmap[i] = 0xFF; 
  }
  free_count = total_frames; 
}

int frame_alloc(void) {
  for (int i = 0; i < total_frames; i++) {
    // check if the frame available 
    if (BIT_GET(frame_bitmap, i)) {
      // if available, mark it as used
      BIT_CLR(frame_bitmap, i);
      free_count--;
      return i; 
    }
  }

  // out of physical memory
  return -1; 
}
void frame_free(int pfn) {
  if (pfn < 0 || pfn > total_frames) {
    // invalid fpn
    return; 
  }

  if (BIT_GET(frame_bitmap, pfn)) {
    // already free
    return; 
  }

  BIT_SET(frame_bitmap, pfn);
  free_count++; 
}

int  frames_available(void) {
  return free_count; 
}
