/***************************************************************************
                            xa.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#ifndef __P_XA_H__
#define __P_XA_H__

INLINE void MixXA(void);
INLINE void FeedXA(xa_decode_t *xap);
INLINE int  FeedCDDA(unsigned char *pcm, int nBytes);

#endif /* __P_XA_H__ */
