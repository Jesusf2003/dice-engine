//==============================
//	textureView.cpp
//==============================

#include "ui/textures/textureView.h"
#include "renderer/texture.h"
#include "renderer/textures.h"
#include "ui/wndMain.h"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

#define LAYOUT_TOP	-4		// a little gap above the first row

TextureView g_vTexture;

TextureView::TextureView() :
	width(0), height(0),
	scale(1.0f), scroll(0), layoutLength(0), stale(true),
	sourceGroup(nullptr), useSourceNames(false), cellSize(0),
	selected(nullptr)
{
}

TextureView::~TextureView()
{
}

void TextureView::ResetScroll()
{
	scroll = 0;
}

void TextureView::Scroll(int dist, bool fast)
{
	int lscale = fast ? 4 : 1;

	scroll += dist * lscale;
	if (scroll < -layoutLength + height)
		scroll = -layoutLength + height;
	if (scroll > 0)
		scroll = 0;
}



/*
============================================================================

TEXTURE LAYOUT

cache texture window layout instead of rebuilding it every frame

	* top of the window is y=0, down is -y *

============================================================================
*/

/*
============
TextureView::ArrangeGroup

appends a wad's textures to the layout, continuing its rows
============
*/
void TextureView::ArrangeGroup(TextureGroup &txGrp, int &curX, int &curY, int &curRowHeight)
{
	for (Texture *q = txGrp.first; q; q = q->next)
	{
		// skip stomped textures
		auto it = Textures::texMap.find(q->name);
		if (it == Textures::texMap.end() || it->second != q)
			continue;
		// don't bother with the blank broken texture
		if (q->name == "none")
			continue;
		if (!Shows(q))
			continue;

		// TODO: skip unused textures when "hide unused" is on, once that setting exists

		float w, h, slot;
		if (cellSize > 0)
		{
			// fixed cell: shrink to fit keeping the aspect, never grow (hammertime's DrawImage)
			w = (float)q->width;
			h = (float)q->height;
			if (w > cellSize && w >= h)
			{
				h = floorf(cellSize * (h / w));
				w = (float)cellSize;
			}
			else if (h > cellSize)
			{
				w = floorf(cellSize * (w / h));
				h = (float)cellSize;
			}
			slot = (float)cellSize;
		}
		else
		{
			w = q->width * scale;	// sikk - Mouse Zoom Texture Window
			h = q->height * scale;
			// never go less than 64, or the names get all crunched up
			slot = w < 64 ? 64 : w;
		}
		float rowH = (cellSize > 0) ? (float)cellSize : h;

		// go to the next row unless the texture is the first on the row
		if (curRowHeight && curX + slot > width - MARGIN_X)
		{
			curX = MARGIN_X;
			curY -= curRowHeight + FONT_HEIGHT + 4;
			curRowHeight = 0;
		}
		layout.emplace_back(curX, curY, w, h, slot, q);

		// Is our texture larger than the row? If so, grow the row height to match it
		if (curRowHeight < rowH)
			curRowHeight = (int)rowH;

		curX += (int)slot + MARGIN_X;
	}
}

/*
============
TextureView::Arrange

the source wad, or every wad when a favourites folder picks textures by name
============
*/
void TextureView::Arrange()
{
	if (!stale) return;

	// a source group that left the library (flushed, replaced) falls back to showing all
	if (sourceGroup && sourceGroup != &Textures::group_unknown &&
		std::find(Textures::groups.begin(), Textures::groups.end(), sourceGroup) == Textures::groups.end())
		sourceGroup = nullptr;

	layout.clear();
	int curX = MARGIN_X, curY = LAYOUT_TOP, curRowHeight = 0;

	for (TextureGroup *tg : Textures::groups)
	{
		if (!sourceGroup || sourceGroup == tg)
			ArrangeGroup(*tg, curX, curY, curRowHeight);
	}
	if (!sourceGroup || sourceGroup == &Textures::group_unknown)
		ArrangeGroup(Textures::group_unknown, curX, curY, curRowHeight);

	layoutLength = layout.empty() ? 0 : curRowHeight + FONT_HEIGHT + 12 - curY;

	Scroll(0, false);	// snap back to bounds
	stale = false;
}

void TextureView::Refresh()
{
	stale = true;
}

/*
============
TextureView::Resize
============
*/
void TextureView::Resize(const int w, const int h)
{
	width = w;
	height = h;
	stale = true;
}


/*
==============
TextureView::SetScale
==============
*/
void TextureView::Scale(float inscale)
{
	SetScale(scale * inscale);
}

void TextureView::SetScale(float inscale)
{
	char	texscalestring[128];
	Texture	*firsttexture = nullptr;

	if (inscale > 8)
		inscale = 8;
	if (inscale < 0.25)
		inscale = 0.25f;

	Arrange();

	// Find first visible texture with old scale and determine the y origin
	// that will place it at the top with the new scale
	for (const texWndItem_t &item : layout)
	{
		if ((item.y - item.h - FONT_HEIGHT < scroll) && (item.y > scroll - 64))
		{
			firsttexture = item.tex;
			break;
		}
	}

	scale = inscale;
	stale = true;
	Arrange();

	if (firsttexture)
	{
		for (const texWndItem_t &item : layout)
		{
			if (item.tex == firsttexture)
			{
				SetScroll(item.y + FONT_HEIGHT);
				break;
			}
		}
	}

	snprintf(texscalestring, sizeof(texscalestring), "Texture scale: %3.0f%%", scale * 100.0f);
	WndMain_Status(texscalestring, 0);
}


/*
==============
TextureView::TexAtPos

gets the texture under the x y layout position (relative to the top edge)
==============
*/
Texture *TextureView::TexAtPos(int x, int y)
{
	Arrange();
	int ly = y + scroll + 4;

	for (const texWndItem_t &item : layout)
	{
		if (x > item.x && x - item.x < item.w &&
			ly < item.y && item.y - ly < item.h + FONT_HEIGHT)
		{
			return item.tex;
		}
	}
	return nullptr;
}

/*
==============
TextureView::TexAtCursorPos

gets the texture currently under the cursor. cx, cy are window coordinates
with y going up from the bottom edge (Window::GetMouseXY)
==============
*/
Texture *TextureView::TexAtCursorPos(const int cx, const int cy)
{
	return TexAtPos(cx, cy - height);
}

/*
============
TextureView::GroupOf
============
*/
TextureGroup *TextureView::GroupOf(const Texture *tex) const
{
	if (!tex)
		return nullptr;
	for (TextureGroup *tg : Textures::groups)
	{
		for (Texture *q = tg->first; q; q = q->next)
		{
			if (q == tex)
				return tg;
		}
	}
	for (Texture *q = Textures::group_unknown.first; q; q = q->next)
	{
		if (q == tex)
			return &Textures::group_unknown;
	}
	return nullptr;
}


/*
============================================================================

FILTERING

============================================================================
*/

/*
============
TextureView::Shows
============
*/
bool TextureView::Shows(const Texture *q) const
{
	if (useSourceNames && !sourceNames.count(q->name))
		return false;
	// names are lowercase already, filter is lowered when set
	if (!filter.empty() && q->name.find(filter) == std::string::npos)
		return false;
	return true;
}

void TextureView::SetFilter(const std::string &text)
{
	std::string f = text;
	for (char &c : f)
		c = (char)tolower((unsigned char)c);
	if (f == filter)
		return;
	filter = f;
	stale = true;
	ResetScroll();
}

void TextureView::SetSourceGroup(TextureGroup *tg)
{
	sourceGroup = tg;
	stale = true;
	ResetScroll();
}

void TextureView::SetSourceNames(const std::set<std::string> *names)
{
	useSourceNames = (names != nullptr);
	sourceNames.clear();
	if (names)
		sourceNames = *names;
	stale = true;
	ResetScroll();
}

void TextureView::SetCellSize(int size)
{
	if (size == cellSize)
		return;
	cellSize = size;
	stale = true;
	Arrange();
}


/*
============================================================================

HIGHLIGHTING

============================================================================
*/

void TextureView::HighlightOnly(Texture *tex)
{
	highlighted.clear();
	lastHighlighted.clear();
	if (!tex)
		return;
	highlighted.insert(tex->name);
	lastHighlighted = tex->name;
}

void TextureView::ToggleHighlight(Texture *tex)
{
	if (!tex)
		return;
	if (highlighted.erase(tex->name))
	{
		lastHighlighted.clear();
		return;
	}
	highlighted.insert(tex->name);
	lastHighlighted = tex->name;
}

/*
============
TextureView::HighlightRange

everything between the last highlighted texture and this one, as laid out
============
*/
void TextureView::HighlightRange(Texture *tex)
{
	if (!tex)
		return;
	if (lastHighlighted.empty())
	{
		ToggleHighlight(tex);
		return;
	}

	Arrange();
	int from = -1, to = -1;
	for (int i = 0; i < (int)layout.size(); i++)
	{
		if (layout[i].tex->name == lastHighlighted) from = i;
		if (layout[i].tex == tex) to = i;
	}
	if (from < 0 || to < 0)
	{
		ToggleHighlight(tex);
		return;
	}
	if (from > to)
		std::swap(from, to);
	for (int i = from; i <= to; i++)
		highlighted.insert(layout[i].tex->name);
}

void TextureView::ClearHighlight()
{
	highlighted.clear();
	lastHighlighted.clear();
}

bool TextureView::IsHighlighted(const Texture *tex) const
{
	return highlighted.count(tex->name) > 0;
}


/*
============================================================================

TOOL

============================================================================
*/

/*
============
TextureView::UpdateStatus
============
*/
void TextureView::UpdateStatus()
{
	char status[256];

	if (selected)
		snprintf(status, sizeof(status), "Selected texture: %s (%dx%d)", selected->name.c_str(), selected->width, selected->height);
	else
		snprintf(status, sizeof(status), "Selected texture: none");
	WndMain_Status(status, 3);
}

/*
============
TextureView::ChooseFirstTexture
============
*/
void TextureView::ChooseFirstTexture()
{
	Arrange();
	for (const texWndItem_t &item : layout)
	{
		if (GroupOf(item.tex) != &Textures::group_unknown)
		{
			ChooseTexture(item.tex);
			return;
		}
	}
}

/*
============
TextureView::ChooseTexture
============
*/
void TextureView::ChooseTexture(Texture *tx)
{
	if (tx && tx->name[0] == '#')
	{
		SDL_Log("Cannot select an entity texture.");
		return;
	}
	selected = tx;
	selectedName = tx ? tx->name : std::string();
	WndMain_UpdateWindows(W_TEXTURE | W_SURF);

	if (tx)
	{
		// scroll so the texture is completely on screen
		Arrange();
		for (const texWndItem_t &item : layout)
		{
			if (item.tex != tx)
				continue;

			if (item.y > scroll)
				scroll = item.y;
			else if (item.y - item.h - 2 * FONT_HEIGHT < scroll - height)
				scroll = (int)(item.y - item.h - 2 * FONT_HEIGHT + height);
			break;
		}
	}
	UpdateStatus();
}
