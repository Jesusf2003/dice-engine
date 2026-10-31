//==============================
//	wndMain.cpp
//==============================

#include "ui/wndMain.h"
#include "ui/window.h"
#include "config.h"
#include "renderer/textures.h"
#include "ui/textures/wndTextures.h"
#include "ui/view/mapView2D.h"
#include "renderer/glFuncs.h"
#include "renderer/gridRenderer.h"

#include <SDL.h>
#include <imgui_internal.h>	// DockBuilder

#include <algorithm>
#include <cstdio>
#include <string>

SDL_Window	*g_sdlMain;

Window	*g_wndCamera;
MapView2D	*g_wndGrid[4];
Window	*g_wndZ;
WndTexture	*g_wndTexture;
Window	*g_wndEntity;
Window	*g_wndConsole;

int		g_nUpdateBits = 0;
int		g_nInspectorMode;		// W_TEXTURE, W_ENTITY, or W_CONSOLE

static int	s_pendingLayout = -1;
static bool	s_pendingSwap = false;

static const char RADIANT_DOCKSPACE[] = "RadiantDockSpace";


/*
==============================================================================

	CHILD WINDOWS

==============================================================================
*/

/*
==================
WndPlaceholder

stands in for a window class that hasn't been ported yet
==================
*/
class WndPlaceholder : public Window
{
public:
	WndPlaceholder(const char *wndName, const char *wndTitle, int wndInstance = 0)
	{
		name = wndName;
		instance = wndInstance;
		SetTitle(wndTitle);
	}

protected:
	int OnPaint() override
	{
		ImGui::TextDisabled("not ported yet");
		return 1;
	}
};

/*
==================
WndMain_SwapGridCam

applied at the start of the next frame, docked windows can't move while being submitted
==================
*/
void WndMain_SwapGridCam()
{
	s_pendingSwap = true;
}

static void WndMain_DoSwapGridCam()
{
	ImGuiWindow *cam = ImGui::FindWindowByName(g_wndCamera->Id());
	ImGuiWindow *grid = ImGui::FindWindowByName(g_wndGrid[0]->Id());

	if (cam && grid && cam->DockId && grid->DockId)
	{
		ImGuiID camDock = cam->DockId;
		ImGuiID gridDock = grid->DockId;
		ImGui::DockBuilderDockWindow(g_wndCamera->Id(), gridDock);
		ImGui::DockBuilderDockWindow(g_wndGrid[0]->Id(), camDock);
	}
	else
	{
		g_wndCamera->Swap(*g_wndGrid[0]);
	}
}

/*
==================
WndMain_SetInspectorMode

console, entity and texture share one dock node as tabs, so switching is
just bringing the right tab to the front
==================
*/
void WndMain_SetInspectorMode(int nType)
{
	Window *to;

	switch (nType)
	{
	case W_ENTITY:
		to = g_wndEntity;
		break;
	case W_TEXTURE:
		to = g_wndTexture;
		break;
	case W_CONSOLE:
	default:
		to = g_wndConsole;
		// TODO: g_wndConsole->ScrollToEnd() once WndConsole is ported
		break;
	}

	to->Show();
	to->Focus();
	g_nInspectorMode = nType;
}

/*
==============
WndMain_DefaultLayout

applied at the start of the next frame, before the dockspace is submitted
=============
*/
void WndMain_DefaultLayout(int nStyle)
{
	s_pendingLayout = nStyle;
}

static void DockInspector(ImGuiID node)
{
	ImGui::DockBuilderDockWindow(g_wndConsole->Id(), node);
	ImGui::DockBuilderDockWindow(g_wndEntity->Id(), node);
	ImGui::DockBuilderDockWindow(g_wndTexture->Id(), node);
	g_wndConsole->Show();
	g_wndEntity->Show();
	g_wndTexture->Show();
}

static void DockGrids(ImGuiID node)
{
	for (int i = 0; i < 3; i++)
		ImGui::DockBuilderDockWindow(g_wndGrid[i]->Id(), node);
	g_wndGrid[0]->Show();
	g_wndGrid[1]->Hide();
	g_wndGrid[2]->Hide();
}

static void WndMain_ApplyLayout(ImGuiID dockspace, int nStyle)
{
	ImGuiViewport *vp = ImGui::GetMainViewport();
	float w = std::max(vp->WorkSize.x, 640.0f);
	float h = std::max(vp->WorkSize.y, 480.0f);

	// camera (or the top row of a 4 view) gets the space above v1
	float v1 = std::min(h - 480, h / 2);
	float camRatio = std::max(v1, 64.0f) / h;

	ImGui::DockBuilderRemoveNode(dockspace);
	ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace, vp->WorkSize);

	ImGuiID rest = dockspace;
	ImGuiID z = 0, insp = 0, cam = 0, grid = 0;

	switch (nStyle)
	{
	case 0:	// QE3 Default
	case 1:	// QE3 Reverse
	{
		ImGuiDir zDir = (nStyle == 0) ? ImGuiDir_Left : ImGuiDir_Right;
		ImGuiDir colDir = (nStyle == 0) ? ImGuiDir_Right : ImGuiDir_Left;
		float colWidth = w * 0.375f;
		ImGuiID col;

		z = ImGui::DockBuilderSplitNode(rest, zDir, 64 / w, nullptr, &rest);
		col = ImGui::DockBuilderSplitNode(rest, colDir, colWidth / (w - 64), nullptr, &grid);
		cam = ImGui::DockBuilderSplitNode(col, ImGuiDir_Up, camRatio, nullptr, &insp);

		ImGui::DockBuilderDockWindow(g_wndZ->Id(), z);
		ImGui::DockBuilderDockWindow(g_wndCamera->Id(), cam);
		DockGrids(grid);
		DockInspector(insp);
		g_wndZ->Show();
		g_wndCamera->Show();
		break;
	}

	default:	// the 4 view presets, 2..9
	{
		bool camLeft = (nStyle % 2) == 0;
		bool reverse = nStyle >= 6;				// inspector on the left, Z on the right
		bool withZ = (nStyle == 2 || nStyle == 3 || nStyle == 6 || nStyle == 7);
		float used = 0;
		ImGuiID top, bottom, tl, tr, bl, br;

		if (withZ)
		{
			z = ImGui::DockBuilderSplitNode(rest, reverse ? ImGuiDir_Right : ImGuiDir_Left, 64 / w, nullptr, &rest);
			used = 64;
		}
		insp = ImGui::DockBuilderSplitNode(rest, reverse ? ImGuiDir_Left : ImGuiDir_Right, 320 / (w - used), nullptr, &rest);

		top = ImGui::DockBuilderSplitNode(rest, ImGuiDir_Up, camRatio, nullptr, &bottom);
		tl = ImGui::DockBuilderSplitNode(top, ImGuiDir_Left, 0.5f, nullptr, &tr);
		bl = ImGui::DockBuilderSplitNode(bottom, ImGuiDir_Left, 0.5f, nullptr, &br);

		ImGui::DockBuilderDockWindow(g_wndCamera->Id(), camLeft ? tl : tr);
		ImGui::DockBuilderDockWindow(g_wndGrid[0]->Id(), camLeft ? tr : tl);
		ImGui::DockBuilderDockWindow(g_wndGrid[1]->Id(), bl);
		ImGui::DockBuilderDockWindow(g_wndGrid[2]->Id(), br);
		DockInspector(insp);

		g_wndCamera->Show();
		for (int i = 0; i < 3; i++)
			g_wndGrid[i]->Show();

		if (withZ)
		{
			ImGui::DockBuilderDockWindow(g_wndZ->Id(), z);
			g_wndZ->Show();
		}
		else
		{
			g_wndZ->Hide();
		}
		break;
	}
	}

	ImGui::DockBuilderFinish(dockspace);
	WndMain_SetInspectorMode(W_CONSOLE);
}

/*
============
WndMain_WindowForPoint

point in main viewport space
============
*/
Window *WndMain_WindowForPoint(ImVec2 point)
{
	for (auto wvIt = Window::windex.begin(); wvIt != Window::windex.end(); ++wvIt)
	{
		if (!(*wvIt)->IsOpen())
			continue;

		Window::wndPos_t pos = (*wvIt)->GetPosition();
		if (point.x >= pos.l && point.x < pos.r && point.y >= pos.t && point.y < pos.b)
			return *wvIt;
	}
	return nullptr;
}


/*
==================
WndMain_UpdateWindows
==================
*/
void WndMain_UpdateWindows(int bits)
{
	g_nUpdateBits |= bits;
}

/*
==================
WndMain_ForceUpdateWindows

ImGui repaints every window every frame, so only the side effects are left here
==================
*/
void WndMain_ForceUpdateWindows(int bits)
{
	bits |= g_nUpdateBits;

	// TODO: W_TARGETGRAPH -> g_map.targetGraph.Refresh(), W_TEXTURE -> g_vTexture.Arrange(),
	// W_SURF -> WndSurf_UpdateUI(), once those are ported

	if (bits & W_TITLE)
		WndMain_UpdateTitle();
	g_nUpdateBits = 0;
}

/*
==================
WndMain_CreateViews
==================
*/
void WndMain_CreateViews()
{
	g_wndConsole = new WndPlaceholder("Console", "Console");
	g_wndConsole->Create();

	g_wndCamera = new WndPlaceholder("Camera", "Camera");
	g_wndCamera->Create();
	for (int i = 0; i < 3; i++)
	{
		static const MapView2D::ViewPlane planes[3] = { MapView2D::XY, MapView2D::YZ, MapView2D::XZ };
		g_wndGrid[i] = new MapView2D(planes[i], i);
		g_wndGrid[i]->Create();
	}
	g_wndZ = new WndPlaceholder("Z", "Z");
	g_wndZ->Create();

	g_wndTexture = new WndTexture();
	g_wndTexture->Initialize();
	g_wndEntity = new WndPlaceholder("Entity", "Entity");
	g_wndEntity->Create();

	g_nInspectorMode = W_CONSOLE;
}



/*
=============================================================

MAIN WINDOW

=============================================================
*/

/*
==================
WndMain_UpdateTitle
==================
*/
void WndMain_UpdateTitle()
{
	char title[256];

	// TODO: map name and modified flag once the map is ported
	std::snprintf(title, sizeof(title), "%s - [%s%s]", radiantdef.title, "unnamed", "");
	SDL_SetWindowTitle(g_sdlMain, title);
}

/*
==============
WndMain_Create
==============
*/
void WndMain_Create(SDL_Window *window)
{
	g_sdlMain = window;

	if (!GL_LoadFunctions())
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "radiant", "Some OpenGL 3.3 functions are missing, the views won't draw.", window);
	g_gridRenderer.Init();

	WndMain_CreateToolbar();

	// TODO: wads come from the map's worldspawn "wad" key once the map is ported,
	// for now every wad in the game dir
	char *base = SDL_GetBasePath();
	std::string wadDir = std::string(base ? base : "") + radiantdef.wadPath;
	SDL_free(base);
	Textures::Init(wadDir);
	Textures::LoadWadDir(wadDir);

	WndMain_CreateViews();
	WndMain_UpdateTitle();
	WndMain_UpdateGridStatusBar();
}

/*
==============
WndMain_Destroy
==============
*/
void WndMain_Destroy()
{
	Window **views[] = { &g_wndCamera, &g_wndZ, &g_wndEntity, &g_wndConsole };

	for (Window **v : views)
	{
		delete *v;
		*v = nullptr;
	}
	for (MapView2D *&grid : g_wndGrid)
	{
		delete grid;
		grid = nullptr;
	}
	delete g_wndTexture;
	g_wndTexture = nullptr;

	Textures::Shutdown();
	g_gridRenderer.Shutdown();
	WndMain_DestroyToolbar();
}

/*
==============
WndMain_Draw
==============
*/
void WndMain_Draw()
{
	// bars first, they shrink the viewport's work area the dockspace fills
	WndMain_DrawMenu();
	WndMain_DrawToolbar();
	WndMain_DrawStatusBar();

	ImGuiID dockspace = ImGui::GetID(RADIANT_DOCKSPACE);

	// no saved layout in radiant.ini yet: start with the QE3 default
	if (!ImGui::DockBuilderGetNode(dockspace) && s_pendingLayout < 0)
		s_pendingLayout = 0;

	if (s_pendingLayout >= 0)
	{
		WndMain_ApplyLayout(dockspace, s_pendingLayout);
		s_pendingLayout = -1;
	}
	if (s_pendingSwap)
	{
		WndMain_DoSwapGridCam();
		s_pendingSwap = false;
	}

	ImGui::DockSpaceOverViewport(dockspace);
	Window::DrawAll();

	Textures::HandleChange();
	WndMain_ForceUpdateWindows();
}
