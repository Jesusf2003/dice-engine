//==============================
//	wndViewports.cpp
//==============================

#include "ui/view/wndViewports.h"
#include "ui/view/mapView2D.h"
#include "ui/view/wndCamera.h"
#include "renderer/mapRenderer.h"
#include "ui/wndMain.h"

#include <imgui_internal.h>	// MarkIniSettingsDirty

#include <algorithm>
#include <cmath>
#include <cstdio>

WndViewports	*g_wndViewports;

#define SPLITTER_SIZE	4.0f		// the gap between cells, also the bars' grab area
#define MIN_CELL_SIZE	48.0f		// hammertime keeps 2% per view, pixels read better here

// hammer's names: front looks down x, side looks down y
static const char *s_viewLabels[4] = { "camera", "top (x/y)", "front (y/z)", "side (x/z)" };
static const MapView2D::ViewPlane s_viewPlanes[4] = { MapView2D::XY, MapView2D::XY, MapView2D::YZ, MapView2D::XZ };

WndViewports::WndViewports() :
	splitX(0.5f), splitY(0.5f),
	maximized(-1), hoveredCell(-1), activeCell(1),
	pendingCell(-1), pendingType(VIEW_CAMERA), pendingMode(-1)
{
	name = "Views";
	vbits = W_CAMERA | W_XY;
	flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	noPadding = true;
	SetTitle("4 View");
	Reset();
}

/*
==================
WndViewports::Reset
==================
*/
void WndViewports::Reset()
{
	static const int hammer[4] = { 0, 1, 2, 3 };	// camera, XY / YZ, XZ

	SetCellViews(hammer);
	splitX = splitY = 0.5f;
	maximized = -1;
	pendingCell = -1;
}

/*
==================
WndViewports::CellView / SetCellViews
==================
*/
int WndViewports::CellView(int cell) const
{
	if (cells[cell] == g_wndCamera)
		return 0;
	for (int i = 0; i < 3; i++)
		if (cells[cell] == g_wndGrid[i])
			return i + 1;
	return 0;
}

void WndViewports::SetCellViews(const int views[4])
{
	bool seen[4] = {};

	for (int i = 0; i < 4; i++)
	{
		if (views[i] < 0 || views[i] > 3 || seen[views[i]])
			return;
		seen[views[i]] = true;
	}
	for (int i = 0; i < 4; i++)
		cells[i] = views[i] ? (Window *)g_wndGrid[views[i] - 1] : g_wndCamera;
}

WndViewports::ViewType WndViewports::CellType(int cell) const
{
	if (cells[cell] == g_wndCamera)
		return VIEW_CAMERA;

	switch (static_cast<MapView2D *>(cells[cell])->Plane())
	{
	case MapView2D::YZ:	return VIEW_FRONT;
	case MapView2D::XZ:	return VIEW_SIDE;
	default:			return VIEW_TOP;
	}
}

void WndViewports::SwapCells(int a, int b)
{
	std::swap(cells[a], cells[b]);
	if (maximized == a)
		maximized = b;
	else if (maximized == b)
		maximized = a;
	ImGui::MarkIniSettingsDirty();
}

/*
==================
WndViewports::SwapGridCam
==================
*/
void WndViewports::SwapGridCam()
{
	int cam = -1, grid = -1;

	for (int i = 0; i < 4; i++)
	{
		if (cells[i] == g_wndCamera)
			cam = i;
		else if (cells[i] == g_wndGrid[0])
			grid = i;
	}
	if (cam >= 0 && grid >= 0)
		SwapCells(cam, grid);
}

/*
==================
WndViewports::ApplyViewType

there is one camera and three grids, so a cell turns into the camera by
trading places with it. a grid just changes plane; the camera cell turning 2D
trades with the grid already showing that plane, or the bottom right one
==================
*/
void WndViewports::ApplyViewType(int cell, ViewType type)
{
	if (type == CellType(cell))
		return;

	if (type == VIEW_CAMERA)
	{
		for (int i = 0; i < 4; i++)
			if (cells[i] == g_wndCamera)
				SwapCells(cell, i);
		return;
	}

	if (cells[cell] == g_wndCamera)
	{
		int other = -1;
		for (int i = 0; i < 4; i++)
		{
			if (cells[i] == g_wndCamera)
				continue;
			if (CellType(i) == type)
			{
				other = i;
				break;
			}
			other = i;
		}
		SwapCells(cell, other);
	}

	static_cast<MapView2D *>(cells[cell])->SetViewPlane(s_viewPlanes[type]);
}

/*
==================
WndViewports::ToggleMaximize

hammertime's FocusOn / Unfocus, but the other three views go away entirely
==================
*/
void WndViewports::ToggleMaximize(int cell)
{
	if (maximized >= 0)
	{
		maximized = -1;
		return;
	}
	if (cell < 0)
		cell = (hoveredCell >= 0) ? hoveredCell : activeCell;
	maximized = cell;
}

/*
==================
WndViewports::DrawLabel

hammer's viewport caption: a small tag in the corner that opens the view menu
==================
*/
void WndViewports::DrawLabel(int cell, ImVec2 pos, bool hovered)
{
	ImDrawList *dl = ImGui::GetWindowDrawList();
	const char *text = s_viewLabels[CellType(cell)];
	ImVec2 ts = ImGui::CalcTextSize(text);
	ImVec2 max(pos.x + ts.x + 8, pos.y + ts.y + 4);
	ImU32 bg = hovered ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : IM_COL32(0, 0, 0, 160);
	ImU32 fg = cells[cell]->HasFocus() ? IM_COL32_WHITE : IM_COL32(200, 200, 200, 255);

	dl->AddRectFilled(pos, max, bg);
	dl->AddText(ImVec2(pos.x + 4, pos.y + 2), fg, text);
}

/*
==================
DrawViewIcon

hammer's little menu icons: the two axes of a 2D view in gray, or a cube in
the camera's draw mode (an outline, flat gray, textured blue, shaded blue,
lit warm with a light by it)
==================
*/
static void DrawViewIcon(ImDrawList *dl, ImVec2 pos, float h, int mode, const char *axes)
{
	if (axes)
	{
		ImVec2 ts = ImGui::CalcTextSize(axes);
		dl->AddText(ImVec2(pos.x + 2, pos.y + (h - ts.y) / 2), ImGui::GetColorU32(ImGuiCol_TextDisabled), axes);
		return;
	}

	float r = h * 0.42f, k = r * 0.87f;
	ImVec2 c(pos.x + 2 + k, pos.y + h / 2);
	ImVec2 top(c.x, c.y - r), ur(c.x + k, c.y - r / 2), lr(c.x + k, c.y + r / 2);
	ImVec2 bottom(c.x, c.y + r), ll(c.x - k, c.y + r / 2), ul(c.x - k, c.y - r / 2);
	ImVec2 faces[3][4] = {
		{ top, ur, c, ul },			// top
		{ ul, c, bottom, ll },		// left
		{ c, ur, lr, bottom },		// right
	};

	ImU32 colors[3];
	switch (mode)
	{
	case MapRenderer::FLATSHADE:
		colors[0] = colors[1] = colors[2] = IM_COL32(150, 150, 150, 255);
		break;
	case MapRenderer::TEXTURED:
		colors[0] = colors[1] = colors[2] = IM_COL32(120, 170, 230, 255);
		break;
	case MapRenderer::TEXTURED_SHADED:
		colors[0] = IM_COL32(170, 210, 255, 255);
		colors[1] = IM_COL32(110, 160, 220, 255);
		colors[2] = IM_COL32(70, 120, 190, 255);
		break;
	case MapRenderer::LIGHTING:
		colors[0] = IM_COL32(255, 230, 140, 255);
		colors[1] = IM_COL32(220, 170, 80, 255);
		colors[2] = IM_COL32(150, 100, 40, 255);
		break;
	default:	// wireframe
		colors[0] = colors[1] = colors[2] = 0;
		break;
	}

	ImU32 edge = mode == MapRenderer::WIREFRAME ? IM_COL32(200, 200, 200, 255) : IM_COL32(30, 30, 30, 255);
	for (int f = 0; f < 3; f++)
	{
		if (colors[f])
			dl->AddConvexPolyFilled(faces[f], 4, colors[f]);
		dl->AddPolyline(faces[f], 4, edge, ImDrawFlags_Closed, 1.0f);
	}
	if (mode == MapRenderer::LIGHTING)
		dl->AddCircleFilled(ImVec2(c.x + k + 2, c.y - r), h * 0.13f, IM_COL32(255, 255, 200, 255));
}

static bool IconMenuItem(const char *label, bool checked, int mode, const char *axes)
{
	// the label moves right with spaces, the icon goes in the gap
	float iconW = ImGui::GetFontSize() * 1.6f;
	int spaces = (int)ceilf(iconW / ImGui::CalcTextSize(" ").x);
	char padded[96];
	std::snprintf(padded, sizeof(padded), "%*s%s", spaces, "", label);

	bool clicked = ImGui::MenuItem(padded, nullptr, checked);
	DrawViewIcon(ImGui::GetWindowDrawList(), ImGui::GetItemRectMin(), ImGui::GetItemRectSize().y, mode, axes);
	return clicked;
}

/*
==================
WndViewports::DrawCell
==================
*/
void WndViewports::DrawCell(int cell, ImVec2 pos, ImVec2 size)
{
	char childId[16];
	std::snprintf(childId, sizeof(childId), "##cell%d", cell);

	ImGui::SetCursorScreenPos(pos);
	ImGui::BeginChild(childId, size, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// the label's button goes in first so it wins the hover over the view below it,
	// it's drawn after the view so it stays on top
	const char *text = s_viewLabels[CellType(cell)];
	ImVec2 ts = ImGui::CalcTextSize(text);
	ImGui::InvisibleButton("##label", ImVec2(ts.x + 8, ts.y + 4));
	bool labelHovered = ImGui::IsItemHovered();
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
		ImGui::OpenPopup("##viewtype");
	ImGui::SetCursorScreenPos(pos);

	cells[cell]->PaintEmbedded();
	DrawLabel(cell, pos, labelHovered);

	if (cells[cell]->HasFocus())
	{
		activeCell = cell;
		if (maximized < 0)
			ImGui::GetWindowDrawList()->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
				ImGui::GetColorU32(ImGuiCol_SeparatorActive));
	}
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
		hoveredCell = cell;

	if (ImGui::BeginPopup("##viewtype"))
	{
		// hammer's view menu: the 2D planes, then the camera's draw modes
		static const struct { const char *label, *axes; ViewType type; } planes[3] = {
			{ "2D X/Y", "xy", VIEW_TOP },
			{ "2D Y/Z", "yz", VIEW_FRONT },
			{ "2D X/Z", "xz", VIEW_SIDE },
		};
		ViewType cur = CellType(cell);

		for (const auto &p : planes)
		{
			if (IconMenuItem(p.label, cur == p.type, -1, p.axes))
			{
				pendingCell = cell;
				pendingType = p.type;
				pendingMode = -1;
			}
		}
		ImGui::Separator();
		for (int m = 0; m < MapRenderer::DRAWMODE_COUNT; m++)
		{
			bool checked = cur == VIEW_CAMERA && WndCamera::drawMode == m;
			if (IconMenuItem(MapRenderer::DrawModeName((MapRenderer::DrawMode)m), checked, m, nullptr))
			{
				pendingCell = cell;
				pendingType = VIEW_CAMERA;
				pendingMode = m;
			}
		}
		ImGui::Separator();
		if (ImGui::MenuItem(maximized >= 0 ? "Restore Views" : "Maximize View", "Shift+Z"))
			ToggleMaximize(cell);
		if (ImGui::MenuItem("Center Splitters", nullptr, false, maximized < 0))
		{
			splitX = splitY = 0.5f;
			ImGui::MarkIniSettingsDirty();
		}
		ImGui::EndPopup();
	}

	ImGui::EndChild();
}

/*
==================
WndViewports::DrawSplitters

hammertime's TableSplitControl: one vertical and one horizontal bar across
the whole grid, the crossing moves both. double click puts a bar back in the middle
==================
*/
void WndViewports::DrawSplitters(ImVec2 origin, ImVec2 size, float w0, float h0)
{
	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 mouse = ImGui::GetIO().MousePos;
	float vx = origin.x + w0;		// left edge of the vertical bar
	float hy = origin.y + h0;		// top edge of the horizontal bar
	bool hotX = false, hotY = false, activeX = false, activeY = false;

	auto bar = [&](const char *barId, ImVec2 min, ImVec2 max, bool moveX, bool moveY)
	{
		ImGui::SetCursorScreenPos(min);
		ImGui::InvisibleButton(barId, ImVec2(max.x - min.x, max.y - min.y));
		bool hovered = ImGui::IsItemHovered();
		bool active = ImGui::IsItemActive();

		if (hovered || active)
		{
			ImGui::SetMouseCursor(moveX && moveY ? ImGuiMouseCursor_ResizeAll :
				moveX ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
			hotX |= moveX;
			hotY |= moveY;
		}
		if (active)
		{
			activeX |= moveX;
			activeY |= moveY;
			if (moveX)
				splitX = (mouse.x - origin.x - SPLITTER_SIZE / 2) / (size.x - SPLITTER_SIZE);
			if (moveY)
				splitY = (mouse.y - origin.y - SPLITTER_SIZE / 2) / (size.y - SPLITTER_SIZE);
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
				ImGui::MarkIniSettingsDirty();
		}
		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (moveX)
				splitX = 0.5f;
			if (moveY)
				splitY = 0.5f;
			ImGui::MarkIniSettingsDirty();
		}
	};

	// the crossing first, overlapping items go to whoever was submitted first
	bar("##splitxy", ImVec2(vx, hy), ImVec2(vx + SPLITTER_SIZE, hy + SPLITTER_SIZE), true, true);
	bar("##splitx", ImVec2(vx, origin.y), ImVec2(vx + SPLITTER_SIZE, origin.y + size.y), true, false);
	bar("##splity", ImVec2(origin.x, hy), ImVec2(origin.x + size.x, hy + SPLITTER_SIZE), false, true);

	auto color = [](bool hot, bool active)
	{
		return ImGui::GetColorU32(active ? ImGuiCol_SeparatorActive : hot ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
	};
	dl->AddRectFilled(ImVec2(vx, origin.y), ImVec2(vx + SPLITTER_SIZE, origin.y + size.y), color(hotX, activeX));
	dl->AddRectFilled(ImVec2(origin.x, hy), ImVec2(origin.x + size.x, hy + SPLITTER_SIZE), color(hotY, activeY));
}

/*
==================
WndViewports::OnPaint
==================
*/
int WndViewports::OnPaint()
{
	if (pendingCell >= 0)
	{
		ApplyViewType(pendingCell, pendingType);
		if (pendingType == VIEW_CAMERA && pendingMode >= 0)
			WndCamera::SetDrawMode((MapRenderer::DrawMode)pendingMode);
		pendingCell = -1;
	}

	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 size = ImGui::GetContentRegionAvail();
	if (size.x < 1 || size.y < 1)
		return 1;

	hoveredCell = -1;

	if (maximized >= 0)
	{
		DrawCell(maximized, origin, size);
	}
	else
	{
		float spanX = std::max(size.x - SPLITTER_SIZE, 2.0f);
		float spanY = std::max(size.y - SPLITTER_SIZE, 2.0f);
		float minX = std::min(MIN_CELL_SIZE / spanX, 0.5f);
		float minY = std::min(MIN_CELL_SIZE / spanY, 0.5f);

		splitX = std::clamp(splitX, minX, 1.0f - minX);
		splitY = std::clamp(splitY, minY, 1.0f - minY);

		float w0 = floorf(spanX * splitX), w1 = spanX - w0;
		float h0 = floorf(spanY * splitY), h1 = spanY - h0;
		float x1 = origin.x + w0 + SPLITTER_SIZE;
		float y1 = origin.y + h0 + SPLITTER_SIZE;

		DrawCell(0, origin, ImVec2(w0, h0));
		DrawCell(1, ImVec2(x1, origin.y), ImVec2(w1, h0));
		DrawCell(2, ImVec2(origin.x, y1), ImVec2(w0, h1));
		DrawCell(3, ImVec2(x1, y1), ImVec2(w1, h1));
		DrawSplitters(origin, size, w0, h0);
	}

	// hammertime's "focus on current view"; netradiant's F12 picks the view under the cursor
	bool inside = hoveredCell >= 0 || ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	if (inside && !ImGui::GetIO().WantTextInput && ImGui::IsKeyChordPressed(ImGuiMod_Shift | ImGuiKey_Z))
		ToggleMaximize();

	return 1;
}
