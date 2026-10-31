//==============================
//	window.cpp
//==============================

#include "ui/window.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>

std::vector<Window*> Window::windex;


/*
==================
Window
==================
*/
Window::Window() :
	v(nullptr),
	instance(0), minWidth(64), minHeight(32), vbits(0),
	clientRect(0, 0, 0, 0),
	mouseWithin(false),
	name("Window"), flags(0), noPadding(false),
	cursorX(-1), cursorY(-1),
	created(false), open(false), focused(false), wantFocus(false), wantPos(false),
	pendingPos({ 0, 0, 0, 0, false }), lastPos({ 0, 0, 0, 0, false }),
	lastSize(0, 0)
{
	windex.push_back(this);
}

/*
==================
~Window
==================
*/
Window::~Window()
{
	windex.erase(std::find(windex.begin(), windex.end(), this));
}

/*
==================
Window::DrawAll
==================
*/
void Window::DrawAll()
{
	for (auto wvIt = windex.begin(); wvIt != windex.end(); ++wvIt)
		(*wvIt)->Draw();
}

/*
==================
Window::Create

open state and placement come from radiant.ini when the window was seen
before, otherwise it starts visible at a default spot
==================
*/
void Window::Create()
{
	if (created)
		return;

	if (title.empty())
		title = name;

	char buf[128];
	std::snprintf(buf, sizeof(buf), "###%s%i", name, instance);
	id = buf;

	created = true;
	open = true;
	OnCreate();
}

/*
==================
Window::Destroy
==================
*/
void Window::Destroy()
{
	if (!created)
		return;

	OnDestroy();
	created = false;
	open = false;
}

/*
==================
Window::Draw
==================
*/
void Window::Draw()
{
	if (!created || !open)
		return;

	if (wantPos)
	{
		ImGui::SetNextWindowPos(ImVec2(pendingPos.l, pendingPos.t), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(pendingPos.r - pendingPos.l, pendingPos.b - pendingPos.t), ImGuiCond_Always);
		wantPos = false;
	}
	else
	{
		ImGui::SetNextWindowPos(ImVec2(32, 64), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(288, 176), ImGuiCond_FirstUseEver);
	}
	if (wantFocus)
	{
		ImGui::SetNextWindowFocus();
		wantFocus = false;
	}
	ImGui::SetNextWindowSizeConstraints(ImVec2((float)minWidth, (float)minHeight), ImVec2(FLT_MAX, FLT_MAX));

	// "###" keeps the id (and the saved layout) tied to name+instance while the caption changes
	std::string label = title + id;

	// only Focus() moves focus, otherwise whichever panel is submitted last on the first frame steals it
	if (noPadding)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	bool visible = ImGui::Begin(label.c_str(), &open, flags | ImGuiWindowFlags_NoFocusOnAppearing);
	if (noPadding)
		ImGui::PopStyleVar();

	ImVec2 pos = ImGui::GetWindowPos();
	ImVec2 size = ImGui::GetWindowSize();
	lastPos = { pos.x, pos.y, pos.x + size.x, pos.y + size.y, open };

	bool nowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	if (nowFocused && !focused)
		OnFocus();
	focused = nowFocused;

	if (visible)
	{
		ImVec2 cmin = ImGui::GetCursorScreenPos();
		ImVec2 avail = ImGui::GetContentRegionAvail();
		clientRect = ImVec4(cmin.x, cmin.y, cmin.x + avail.x, cmin.y + avail.y);

		if (avail.x != lastSize.x || avail.y != lastSize.y)
		{
			lastSize = avail;
			OnResized();
		}

		mouseWithin = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
		GetMouseXY(cursorX, cursorY);

		OnPaint();
	}
	else
	{
		mouseWithin = false;
	}

	ImGui::End();
}

/*
==================
Window::SetPosition

coordinates are absolute (main viewport space); applied on the next Draw.
a docked window keeps its dock node's rect
==================
*/
void Window::SetPosition(float l, float t, float r, float b, bool show)
{
	pendingPos = { l, t, r, b, show };
	wantPos = true;

	if (show)
		Show();
	else
		Hide();
}

void Window::SetPosition(wndPos_t pos)
{
	SetPosition(pos.l, pos.t, pos.r, pos.b, pos.vis);
}

/*
==================
Window::GetPosition

rect from the last frame this window was drawn
==================
*/
Window::wndPos_t Window::GetPosition()
{
	wndPos_t pos = lastPos;
	pos.vis = open;
	return pos;
}

/*
==================
Window::Toggle
==================
*/
void Window::Toggle()
{
	if (open)
	{
		if (!IsOnTop())
			Focus();
		else
			Hide();
	}
	else
	{
		Show();
		Focus();
	}
}

/*
==================
Window::Swap
==================
*/
void Window::Swap(Window &other)
{
	wndPos_t myPos = GetPosition();
	wndPos_t otherPos = other.GetPosition();

	SetPosition(otherPos);
	other.SetPosition(myPos);
}

void Window::CopyPositionFrom(Window &other)
{
	wndPos_t mypos = GetPosition();
	wndPos_t pos = other.GetPosition();
	pos.vis = mypos.vis;
	SetPosition(pos);
}

/*
==================
Window::SetTitle
==================
*/
void Window::SetTitle(const char *t)
{
	title = t ? t : "";
}

/*
==================
Window::GetMouseXY
==================
*/
void Window::GetMouseXY(int &x, int &y)
{
	ImVec2 m = ImGui::GetIO().MousePos;
	x = (int)(m.x - clientRect.x);						// horizontal position of cursor
	y = (int)(clientRect.w - 1 - m.y);					// vertical position of cursor
}

int Window::OnCreate() { return 1; }
int Window::OnDestroy() { return 1; }
int Window::OnPaint() { return 1; }
int Window::OnResized() { return 1; }
