//==============================
//	wndMain_menu.cpp
//==============================

#include "ui/wndMain.h"
#include "ui/window.h"
#include "config.h"
#include "renderer/textures.h"
#include "ui/textures/textureView.h"
#include "ui/view/mapView2D.h"
#include "map/grid.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <vulkan/vulkan.h>

/*
==============================================================================

	MENUS

	same layout as quaked3's IDR_MENU1. commands whose systems aren't ported
	yet (map, selection, tools, views...) are drawn disabled; shortcuts are
	only shown, there are no accelerators yet
==============================================================================
*/

static bool s_openFilterPopup;
static bool s_openAbout;

/*
==============
WndMain_Command

everything the old WM_COMMAND switch did that can work today
==============
*/
bool WndMain_Command(int id)
{
	switch (id)
	{
	//===================================
	// File menu
	//===================================
	case ID_FILE_EXIT:	// exit application
	{
		// TODO: ConfirmModified() once the map is ported
		SDL_Event quit;
		quit.type = SDL_QUIT;
		SDL_PushEvent(&quit);
		break;
	}

	//===================================
	// View menu
	//===================================
	case ID_VIEW_FILTER_POPUP:
		WndMain_ViewFilterPopup();
		break;

	case ID_VIEW_SWAPGRIDCAM:
		WndMain_SwapGridCam();
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		break;

	//===================================
	// Texture menu
	//===================================
	case ID_VIEW_SHOWCOORDINATES:
		MapView2D::showCoordinates ^= true;
		WndMain_UpdateWindows(W_XY | W_Z);
		break;
	case ID_VIEW_SHOWAXIS:
		MapView2D::showAxis ^= true;
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		break;

	// the XY view; the other grid views have their own camera
	case ID_VIEW_NEXTVIEW:
		g_wndGrid[0]->CycleViewPlane();
		break;
	case ID_VIEW_XY:
		g_wndGrid[0]->SetViewPlane(MapView2D::XY);
		break;
	case ID_VIEW_XZ:
		g_wndGrid[0]->SetViewPlane(MapView2D::XZ);
		break;
	case ID_VIEW_YZ:
		g_wndGrid[0]->SetViewPlane(MapView2D::YZ);
		break;
	case ID_VIEW_100:
		g_wndGrid[0]->ResetZoom();
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_ZOOMIN:
		g_wndGrid[0]->ZoomIn();
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_ZOOMOUT:
		g_wndGrid[0]->ZoomOut();
		WndMain_UpdateWindows(W_XY);
		break;

	//===================================
	// Grid Menu
	//===================================
	case ID_GRID_1:
	case ID_GRID_2:
	case ID_GRID_4:
	case ID_GRID_8:
	case ID_GRID_16:
	case ID_GRID_32:
	case ID_GRID_64:
	case ID_GRID_128:
	case ID_GRID_256:
		g_grid.SetSize(id - ID_GRID_1);		// the ids run 1, 2, 4 ... 256: 2^0 to 2^8
		WndMain_UpdateWindows(W_XY | W_Z);
		WndMain_UpdateGridStatusBar();
		break;

	case ID_GRID_TOGGLE:
		g_grid.ToggleVisible();
		WndMain_UpdateWindows(W_Z | W_XY);
		break;

	case ID_GRID_SNAPTOGRID:
		g_grid.ToggleSnap();
		WndMain_UpdateGridStatusBar();
		break;

	case ID_TEXTURES_RELOAD:
		Textures::MenuReloadAll();
		break;
	case ID_TEXTURES_RESETSCALE:
		g_vTexture.SetScale(1.0f);
		WndMain_UpdateWindows(W_TEXTURE);
		break;

	//===================================
	// Window menu
	//===================================
	case ID_VIEW_CAMERA:
		g_wndCamera->Toggle();
		break;

	case ID_VIEW_ENTITY:
		WndMain_SetInspectorMode(W_ENTITY);
		break;
	case ID_VIEW_CONSOLE:
		WndMain_SetInspectorMode(W_CONSOLE);
		break;
	case ID_VIEW_TEXTURE:
		WndMain_SetInspectorMode(W_TEXTURE);
		break;

	case ID_VIEW_TOGGLE_XY:
		g_wndGrid[0]->Toggle();
		break;
	case ID_VIEW_TOGGLE_XZ:
		g_wndGrid[2]->Toggle();
		break;
	case ID_VIEW_TOGGLE_YZ:
		g_wndGrid[1]->Toggle();
		break;
	case ID_VIEW_TOGGLE_Z:
		g_wndZ->Toggle();
		break;

	case ID_VIEW_TOOLBAR_FILEBAND:
	case ID_VIEW_TOOLBAR_EDITBAND:
	case ID_VIEW_TOOLBAR_WINDOWBAND:
	case ID_VIEW_TOOLBAR_EDIT2BAND:
	case ID_VIEW_TOOLBAR_SELECTBAND:
	case ID_VIEW_TOOLBAR_CSGBAND:
	case ID_VIEW_TOOLBAR_MODEBAND:
	case ID_VIEW_TOOLBAR_ENTITYBAND:
	case ID_VIEW_TOOLBAR_BRUSHBAND:
	case ID_VIEW_TOOLBAR_TEXTUREBAND:
	case ID_VIEW_TOOLBAR_VIEWBAND:
	case ID_VIEW_TOOLBAR_MISCBAND:
		g_bToolbarBand[id - ID_VIEW_TOOLBAR_FILEBAND] ^= true;
		break;

	case ID_VIEW_STATUSBAR:
		g_bStatusBar ^= true;
		break;

	case ID_WINDOW_QE3DEFAULT:
	case ID_WINDOW_QE3REVERSE:
	case ID_WINDOW_4WINDOWZCAMLEFT:
	case ID_WINDOW_4WINDOWZCAMRIGHT:
	case ID_WINDOW_4WINDOWNOZCAMLEFT:
	case ID_WINDOW_4WINDOWNOZCAMRIGHT:
	case ID_WINDOW_4REVERSEZCAMLEFT:
	case ID_WINDOW_4REVERSEZCAMRIGHT:
	case ID_WINDOW_4REVERSENOZCAMLEFT:
	case ID_WINDOW_4REVERSENOZCAMRIGHT:
		WndMain_DefaultLayout(id - ID_WINDOW_QE3DEFAULT);
		break;

	//===================================
	// help menu
	//===================================
	case ID_HELP_ABOUT:
		s_openAbout = true;
		break;

	// TODO: the rest of quaked3's WndMain_Command (map, undo, selection, tools,
	// filters, grid, textures, region, dialogs) as each system gets ported
	default:
		return false;
	}

	return true;
}

/*
==============
WndMain_CommandEnabled
==============
*/
bool WndMain_CommandEnabled(int id)
{
	switch (id)
	{
	case ID_FILE_EXIT:
	case ID_VIEW_FILTER_POPUP:
	case ID_VIEW_SWAPGRIDCAM:
	case ID_VIEW_SHOWCOORDINATES:
	case ID_VIEW_SHOWAXIS:
	case ID_VIEW_NEXTVIEW:
	case ID_VIEW_XY:
	case ID_VIEW_XZ:
	case ID_VIEW_YZ:
	case ID_VIEW_100:
	case ID_VIEW_ZOOMIN:
	case ID_VIEW_ZOOMOUT:
	case ID_GRID_TOGGLE:
	case ID_GRID_SNAPTOGRID:
	case ID_TEXTURES_RELOAD:
	case ID_TEXTURES_RESETSCALE:
	case ID_HELP_ABOUT:
		return true;
	}

	if (id >= ID_GRID_1 && id <= ID_GRID_256)
		return true;

	if (id >= ID_VIEW_CAMERA && id <= ID_WINDOW_4REVERSENOZCAMRIGHT)
		return true;
	return false;
}

/*
==============
WndMain_CommandChecked

what WndMain_UpdateMenu used to push into the menu and toolbar, now asked every frame
==============
*/
bool WndMain_CommandChecked(int id)
{
	switch (id)
	{
	case ID_VIEW_CAMERA:	return g_wndCamera->IsOpen();
	case ID_VIEW_TOGGLE_XY:	return g_wndGrid[0]->IsOpen();
	case ID_VIEW_TOGGLE_XZ:	return g_wndGrid[2]->IsOpen();
	case ID_VIEW_TOGGLE_YZ:	return g_wndGrid[1]->IsOpen();
	case ID_VIEW_TOGGLE_Z:	return g_wndZ->IsOpen();
	case ID_VIEW_STATUSBAR:	return g_bStatusBar;

	case ID_VIEW_SHOWCOORDINATES:	return MapView2D::showCoordinates;
	case ID_VIEW_SHOWAXIS:			return MapView2D::showAxis;
	case ID_VIEW_XY:		return g_wndGrid[0]->Plane() == MapView2D::XY;
	case ID_VIEW_XZ:		return g_wndGrid[0]->Plane() == MapView2D::XZ;
	case ID_VIEW_YZ:		return g_wndGrid[0]->Plane() == MapView2D::YZ;
	case ID_GRID_TOGGLE:	return g_grid.Visible();
	case ID_GRID_SNAPTOGRID:	return g_grid.Snap();
	}

	if (id >= ID_GRID_1 && id <= ID_GRID_256)
		return g_grid.Size() == id - ID_GRID_1;

	if (id >= ID_VIEW_TOOLBAR_FILEBAND && id <= ID_VIEW_TOOLBAR_MISCBAND)
		return g_bToolbarBand[id - ID_VIEW_TOOLBAR_FILEBAND];
	return false;
}

/*
==============
Item
==============
*/
static void Item(const char *label, const char *shortcut, int id)
{
	if (ImGui::MenuItem(label, shortcut, WndMain_CommandChecked(id), WndMain_CommandEnabled(id)))
		WndMain_Command(id);
}

/*
==============
WndMain_DrawFilterItems

shared by View > Filter and the toolbar's filter popup
==============
*/
static void WndMain_DrawFilterItems()
{
	Item("Show Clip", nullptr, ID_VIEW_SHOWCLIP);
	Item("Show Hint", nullptr, ID_VIEW_SHOWHINT);
	Item("Show Sky", nullptr, ID_VIEW_SHOWSKY);
	Item("Show Liquid", nullptr, ID_VIEW_SHOWWATER);
	ImGui::Separator();
	Item("Show Detail", nullptr, ID_VIEW_SHOWDETAIL);
	Item("Show Func Wall", nullptr, ID_VIEW_SHOWFUNCWALL);
	Item("Show Lights", nullptr, ID_VIEW_SHOWLIGHTS);
	Item("Show Monsters", nullptr, ID_VIEW_SHOWMONSTERS);
	Item("Show Triggers", nullptr, ID_VIEW_SHOWTRIGGERS);
	ImGui::Separator();
	Item("Show World", nullptr, ID_VIEW_SHOWWORLD);
	Item("Show Brush Entities", nullptr, ID_VIEW_SHOWBRUSHENTS);
	Item("Show Point Entities", nullptr, ID_VIEW_SHOWPOINTENTS);
	ImGui::Separator();
	Item("Show All Skills", nullptr, ID_FILTER_SHOWALLSKILLS);
	Item("Show Easy Skill", nullptr, ID_FILTER_SHOWEASYSKILL);
	Item("Show Medium Skill", nullptr, ID_FILTER_SHOWMEDIUMSKILL);
	Item("Show Hard Skill", nullptr, ID_FILTER_SHOWHARDSKILL);
	Item("Show Deathmatch", nullptr, ID_FILTER_SHOWDEATHMATCH);
}

/*
============
WndMain_ViewFilterPopup

opened at the mouse on the next WndMain_DrawMenu
============
*/
void WndMain_ViewFilterPopup()
{
	s_openFilterPopup = true;
}

/*
==================
WndMain_RotForModifiers
==================
*/
int WndMain_RotForModifiers()
{
	SDL_Keymod mod = SDL_GetModState();
	if (mod & KMOD_SHIFT) return 180;
	if (mod & KMOD_CTRL) return -90;
	return 90;
}

/*
==================
WndMain_DrawAbout
==================
*/
static void WndMain_DrawAbout()
{
	if (s_openAbout)
	{
		ImGui::OpenPopup("About##radiant");
		s_openAbout = false;
	}

	if (!ImGui::BeginPopupModal("About##radiant", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	SDL_version sdlVersion;
	SDL_GetVersion(&sdlVersion);
	const char *glVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	const char *glRenderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
	uint32_t vkVersion = 0;
	VkResult vkResult = vkEnumerateInstanceVersion(&vkVersion);

	ImGui::TextUnformatted(radiantdef.title);
	ImGui::TextDisabled("based on QuakeEd 3");
	ImGui::Separator();
	ImGui::Text("SDL     %d.%d.%d", sdlVersion.major, sdlVersion.minor, sdlVersion.patch);
	ImGui::Text("OpenGL  %s", glVersion ? glVersion : "?");
	ImGui::Text("        %s", glRenderer ? glRenderer : "?");
	if (vkResult == VK_SUCCESS)
		ImGui::Text("Vulkan  %u.%u.%u", VK_API_VERSION_MAJOR(vkVersion), VK_API_VERSION_MINOR(vkVersion), VK_API_VERSION_PATCH(vkVersion));
	else
		ImGui::Text("Vulkan  not available (%d)", static_cast<int>(vkResult));
	ImGui::Text("ImGui   %s", IMGUI_VERSION);
	ImGui::Separator();
	if (ImGui::Button("OK", ImVec2(120, 0)))
		ImGui::CloseCurrentPopup();
	ImGui::EndPopup();
}

/*
==================
WndMain_DrawMenu
==================
*/
void WndMain_DrawMenu()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			Item("New", "Ctrl+N", ID_FILE_NEW);
			Item("Open...", "Ctrl+O", ID_FILE_OPEN);
			Item("Save", "Ctrl+S", ID_FILE_SAVE);
			Item("Save As...", "Ctrl+Shift+S", ID_FILE_SAVEAS);
			ImGui::Separator();
			Item("Import...", nullptr, ID_FILE_IMPORTMAP);
			Item("Export Selection...", nullptr, ID_SELECTION_EXPORTMAP);
			ImGui::Separator();
			Item("Pointfile", nullptr, ID_FILE_POINTFILE);
			ImGui::Separator();
			// TODO: MRU list goes here
			Item("Exit", nullptr, ID_FILE_EXIT);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			Item("Undo", "Ctrl+Z", ID_EDIT_UNDO);
			Item("Redo", "Ctrl+Y", ID_EDIT_REDO);
			ImGui::Separator();
			Item("Cut", "Ctrl+X", ID_EDIT_CUT);
			Item("Copy", "Ctrl+C", ID_EDIT_COPY);
			Item("Paste", "Ctrl+V", ID_EDIT_PASTE);
			ImGui::Separator();
			Item("Clone", "Space", ID_SELECTION_CLONE);
			Item("Delete", "Backspace", ID_SELECTION_DELETE);
			ImGui::Separator();
			Item("Find...", "Ctrl+F", ID_EDIT_FINDBRUSH);
			Item("Map Info...", "F2", ID_EDIT_MAPINFO);
			Item("Entity Info...", "F3", ID_EDIT_ENTITYINFO);
			ImGui::Separator();
			Item("Preferences...", "F4", ID_EDIT_PREFERENCES);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::BeginMenu("Filter"))
			{
				WndMain_DrawFilterItems();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Hide/Show"))
			{
				Item("Hide Selected", "H", ID_VIEW_HIDESHOW_HIDESELECTED);
				Item("Hide Unselected", "Shift+H", ID_VIEW_HIDESHOW_HIDEUNSELECTED);
				Item("Show Hidden", "Ctrl+H", ID_VIEW_HIDESHOW_SHOWHIDDEN);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Toggle"))
			{
				Item("Show Axis", nullptr, ID_VIEW_SHOWAXIS);
				Item("Show Angles", nullptr, ID_VIEW_SHOWANGLES);
				Item("Show Blocks", nullptr, ID_VIEW_SHOWBLOCKS);
				Item("Show Camera Grid", nullptr, ID_VIEW_SHOWCAMERAGRID);
				Item("Show Coordinates", nullptr, ID_VIEW_SHOWCOORDINATES);
				Item("Show Light Radius", nullptr, ID_VIEW_SHOWLIGHTRADIUS);
				Item("Show Map Boundary", nullptr, ID_VIEW_SHOWMAPBOUNDARY);
				Item("Show Names", nullptr, ID_VIEW_SHOWNAMES);
				Item("Show Sizing Info", nullptr, ID_VIEW_SHOWSIZEINFO);
				Item("Show View Name", nullptr, ID_VIEW_SHOWVIEWNAME);
				Item("Show Workzone", nullptr, ID_VIEW_SHOWWORKZONE);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Target Lines"))
			{
				Item("All", nullptr, ID_TARGETLINES_ALL);
				Item("Selection", nullptr, ID_TARGETLINES_SEL);
				Item("Selection + Paths", nullptr, ID_TARGETLINES_SELPATH);
				Item("None", nullptr, ID_TARGETLINES_NONE);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			Item("Center", "End", ID_VIEW_CENTER);
			Item("Up Floor", "Page Up", ID_VIEW_UPFLOOR);
			Item("Down Floor", "Page Down", ID_VIEW_DOWNFLOOR);
			ImGui::Separator();
			Item("Next (XY->XZ->YZ)", "Tab", ID_VIEW_NEXTVIEW);
			Item("Swap Grid/Cam", "Shift+Tab", ID_VIEW_SWAPGRIDCAM);
			if (ImGui::BeginMenu("Layout"))
			{
				Item("XY (Top)", nullptr, ID_VIEW_XY);
				Item("XZ (Front)", nullptr, ID_VIEW_XZ);
				Item("YZ (Side)", nullptr, ID_VIEW_YZ);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Zoom"))
			{
				Item("XY 100%", "Home", ID_VIEW_100);
				Item("XY Zoom In", "Insert", ID_VIEW_ZOOMIN);
				Item("XY Zoom Out", "Delete", ID_VIEW_ZOOMOUT);
				ImGui::Separator();
				Item("Z 100%", "Ctrl+Home", ID_VIEW_Z100);
				Item("Z Zoom In", "Ctrl+Insert", ID_VIEW_ZZOOMIN);
				Item("Z Zoom Out", "Ctrl+Delete", ID_VIEW_ZZOOMOUT);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			Item("Center On Selection", "\\", ID_VIEW_CENTERONSELECTION);
			Item("Next Leak Spot", "Ctrl+>", ID_MISC_NEXTLEAKSPOT);
			Item("Previous Leak Spot", "Ctrl+<", ID_MISC_PREVIOUSLEAKSPOT);
			ImGui::Separator();
			if (ImGui::BeginMenu("Cubic Clipping"))
			{
				Item("Cubic Clip", "Ctrl+\\", ID_VIEW_CUBICCLIP);
				ImGui::Separator();
				Item("Cubic Clip Nearer", "Ctrl+[", ID_VIEW_CUBICCLIPNEARER);
				Item("Cubic Clip Farther", "Ctrl+]", ID_VIEW_CUBICCLIPFARTHER);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Draw Mode"))
			{
				Item("Wireframe", nullptr, ID_DRAWMODE_WIREFRAME);
				Item("Flat Color", nullptr, ID_DRAWMODE_FLATSHADE);
				Item("Textured", nullptr, ID_DRAWMODE_TEXTURED);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Selection"))
		{
			Item("Complete Tall", "Alt+C", ID_SELECTION_SELECTCOMPLETETALL);
			Item("Touching", "Alt+T", ID_SELECTION_SELECTTOUCHING);
			Item("Partial Tall", "Alt+P", ID_SELECTION_SELECTPARTIALTALL);
			Item("Inside", "Alt+I", ID_SELECTION_SELECTINSIDE);
			ImGui::Separator();
			Item("Select All", "Ctrl+A", ID_SELECTION_SELECTALL);
			Item("Deselect/Cancel", "Esc", ID_SELECTION_DESELECT);
			Item("Invert", "I", ID_SELECTION_INVERT);
			Item("Next Brush In Group", "Ctrl+Tab", ID_SELECTION_GROUPNEXTBRUSH);
			ImGui::Separator();
			Item("Select All of Type", "Ctrl+Shift+A", ID_SELECTION_SELECTALLTYPE);
			Item("Select Matching Textures", "Ctrl+T", ID_SELECTION_SELECTMATCHINGTEXTURES);
			Item("Select by Key/Value", "Ctrl+E", ID_SELECTION_SELECTMATCHINGKEYVALUE);
			ImGui::Separator();
			if (ImGui::BeginMenu("Convert"))
			{
				Item("Brushes to Faces", nullptr, ID_SELECTION_BRUSHESTOFACES);
				Item("Faces to Brushes (Complete)", nullptr, ID_SELECTION_FACESTOBRUSHESCOMPLETE);
				Item("Faces to Brushes (Partial)", nullptr, ID_SELECTION_FACESTOBRUSHESPARTIAL);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			Item("Draw Brushes Tool", "B", ID_TOOLS_DRAWBRUSHESTOOL);
			if (ImGui::BeginMenu("Clipper"))
			{
				Item("Toggle Clipper", "X", ID_SELECTION_CLIPPER);
				ImGui::Separator();
				Item("Clip Selection", "Enter", ID_SELECTION_CLIPSELECTED);
				Item("Split Selection", "Shift+Enter", ID_SELECTION_SPLITSELECTED);
				Item("Flip Clip Orientation", "Ctrl+Enter", ID_SELECTION_FLIPCLIP);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			Item("Drag Edges", "E", ID_SELECTION_DRAGEDGES);
			Item("Drag Vertices", "V", ID_SELECTION_DRAGVERTICES);
			Item("Drag Faces", "F", ID_SELECTION_DRAGFACES);
			ImGui::Separator();
			Item("Select Entity Color...", "K", ID_MISC_SELECTENTITYCOLOR);
			Item("Set Entity Key/Values...", nullptr, ID_TOOLS_SETENTITYKEYS);
			Item("Connect Entities", "Ctrl+K", ID_SELECTION_CONNECT);
			Item("Move to World", "Ctrl+U", ID_SELECTION_UNGROUPENTITY);
			Item("Move to Entity", "Ctrl+I", ID_SELECTION_INSERTBRUSH);
			ImGui::Separator();
			if (ImGui::BeginMenu("Flip"))
			{
				Item("Flip X", nullptr, ID_SELECTION_FLIPX);
				Item("Flip Y", nullptr, ID_SELECTION_FLIPY);
				Item("Flip Z", nullptr, ID_SELECTION_FLIPZ);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Rotate"))
			{
				Item("Rotate X", nullptr, ID_SELECTION_ROTATEX);
				Item("Rotate Y", nullptr, ID_SELECTION_ROTATEY);
				Item("Rotate Z", nullptr, ID_SELECTION_ROTATEZ);
				ImGui::Separator();
				Item("Arbitrary Rotation...", "R", ID_SELECTION_ARBITRARYROTATION);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Scale"))
			{
				Item("Scale Lock X", nullptr, ID_SELECTION_SCALELOCKX);
				Item("Scale Lock Y", nullptr, ID_SELECTION_SCALELOCKY);
				Item("Scale Lock Z", nullptr, ID_SELECTION_SCALELOCKZ);
				ImGui::Separator();
				Item("Scale...", nullptr, ID_SELECTION_SCALE);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu("CSG"))
			{
				Item("CSG Hollow", "Alt+H", ID_SELECTION_CSGHOLLOW);
				Item("CSG Subtract", "Alt+S", ID_SELECTION_CSGSUBTRACT);
				Item("CSG Merge", "M", ID_SELECTION_CSGMERGE);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu("Primitives"))
			{
				Item("Czg Cylinder (12)", nullptr, ID_PRIMITIVES_CZGCYLINDER1);
				Item("Czg Cylinder (24)", nullptr, ID_PRIMITIVES_CZGCYLINDER2);
				ImGui::Separator();
				Item("Cylinder...", nullptr, ID_BRUSH_CYLINDER);
				Item("Cone...", nullptr, ID_BRUSH_CONE);
				Item("Sphere...", nullptr, ID_BRUSH_SPHERE);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Grid"))
		{
			Item("Grid 1", "1", ID_GRID_1);
			Item("Grid 2", "2", ID_GRID_2);
			Item("Grid 4", "3", ID_GRID_4);
			Item("Grid 8", "4", ID_GRID_8);
			Item("Grid 16", "5", ID_GRID_16);
			Item("Grid 32", "6", ID_GRID_32);
			Item("Grid 64", "7", ID_GRID_64);
			Item("Grid 128", "8", ID_GRID_128);
			Item("Grid 256", "9", ID_GRID_256);
			ImGui::Separator();
			Item("Show Grid", "0", ID_GRID_TOGGLE);
			ImGui::Separator();
			Item("Snap To Grid", nullptr, ID_GRID_SNAPTOGRID);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Textures"))
		{
			Item("Surface Inspector...", "S", ID_TEXTURES_INSPECTOR);
			Item("Texture Lock", "L", ID_TEXTURES_LOCK);
			Item("Find / Replace...", "Ctrl+R", ID_TEXTURES_REPLACEALL);
			ImGui::Separator();
			Item("Default Texture Scale...", nullptr, ID_TEXTURES_DEFAULTSCALE);
			ImGui::Separator();
			Item("Hide Unused", "U", ID_TEXTURES_HIDEUNUSED);
			Item("Reload Wads", nullptr, ID_TEXTURES_RELOAD);
			ImGui::Separator();
			Item("Reset Window Scale", nullptr, ID_TEXTURES_RESETSCALE);
			ImGui::Separator();
			Item("Load/Add Wad...", nullptr, ID_TEXTURES_ADDWAD);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Region"))
		{
			Item("Off", "Alt+O", ID_REGION_OFF);
			ImGui::Separator();
			Item("Set Selection", nullptr, ID_REGION_SETSELECTION);
			Item("Set XY", "Alt+X", ID_REGION_SETXY);
			Item("Set XZ", "Alt+Z", ID_REGION_SETXZ);
			Item("Set YZ", "Alt+Y", ID_REGION_SETYZ);
			Item("Set Brush", "Alt+B", ID_REGION_SETBRUSH);
			Item("Set Brush Tall", nullptr, ID_REGION_SETTALLBRUSH);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			Item("Camera View", nullptr, ID_VIEW_CAMERA);
			Item("Console View", "O", ID_VIEW_CONSOLE);
			Item("Entity View", "N", ID_VIEW_ENTITY);
			Item("Texture View", "T", ID_VIEW_TEXTURE);
			Item("XY View (Top)", "Alt+1", ID_VIEW_TOGGLE_XY);
			Item("XZ View (Front)", "Alt+2", ID_VIEW_TOGGLE_XZ);
			Item("YZ View (Side)", "Alt+3", ID_VIEW_TOGGLE_YZ);
			Item("Z View", "Alt+4", ID_VIEW_TOGGLE_Z);
			ImGui::Separator();
			if (ImGui::BeginMenu("Tool Bar"))
			{
				Item("File Band", nullptr, ID_VIEW_TOOLBAR_FILEBAND);
				Item("Edit Band", nullptr, ID_VIEW_TOOLBAR_EDITBAND);
				Item("Window Band", nullptr, ID_VIEW_TOOLBAR_WINDOWBAND);
				Item("Transform Band", nullptr, ID_VIEW_TOOLBAR_EDIT2BAND);
				Item("Select Band", nullptr, ID_VIEW_TOOLBAR_SELECTBAND);
				Item("CSG Band", nullptr, ID_VIEW_TOOLBAR_CSGBAND);
				Item("Tool Band", nullptr, ID_VIEW_TOOLBAR_MODEBAND);
				Item("Entity Band", nullptr, ID_VIEW_TOOLBAR_ENTITYBAND);
				Item("Brush Band", nullptr, ID_VIEW_TOOLBAR_BRUSHBAND);
				Item("Texture Band", nullptr, ID_VIEW_TOOLBAR_TEXTUREBAND);
				Item("View Band", nullptr, ID_VIEW_TOOLBAR_VIEWBAND);
				Item("Misc Band", nullptr, ID_VIEW_TOOLBAR_MISCBAND);
				ImGui::EndMenu();
			}
			Item("Status Bar", nullptr, ID_VIEW_STATUSBAR);
			ImGui::Separator();
			if (ImGui::BeginMenu("Presets"))
			{
				Item("QE3 Default", nullptr, ID_WINDOW_QE3DEFAULT);
				Item("QE3 Reverse", nullptr, ID_WINDOW_QE3REVERSE);
				ImGui::Separator();
				Item("4 View w/ Z (Cam Left)", nullptr, ID_WINDOW_4WINDOWZCAMLEFT);
				Item("4 View w/ Z (Cam Right)", nullptr, ID_WINDOW_4WINDOWZCAMRIGHT);
				Item("4 View w/o Z (Cam Left)", nullptr, ID_WINDOW_4WINDOWNOZCAMLEFT);
				Item("4 View w/o Z (Cam Right)", nullptr, ID_WINDOW_4WINDOWNOZCAMRIGHT);
				ImGui::Separator();
				Item("4 View Reverse w/ Z (Cam Left)", nullptr, ID_WINDOW_4REVERSEZCAMLEFT);
				Item("4 View Reverse w/ Z (Cam Right)", nullptr, ID_WINDOW_4REVERSEZCAMRIGHT);
				Item("4 View Reverse w/o Z (Cam Left)", nullptr, ID_WINDOW_4REVERSENOZCAMLEFT);
				Item("4 View Reverse w/o Z (Cam Right)", nullptr, ID_WINDOW_4REVERSENOZCAMRIGHT);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			Item("Help Topics...", "F1", ID_HELP_HELP);
			ImGui::Separator();
			Item("Key Commands...", nullptr, ID_HELP_KEYLIST);
			Item("Mouse Commands...", nullptr, ID_HELP_MOUSELIST);
			ImGui::Separator();
			Item("About...", nullptr, ID_HELP_ABOUT);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// popups live at the top of the id stack so commands from anywhere can open them
	if (s_openFilterPopup)
	{
		ImGui::OpenPopup("ViewFilter");
		s_openFilterPopup = false;
	}
	if (ImGui::BeginPopup("ViewFilter"))
	{
		WndMain_DrawFilterItems();
		ImGui::EndPopup();
	}

	WndMain_DrawAbout();
}
