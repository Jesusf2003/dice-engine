//==============================
//	wndMain.cpp
//==============================

#include "ui/wndMain.h"
#include "ui/window.h"
#include "config.h"
#include "renderer/textures.h"
#include "ui/textures/wndTextures.h"
#include "ui/view/mapView2D.h"
#include "ui/view/wndCamera.h"
#include "ui/view/mapViewZ.h"
#include "ui/view/wndViewports.h"
#include "ui/console/wndConsole.h"
#include "ui/preferences/wndPreferences.h"
#include "core/common/log.h"
#include "core/common/preferences.h"
#include "core/utils/fileDialog.h"
#include "map/map.h"
#include "map/entityDefs.h"
#include "core/common/gameConfig.h"
#include "ui/options/wndOptions.h"
#include "renderer/glFuncs.h"
#include "renderer/gridRenderer.h"
#include "renderer/mapRenderer.h"

#include <SDL.h>
#include <imgui_internal.h>	// DockBuilder

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

SDL_Window	*g_sdlMain;

WndCamera	*g_wndCamera;
MapView2D	*g_wndGrid[4];
MapViewZ	*g_wndZ;
WndTexture	*g_wndTexture;
Window	*g_wndEntity;
WndConsole	*g_wndConsole;

int		g_nUpdateBits = 0;
int		g_nInspectorMode;		// W_TEXTURE, W_ENTITY, or W_CONSOLE

static int	s_pendingLayout = -1;
static bool	s_pendingSwap = false;
static int	s_layout = -1;			// the last preset applied, saved in radiant.ini. -1: none yet
static std::string	s_pendingMap;		// from the command line, opened once radiant.ini (the log detail) is read
static bool	s_pendingGame = true;		// entity definitions and wads, on the first frame for the same reason

static const int LAYOUT_HAMMER = ID_WINDOW_HAMMER - ID_WINDOW_QE3DEFAULT;

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
		if (embedded)	// clear of the 4 view's label
			ImGui::SetCursorPos(ImVec2(8, ImGui::GetFrameHeight() + 4));
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
	if (WndMain_HammerLayout())
		g_wndViewports->SwapGridCam();	// not docked, nothing to wait for
	else
		s_pendingSwap = true;
}

bool WndMain_HammerLayout()
{
	return g_wndViewports && g_wndViewports->IsOpen();
}

/*
==================
WndMain_ShowHammerViews

the 4 view grid paints the camera and the grids itself, their own windows
stay hidden so they aren't drawn twice
==================
*/
static void WndMain_ShowHammerViews(bool show)
{
	if (!show)
	{
		g_wndViewports->Hide();
		return;
	}

	g_wndViewports->Show();
	g_wndCamera->Hide();
	g_wndZ->Hide();
	for (int i = 0; i < 3; i++)
		g_wndGrid[i]->Hide();
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
		g_wndConsole->ScrollToEnd();
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

	WndMain_ShowHammerViews(false);

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

	case LAYOUT_HAMMER:		// inspector on the right, everything else one 2x2 grid
	{
		insp = ImGui::DockBuilderSplitNode(rest, ImGuiDir_Right, 320 / w, nullptr, &rest);
		ImGui::DockBuilderDockWindow(g_wndViewports->Id(), rest);
		// no tab bar, the cells carry their own labels
		ImGui::DockBuilderGetNode(rest)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
		DockInspector(insp);

		g_wndViewports->Reset();
		WndMain_ShowHammerViews(true);
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

	s_layout = nStyle;
	ImGui::MarkIniSettingsDirty();
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
	g_wndConsole = new WndConsole();
	g_wndConsole->Create();

	g_wndCamera = new WndCamera();
	g_wndCamera->Create();
	for (int i = 0; i < 3; i++)
	{
		static const MapView2D::ViewPlane planes[3] = { MapView2D::XY, MapView2D::YZ, MapView2D::XZ };
		g_wndGrid[i] = new MapView2D(planes[i], i);
		g_wndGrid[i]->Create();
	}
	g_wndZ = new MapViewZ();
	g_wndZ->Create();

	// after the camera and grids, its cells point at them
	g_wndViewports = new WndViewports();
	g_wndViewports->Create();
	g_wndViewports->Hide();

	g_wndTexture = new WndTexture();
	g_wndTexture->Initialize();
	g_wndEntity = new WndPlaceholder("Entity", "Entity");
	g_wndEntity->Create();

	g_nInspectorMode = W_CONSOLE;
}



/*
=============================================================

LAYOUT SETTINGS

the dock layout is ImGui's own, this is what it can't know: which preset is
up, the 4 view's splitters and cells and the camera's draw mode. a
[Radiant][Layout] entry in radiant.ini
=============================================================
*/

static struct
{
	float	splitX = 0.5f, splitY = 0.5f;
	int		cells[4] = { 0, 1, 2, 3 };
} s_savedViews;

// copied out while g_wndViewports is alive, ImGui writes the ini once more after it's gone
static void SnapshotViewports()
{
	if (!g_wndViewports)
		return;
	s_savedViews.splitX = g_wndViewports->splitX;
	s_savedViews.splitY = g_wndViewports->splitY;
	for (int i = 0; i < 4; i++)
		s_savedViews.cells[i] = g_wndViewports->CellView(i);
}

static void *Settings_ReadOpen(ImGuiContext *, ImGuiSettingsHandler *, const char *name)
{
	return strcmp(name, "Layout") == 0 ? (void *)&s_savedViews : nullptr;
}

static void Settings_ReadLine(ImGuiContext *, ImGuiSettingsHandler *, void *, const char *line)
{
	char *p;

	if (strncmp(line, "Style=", 6) == 0)
		s_layout = (int)strtol(line + 6, nullptr, 10);
	else if (strncmp(line, "Split=", 6) == 0)
	{
		s_savedViews.splitX = strtof(line + 6, &p);
		if (*p == ',')
			s_savedViews.splitY = strtof(p + 1, nullptr);
	}
	else if (strncmp(line, "CameraMode=", 11) == 0)
		WndCamera::drawMode = (MapRenderer::DrawMode)std::clamp((int)strtol(line + 11, nullptr, 10), 0, MapRenderer::DRAWMODE_COUNT - 1);
	else if (strncmp(line, "Cells=", 6) == 0)
	{
		p = (char *)line + 5;
		for (int i = 0; i < 4 && (*p == '=' || *p == ','); i++)
			s_savedViews.cells[i] = (int)strtol(p + 1, &p, 10);
	}
}

static void Settings_ApplyAll(ImGuiContext *, ImGuiSettingsHandler *)
{
	g_wndViewports->splitX = s_savedViews.splitX;
	g_wndViewports->splitY = s_savedViews.splitY;
	g_wndViewports->SetCellViews(s_savedViews.cells);
	if (s_layout == LAYOUT_HAMMER)
		WndMain_ShowHammerViews(true);
}

static void Settings_WriteAll(ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
{
	SnapshotViewports();
	buf->appendf("[%s][Layout]\n", handler->TypeName);
	buf->appendf("Style=%d\n", s_layout);
	buf->appendf("Split=%.4f,%.4f\n", s_savedViews.splitX, s_savedViews.splitY);
	buf->appendf("Cells=%d,%d,%d,%d\n", s_savedViews.cells[0], s_savedViews.cells[1], s_savedViews.cells[2], s_savedViews.cells[3]);
	buf->appendf("CameraMode=%d\n", (int)WndCamera::drawMode);
	buf->append("\n");
}

// before the first NewFrame, that's when radiant.ini is read
static void WndMain_RegisterSettings()
{
	ImGuiSettingsHandler handler;
	handler.TypeName = "Radiant";
	handler.TypeHash = ImHashStr("Radiant");
	handler.ReadOpenFn = Settings_ReadOpen;
	handler.ReadLineFn = Settings_ReadLine;
	handler.ApplyAllFn = Settings_ApplyAll;
	handler.WriteAllFn = Settings_WriteAll;
	ImGui::AddSettingsHandler(&handler);
}



/*
=============================================================

MAIN WINDOW

=============================================================
*/

/*
==================
WndMain_OpenMap
==================
*/
void WndMain_OpenMap(const std::string &path)
{
	if (!g_map.Load(path))
	{
		WndMain_Status("Map not loaded, see the console", 0);
		return;
	}
	g_wndCamera->ResetForMap();

	char status[128];
	std::snprintf(status, sizeof(status), "%s: %zu entities, %d brushes", g_map.Name().c_str(),
		g_map.entities.size(), g_map.stats.worldBrushes + g_map.stats.entityBrushes);
	WndMain_Status(status, 0);
	WndMain_UpdateTitle();
}

void WndMain_QueueOpenMap(const std::string &path)
{
	s_pendingMap = path;
}

/*
==================
WndMain_OpenMapDialog

starts where the current map is, or in the game's maps folder
==================
*/
void WndMain_OpenMapDialog()
{
	namespace fs = std::filesystem;
	std::string dir;

	const gameConfig_t *game = GameConfigs::Active();
	if (!g_map.filename.empty())
		dir = fs::u8path(g_map.filename).parent_path().u8string();
	else if (game)
		dir = GameConfigs::MapsDir(*game);

	std::string path;
	if (FileDialog::OpenFile(path, "Open Map", "Quake map", "*.map", dir))
		WndMain_OpenMap(path);
}

/*
==================
WndMain_LoadGame

the active game configuration (Tools > Options): its folders become the texture
search paths, then its entity definitions, then the wads their wadlist names
(every wad in the game folder when there's no wadlist). with no configurations
at all the games next to the editor are detected; with none found, Options opens
==================
*/
static void WndMain_NewMapDefaults()
{
	std::string wads = g_entityDefs.DefaultWads();
	if (!wads.empty() && g_map.World())
		g_map.World()->SetKey("wad", wads);
}

static const gameConfig_t *PickGame()
{
	if (const gameConfig_t *cfg = GameConfigs::Active())
		return cfg;

	if (!GameConfigs::configs.empty())
	{
		Log::Warning("Game configuration \"%s\" doesn't exist, using \"%s\"",
			GameConfigs::activeName.c_str(), GameConfigs::configs.front().name.c_str());
		GameConfigs::Apply(GameConfigs::configs, GameConfigs::configs.front().name);
		return GameConfigs::Active();
	}

	std::vector<gameConfig_t> found = GameConfigs::Detect();
	if (found.empty())
		return nullptr;

	std::string names;
	for (const gameConfig_t &g : found)
		names += (names.empty() ? "" : ", ") + g.name;
	Log::Info("No game configurations yet, detected %zu next to the editor: %s (Tools > Options to change them)",
		found.size(), names.c_str());
	GameConfigs::Apply(found, found.front().name);
	return GameConfigs::Active();
}

void WndMain_LoadGame()
{
	const gameConfig_t *cfg = PickGame();
	if (!cfg)
	{
		Log::Warning("No game configuration: set one up in Tools > Options");
		Textures::SetSearchPaths({});
		g_entityDefs = EntityDefs();
		WndOptions::Open();
		WndMain_UpdateTitle();
		return;
	}

	Log::Info("---- Game: %s ----", cfg->name.c_str());

	std::vector<std::string> dirs = GameConfigs::SearchPaths(*cfg);
	if (dirs.empty())
		Log::Warning("%s has no game directory (Tools > Options)", cfg->name.c_str());
	for (const std::string &d : dirs)
	{
		std::error_code ec;
		if (!std::filesystem::is_directory(std::filesystem::u8path(d), ec))
			Log::Warning("%s: folder %s doesn't exist", cfg->name.c_str(), d.c_str());
		else
			Log::Verbose("  search path %s", d.c_str());
	}
	Textures::SetSearchPaths(dirs);

	std::vector<std::string> fgds;
	for (const std::string &f : cfg->fgds)
		fgds.push_back(GameConfigs::Resolve(f));
	if (fgds.empty())
		Log::Warning("%s has no game data files (FGD), Tools > Options", cfg->name.c_str());

	std::vector<std::string> wads;
	if (g_entityDefs.Load(fgds))
		wads = g_entityDefs.WadList();

	if (!wads.empty())
	{
		for (const std::string &w : wads)
			Textures::LoadWad(w);
	}
	else if (!dirs.empty())
	{
		Log::Warning("No wadlist in the entity definitions, loading every wad in %s", dirs.front().c_str());
		Textures::LoadWadDir(dirs.front());
	}
	Textures::LibraryChanged();

	if (g_map.filename.empty())
		WndMain_NewMapDefaults();
	WndMain_UpdateTitle();
}

void WndMain_NewMap()
{
	// TODO: ConfirmModified() once maps can be edited
	g_map.New();
	g_wndCamera->ResetForMap();
	WndMain_NewMapDefaults();
	Log::Info("New map");
	WndMain_Status("New map", 0);
	WndMain_UpdateTitle();
}

/*
==================
WndMain_UpdateTitle
==================
*/
void WndMain_UpdateTitle()
{
	char title[256];

	// TODO: the modified flag once the map can be edited
	const gameConfig_t *game = GameConfigs::Active();
	std::snprintf(title, sizeof(title), "%s - [%s%s] - %s", radiantdef.title, g_map.Name().c_str(), "",
		game ? game->name.c_str() : "no game");
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
	Log::Init();	// first, so the console gets the texture and GL messages too
	Log::Info("%s %s", radiantdef.title, radiantdef.version);

	if (!GL_LoadFunctions())
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "radiant", "Some OpenGL 3.3 functions are missing, the views won't draw.", window);
	g_gridRenderer.Init();
	g_mapRenderer.Init();

	WndMain_CreateToolbar();

	// the game (folders, definitions, wads) loads on the first frame, WndMain_LoadGame
	Textures::Init();

	WndMain_CreateViews();
	WndMain_RegisterSettings();
	Preferences::RegisterSettings();
	GameConfigs::RegisterSettings();
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
	SnapshotViewports();
	delete g_wndViewports;
	g_wndViewports = nullptr;

	delete g_wndZ;
	g_wndZ = nullptr;

	delete g_wndConsole;
	g_wndConsole = nullptr;

	delete g_wndCamera;
	g_wndCamera = nullptr;
	delete g_wndEntity;
	g_wndEntity = nullptr;
	for (MapView2D *&grid : g_wndGrid)
	{
		delete grid;
		grid = nullptr;
	}
	delete g_wndTexture;
	g_wndTexture = nullptr;

	Textures::Shutdown();
	g_gridRenderer.Shutdown();
	g_mapRenderer.Shutdown();
	WndMain_DestroyToolbar();
	Log::Shutdown();
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

	// no saved layout in radiant.ini yet (or one from before the presets were saved):
	// start with hammer's 4 view, the main layout
	if ((!ImGui::DockBuilderGetNode(dockspace) || s_layout < 0) && s_pendingLayout < 0)
		s_pendingLayout = LAYOUT_HAMMER;

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
	WndPreferences::Draw();
	WndOptions::Draw();

	if (s_pendingGame)
	{
		WndMain_LoadGame();
		s_pendingGame = false;
	}
	if (!s_pendingMap.empty())
	{
		std::string path;
		path.swap(s_pendingMap);
		WndMain_OpenMap(path);
	}
	if (Log::TakeErrorFlag() && g_prefs.console.showOnError)
		WndMain_SetInspectorMode(W_CONSOLE);

	Textures::HandleChange();
	WndMain_ForceUpdateWindows();
}
