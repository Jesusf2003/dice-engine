//==============================
//	wndMain_status.cpp
//==============================

#include "ui/wndMain.h"
#include "map/grid.h"

#include <imgui_internal.h>	// BeginViewportSideBar

#include <cstdio>
#include <string>

/*
===============================================================

	STATUS BAR

===============================================================
*/

/*
0: help/mouseover/etc
1: grid
2: brush
3: selection
*/
#define STATUS_PARTS 4

bool g_bStatusBar = true;

static std::string	s_status[STATUS_PARTS] = { "Ready" };
static const float	s_partEnd[STATUS_PARTS - 1] = { 256, 720, 960 };	// right edge of each part, the last one fills the rest

/*
==============
WndMain_Status
==============
*/
void WndMain_Status(const char *psz, int part)
{
	if (part < 0 || part >= STATUS_PARTS)
		return;
	s_status[part] = psz ? psz : "";
}

// TODO: WndMain_UpdateBrushStatusBar (brush/entity/texture counts) comes back with the map

/*
==============
WndMain_UpdateGridStatusBar

TODO: tool, texture lock and cubic clip too, once those exist
==============
*/
void WndMain_UpdateGridStatusBar()
{
	char gridstatus[128];

	snprintf(gridstatus, sizeof(gridstatus), "Grid Size: %g  Grid Snap: %s",
		Grid::ActualSize(g_grid.Size()), g_grid.Snap() ? "On" : "Off");
	WndMain_Status(gridstatus, 1);
}

/*
==============
WndMain_DrawStatusBar
==============
*/
void WndMain_DrawStatusBar()
{
	if (!g_bStatusBar)
		return;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();

	if (ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, height, flags))
	{
		if (ImGui::BeginMenuBar())
		{
			float x0 = ImGui::GetCursorPosX();
			for (int i = 0; i < STATUS_PARTS; i++)
			{
				if (i > 0)
				{
					ImGui::SameLine(x0 + s_partEnd[i - 1]);
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
					ImGui::SameLine();
				}
				ImGui::TextUnformatted(s_status[i].c_str());
			}
			ImGui::EndMenuBar();
		}
	}
	ImGui::End();
}
