//==============================
//	wndPreferences.h
//
//	Edit > Preferences: a page tree on the left (netradiant's preferences
//	dialog), the page on the right, OK / Apply / Cancel. edits a copy of
//	g_prefs until applied. pages so far: Console, Console > Advanced
//==============================

#ifndef __WND_PREFERENCES_H__
#define __WND_PREFERENCES_H__

namespace WndPreferences
{
	void	Open();
	void	Draw();		// every frame, after the dockspace: a floating window when open
}

#endif
