//==============================
//	window.h
//
//	base class for every editor panel. port of quaked3's Window: the win32
//	HWND/WNDPROC plumbing is replaced by a Dear ImGui window that lives in the
//	main dockspace. docking/snapping and position persistence are handled by
//	ImGui itself (radiant.ini), so TryDocking/Snap*/Save/RestorePosition are gone.
//==============================

#ifndef __WND_BASE_H__
#define __WND_BASE_H__

#include <imgui.h>

#include <string>
#include <vector>

class View;
class Window
{
public:
	Window();
	virtual ~Window();

	static std::vector<Window*> windex;
	static void DrawAll();		// call once per frame, between ImGui::NewFrame and ImGui::Render

	void Create();
	void Destroy();
	void Draw();

	View	*v;
	int		instance, minWidth, minHeight, vbits;
	ImVec4	clientRect;			// content region in screen space: x,y = min, z,w = max
	bool	mouseWithin;

	struct wndPos_t {
		float	l, t, r, b;
		bool	vis;
	};

	inline void BringToTop()	{ wantFocus = true; }
	inline void Focus()			{ BringToTop(); }	// OnFocus fires from Draw once ImGui actually focuses it
	inline bool HasFocus()		{ return focused; }
	inline bool IsOpen()		{ return open; }
	inline bool IsOnTop()		{ return focused; }
	inline void Show()			{ open = true; OnShow(); }	// no focus steal, layouts show several at once
	inline void Hide()			{ open = false; }
	void Toggle();
	void Swap(Window &other);
	void CopyPositionFrom(Window &other);
	void SetPosition(float l, float t, float r, float b, bool show);
	void SetPosition(wndPos_t pos);
	wndPos_t GetPosition();

	void GetMouseXY(int &x, int &y);	// cursor relative to the content region, y up (GL style)
	void SetTitle(const char *title);
	inline const char *Id() const	{ return id.c_str(); }	// "###name<instance>", what DockBuilder wants

protected:
	const char		*name;		// stable id, also the key in radiant.ini
	std::string		title;		// visible caption, can change freely
	std::string		id;
	ImGuiWindowFlags	flags;
	bool	noPadding;			// content reaches the window edges (views)
	int		cursorX, cursorY;

	virtual int OnCreate();
	virtual int OnDestroy();
	virtual int OnPaint();		// draw the window contents, called inside Begin/End
	virtual int OnResized();
	virtual void OnShow() {};
	virtual void OnFocus() {};

private:
	bool	created, open, focused, wantFocus, wantPos;
	wndPos_t	pendingPos, lastPos;
	ImVec2	lastSize;
};

#endif // __WND_BASE_H__
