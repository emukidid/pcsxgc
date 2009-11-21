// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A412EBA3_CBB9_11D6_A315_008048C61B72__INCLUDED_)
#define AFX_STDAFX_H__A412EBA3_CBB9_11D6_A315_008048C61B72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>

// TODO: reference additional headers your program requires here

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>
#include <commdlg.h>
#include <windowsx.h>
#include <stddef.h>
#include "PSEmu Plugin Defs.h"
#include "wnaspi32.h"
#include "scsidefs.h"

#include "defines.h"
#include "cdda.h"
#include "cdr.h"
#include "cfg.h"
#include "generic.h"
#include "ioctrl.h"
#include "ppf.h"
#include "read.h"
#include "scsi.h"
#include "sub.h"
#include "toc.h"

//#define SMALLDEBUG 1   
//#include <dbgout.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A412EBA3_CBB9_11D6_A315_008048C61B72__INCLUDED_)
