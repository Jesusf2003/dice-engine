//==============================
//	wndOptions.h
//
//	Tools > Options, hammer's options dialog. for now its Game Configurations
//	tab: the games and mods, each with its game data files (FGDs) and folders
//	(core/common/gameConfig.h). edits a copy until OK / Apply; applying a
//	change to the active configuration reloads its definitions and wads.
//	TODO: Build Programs (qbsp, vis, light, the engine) once maps compile from here
//==============================

#ifndef __WND_OPTIONS_H__
#define __WND_OPTIONS_H__

namespace WndOptions
{
	void	Open();
	void	Draw();		// every frame, a floating window when open
}

#endif
