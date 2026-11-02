//==============================
//	mapView2D.h
//
//	the 2D grid views (XY top, XZ front, YZ side). structure from trenchbroom's
//	ui::MapView2D: an OrthographicCamera per view, CameraTool2D for pan and
//	zoom, the grid drawn by GridRenderer. the overlays are netradiant-custom's
//	XYWnd: coordinates along the edges, the axis widget in the corner and the
//	small axes at the world origin, and the 3D view's camera (control +
//	middle click or drag moves it there).
//
//	each view renders into its own framebuffer, shown with the ImGui draw list:
//	the grid, the map in wireframe (MapRenderer) and the selection in red.
//
//	selecting (map/selection.h has the rules):
//		left click			select what's under the cursor; again on the same
//							spot: the next one behind it
//		control + click		add it to the selection, or take it away
//		double click		a brush entity's every brush
//		left drag			select what's entirely inside the box (control: add)
//		escape				drop the box being dragged, or the selection
//		arrows				scroll the view (hammer), shift for a page
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

	void	CenterOn(const vec3 &point);	// the view's two axes, the depth stays
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
	void	DrawCamera(ImDrawList *dl, ImVec2 origin, ImVec2 size);
	void	MoveCamera(ImVec2 mouse);
	ImVec2	ToScreen(const vec3 &world, ImVec2 origin, ImVec2 size) const;

	// selection (map/selection.h)
	void	HandleKeys();
	void	ClickSelect(ImVec2 mouse, bool toggle, bool doubleClick);
	void	MarqueeSelect(ImVec2 a, ImVec2 b, bool add);
	void	DrawMarquee(ImDrawList *dl, ImVec2 origin);

	ViewPlane			plane;
	OrthographicCamera	camera;
	Framebuffer			fbo;

	// DRAG_SELECT: the left button is down, a click until it moves, then DRAG_MARQUEE
	enum { DRAG_NONE, DRAG_PAN, DRAG_ZOOM, DRAG_CAMERA, DRAG_SELECT, DRAG_MARQUEE };
	int		drag;
	ImVec2	lastMouse;				// view local, y down
	ImVec2	selectStart;			// where the left button went down
	bool	selectToggle;			// control was held then
	bool	selectDouble;
};

#endif
