//==============================
//	wndCamera.h
//
//	the 3D view. a PerspectiveCamera over g_map, drawn by MapRenderer into the
//	view's framebuffer. the controls mix trenchbroom's CameraTool3D and
//	netradiant-custom's free look:
//
//		right drag			look around (the cursor is hidden and locked meanwhile)
//		  + wheel			change the fly speed
//		alt + right drag	orbit around what's under the cursor
//		middle drag			pan, what's under the cursor stays under it
//		wheel				move along the cursor's ray, shift for 4x
//		W A S D, Q E		fly while the view has focus (or during the look),
//		  arrows			  shift for 3x; left / right arrows turn
//		End					level the view (View > Center)
//		Page Up / Down		up or down a floor (qe4's ChangeFloor)
//
//	the 2D views draw the camera and move it with control + middle click,
//	the Z view sets its height the same way
//==============================

#ifndef __WND_CAMERA_H__
#define __WND_CAMERA_H__

#include "ui/window.h"
#include "renderer/perspectiveCamera.h"
#include "renderer/framebuffer.h"
#include "renderer/mapRenderer.h"

class WndCamera : public Window
{
public:
	WndCamera();
	~WndCamera();

	inline PerspectiveCamera &GetCamera()	{ return camera; }

	void	ResetForMap();				// at info_player_start, looking its way
	void	LevelView();				// pitch back to the horizon
	void	ChangeFloor(bool up);
	void	SetPosition2D(int axis1, float v1, int axis2, float v2);	// from a 2D view, the third axis stays
	void	SetHeight(float z);			// from the Z view

	static void	SetDrawMode(MapRenderer::DrawMode mode);	// saved in radiant.ini

	void	ToggleCubicClip();
	void	CubicClipNearer();
	void	CubicClipFarther();

	// shared view settings, like MapView2D's
	static MapRenderer::DrawMode	drawMode;
	static bool		cubicClip;
	static float	cubicDistance;		// the far plane while cubic clipping is on
	static float	moveSpeed;			// units per second

protected:
	int		OnPaint() override;

private:
	void	HandleMouse(bool hovered, bool active, ImVec2 mouse);
	void	HandleKeys();
	void	EndDrag();
	bool	PickPoint(ImVec2 mouse, vec3 &point, float &depth) const;	// depth along the view direction
	void	RenderScene();
	void	DrawOverlays(ImDrawList *dl, ImVec2 origin, ImVec2 size);
	void	ShowStatus();

	PerspectiveCamera	camera;
	Framebuffer			fbo;

	enum { DRAG_NONE, DRAG_LOOK, DRAG_ORBIT, DRAG_PAN };
	int		drag;
	vec3	orbitCenter;
	float	panScale;				// world units per pixel at the grabbed depth
};

extern WndCamera *g_wndCamera;	// ui/wndMain.cpp

#endif
