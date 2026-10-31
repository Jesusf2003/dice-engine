//==============================
//	mapView2D.h
//
//	the 2D grid views (XY top, XZ front, YZ side). structure from trenchbroom's
//	ui::MapView2D: an OrthographicCamera per view, CameraTool2D for pan and
//	zoom, the grid drawn by GridRenderer. the overlays are netradiant-custom's
//	XYWnd: coordinates along the edges, the axis widget in the corner and the
//	small axes at the world origin.
//
//	each view renders into its own framebuffer, shown with the ImGui draw list.
//	brushes, entities and the tools come once the map is ported
//==============================

#ifndef __MAP_VIEW_2D_H__
#define __MAP_VIEW_2D_H__

#include "ui/window.h"
#include "renderer/orthographicCamera.h"
#include "renderer/framebuffer.h"

class MapView2D : public Window
{
public:
	enum ViewPlane { XY, XZ, YZ };

	MapView2D(ViewPlane plane, int instance);

	inline ViewPlane Plane() const			{ return plane; }
	void	SetViewPlane(ViewPlane plane);	// keeps the zoom and what's shared of the position
	void	CycleViewPlane();				// netradiant's next view: XY -> XZ -> YZ

	void	ResetZoom();
	void	ZoomIn();						// around the view center
	void	ZoomOut();

	inline OrthographicCamera &GetCamera()	{ return camera; }

	static const char *PlaneTitle(ViewPlane plane);

	// netradiant's view toggles, shared by every 2D view
	static bool	showCoordinates;
	static bool	showAxis;

protected:
	int		OnPaint() override;

private:
	void	InitializeCamera();
	void	HandleInput(bool hovered, bool active, ImVec2 mouse);
	void	ZoomAt(ImVec2 mouse, float factor);		// keeps the point under the mouse in place
	void	RenderScene();
	void	DrawOverlays(ImDrawList *dl, ImVec2 origin, ImVec2 size);
	ImVec2	ToScreen(const vec3 &world, ImVec2 origin, ImVec2 size) const;

	ViewPlane			plane;
	OrthographicCamera	camera;
	Framebuffer			fbo;

	enum { DRAG_NONE, DRAG_PAN, DRAG_ZOOM };
	int		drag;
	ImVec2	lastMouse;				// view local, y down
};

#endif
