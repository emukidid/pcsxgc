
// byteswappings

#define SWAPSPU16(x) (((x)>>8 & 0xff) | ((x)<<8 & 0xff00))
#define SWAPSPU32(x) (((x)>>24 & 0xfful) | ((x)>>8 & 0xff00ul) | ((x)<<8 & 0xff0000ul) | ((x)<<24 & 0xff000000ul))

// big endian config
/*
#define HOST2LE32(x) SWAP32(x)
#define HOST2BE32(x) (x)
#define LE2HOST32(x) SWAP32(x)
#define BE2HOST32(x) (x)
*/

#define HOST2LE16(x) SWAPSPU16(x)
#define HOST2BE16(x) (x)
#define LE2HOST16(x) SWAPSPU16(x)
#define BE2HOST16(x) (x)
