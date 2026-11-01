//==============================
//	wndConsole.h
//
//	the console tab of the inspector: the log (core/common/log.h) with a color
//	per level, level toggles, a text filter, copy and clear. port of
//	quaked3's WndConsole, behaviour from Preferences > Console
//==============================

#ifndef __WND_CONSOLE_H__
#define __WND_CONSOLE_H__

#include "ui/window.h"
#include "core/common/log.h"

#include <cstdint>
#include <vector>

class WndConsole : public Window
{
public:
	WndConsole();

	void	ScrollToEnd();

protected:
	int		OnPaint() override;

private:
	void	DrawToolbar();
	void	RebuildIndex();
	void	CopyToClipboard();

	bool	showLevel[LOG_LEVELS];
	ImGuiTextFilter	filter;

	std::vector<size_t>	visible;		// log line indices that pass the filters
	uint64_t	indexedGeneration;
	bool		indexDirty;
	bool		scrollToEnd;
};

#endif
