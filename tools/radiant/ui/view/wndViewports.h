//==============================
//	wndViewports.h
//
//	hammer's 4 view: camera, top, front and side in one 2x2 grid that shares a
//	single splitter cross, so the rows and columns always line up. layout from
//	hammertime's TableSplitControl (drag a bar or the crossing, double click to
//	center it, shift+z maximizes the current view) and netradiant-custom's 4 way
//	style + MaximizeView (the view under the cursor is the one maximized).
//
//	each cell paints one of the existing views (g_wndCamera, g_wndGrid[0..2])
//	with Window::PaintEmbedded; those windows stay hidden while this one is up.
//	the label in each cell's corner switches what it shows, hammer style: the
//	three 2D planes, or the camera in one of its draw modes
//==============================

#ifndef __WND_VIEWPORTS_H__
#define __WND_VIEWPORTS_H__

#include "ui/window.h"

class WndViewports : public Window
{
public:
	enum ViewType { VIEW_CAMERA, VIEW_TOP, VIEW_FRONT, VIEW_SIDE };

	WndViewports();

	void	Reset();						// hammer's default: camera, top / front, side, centered split
	void	SwapGridCam();					// the camera trades cells with the main (XY) grid
	void	ToggleMaximize(int cell = -1);	// -1: the cell under the mouse, else the focused one
	inline bool IsMaximized() const	{ return maximized >= 0; }

	// radiant.ini: which view each cell holds, 0 camera, 1..3 g_wndGrid[n-1]
	int		CellView(int cell) const;
	void	SetCellViews(const int views[4]);	// ignored unless it's a permutation of 0..3

	float	splitX, splitY;					// splitter cross, 0..1 of the content region

protected:
	int		OnPaint() override;

private:
	void	DrawCell(int cell, ImVec2 pos, ImVec2 size);
	void	DrawLabel(int cell, ImVec2 pos, bool hovered);
	void	DrawSplitters(ImVec2 origin, ImVec2 size, float w0, float h0);
	void	SwapCells(int a, int b);
	void	ApplyViewType(int cell, ViewType type);
	ViewType	CellType(int cell) const;

	Window	*cells[4];				// top left, top right, bottom left, bottom right
	int		maximized;
	int		hoveredCell, activeCell;
	int		pendingCell;			// a label menu pick, applied before the next paint
	ViewType	pendingType;
	int		pendingMode;			// MapRenderer::DrawMode with VIEW_CAMERA, -1 keeps it
};

extern WndViewports	*g_wndViewports;

#endif
