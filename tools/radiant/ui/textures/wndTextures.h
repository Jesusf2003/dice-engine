//==============================
//	wndTextures.h
//
//	the texture browser window. port of quaked3's WndTexture: instead of a GL
//	child window with its own renderer (TexBrowserRenderer) it draws the
//	TextureView layout with the ImGui draw list.
//
//	laid out like netradiant-custom / hammertime:
//	+---------+---------------------------+
//	| wads    |                           |
//	| ...     |    textures               |
//	|---------|                           |
//	| favs    +---------------------------+
//	| ...     | size, filter  |  tex info |
//	+---------+---------------------------+
//	the source panel picks a wad or a favourites folder (textures can be dragged
//	onto a folder), the bottom bar has the cell size, the name filter (typing over
//	the textures goes to it too) and name / size / wad of the texture under the
//	cursor or the selected one
//==============================

#ifndef __WND_TEXTURE_H__
#define __WND_TEXTURE_H__

#include "ui/window.h"

#include <set>
#include <string>
#include <vector>

class TextureView;
class Texture;
class TextureGroup;
struct favFolder_t;

class WndTexture : public Window
{
public:
	WndTexture();
	~WndTexture();

	TextureView *texv;

	void	Initialize();

	void	UpdateTitle();
	void	MouseDown(const int x, const int y, const int btndown);
	void	MouseUp(const int x, const int y, const int btnup);
	void	MouseMoved(const int x, const int y, const float dy);
	void	MouseOver(const int x, const int y);

	void	RefreshSource();		// favourites changed, re-read the folder being shown

protected:
	int		OnPaint() override;
	void	OnShow() override;

private:
	void	DrawSourcePanel();
	void	DrawFolderTree(const std::vector<favFolder_t> &list, const std::string &parent);
	void	DrawBottomBar();
	void	DrawSizeCombo();
	void	DrawTextureInfo();
	void	DrawFolderDialog();
	bool	DrawFolderMenuItems(std::set<std::string> &items);	// "add to" submenu
	void	Browser(ImVec2 size);
	void	TypeToFilter();
	void	Render();
	void	Scrollbar(ImVec2 min, ImVec2 max);
	void	DoPopupMenu();
	void	StepCellSize(int dir);
	void	UpdateHighlightStatus();

	void	ShowWad(TextureGroup *tg);
	void	ShowFavorites(const std::string &path);
	void	OpenFolderDialog(bool rename, const std::string &path);

	// the browser area, screen space
	ImVec2	browserMin, browserMax;
	ImVec2	lastBrowserSize;
	void	BrowserMouseXY(int &x, int &y);

	Texture	*hoverTex;		// under the cursor this frame, for the info box

	char	filterBuf[64];

	enum { SRC_NONE, SRC_WAD, SRC_FAVORITES };	// SRC_NONE only until the first wad is picked
	int			sourceKind;
	std::string	sourceFav;		// favourites folder path, empty = all favourites

	// new / rename folder dialog
	bool		wantFolderDialog;
	bool		folderRename;
	std::string	folderTarget;			// folder to rename, or the parent of the new one
	std::set<std::string> folderAddItems;	// "add to new folder": put these in it once made
	char		folderBuf[64];
	std::string	pendingDelete;			// folders are deleted after the tree is drawn

	int		mDownX, mDownY;
	bool	rMoved;
	bool	rPressed;		// right button went down in the browser, so its release is ours
	bool	wantPopup;
	Texture			*popupTex;		// what was under the cursor when the context menu opened
	TextureGroup	*popupGroup;
};

extern WndTexture *g_wndTexture;

#endif
