#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"

void 
syscall_init (void);

/* FUNCTIONS TO BE USED BY PROCESSES(WRAPPERS) */

void
process_close(int fd, struct file* ptr);


/* ******************_END OF WRAPPER FUNCTIONS_********************** */


#endif /* userprog/syscall.h */
