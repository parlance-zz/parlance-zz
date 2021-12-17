#ifndef _H_STDTYPES_
#define _H_STDTYPES_

// specific size integer / float types, important in 64bit environment, from stdtypes.h

#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ int8;
#else
typedef signed char int8;
#endif
#ifdef __INT16_TYPE__
typedef __INT16_TYPE__ int16;
#else
typedef signed short int16;
#endif
#ifdef __INT32_TYPE__
typedef __INT32_TYPE__ int32;
#else
typedef signed int int32;
#endif
#ifdef __INT64_TYPE__
typedef __INT64_TYPE__ int64;
#else
typedef signed long long int64;
#endif
#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ uint8;
#else
typedef unsigned char uint8;
#endif
#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ uint16;
#else
typedef unsigned short uint16;
#endif
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32;
#else
typedef unsigned int uint32;
#endif
#ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ uint64;
#else
typedef unsigned long long uint64;
#endif

#define NULL	0

typedef float  float32;
typedef double float64;

// variable args stuff from vadefs.h and _mingw_stdarg.h

#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
  typedef __builtin_va_list __gnuc_va_list;
#endif

#ifndef _VA_LIST_DEFINED
#define _VA_LIST_DEFINED
  typedef __gnuc_va_list va_list;
#endif

#define _crt_va_start(v,l)	__builtin_va_start(v,l)
#define _crt_va_arg(v,l)	__builtin_va_arg(v,l)
#define _crt_va_end(v)	__builtin_va_end(v)

#ifndef va_start
#define va_start _crt_va_start
#endif
#ifndef va_arg
#define va_arg _crt_va_arg
#endif
#ifndef va_end
#define va_end _crt_va_end
#endif

#endif