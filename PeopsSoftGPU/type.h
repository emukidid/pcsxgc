#ifndef TYPE_H
#define TYPE_H

// Return values (should be applied to the entire code).
#define FPSE_OK         0
#define FPSE_ERR        -1
#define FPSE_WARN       1


typedef signed char        INT8;
typedef signed short int   INT16;
#ifndef _WINDOWS
typedef s32    INT32;
#endif

typedef unsigned char      UINT8;
typedef unsigned short int UINT16;
#ifndef _WINDOWS
typedef u32   UINT32;
#endif

#ifdef __GNUC__
#define INT64 s64
#else
#define INT64    __int64
#endif

#endif
