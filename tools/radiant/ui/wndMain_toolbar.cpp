//==============================
//	wndMain_toolbar.cpp
//==============================

#include "ui/wndMain.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui_internal.h>	// BeginViewportSideBar

#include <cstdint>
#include <string>

/*
===============================================================

	TOOLBAR

	the rebar bands from quaked3, drawn as one ImGui strip under the menu.
	icons come from quaked3's toolbar2.bmp (70 16x16 icons in a row), copied
	next to the exe as radiant/toolbar.bmp by the build
===============================================================
*/

#define TB_ICON_SIZE	16
#define TB_ICON_COUNT	70

bool g_bToolbarBand[REBARBANDCOUNT] = { true, true, true, true, true, true, true, true, true, true, true, true };

struct tbButton_t {
	int			bitmap;		// icon index, -1 for a separator
	int			cmd;
	const char	*tip;
};

#define TB_SEP	{ -1, ID_NONE, nullptr }

static const tbButton_t tb_file[] = {	// 0
	{ 0, ID_FILE_NEW, "Begin a New Map  (Ctrl+N)" },
	{ 1, ID_FILE_OPEN, "Open an Existing Map  (Ctrl+O)" },
	{ 2, ID_FILE_SAVE, "Save the Active Map  (Ctrl+S)" },
	{ 3, ID_FILE_SAVEAS, "Save Active Map with New Name (Ctrl+Shift+S)" },
};
static const tbButton_t tb_edit[] = {	// 1
	{ 6, ID_EDIT_CUT, "Cut (Ctrl+X)" },
	{ 7, ID_EDIT_COPY, "Copy (Ctrl+C)" },
	{ 8, ID_EDIT_PASTE, "Paste (Ctrl+V)" },
	{ 14, ID_EDIT_FINDBRUSH, "Find Brush or Entity (Ctrl+F)" },
	TB_SEP,
	{ 9, ID_SELECTION_CLONE, "Clone Selection (Space)" },
	{ 10, ID_SELECTION_INVERT, "Invert Selection (I)" },
	{ 11, ID_SELECTION_DELETE, "Delete Selection (Backspace)" },
	TB_SEP,
	{ 12, ID_EDIT_UNDO, "Undo (Ctrl+Z)" },
	{ 13, ID_EDIT_REDO, "Redo (Ctrl+Y)" },
};
static const tbButton_t tb_window[] = {	// 2
	{ 45, ID_VIEW_CONSOLE, "View Console Window (O)" },
	{ 46, ID_VIEW_ENTITY, "View Entity Window (N)" },
	{ 47, ID_VIEW_TEXTURE, "View Texture Window (T)" },
	TB_SEP,
	{ 16, ID_EDIT_MAPINFO, "Open Map Info Dialog (F2)" },
	{ 17, ID_EDIT_ENTITYINFO, "Open Entity Info Dialog (F3)" },
	{ 15, ID_EDIT_PREFERENCES, "Open Preferences Dialog (F4)" },
};
static const tbButton_t tb_trans[] = {	// 3
	{ 18, ID_SELECTION_FLIPX, "Flip Selection Along X-axis" },
	{ 19, ID_SELECTION_ROTATEX, "Rotate Selection 90 Along X-axis" },
	{ 20, ID_SELECTION_FLIPY, "Flip Selection Along Y-axis" },
	{ 21, ID_SELECTION_ROTATEY, "Rotate Selection 90 Along Y-axis" },
	{ 22, ID_SELECTION_FLIPZ, "Flip Selection Along Z-axis" },
	{ 23, ID_SELECTION_ROTATEZ, "Rotate Selection 90 Along Z-axis" },
};
static const tbButton_t tb_sel[] = {	// 4
	{ 24, ID_SELECTION_SELECTCOMPLETETALL, "Select Complete Tall (Alt+C)" },
	{ 25, ID_SELECTION_SELECTTOUCHING, "Select Touching (Alt+T)" },
	{ 26, ID_SELECTION_SELECTPARTIALTALL, "Select Partial Tall (Alt+P)" },
	{ 27, ID_SELECTION_SELECTINSIDE, "Select Inside (Alt+I)" },
};
static const tbButton_t tb_csg[] = {	// 5
	{ 28, ID_SELECTION_CSGHOLLOW, "CSG Hollow Selection  (Alt+H)" },
	{ 29, ID_SELECTION_CSGSUBTRACT, "CSG Subtract (Alt+S)" },
	{ 30, ID_SELECTION_CSGMERGE, "CSG Merge  (Alt+M)" },
};
static const tbButton_t tb_tools[] = {	// 6
	{ 58, ID_TOOLS_DRAWBRUSHESTOOL, "Draw Brush Tool (B)" },
	{ 31, ID_SELECTION_CLIPPER, "Set Clipper Mode (X)" },
	{ 32, ID_SELECTION_DRAGEDGES, "Set Drag Edges Mode (E)" },
	{ 33, ID_SELECTION_DRAGVERTICES, "Set Drag Vertex Mode (V)" },
	{ 57, ID_SELECTION_DRAGFACES, "Set Drag Faces Mode (F)" },
};
static const tbButton_t tb_ent[] = {	// 7
	{ 37, ID_SELECTION_UNGROUPENTITY, "Ungroup Selected Entity (Ctrl+U)" },
	{ 38, ID_SELECTION_CONNECT, "Connect Entities (Ctrl+K)" },
	{ 39, ID_SELECTION_GROUPNEXTBRUSH, "Select Next Brush in Group (Ctrl+Tab)" },
};
static const tbButton_t tb_czg[] = {	// 8
	{ 40, ID_PRIMITIVES_CZGCYLINDER1, "Create 12-sided CZG Cylinder" },
	{ 41, ID_PRIMITIVES_CZGCYLINDER2, "Create 24-sided CZG Cylinder" },
};
static const tbButton_t tb_tex[] = {	// 9
	{ 43, ID_TEXTURES_REPLACEALL, "Replace Textures (Ctrl+R)" },
	{ 44, ID_TEXTURES_INSPECTOR, "Open Surface Inspector (S)" },
};
static const tbButton_t tb_view[] = {	// 10
	{ 59, ID_VIEW_FILTER_POPUP, "Set View Filters" },
	{ 48, ID_VIEW_NEXTVIEW, "Cycle 2D View (Tab)" },
	{ 49, ID_VIEW_CENTERONSELECTION, "Center Views on Selection (\\)" },
	TB_SEP,
	{ 51, ID_VIEW_CAMSPEED, "Adjust Camera Movement Speed" },
	{ 52, ID_VIEW_CUBICCLIP, "Cubic Clip the Camera View (Ctrl+\\)" },
	{ 53, ID_VIEW_CUBICCLIPNEARER, "Cubic Clip Nearer (Ctrl+[)" },
	{ 54, ID_VIEW_CUBICCLIPFARTHER, "Cubic Clip Farther (Ctrl+])" },
};
static const tbButton_t tb_misc[] = {	// 11
	{ 55, ID_MISC_TESTMAP, "Test Current Map (F12)" },
	{ 56, ID_HELP_HELP, "Open Manual (F1)" },
	{ 5, ID_HELP_ABOUT, "About..." },
};

struct tbBand_t {
	const tbButton_t	*buttons;
	int					count;
};

#define TB_BAND(x)	{ (x), (int)(sizeof(x) / sizeof(tbButton_t)) }

static const tbBand_t s_bands[REBARBANDCOUNT] = {
	TB_BAND(tb_file), TB_BAND(tb_edit), TB_BAND(tb_window), TB_BAND(tb_trans),
	TB_BAND(tb_sel), TB_BAND(tb_csg), TB_BAND(tb_tools), TB_BAND(tb_ent),
	TB_BAND(tb_czg), TB_BAND(tb_tex), TB_BAND(tb_view), TB_BAND(tb_misc),
};

static GLuint s_iconTex;

/*
==============
WndMain_CreateToolbar

loads the icon strip, the old toolbar's light grey background becomes transparent
==============
*/
void WndMain_CreateToolbar()
{
	char *base = SDL_GetBasePath();
	std::string path = std::string(base ? base : "") + "radiant/toolbar.bmp";
	SDL_free(base);

	SDL_Surface *bmp = SDL_LoadBMP(path.c_str());
	if (!bmp)
	{
		SDL_Log("toolbar: %s", SDL_GetError());
		return;
	}
	SDL_Surface *rgba = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_ABGR8888, 0);	// R,G,B,A bytes in memory
	SDL_FreeSurface(bmp);
	if (!rgba)
		return;

	for (int y = 0; y < rgba->h; y++)
	{
		uint8_t *p = (uint8_t *)rgba->pixels + y * rgba->pitch;
		for (int x = 0; x < rgba->w; x++, p += 4)
		{
			if (p[0] == 192 && p[1] == 192 && p[2] == 192)
				p[3] = 0;
		}
	}

	glGenTextures(1, &s_iconTex);
	glBindTexture(GL_TEXTURE_2D, s_iconTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, rgba->pitch / 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	SDL_FreeSurface(rgba);
}

/*
==============
WndMain_DestroyToolbar
==============
*/
void WndMain_DestroyToolbar()
{
	if (s_iconTex)
		glDeleteTextures(1, &s_iconTex);
	s_iconTex = 0;
}

/*
==============
WndMain_ToolbarButton
==============
*/
static void WndMain_ToolbarButton(const tbButton_t &b)
{
	bool enabled = WndMain_CommandEnabled(b.cmd);
	bool checked = WndMain_CommandChecked(b.cmd);
	bool pressed;

	ImGui::PushID(b.cmd);
	ImGui::BeginDisabled(!enabled);
	if (checked)
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

	if (s_iconTex)
	{
		float u0 = (float)b.bitmap / TB_ICON_COUNT;
		float u1 = (float)(b.bitmap + 1) / TB_ICON_COUNT;
		pressed = ImGui::ImageButton("##tb", (ImTextureID)(uintptr_t)s_iconTex,
			ImVec2(TB_ICON_SIZE, TB_ICON_SIZE), ImVec2(u0, 0), ImVec2(u1, 1));
	}
	else
	{
		// no icons: first word of the tooltip
		std::string label(b.tip);
		label = label.substr(0, label.find(' '));
		pressed = ImGui::Button(label.c_str());
	}

	if (checked)
		ImGui::PopStyleColor();
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("%s", b.tip);
	ImGui::PopID();

	if (pressed)
		WndMain_Command(b.cmd);
}

/*
==============
WndMain_DrawToolbar
==============
*/
void WndMain_DrawToolbar()
{
	ImGuiStyle &style = ImGui::GetStyle();
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
	float height = TB_ICON_SIZE + style.FramePadding.y * 2 + style.WindowPadding.y * 2;
	bool first = true;

	if (ImGui::BeginViewportSideBar("##Toolbar", ImGui::GetMainViewport(), ImGuiDir_Up, height, flags))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, style.ItemSpacing.y));
		for (int i = 0; i < REBARBANDCOUNT; i++)
		{
			if (!g_bToolbarBand[i])
				continue;

			if (!first)
			{
				ImGui::SameLine(0, 8);
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				ImGui::SameLine(0, 8);
			}
			first = false;

			ImGui::PushID(i);
			for (int j = 0; j < s_bands[i].count; j++)
			{
				const tbButton_t &b = s_bands[i].buttons[j];
				if (j > 0)
					ImGui::SameLine(0, b.bitmap < 0 ? 6.0f : -1.0f);
				if (b.bitmap < 0)
				{
					ImGui::Dummy(ImVec2(0, TB_ICON_SIZE));	// TBSTYLE_SEP, just a gap
					continue;
				}
				WndMain_ToolbarButton(b);
			}
			ImGui::PopID();
		}
		ImGui::PopStyleVar();
	}
	ImGui::End();
}
