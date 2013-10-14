#ifndef _MACHDEFS_H_
#define _MACHDEFS_H_

typedef unsigned short	u_int16;

#if defined(__alpha)
typedef unsigned int	u_int32;
#else
typedef unsigned long	u_int32;
#endif

#endif
