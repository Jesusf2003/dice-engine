//==============================
//	textureView.h
//
//	texture browser layout, scrolling and picking. started as a port of quaked3's
//	TextureView; the per wad headers and folding are gone, the source panel picks
//	the wad (or a favourites folder) and the browser lays it out as one block.
//	drawing lives in WndTexture
//==============================

#ifndef __TEXTUREVIEW_H__
#define __TEXTUREVIEW_H__

#include <set>
#include <string>
#include <vector>

#define	FONT_HEIGHT	14		// one line of ImGui's default font plus a pixel
#define MARGIN_X	8

class Texture;
class TextureGroup;

class TextureView
{
public:
	TextureView();
	~TextureView();

	/*
	layout space: x right, the top of the layout is y=0 and down is -y.
	the window shows y from scroll (top edge) down to scroll - height.
	y is the top of an item's name, the image hangs FONT_HEIGHT below it
	*/
	struct texWndItem_t {
		texWndItem_t(int _x, int _y, float _w, float _h, float _slot, Texture *_tex) : x(_x), y(_y), w(_w), h(_h), slot(_slot), tex(_tex) {};
		int		x, y;
		float	w, h;		// image size as drawn
		float	slot;		// width the item takes in its row, the name is clipped to it
		Texture	*tex;
	};
	std::vector<texWndItem_t> layout;	// in row order, rebuilt by Arrange when stale

	// VIEW:
	void	Arrange();
	void	Refresh();
	void	Resize(const int w, const int h);

	Texture	*TexAtCursorPos(const int cx, const int cy);	// window coordinates, y up from the bottom edge

	void	SetScale(float inscale);
	void	Scale(float inscale);
	void	Scroll(int dist, bool fast);
	void	ResetScroll();
	inline void	SetScroll(int s)			{ scroll = s; Scroll(0, false); }	// clamped to the layout
	inline int	GetScroll() const			{ return scroll; }
	inline int	GetLayoutLength() const		{ return layoutLength; }
	inline int	GetWidth() const			{ return width; }
	inline int	GetHeight() const			{ return height; }
	inline float GetScale() const			{ return scale; }

	TextureGroup *GroupOf(const Texture *tex) const;	// the wad a texture came from

	// FILTERING: which textures the layout shows (ideas from hammertime's TextureBrowser)
	void	SetFilter(const std::string &text);					// substring of the name, any case
	void	SetSourceGroup(TextureGroup *tg);					// one wad only, nullptr for all
	void	SetSourceNames(const std::set<std::string> *names);	// only these names (a favourites folder), nullptr for all
	void	SetCellSize(int size);								// 0 = real size * scale, otherwise a fixed square cell
	inline const std::string &GetFilter() const	{ return filter; }
	inline TextureGroup	*GetSourceGroup() const	{ return sourceGroup; }
	inline int	GetCellSize() const				{ return cellSize; }

	// HIGHLIGHTING: the set of textures picked for favourites and other
	// multi texture commands, separate from the one work texture
	void	HighlightOnly(Texture *tex);
	void	ToggleHighlight(Texture *tex);
	void	HighlightRange(Texture *tex);		// from the last highlighted one, in layout order
	void	ClearHighlight();
	bool	IsHighlighted(const Texture *tex) const;
	inline const std::set<std::string> &Highlighted() const	{ return highlighted; }

	// TOOL:
	// TODO: quaked3 keeps a whole TexDef (g_qeglobals.d_workTexDef) as the work
	// texture; until TexDef is ported the browser just remembers the texture
	void	ChooseTexture(Texture *tex);
	void	ChooseFirstTexture();
	inline Texture *Selected() const				{ return selected; }
	inline const std::string &SelectedName() const	{ return selectedName; }

private:
	int		width, height;
	float	scale;
	int		scroll, layoutLength;	// height of layout in pixels
	bool	stale;					// layout is old and should be rebuilt before drawing

	void	ArrangeGroup(TextureGroup &txGrp, int &curX, int &curY, int &curRowHeight);
	bool	Shows(const Texture *q) const;
	Texture	*TexAtPos(int x, int y);
	void	UpdateStatus();

	std::string				filter;
	TextureGroup			*sourceGroup;
	std::set<std::string>	sourceNames;
	bool					useSourceNames;
	int						cellSize;

	std::set<std::string>	highlighted;		// names, so they survive a wad reload
	std::string				lastHighlighted;

	Texture		*selected;
	std::string	selectedName;
};


//========================================================================

extern TextureView g_vTexture;


#endif
