#ifndef _yalnix_validate_h
#define _yalnix_validate_h

int validate_user_buffer(const void *buf, int len, int prot);
int validate_user_string(const char *str); 


#endif