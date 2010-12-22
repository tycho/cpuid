#ifndef __stdint_h
#define __stdint_h

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
#if _MSC_VER >= 1300
typedef signed long long int64_t;
#else
typedef signed __int64 int64_t;
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#if _MSC_VER >= 1300
typedef unsigned long long uint64_t;
#else
typedef unsigned __int64 uint64_t;
#endif

#endif
