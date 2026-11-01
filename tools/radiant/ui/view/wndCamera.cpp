//==============================
//	wndCamera.cpp
//==============================

#include "ui/view/wndCamera.h"
#include "ui/wndMain.h"
#include "ui/view/mapView2D.h"
#include "renderer/glFuncs.h"
#include "map/grid.h"
#include "map/map.h"

#include <SDL.h>
#include <imgui_internal.h>	// MarkIniSettingsDirty

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

MapRenderer::DrawMode WndCamera::drawMode = MapRenderer::TEXTURED_SHADED;
bool	WndCamera::cubicClip = false;
float	WndCamera::cubicDistance = 2048.0f;
float	WndCamera::moveSpeed = 512.0f;

static const float s_backgroundColor[3] = { 0.15f, 0.15f, 0.15f };
static const ImU32 s_axisColors[3] = {		// mapView2D.cpp's
	IM_COL32(255, 61, 0, 230),
	IM_COL32(74, 148, 0, 230),
	IM_COL32(15, 156, 255, 230),
};
static const ImU32 s_textColor = IM_COL32(170, 170, 170, 255);

#define LOOK_SENSITIVITY	0.2f		// degrees per pixel
#define ORBIT_SENSITIVITY	0.3f
#define TURN_SPEED			90.0f		// degrees per second, the arrow keys
#define WHEEL_STEP			64.0f		// units per notch
#define PICK_FALLBACK		256.0f		// how far "under the cursor" is when nothing's there
#define EYE_HEIGHT			46.0f		// quake's: 24 from the floor to the origin, 22 more to the eye
#define MIN_SPEED			16.0f
#define MAX_SPEED			8192.0f
#define CUBIC_STEP			256.0f

WndCamera::WndCamera() :
	drag(DRAG_NONE), panScale(1.0f)
{
	name = "Camera";
	vbits = W_CAMERA;
	flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	noPadding = true;
	SetTitle("Camera");
	ResetForMap();
}

WndCamera::~WndCamera()
{
	EndDrag();
}

/*
==================
WndCamera::ResetForMap

quaked3 puts the camera where the player starts; without one it stands at the
origin looking down +x
==================
*/
void WndCamera::ResetForMap()
{
	EndDrag();

	const Entity *start = g_map.FindEntity("info_player_start");
	if (!start)
		start = g_map.FindEntity("info_player_deathmatch");

	float yaw = 0, pitch = 0;
	vec3 pos(0, 0, EYE_HEIGHT);
	if (start)
	{
		pos = to_vec3(start->Origin()) + vec3(0, 0, 22);
		start->Angles(yaw, pitch);
	}
	camera.MoveTo(pos);
	camera.SetAngles(yaw, pitch);
}

void WndCamera::LevelView()
{
	camera.SetAngles(camera.Yaw(), 0);
}

/*
==================
WndCamera::ChangeFloor

qe4's Camera_ChangeFloor: the next brush top straight below the camera's
column, above or below the floor it stands on now
==================
*/
void WndCamera::ChangeFloor(bool up)
{
	vec3 pos = camera.Position();
	double feet = pos.z - EYE_HEIGHT;
	double best = up ? DBL_MAX : -DBL_MAX;
	bool found = false;

	for (const Entity &e : g_map.entities)
	{
		for (const Brush &b : e.brushes)
		{
			if (!b.valid || pos.x <= b.mins.x || pos.x >= b.maxs.x || pos.y <= b.mins.y || pos.y >= b.maxs.y)
				continue;

			dvec3 from(pos.x, pos.y, b.maxs.z + 1);
			double d;
			if (!b.RayCast(from, dvec3(0, 0, -1), d))
				continue;

			double top = from.z - d;
			if (up ? (top > feet + 1 && top < best) : (top < feet - 1 && top > best))
			{
				best = top;
				found = true;
			}
		}
	}

	if (found)
	{
		pos.z = (float)best + EYE_HEIGHT;
		camera.MoveTo(pos);
	}
}

void WndCamera::SetPosition2D(int axis1, float v1, int axis2, float v2)
{
	vec3 pos = camera.Position();
	pos[axis1] = v1;
	pos[axis2] = v2;
	camera.MoveTo(pos);
}

void WndCamera::SetHeight(float z)
{
	vec3 pos = camera.Position();
	pos.z = z;
	camera.MoveTo(pos);
}

void WndCamera::SetDrawMode(MapRenderer::DrawMode mode)
{
	if (mode < 0 || mode >= MapRenderer::DRAWMODE_COUNT || mode == drawMode)
		return;
	drawMode = mode;
	ImGui::MarkIniSettingsDirty();
}

void WndCamera::ToggleCubicClip()
{
	cubicClip ^= true;
}

void WndCamera::CubicClipNearer()
{
	cubicDistance = std::max(cubicDistance - CUBIC_STEP, CUBIC_STEP);
}

void WndCamera::CubicClipFarther()
{
	cubicDistance = std::min(cubicDistance + CUBIC_STEP, MAX_WORLD_COORD * 2);
}

/*
==================
WndCamera::PickPoint

the first brush or entity under the cursor, or a point PICK_FALLBACK ahead
==================
*/
bool WndCamera::PickPoint(ImVec2 mouse, vec3 &point, float &depth) const
{
	vec3 origin, dir;
	camera.PickRay(mouse.x, mouse.y, origin, dir);

	double dist;
	bool hit = g_map.RayCast(to_dvec3(origin), to_dvec3(dir), dist);
	if (!hit)
		dist = PICK_FALLBACK / std::max(dot(dir, camera.Direction()), 0.1f);

	point = origin + dir * (float)dist;
	depth = std::max(dot(point - origin, camera.Direction()), 1.0f);
	return hit;
}

void WndCamera::EndDrag()
{
	if (drag == DRAG_LOOK)
		SDL_SetRelativeMouseMode(SDL_FALSE);
	drag = DRAG_NONE;
}

/*
==================
WndCamera::HandleMouse

trenchbroom's CameraTool3D, with netradiant-custom's free look on the right button
==================
*/
void WndCamera::HandleMouse(bool hovered, bool active, ImVec2 mouse)
{
	ImGuiIO &io = ImGui::GetIO();

	if (hovered && drag == DRAG_NONE)
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			Focus();
			if (io.KeyAlt)
			{
				float depth;
				PickPoint(mouse, orbitCenter, depth);
				drag = DRAG_ORBIT;
			}
			else
			{
				drag = DRAG_LOOK;
				SDL_SetRelativeMouseMode(SDL_TRUE);
				SDL_GetRelativeMouseState(nullptr, nullptr);	// drop what piled up before
			}
		}
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
		{
			Focus();
			vec3 point;
			float depth;
			PickPoint(mouse, point, depth);
			panScale = camera.PixelSize(depth);
			drag = DRAG_PAN;
		}
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			Focus();	// TODO: selection, once the map can be edited
		}
	}

	if (drag != DRAG_NONE && !active)
		EndDrag();

	switch (drag)
	{
	case DRAG_LOOK:
	{
		int dx, dy;
		SDL_GetRelativeMouseState(&dx, &dy);
		if (dx || dy)
			camera.Rotate(-dx * LOOK_SENSITIVITY, -dy * LOOK_SENSITIVITY);

		// trenchbroom: the wheel sets the fly speed while flying
		if (io.MouseWheel != 0)
		{
			moveSpeed = std::clamp(moveSpeed * powf(1.25f, io.MouseWheel), MIN_SPEED, MAX_SPEED);
			char status[64];
			snprintf(status, sizeof(status), "Camera speed: %.0f", moveSpeed);
			WndMain_Status(status, 0);
		}
		break;
	}
	case DRAG_ORBIT:
		if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)
			camera.Orbit(orbitCenter, -io.MouseDelta.x * ORBIT_SENSITIVITY, -io.MouseDelta.y * ORBIT_SENSITIVITY);
		break;
	case DRAG_PAN:
		// grab and drag: the point under the cursor follows it
		if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)
			camera.MoveBy(camera.Right() * (-io.MouseDelta.x * panScale) + camera.Up() * (io.MouseDelta.y * panScale));
		break;
	default:
		if (hovered && io.MouseWheel != 0)
		{
			Focus();
			vec3 origin, dir;
			camera.PickRay(mouse.x, mouse.y, origin, dir);
			camera.MoveBy(dir * (io.MouseWheel * WHEEL_STEP * (io.KeyShift ? 4.0f : 1.0f)));
		}
		break;
	}
}

/*
==================
WndCamera::HandleKeys

trenchbroom's fly keys, quake radiant's arrows
==================
*/
void WndCamera::HandleKeys()
{
	ImGuiIO &io = ImGui::GetIO();

	if ((!HasFocus() && drag != DRAG_LOOK) || io.WantTextInput || io.KeyCtrl || io.KeyAlt)
		return;

	float dt = io.DeltaTime;
	vec3 forward = camera.Direction(), right = camera.Right(), up(0, 0, 1);
	vec3 move;

	if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow))		move += forward;
	if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow))	move -= forward;
	if (ImGui::IsKeyDown(ImGuiKey_D))	move += right;
	if (ImGui::IsKeyDown(ImGuiKey_A))	move -= right;
	if (ImGui::IsKeyDown(ImGuiKey_E))	move += up;
	if (ImGui::IsKeyDown(ImGuiKey_Q))	move -= up;

	if (move != vec3())
		camera.MoveBy(normalize(move) * (moveSpeed * (io.KeyShift ? 3.0f : 1.0f) * dt));

	float turn = 0;
	if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))	turn += TURN_SPEED * dt;
	if (ImGui::IsKeyDown(ImGuiKey_RightArrow))	turn -= TURN_SPEED * dt;
	if (turn != 0)
		camera.Rotate(turn, 0);

	if (ImGui::IsKeyPressed(ImGuiKey_End, false))
		LevelView();
	if (ImGui::IsKeyPressed(ImGuiKey_PageUp, false))
		ChangeFloor(true);
	if (ImGui::IsKeyPressed(ImGuiKey_PageDown, false))
		ChangeFloor(false);
}

/*
==================
WndCamera::RenderScene
==================
*/
void WndCamera::RenderScene()
{
	camera.SetFarPlane(cubicClip ? cubicDistance : MAX_WORLD_COORD * 4.0f);

	fbo.Bind();
	glDisable(GL_SCISSOR_TEST);
	glClearColor(s_backgroundColor[0], s_backgroundColor[1], s_backgroundColor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	g_mapRenderer.Render(camera, drawMode);

	// TODO: selection, tools

	Framebuffer::Unbind();
}

/*
==================
WndCamera::DrawOverlays

trenchbroom's compass, flattened: the world axes as the camera sees them, in
the bottom left corner, the far ones first
==================
*/
void WndCamera::DrawOverlays(ImDrawList *dl, ImVec2 origin, ImVec2 size)
{
	static const char names[3] = { 'X', 'Y', 'Z' };
	float fontH = ImGui::GetFontSize();

	if (MapView2D::showAxis)	// View > Toggle > Show Axis, shared with the 2D views
	{
		ImVec2 c(origin.x + 40, origin.y + size.y - 40);
		int order[3] = { 0, 1, 2 };
		vec3 axes[3] = { vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1) };

		std::sort(order, order + 3, [&](int a, int b)
		{
			return dot(axes[a], camera.Direction()) > dot(axes[b], camera.Direction());
		});
		for (int i : order)
		{
			ImVec2 end(c.x + dot(axes[i], camera.Right()) * 25, c.y - dot(axes[i], camera.Up()) * 25);
			dl->AddLine(c, end, s_axisColors[i], 2.0f);
			ImVec2 label(c.x + (end.x - c.x) * 1.35f - fontH * 0.3f, c.y + (end.y - c.y) * 1.35f - fontH * 0.5f);
			dl->AddText(label, s_axisColors[i], &names[i], &names[i] + 1);
		}
	}

	// bottom right: what's limiting or shaping the view
	char text[96];
	text[0] = 0;
	if (drawMode == MapRenderer::LIGHTING)
	{
		float progress = g_mapRenderer.BakeProgress();
		if (progress >= 0)
			snprintf(text, sizeof(text), "baking lights %.0f%%...", progress * 100.0f);
		else
			snprintf(text, sizeof(text), "lighting preview: %d light%s, %d sun%s", g_mapRenderer.LightCount(),
				g_mapRenderer.LightCount() == 1 ? "" : "s", g_mapRenderer.SunCount(), g_mapRenderer.SunCount() == 1 ? "" : "s");
	}
	if (cubicClip)
		snprintf(text + strlen(text), sizeof(text) - strlen(text), "%scubic clip %.0f", text[0] ? "  |  " : "", cubicDistance);
	if (text[0])
	{
		ImVec2 ts = ImGui::CalcTextSize(text);
		dl->AddText(ImVec2(origin.x + size.x - ts.x - 6, origin.y + size.y - ts.y - 4), s_textColor, text);
	}
}

void WndCamera::ShowStatus()
{
	const vec3 &p = camera.Position();
	char status[96];
	snprintf(status, sizeof(status), "Camera: %.0f %.0f %.0f  yaw %.0f  pitch %.0f", p.x + 0.0f, p.y + 0.0f, p.z + 0.0f,
		camera.Yaw() + 0.0f, camera.Pitch() + 0.0f);
	WndMain_Status(status, 0);
}

/*
==================
WndCamera::OnPaint
==================
*/
int WndCamera::OnPaint()
{
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 size = ImGui::GetContentRegionAvail();
	if (size.x < 1 || size.y < 1)
		return 1;

	camera.SetViewport(Camera::Viewport{ 0, 0, (int)size.x, (int)size.y });

	ImGui::InvisibleButton("##view3d", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	bool hovered = ImGui::IsItemHovered();
	bool active = ImGui::IsItemActive();
	ImVec2 mouse(ImGui::GetIO().MousePos.x - origin.x, ImGui::GetIO().MousePos.y - origin.y);

	vec3 oldPos = camera.Position(), oldDir = camera.Direction();
	HandleMouse(hovered, active, mouse);
	HandleKeys();
	if (camera.Position() != oldPos || camera.Direction() != oldDir)
		ShowStatus();

	if (drag == DRAG_PAN || drag == DRAG_ORBIT)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

	if (!fbo.Resize((int)size.x, (int)size.y))
		return 1;
	RenderScene();

	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 max(origin.x + size.x, origin.y + size.y);
	dl->PushClipRect(origin, max, true);
	dl->AddImage((ImTextureID)(uintptr_t)fbo.ColorTexture(), origin, max, ImVec2(0, 1), ImVec2(1, 0));
	DrawOverlays(dl, origin, size);
	dl->PopClipRect();

	return 1;
}
