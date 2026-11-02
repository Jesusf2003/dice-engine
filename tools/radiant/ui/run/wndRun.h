//==============================
//	wndRun.h
//
//	the Run menu's windows, trenchbroom's CompilationDialog and
//	LaunchGameEngineDialog (core/compile/compilation.h does the work):
//
//	-	Compile Map: the active game's compilation profiles (a mod uses its
//		game's), each a working folder and its tasks, edited in place; Test
//		prints what a run would do, Compile runs it with its output below
//	-	Launch Engine: the active profile's engine, its launch parameters and
//		what they come to for the open map
//==============================

#ifndef __WND_RUN_H__
#define __WND_RUN_H__

namespace WndCompile
{
	void	Open();
	void	Draw();		// every frame, a floating window when open
}

namespace WndLaunch
{
	void	Open();
	void	Draw();
}

#endif
