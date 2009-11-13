/*****************************************************************************
 * font.h - C functions to call IPLFont singleton class
 *****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void IplFont_drawInit(GXColor fontColor);
void IplFont_drawString(int x, int y, char *string, float scale, bool centered);

#ifdef __cplusplus
}
#endif
