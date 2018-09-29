
// byteswappings

#define SWAP16(x) (((x)>>8 & 0xff) | ((x)<<8 & 0xff00))
#define SWAP32(x) (__builtin_bswap32(x))

// big endian config
#define HOST2LE32(x) SWAP32(x)
#define HOST2BE32(x) (x)
#define LE2HOST32(x) SWAP32(x)
#define BE2HOST32(x) (x)

#define HOST2LE16(x) SWAP16(x)
#define HOST2BE16(x) (x)
#define LE2HOST16(x) SWAP16(x)
#define BE2HOST16(x) (x)

#define GETLEs16(X) ((short)GETLE16((unsigned short *)X))
#define GETLEs32(X) ((short)GETLE32((unsigned short *)X))

#ifdef _BIG_ENDIAN
#if 0
// Metrowerks styles
#if 1
#define GETLE16(X) ((unsigned short)__lhbrx(X, 0))
#define GETLE32(X) ((u32)__lwbrx(X, 0))
#define GETLE16D(X) ((u32)__rlwinm(GETLE32(X), 16, 0, 31))
#define PUTLE16(X, Y) __sthbrx(Y, X, 0)
#define PUTLE32(X, Y) __stwbrx(Y, X, 0)
#else
__inline__ unsigned short GETLE16(register unsigned short *ptr) {
  register unsigned short ret;
  asm {
    lhbrx ret, r0, ptr;
  }
  return ret;
}
__inline__ u32 GETLE32(register u32 *ptr) {
  register u32 ret;
  asm {
    lwbrx ret, r0, ptr;
  }
  return ret;
}
__inline__ u32 GETLE16D(register u32 *ptr) {
  register unsigned short ret;
  asm {
    lwbrx ret, r0, ptr;
    rlwinm ret, ret, 16, 0, 31;
  }
  return ret;
}
__inline__ void PUTLE16(register unsigned short *ptr, register unsigned short val) {
  asm {
    sthbrx val, r0, ptr;
  }
}
__inline__ void PUTLE32(register u32 *ptr, register u32 val) {
  asm {
    stwbrx val, r0, ptr;
  }
}
#endif
#else
// GCC style
__inline__ unsigned short GETLE16(unsigned short *ptr) {
    unsigned short ret; __asm__ ("lhbrx %0, 0, %1" : "=r" (ret) : "r" (ptr));
    return ret;
}
__inline__ u32 GETLE32(u32 *ptr) {
    u32 ret;
    __asm__ ("lwbrx %0, 0, %1" : "=r" (ret) : "r" (ptr));
    return ret;
}
__inline__ u32 GETLE16D(u32 *ptr) {
    u32 ret;
    __asm__ ("lwbrx %0, 0, %1\n"
             "rlwinm %0, %0, 16, 0, 31" : "=r" (ret) : "r" (ptr));
    return ret;
}

__inline__ void PUTLE16(unsigned short *ptr, unsigned short val) {
    __asm__ ("sthbrx %0, 0, %1" : : "r" (val), "r" (ptr) : "memory");
}
__inline__ void PUTLE32(u32 *ptr, u32 val) {
    __asm__ ("stwbrx %0, 0, %1" : : "r" (val), "r" (ptr) : "memory");
}
#endif
#else // _BIG_ENDIAN
#define GETLE16(X) ((unsigned short *)X)
#define GETLE32(X) ((u32 *)X)
#define GETLE16D(X) ({u32 val = GETLE32(X); (val<<16 | val >> 16)})
#define PUTLE16(X, Y) {((unsigned short *)X)=(unsigned short)X}
#define PUTLE32(X, Y) {((u32 *)X)=(u32)X}
#endif //!_BIG_ENDIAN
