//==============================
//	mapViewZ.cpp
//==============================

#include "ui/view/mapViewZ.h"
#include "ui/view/mapView2D.h"
#include "ui/view/wndCamera.h"
#include "ui/wndMain.h"
#include "map/grid.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// same colors as the 2D views (mapView2D.cpp, trenchbroom's defaults)
static const ImU32 s_backgroundColor = IM_COL32(38, 38, 38, 255);
static const ImU32 s_gridTextColor = IM_COL32(170, 170, 170, 255);
static const float s_gridColor = 0.8f;
static const float s_gridAlpha = 0.8f * 0.5f;	// GridColor alpha * GridAlpha
static const float s_minorOpacity = 0.65f;		// GridCommon.fragsh
static const ImU32 s_cameraColor = IM_COL32(80, 140, 255, 255);		// mapView2D.cpp's

#define ZOOM_STEP	1.25f		// quaked3's ScaleUp / ScaleDown
#define MIN_SCALE	0.05f
#define MAX_SCALE	32.0f

MapViewZ::MapViewZ() :
	origin(0, 0, 0), scale(1.0f), viewHeight(0),
	drag(DRAG_NONE), lastY(0)
{
	name = "Z";			// keeps the id ("###Z0") the layouts dock by
	vbits = W_Z;
	flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	noPadding = true;
	minWidth = 32;
	SetTitle("Z");
}

void MapViewZ::SetOrigin(const vec3 &org)
{
	origin = org;
	origin.z = std::clamp(origin.z, -MAX_WORLD_COORD, MAX_WORLD_COORD);
	WndMain_UpdateWindows(W_Z);
}

void MapViewZ::ResetScale()
{
	scale = 1.0f;
}

void MapViewZ::ScaleUp()
{
	scale = std::min(MAX_SCALE, scale * ZOOM_STEP);
}

void MapViewZ::ScaleDown()
{
	scale = std::max(MIN_SCALE, scale / ZOOM_STEP);
}

/*
==================
MapViewZ::ScreenToZ / ZToScreen

quaked3's ScreenToWorld, with y going down from the top of the view
==================
*/
float MapViewZ::ScreenToZ(float y) const
{
	return origin.z + (viewHeight / 2 - y) / scale;
}

float MapViewZ::ZToScreen(float z) const
{
	return viewHeight / 2 - (z - origin.z) * scale;
}

void MapViewZ::ScaleAt(float y, float factor)
{
	float before = ScreenToZ(y);
	scale = std::clamp(scale * factor, MIN_SCALE, MAX_SCALE);
	origin.z += before - ScreenToZ(y);
}

/*
==================
MapViewZ::HandleInput

the 2D views' controls (wheel zooms to the mouse, right or middle drag scrolls,
alt + middle drag zooms) plus quaked3's control + right drag zoom and control +
middle drag for the camera's height
==================
*/
void MapViewZ::HandleInput(bool hovered, bool active, ImVec2 mouse)
{
	ImGuiIO &io = ImGui::GetIO();

	if (hovered && io.MouseWheel != 0 && !io.KeyCtrl && !io.KeyShift && drag == DRAG_NONE)
	{
		Focus();
		ScaleAt(mouse.y, powf(ZOOM_STEP, io.MouseWheel));
	}

	if (hovered && drag == DRAG_NONE)
	{
		bool right = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
		bool middle = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);

		if (middle && io.KeyCtrl)
		{
			Focus();
			drag = DRAG_CAMERA;
		}
		else if ((right && io.KeyCtrl) || (middle && io.KeyAlt))
		{
			Focus();
			drag = DRAG_SCALE;
			lastY = mouse.y;		// the zoom anchor for the whole drag
		}
		else if (right || middle)
		{
			Focus();
			drag = DRAG_SCROLL;
			lastY = mouse.y;
		}
	}

	if (drag != DRAG_NONE && !active)
		drag = DRAG_NONE;

	if (drag == DRAG_SCROLL)
	{
		origin.z = std::clamp(origin.z + (mouse.y - lastY) / scale, -MAX_WORLD_COORD, MAX_WORLD_COORD);
		lastY = mouse.y;
	}
	else if (drag == DRAG_SCALE)
	{
		float factor = 1.0f - io.MouseDelta.y / 100.0f;
		if (factor > 0)
			ScaleAt(lastY, factor);
	}
	else if (drag == DRAG_CAMERA)
	{
		g_wndCamera->SetHeight((float)g_grid.Snap(ScreenToZ(mouse.y)));
	}

	// quaked3: the snapped height under the cursor
	if (hovered && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0))
	{
		char status[64];
		snprintf(status, sizeof(status), "Z: %.0f", g_grid.Snap(ScreenToZ(mouse.y)));
		WndMain_Status(status, 0);
	}
}

/*
==================
MapViewZ::DrawGrid

quaked3's ZViewRenderer::DrawGrid: a major line every 64 units (or every grid
step when that's bigger), minor lines at the grid size while they're at least
4 pixels apart, and the center line the brush column hangs from
==================
*/
void MapViewZ::DrawGrid(ImDrawList *dl, ImVec2 o, ImVec2 size)
{
	float alpha = s_gridAlpha * (g_grid.Snap() ? 1.0f : 0.3f);	// netradiant dims the grid while snapping is off
	ImU32 major = ImGui::ColorConvertFloat4ToU32(ImVec4(s_gridColor, s_gridColor, s_gridColor, alpha));
	ImU32 minor = ImGui::ColorConvertFloat4ToU32(ImVec4(s_gridColor, s_gridColor, s_gridColor, alpha * s_minorOpacity));
	float gridSize = (float)Grid::ActualSize(g_grid.Size());
	float majorSize = std::max(64.0f, gridSize);
	float halfH = size.y / 2 / scale;

	float zb = std::max(origin.z - halfH, -MAX_WORLD_COORD);
	float ze = std::min(origin.z + halfH, MAX_WORLD_COORD);
	float yTop = o.y + ZToScreen(ze);
	float yBottom = o.y + ZToScreen(zb);

	if (g_grid.Visible() && gridSize < majorSize && gridSize * scale >= 4)
	{
		for (float z = gridSize * floorf(zb / gridSize); z <= ze; z += gridSize)
		{
			if (fmodf(z, majorSize) == 0)
				continue;
			float y = o.y + ZToScreen(z);
			dl->AddLine(ImVec2(o.x, y), ImVec2(o.x + size.x, y), minor);
		}
	}

	// major lines stay even with the grid off, they're the strip's ruler
	for (float z = majorSize * floorf(zb / majorSize); z <= ze; z += majorSize)
	{
		float y = o.y + ZToScreen(z);
		dl->AddLine(ImVec2(o.x, y), ImVec2(o.x + size.x, y), major);
	}

	float cx = floorf(o.x + size.x / 2) + 0.5f;
	dl->AddLine(ImVec2(cx, yTop), ImVec2(cx, yBottom), major);
}

/*
==================
MapViewZ::DrawCoords

quaked3's DrawCoords: heights down the left edge, spread out as the view zooms out
==================
*/
void MapViewZ::DrawCoords(ImDrawList *dl, ImVec2 o, ImVec2 size)
{
	float fontH = ImGui::GetFontSize();
	float step = std::max(64.0f, (float)Grid::ActualSize(g_grid.Size()));
	float halfH = size.y / 2 / scale;
	char text[16];

	while (step * scale < fontH * 2 && step < MAX_WORLD_COORD)
		step *= 2;

	float zb = step * floorf(std::max(origin.z - halfH, -MAX_WORLD_COORD) / step);
	float ze = std::min(origin.z + halfH, MAX_WORLD_COORD);

	for (float z = zb; z <= ze; z += step)	// sikk: '<=' so the last one is drawn
	{
		snprintf(text, sizeof(text), "%d", (int)z);
		dl->AddText(ImVec2(o.x + 2, o.y + ZToScreen(z) - fontH), s_gridTextColor, text);
	}
}

/*
==================
MapViewZ::DrawCamera

quaked3's camera icon: a small box on the center line at the camera's height,
with a tick showing which way it looks up or down
==================
*/
void MapViewZ::DrawCamera(ImDrawList *dl, ImVec2 o, ImVec2 size)
{
	if (!g_wndCamera)
		return;

	const PerspectiveCamera &cam = g_wndCamera->GetCamera();
	float cx = floorf(o.x + size.x / 2) + 0.5f;
	float y = o.y + ZToScreen(cam.Position().z);
	float pitch = DEG2RAD(cam.Pitch());

	dl->AddRect(ImVec2(cx - 5, y - 5), ImVec2(cx + 5, y + 5), s_cameraColor, 0.0f, 0, 1.5f);
	dl->AddLine(ImVec2(cx, y), ImVec2(cx + cosf(pitch) * 14, y - sinf(pitch) * 14), s_cameraColor, 1.5f);
}

/*
==================
MapViewZ::OnPaint
==================
*/
int MapViewZ::OnPaint()
{
	ImVec2 o = ImGui::GetCursorScreenPos();
	ImVec2 size = ImGui::GetContentRegionAvail();
	if (size.x < 1 || size.y < 1)
		return 1;
	viewHeight = size.y;

	ImGui::InvisibleButton("##viewz", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	bool hovered = ImGui::IsItemHovered();
	bool active = ImGui::IsItemActive();
	ImVec2 mouse(ImGui::GetIO().MousePos.x - o.x, ImGui::GetIO().MousePos.y - o.y);
	HandleInput(hovered, active, mouse);
	if (drag != DRAG_NONE)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 max(o.x + size.x, o.y + size.y);
	dl->PushClipRect(o, max, true);
	dl->AddRectFilled(o, max, s_backgroundColor);

	DrawGrid(dl, o, size);

	// TODO: ZViewRenderer::Draw's brush column: every brush whose x,y bounds
	// hold origin, ray tested down and up at origin for its top and bottom
	// there, a quad in its texture color width / 3 either side of the center,
	// outlines on top; the selection over that, tools first (Tool::Draw1D)
	DrawCamera(dl, o, size);

	if (MapView2D::showCoordinates)	// the 2D views' toggle covers this one too
		DrawCoords(dl, o, size);

	dl->PopClipRect();
	return 1;
}
