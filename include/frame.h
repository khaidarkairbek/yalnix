#ifndef _yalnix_frame_h
#define _yalnix_frame_h

void frame_init(unsigned int pmem_size);
int  frame_alloc(void); 
void frame_free(int pfn);
int  frames_available(void);

#endif