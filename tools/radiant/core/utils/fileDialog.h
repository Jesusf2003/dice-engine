//==============================
//	fileDialog.h
//
//	the system's open file dialog. SDL2 has none (SDL3 does), so windows uses
//	GetOpenFileName directly. paths are utf-8.
//	TODO: linux / mac (zenity, NSOpenPanel), until then maps open from the command line
//==============================

#ifndef __FILE_DIALOG_H__
#define __FILE_DIALOG_H__

#include <string>

namespace FileDialog
{
	// filterName "Quake map", pattern "*.map". false when cancelled or unavailable
	bool	OpenFile(std::string &outPath, const char *title, const char *filterName, const char *pattern,
				const std::string &initialDir);
	bool	OpenFolder(std::string &outPath, const char *title, const std::string &initialDir);
}

#endif
