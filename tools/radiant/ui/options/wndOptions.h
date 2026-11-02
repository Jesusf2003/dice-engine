//==============================
//	wndOptions.h
//
//	Tools > Options, hammer's options dialog. for now its Game Configurations
//	tab: the engine, game and mod profiles (core/common/gameConfig.h), each
//	under the one it builds on: an engine's folder, program, launch parameters and compilation tools,
//	a game's or mod's game data files (FGDs) and folders. edits a copy until
//	OK / Apply; applying a change to what the active game or mod loads reloads
//	its definitions and wads.
//	the compilation tools and launch parameters set here are used by the Run
//	menu (ui/run/wndRun.h)
//==============================

#ifndef __WND_OPTIONS_H__
#define __WND_OPTIONS_H__

namespace WndOptions
{
	void	Open();
	void	Draw();		// every frame, a floating window when open
}

#endif
