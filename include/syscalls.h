#ifndef _yalnix_syscalls_h
#define _yalnix_syscalls_h

extern int kernel_Nop (int,int,int,int);

extern int kernel_Fork (void);
extern int kernel_Exec (char *, char **);
extern void kernel_Exit (int);
extern int kernel_Wait (int *);
extern int kernel_GetPid (void);
extern int kernel_Brk (void *);
extern int kernel_Delay (int);
extern int kernel_TtyRead (int, void *, int);
extern int kernel_TtyWrite (int, void *, int);

extern int kernel_ReadSector (int, void *);
extern int kernel_WriteSector (int, void *);

extern int kernel_PipeInit (int *);
extern int kernel_PipeRead (int, void *, int);
extern int kernel_PipeWrite (int, void *, int);

extern int kernel_SemInit (int *, int);
extern int kernel_SemUp (int);
extern int kernel_SemDown (int);
extern int kernel_LockInit (int *);
extern int kernel_Acquire (int);
extern int kernel_Release (int);
extern int kernel_CvarInit (int *);
extern int kernel_CvarWait (int, int);
extern int kernel_CvarSignal (int);
extern int kernel_CvarBroadcast (int);

extern int kernel_Reclaim (int);

#endif