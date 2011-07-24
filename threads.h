#ifndef __threads_h
#define __threads_h

unsigned int thread_count(void);
unsigned int thread_get_binding(void);
unsigned int thread_bind(unsigned int id);
unsigned int thread_bind_mask(unsigned int mask);

#endif
