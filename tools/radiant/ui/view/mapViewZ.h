//==============================
//	mapViewZ.h
//
//	the Z view ("Z Checker"): a narrow strip with height going up. port of
//	quaked3's ZView + ZViewRenderer + WndZChecker. the strip looks straight
//	down one column of the map at origin x,y; the brushes that column passes
//	through are drawn as bars spanning their height, plus the camera icon
//	(control + middle drag sets the camera's height).
//
//	everything is a handful of lines and quads, so it draws with the ImGui
//	draw list instead of a framebuffer. the brush column comes next
//==============================

#ifndef __MAP_VIEW_Z_H__
#define __MAP_VIEW_Z_H__

#include "ui/window.h"
#include "mathlib.h"

class MapViewZ : public Window
{
public:
	MapViewZ();

	inline const vec3 &Origin() const	{ return origin; }
	void	SetOrigin(const vec3 &org);		// x,y pick the column, z is the strip's center
	inline float Scale() const			{ return scale; }

	void	ResetScale();
	void	ScaleUp();						// around the strip's center
	void	ScaleDown();

	float	ScreenToZ(float y) const;		// view local, y down
	float	ZToScreen(float z) const;

protected:
	int		OnPaint() override;

private:
	void	HandleInput(bool hovered, bool active, ImVec2 mouse);
	void	ScaleAt(float y, float factor);		// keeps the height under the mouse in place
	void	DrawGrid(ImDrawList *dl, ImVec2 origin, ImVec2 size);
	void	DrawCoords(ImDrawList *dl, ImVec2 origin, ImVec2 size);
	void	DrawCamera(ImDrawList *dl, ImVec2 origin, ImVec2 size);

	vec3	origin;
	float	scale;					// pixels per unit
	float	viewHeight;				// last painted height, for ScreenToZ outside OnPaint

	enum { DRAG_NONE, DRAG_SCROLL, DRAG_SCALE, DRAG_CAMERA };
	int		drag;
	float	lastY;
};

#endif
