#ifndef __BAB_TPYES_T_H
#define __BAB_TPYES_T_H

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
//typedef unsigned long  size_t;
#pragma push_macro("u_long")
#undef u_long
typedef unsigned long u_long;
#pragma pop_macro("u_long")

#if defined(__GNUC__) || \
    defined(__GNUG__)
__extension__
#endif /* gcc / g++ */
typedef unsigned long long u_int64;

#endif /* _BSDTYPES_DEFINED */

#ifdef __OHOS__
#include "sys/time.h"
typedef char* caddr_t;
#ifndef timersub
#define timersub(s,t,a) (void) ( (a)->tv_sec = (s)->tv_sec - (t)->tv_sec, \
	((a)->tv_usec = (s)->tv_usec - (t)->tv_usec) < 0 && \
	((a)->tv_usec += 1000000, (a)->tv_sec--) )
#endif  // timersub

#ifndef timercmp
#define timercmp(s,t,op) ((s)->tv_sec == (t)->tv_sec ? \
	(s)->tv_usec op (t)->tv_usec : (s)->tv_sec op (t)->tv_sec)
#endif  // timercmp

#ifndef timeradd
#define timeradd(s,t,a) (void) ( (a)->tv_sec = (s)->tv_sec + (t)->tv_sec, \
	((a)->tv_usec = (s)->tv_usec + (t)->tv_usec) >= 1000000 && \
	((a)->tv_usec -= 1000000, (a)->tv_sec++) )
#endif  // timeradd

#endif  // __OHOS__


#if defined (__LP64__) && defined (u_long)
typedef unsigned __LONG32 u_long;

#endif
