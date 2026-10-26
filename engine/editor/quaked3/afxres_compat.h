// Stand-in for MFC's afxres.h, so the editor builds without MFC installed.
// Provides the plain resource definitions plus the few standard MFC command ids
// the menus, toolbar and accelerators use (same values as afxres.h).
#ifndef __AFXRES_COMPAT_H__
#define __AFXRES_COMPAT_H__

#ifdef RC_INVOKED
#include <winres.h>
#else
// winres.h must stay out of C++: it redefines LB_ADDSTRING/CB_ADDSTRING with
// their Win16 values, which silently breaks every listbox and combobox.
#include <windows.h>
#ifndef IDC_STATIC
#define IDC_STATIC          (-1)
#endif
#endif

#ifndef ID_FILE_NEW
#define ID_FILE_NEW         0xE100
#define ID_FILE_OPEN        0xE101
#define ID_FILE_SAVE        0xE103
#define ID_FILE_SAVE_AS     0xE104
#define ID_EDIT_COPY        0xE122
#define ID_EDIT_CUT         0xE123
#define ID_EDIT_PASTE       0xE125
#define ID_EDIT_UNDO        0xE12B
#define ID_EDIT_REDO        0xE12C
#define ID_CONTEXT_HELP     0xE145
#endif

#endif
