#include <imgui.h>
#include <imgui_internal.h>	//DockBuilder
#include <imgui_stdlib.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <SDL.h>	//renames main to SDL_main, SDL2main provides the WinMain that calls it
#include <SDL_syswm.h>
#include <SDL_opengl.h>
#include <sys/stat.h>

#include <string>
#include <vector>

#include "qccgui.hpp"

extern "C"
{
#include "qcc.h"
#include "gui.h"

extern int qcc_compileactive;
};

struct document_s
{
	std::string name;
	std::string text;
	bool dirty = false;
};
static std::vector<document_s> docs;
static int docs_select = -1;	//tab to bring to front next frame

struct sourcefile_s
{
	std::string parent;
	std::string name;
};
static std::vector<sourcefile_s> sourcefiles;

struct logline_s
{
	std::string text;
	ImU32 colour;
};
static std::vector<logline_s> loglines;
static std::string logpartial;
static bool log_scrolltobottom;

void RunCompiler(const char *args, pbool quick);
void AddSourceFile(const char *parentpath, const char *filename);