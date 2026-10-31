//==============================
//	mapView2D.cpp
//==============================

#include "ui/view/mapView2D.h"
#include "ui/wndMain.h"
#include "renderer/glFuncs.h"
#include "renderer/gridRenderer.h"
#include "map/grid.h"

#include <cmath>
#include <cstdint>
#include <cstdio>

bool MapView2D::showCoordinates = true;
bool MapView2D::showAxis = true;

// trenchbroom's preference defaults
static const float s_backgroundColor[3] = { 38 / 255.0f, 38 / 255.0f, 38 / 255.0f };
static const float s_gridColor[4] = { 0.8f, 0.8f, 0.8f, 0.8f };
static const float s_gridAlpha = 0.5f;
static const ImU32 s_axisColors[3] = {
	IM_COL32(255, 61, 0, 230),		// x
	IM_COL32(74, 148, 0, 230),		// y
	IM_COL32(15, 156, 255, 230),	// z
};
static const ImU32 s_gridTextColor = IM_COL32(170, 170, 170, 255);
static const char s_axisNames[3] = { 'X', 'Y', 'Z' };

#define ZOOM_STEP	1.25f		// netradiant: 5/4 per wheel notch

MapView2D::MapView2D(ViewPlane p, int inst) :
	plane(p), drag(DRAG_NONE), lastMouse(0, 0)
{
	name = "Grid";		// keeps the ids ("###Grid0".."###Grid2") the layouts dock by
	instance = inst;
	vbits = W_XY;
	flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	noPadding = true;
	SetTitle(PlaneTitle(plane));
	InitializeCamera();
}

const char *MapView2D::PlaneTitle(ViewPlane p)
{
	switch (p)
	{
	case XZ:	return "XZ Front";
	case YZ:	return "YZ Side";
	default:	return "XY Top";
	}
}

/*
==================
MapView2D::InitializeCamera

trenchbroom's MapView2D::initializeCamera: the camera sits on the edge of the world
looking in, the grid is drawn on the far edge
==================
*/
void MapView2D::InitializeCamera()
{
	vec3 pos = camera.Position();

	switch (plane)
	{
	case XY:
		camera.SetDirection(vec3(0, 0, -1), vec3(0, 1, 0));
		pos.z = MAX_WORLD_COORD;
		break;
	case XZ:
		camera.SetDirection(vec3(0, 1, 0), vec3(0, 0, 1));
		pos.y = -MAX_WORLD_COORD;
		break;
	case YZ:
		camera.SetDirection(vec3(-1, 0, 0), vec3(0, 0, 1));
		pos.x = MAX_WORLD_COORD;
		break;
	}
	camera.MoveTo(pos);
	camera.SetNearPlane(1.0f);
	// the grid is drawn at the far side of the world bounds, keep it off the far plane
	camera.SetFarPlane(MAX_WORLD_COORD * 2.0f + 16.0f);
}

/*
==================
MapView2D::SetViewPlane
==================
*/
void MapView2D::SetViewPlane(ViewPlane p)
{
	if (p == plane)
		return;

	// the old depth axis becomes a screen axis: start it at the origin
	vec3 pos = camera.Position();
	pos[abs_max_axis(camera.Direction())] = 0;
	camera.MoveTo(pos);

	plane = p;
	InitializeCamera();
	SetTitle(PlaneTitle(plane));
	WndMain_UpdateWindows(W_XY);
}

void MapView2D::CycleViewPlane()
{
	switch (plane)
	{
	case XY:	SetViewPlane(XZ); break;
	case XZ:	SetViewPlane(YZ); break;
	default:	SetViewPlane(XY); break;
	}
}

void MapView2D::ResetZoom()
{
	camera.SetZoom(1.0f);
}

void MapView2D::ZoomIn()
{
	camera.ZoomBy(ZOOM_STEP);
}

void MapView2D::ZoomOut()
{
	camera.ZoomBy(1.0f / ZOOM_STEP);
}

/*
==================
MapView2D::ZoomAt

trenchbroom's CameraTool2D zoom: whatever is under the mouse stays there
==================
*/
void MapView2D::ZoomAt(ImVec2 mouse, float factor)
{
	vec3 before = camera.Unproject(mouse.x, mouse.y, 0.0f);
	camera.ZoomBy(factor);
	vec3 after = camera.Unproject(mouse.x, mouse.y, 0.0f);
	camera.MoveBy(-(after - before));
}

/*
==================
MapView2D::HandleInput

CameraTool2D: wheel zooms to the mouse, right or middle drag pans, alt + middle
drag zooms
==================
*/
void MapView2D::HandleInput(bool hovered, bool active, ImVec2 mouse)
{
	ImGuiIO &io = ImGui::GetIO();

	if (hovered && io.MouseWheel != 0 && !io.KeyCtrl && !io.KeyShift && drag == DRAG_NONE)
	{
		Focus();
		ZoomAt(mouse, powf(ZOOM_STEP, io.MouseWheel));
	}

	if (hovered && drag == DRAG_NONE)
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
			(ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && !io.KeyAlt))
		{
			Focus();
			drag = DRAG_PAN;
			lastMouse = mouse;
		}
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && io.KeyAlt)
		{
			Focus();
			drag = DRAG_ZOOM;
			lastMouse = mouse;		// the zoom anchor for the whole drag
		}
	}

	if (drag == DRAG_PAN)
	{
		if (!active)
		{
			drag = DRAG_NONE;
			return;
		}
		vec3 last = camera.Unproject(lastMouse.x, lastMouse.y, 0.0f);
		vec3 cur = camera.Unproject(mouse.x, mouse.y, 0.0f);
		camera.MoveBy(-(cur - last));
		lastMouse = mouse;
	}
	else if (drag == DRAG_ZOOM)
	{
		if (!active)
		{
			drag = DRAG_NONE;
			return;
		}
		float factor = 1.0f - io.MouseDelta.y / 100.0f;
		if (factor > 0)
			ZoomAt(lastMouse, factor);
	}

	// cursor position in the status bar, in the view's two axes
	if (hovered && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0))
	{
		vec3 world = camera.Unproject(mouse.x, mouse.y, 0.0f);
		int d1 = abs_max_axis(camera.Right());
		int d2 = abs_max_axis(camera.Up());
		char status[64];
		snprintf(status, sizeof(status), "%c: %.0f  %c: %.0f", s_axisNames[d1], world[d1], s_axisNames[d2], world[d2]);
		WndMain_Status(status, 0);
	}
}

/*
==================
MapView2D::RenderScene
==================
*/
void MapView2D::RenderScene()
{
	fbo.Bind();
	glDisable(GL_SCISSOR_TEST);
	glClearColor(s_backgroundColor[0], s_backgroundColor[1], s_backgroundColor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (g_grid.Visible())
	{
		// netradiant dims the grid while snapping is off
		float alpha = s_gridAlpha * (g_grid.Snap() ? 1.0f : 0.3f);
		g_gridRenderer.Render(camera, (float)Grid::ActualSize(g_grid.Size()), s_gridColor, alpha);
	}

	// TODO: map (brushes, entities), selection and tool rendering

	Framebuffer::Unbind();
}

/*
==================
MapView2D::ToScreen
==================
*/
ImVec2 MapView2D::ToScreen(const vec3 &world, ImVec2 origin, ImVec2 size) const
{
	vec3 p = camera.Project(world);	// y up from the bottom of the viewport
	return ImVec2(origin.x + p.x, origin.y + size.y - p.y);
}

/*
==================
MapView2D::DrawOverlays

netradiant-custom's XY_DrawGrid text and XY_DrawAxis
==================
*/
void MapView2D::DrawOverlays(ImDrawList *dl, ImVec2 origin, ImVec2 size)
{
	float zoom = camera.Zoom();
	float fontH = ImGui::GetFontSize();
	int d1 = abs_max_axis(camera.Right());		// screen x axis
	int d2 = abs_max_axis(camera.Up());			// screen y axis
	const vec3 &c = camera.Position();
	float halfW = size.x / 2 / zoom;
	float halfH = size.y / 2 / zoom;
	char text[32];

	auto worldAt = [&](float horz, float vert)
	{
		vec3 p = c;
		p[d1] = horz;
		p[d2] = vert;
		return p;
	};

	// coordinates along the top and left edges
	if (showCoordinates)
	{
		float gridSize = (float)Grid::ActualSize(g_grid.Size());

		// x labels need room for their text, y labels just a line each
		float widest = ImGui::CalcTextSize("-32768").x + 8;
		float stepx = gridSize, stepy = gridSize;
		while (stepx * zoom < widest)
			stepx *= 2;
		while (stepy * zoom < 32)
			stepy *= 2;

		// the top left corner belongs to the y labels, the x labels start after it
		float leftColumn = ImGui::CalcTextSize("-32768").x + 8;

		float xb = stepx * floorf(fmaxf(c[d1] - halfW, -MAX_WORLD_COORD) / stepx);
		float xe = fminf(c[d1] + halfW, MAX_WORLD_COORD);
		for (float x = xb; x <= xe; x += stepx)
		{
			ImVec2 s = ToScreen(worldAt(x, c[d2]), origin, size);
			if (s.x < origin.x + leftColumn)
				continue;
			snprintf(text, sizeof(text), "%g", x);
			dl->AddText(ImVec2(s.x + 2, origin.y + 1), s_gridTextColor, text);
		}

		float yb = stepy * floorf(fmaxf(c[d2] - halfH, -MAX_WORLD_COORD) / stepy);
		float ye = fminf(c[d2] + halfH, MAX_WORLD_COORD);
		for (float y = yb; y <= ye; y += stepy)
		{
			ImVec2 s = ToScreen(worldAt(c[d1], y), origin, size);
			if (s.y - fontH < origin.y + fontH + 2)		// under the x labels row
				continue;
			snprintf(text, sizeof(text), "%g", y);
			dl->AddText(ImVec2(origin.x + 4, s.y - fontH), s_gridTextColor, text);
		}
	}

	if (showAxis)
	{
		// 32 pixel axes at the world origin
		ImVec2 o = ToScreen(worldAt(0, 0), origin, size);
		dl->AddLine(o, ImVec2(o.x + 32, o.y), s_axisColors[d1], 2.0f);
		dl->AddLine(o, ImVec2(o.x, o.y - 32), s_axisColors[d2], 2.0f);
		dl->AddText(ImVec2(o.x + 32 - fontH * 0.3f, o.y + 3), s_axisColors[d1], &s_axisNames[d1], &s_axisNames[d1] + 1);
		dl->AddText(ImVec2(o.x - 3 - fontH * 0.6f, o.y - 32 - fontH / 2), s_axisColors[d2], &s_axisNames[d2], &s_axisNames[d2] + 1);

		// the corner widget: which two axes this view shows
		ImVec2 a(origin.x + 40, origin.y + 45 + (showCoordinates ? fontH : 0));
		dl->AddLine(a, ImVec2(a.x + 25, a.y), s_axisColors[d1], 2.0f);
		dl->AddLine(a, ImVec2(a.x, a.y - 25), s_axisColors[d2], 2.0f);
		dl->AddText(ImVec2(a.x + 25 - fontH * 0.55f, a.y + 3), s_axisColors[d1], &s_axisNames[d1], &s_axisNames[d1] + 1);
		dl->AddText(ImVec2(a.x - 4 - fontH * 0.55f, a.y - 25 - 3), s_axisColors[d2], &s_axisNames[d2], &s_axisNames[d2] + 1);
	}
	else
	{
		dl->AddText(ImVec2(origin.x + 35, origin.y + fontH * (showCoordinates ? 2.0f : 1.0f)),
			HasFocus() ? IM_COL32_WHITE : s_gridTextColor, PlaneTitle(plane));
	}
}

/*
==================
MapView2D::OnPaint
==================
*/
int MapView2D::OnPaint()
{
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 size = ImGui::GetContentRegionAvail();
	if (size.x < 1 || size.y < 1)
		return 1;

	camera.SetViewport(Camera::Viewport{ 0, 0, (int)size.x, (int)size.y });

	ImGui::InvisibleButton("##view2d", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	bool hovered = ImGui::IsItemHovered();
	bool active = ImGui::IsItemActive();
	ImVec2 mouse(ImGui::GetIO().MousePos.x - origin.x, ImGui::GetIO().MousePos.y - origin.y);
	HandleInput(hovered, active, mouse);
	if (drag != DRAG_NONE)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

	if (!fbo.Resize((int)size.x, (int)size.y))
		return 1;
	RenderScene();

	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 max(origin.x + size.x, origin.y + size.y);
	dl->PushClipRect(origin, max, true);
	// GL textures start at the bottom row
	dl->AddImage((ImTextureID)(uintptr_t)fbo.ColorTexture(), origin, max, ImVec2(0, 1), ImVec2(1, 0));
	DrawOverlays(dl, origin, size);
	dl->PopClipRect();

	return 1;
}
