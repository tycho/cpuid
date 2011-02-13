#ifndef __threads_h
#define __threads_h

unsigned int thread_count();
unsigned int thread_get_binding();
unsigned int thread_bind(unsigned int id);
unsigned int thread_bind_mask(unsigned int mask);

#endif
