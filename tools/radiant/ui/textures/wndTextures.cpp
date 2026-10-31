//==============================
//	wndTextures.cpp
//==============================

#include "ui/textures/wndTextures.h"
#include "ui/wndMain.h"
#include "config.h"
#include "renderer/texture.h"
#include "renderer/textures.h"
#include "ui/textures/textureView.h"
#include "ui/favourites/textureFavorites.h"

#include <imgui_internal.h>	// ScrollbarEx

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>

#define MOUSE_LEFT		1
#define MOUSE_RIGHT		2
#define MOUSE_MIDDLE	4

#define TEXTURES_PAYLOAD	"RADIANT_TEXTURES"	// drag & drop: the highlighted textures

// fixed cell sizes, same choices as hammertime; 0 is quaked3's real size * scale
static const int	s_cellSizes[] = { 0, 64, 128, 256, 512 };
static const char	*s_cellNames[] = { "Real", "64 x 64", "128 x 128", "256 x 256", "512 x 512" };
#define NUM_CELL_SIZES	(int)(sizeof(s_cellSizes) / sizeof(s_cellSizes[0]))

// three framed rows (size, filter, checkbox) inside a bordered child
static float BottomBarHeight()
{
	ImGuiStyle &style = ImGui::GetStyle();
	return ImGui::GetFrameHeightWithSpacing() * 3 - style.ItemSpacing.y + style.WindowPadding.y * 2 + style.ChildBorderSize * 2;
}

WndTexture::WndTexture() :
	texv(nullptr),
	browserMin(0, 0), browserMax(0, 0), lastBrowserSize(0, 0),
	hoverTex(nullptr),
	sourceKind(SRC_NONE),
	wantFolderDialog(false), folderRename(false),
	mDownX(0), mDownY(0), rMoved(false), rPressed(false), wantPopup(false),
	popupTex(nullptr), popupGroup(nullptr)
{
	name = "Texture";
	vbits = W_TEXTURE;
	flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	filterBuf[0] = 0;
	folderBuf[0] = 0;
}

WndTexture::~WndTexture()
{
}

void WndTexture::Initialize()
{
	texv = &g_vTexture;
	TextureFavorites::RegisterSettings();
	UpdateTitle();
	Create();
}

void WndTexture::UpdateTitle()
{
	// TODO: "Textures (used)" when hide unused is on, once that setting exists
	SetTitle("Textures");
}

void WndTexture::OnShow()
{
	UpdateTitle();
}

/*
==============
WndTexture::OnPaint

source panel on the left, textures and the bottom bar on the right
==============
*/
int WndTexture::OnPaint()
{
	ImGuiStyle &style = ImGui::GetStyle();
	ImVec2 avail = ImGui::GetContentRegionAvail();
	if (avail.x < 64 || avail.y < 64)
		return 1;

	// left: wads and favourites, resizable by dragging its right edge
	ImGui::SetNextWindowSizeConstraints(ImVec2(80, -1), ImVec2(avail.x * 0.6f, -1));
	if (ImGui::BeginChild("##sources", ImVec2(180, avail.y), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX))
		DrawSourcePanel();
	ImGui::EndChild();

	ImGui::SameLine();

	// right: textures over the bottom bar
	float bottomH = BottomBarHeight();
	ImGui::BeginGroup();
	ImVec2 right = ImGui::GetContentRegionAvail();
	Browser(ImVec2(right.x, avail.y - bottomH - style.ItemSpacing.y));
	DrawBottomBar();
	ImGui::EndGroup();

	if (wantPopup)
	{
		ImGui::OpenPopup("##texcontext");
		wantPopup = false;
	}
	DoPopupMenu();
	DrawFolderDialog();

	return 1;
}


/*
========================================================================

	SOURCE PANEL

	hammertime's package and favourites trees, netradiant-custom's spot
========================================================================
*/

void WndTexture::ShowWad(TextureGroup *tg)
{
	sourceKind = SRC_WAD;
	texv->SetSourceNames(nullptr);
	texv->SetSourceGroup(tg);
}

void WndTexture::ShowFavorites(const std::string &path)
{
	sourceKind = SRC_FAVORITES;
	sourceFav = path;
	texv->SetSourceGroup(nullptr);
	RefreshSource();
}

/*
==============
WndTexture::RefreshSource
==============
*/
void WndTexture::RefreshSource()
{
	if (sourceKind != SRC_FAVORITES)
		return;

	// the folder may be gone (deleted, renamed): fall back to all favourites
	if (!sourceFav.empty() && !TextureFavorites::Find(sourceFav))
		sourceFav.clear();

	std::set<std::string> names;
	TextureFavorites::Collect(sourceFav, names);
	texv->SetSourceNames(&names);
}

void WndTexture::OpenFolderDialog(bool rename, const std::string &path)
{
	wantFolderDialog = true;
	folderRename = rename;
	folderTarget = path;
	folderBuf[0] = 0;
	if (rename)
	{
		const favFolder_t *f = TextureFavorites::Find(path);
		snprintf(folderBuf, sizeof(folderBuf), "%s", f ? f->name.c_str() : "");
	}
}

/*
==============
TexturesDropTarget

the last item takes highlighted textures dragged from the browser
==============
*/
static bool TexturesDropTarget()
{
	bool dropped = false;
	if (ImGui::BeginDragDropTarget())
	{
		if (ImGui::AcceptDragDropPayload(TEXTURES_PAYLOAD))
			dropped = true;
		ImGui::EndDragDropTarget();
	}
	return dropped;
}

/*
==============
WndTexture::DrawFolderTree
==============
*/
void WndTexture::DrawFolderTree(const std::vector<favFolder_t> &list, const std::string &parent)
{
	char label[256];

	for (const favFolder_t &f : list)
	{
		std::string path = parent.empty() ? f.name : parent + "/" + f.name;

		ImGuiTreeNodeFlags tf = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
		if (f.children.empty())
			tf |= ImGuiTreeNodeFlags_Leaf;
		if (sourceKind == SRC_FAVORITES && sourceFav == path)
			tf |= ImGuiTreeNodeFlags_Selected;

		snprintf(label, sizeof(label), "%s (%d)##fav_%s", f.name.c_str(), (int)f.items.size(), path.c_str());
		bool expanded = ImGui::TreeNodeEx(label, tf);
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			ShowFavorites(path);

		if (TexturesDropTarget())
		{
			TextureFavorites::AddItems(path, texv->Highlighted());
			RefreshSource();
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("New Subfolder..."))
				OpenFolderDialog(false, path);
			if (ImGui::MenuItem("Rename..."))
				OpenFolderDialog(true, path);
			if (ImGui::MenuItem("Delete"))
				pendingDelete = path;
			ImGui::Separator();
			if (ImGui::MenuItem("Add Highlighted Textures", nullptr, false, !texv->Highlighted().empty()))
			{
				TextureFavorites::AddItems(path, texv->Highlighted());
				RefreshSource();
			}
			ImGui::EndPopup();
		}

		if (expanded)
		{
			DrawFolderTree(f.children, path);
			ImGui::TreePop();
		}
	}
}

/*
==============
WndTexture::DrawSourcePanel
==============
*/
void WndTexture::DrawSourcePanel()
{
	char label[256];

	// the browser shows one wad at a time: start on the first one, and move back to
	// it when the wad picked was reloaded out from under us
	if (sourceKind == SRC_NONE || (sourceKind == SRC_WAD && !texv->GetSourceGroup()))
	{
		if (!Textures::groups.empty())
			ShowWad(Textures::groups.front());
		else if (Textures::group_unknown.numTextures > 0)
			ShowWad(&Textures::group_unknown);
	}

	ImGui::SeparatorText("Wads");
	auto wadItem = [&](TextureGroup *tg)
	{
		snprintf(label, sizeof(label), "%s (%d)##wad%p", tg->name.c_str(), tg->numTextures, (void *)tg);
		if (ImGui::Selectable(label, sourceKind == SRC_WAD && texv->GetSourceGroup() == tg))
			ShowWad(tg);
		if (tg != &Textures::group_unknown && ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Reload Wad"))
				Textures::MenuReloadGroup(tg);
			ImGui::EndPopup();
		}
	};
	for (TextureGroup *tg : Textures::groups)
		wadItem(tg);
	if (Textures::group_unknown.numTextures > 0)
		wadItem(&Textures::group_unknown);

	// favourites header with a new folder button at the right
	ImGui::Spacing();
	ImGui::SeparatorText("Favorites");
	ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::GetFrameHeight());
	if (ImGui::SmallButton("+##newfav"))
		OpenFolderDialog(false, std::string());
	ImGui::SetItemTooltip("New favorites folder");

	std::set<std::string> all;
	TextureFavorites::Collect(std::string(), all);
	snprintf(label, sizeof(label), "All Favorites (%d)", (int)all.size());
	if (ImGui::Selectable(label, sourceKind == SRC_FAVORITES && sourceFav.empty()))
		ShowFavorites(std::string());

	DrawFolderTree(TextureFavorites::folders, std::string());

	if (TextureFavorites::folders.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::TextWrapped("Use + to add a folder, then drag textures onto it.");
		ImGui::PopStyleColor();
	}

	// right click on empty space
	if (ImGui::BeginPopupContextWindow("##sourcectx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("New Favorites Folder..."))
			OpenFolderDialog(false, std::string());
		ImGui::EndPopup();
	}

	if (!pendingDelete.empty())
	{
		TextureFavorites::DeleteFolder(pendingDelete);
		pendingDelete.clear();
		RefreshSource();
	}
}


/*
========================================================================

	BOTTOM BAR

	hammertime's size / filter / texture info strip
========================================================================
*/

/*
==============
WndTexture::DrawSizeCombo
==============
*/
void WndTexture::DrawSizeCombo()
{
	int cur = 0;
	for (int i = 0; i < NUM_CELL_SIZES; i++)
	{
		if (s_cellSizes[i] == texv->GetCellSize())
			cur = i;
	}

	if (!ImGui::BeginCombo("##size", s_cellNames[cur]))
		return;

	for (int i = 0; i < NUM_CELL_SIZES; i++)
	{
		if (ImGui::Selectable(s_cellNames[i], i == cur))
			texv->SetCellSize(s_cellSizes[i]);
		if (i == 0)
			ImGui::SetItemTooltip("Real texture size, Ctrl+wheel zooms");
	}
	ImGui::EndCombo();
}

/*
==============
WndTexture::StepCellSize

Ctrl+wheel with a fixed cell size goes up/down the size list instead of zooming
==============
*/
void WndTexture::StepCellSize(int dir)
{
	int cur = 1;
	for (int i = 1; i < NUM_CELL_SIZES; i++)
	{
		if (s_cellSizes[i] == texv->GetCellSize())
			cur = i;
	}
	cur += dir;
	if (cur < 1) cur = 1;
	if (cur >= NUM_CELL_SIZES) cur = NUM_CELL_SIZES - 1;
	texv->SetCellSize(s_cellSizes[cur]);

	char status[64];
	snprintf(status, sizeof(status), "Texture cell size: %d", s_cellSizes[cur]);
	WndMain_Status(status, 0);
}

/*
==============
WndTexture::DrawTextureInfo

the texture under the cursor, or the selected one: name, size, which wad
==============
*/
void WndTexture::DrawTextureInfo()
{
	Texture *tex = hoverTex ? hoverTex : texv->Selected();
	size_t n = texv->Highlighted().size();

	if (!tex)
	{
		ImGui::TextDisabled("No texture selected");
		return;
	}

	ImGui::TextUnformatted(tex->name.c_str());
	ImGui::Text("%d x %d", tex->width, tex->height);

	TextureGroup *tg = texv->GroupOf(tex);
	std::string where;
	if (!tg)
		where = "?";
	else if (tg == &Textures::group_unknown)
		where = "not found in any wad";
	else if (std::filesystem::u8path(tg->name).is_absolute())
		where = tg->name;
	else
		where = std::string(radiantdef.wadPath) + "/" + tg->name;
	ImGui::Text("Group: \"%s\"", where.c_str());
	ImGui::SetItemTooltip("%s", where.c_str());

	if (n > 1)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(%d highlighted)", (int)n);
	}
}

/*
==============
WndTexture::DrawBottomBar
==============
*/
void WndTexture::DrawBottomBar()
{
	ImGuiStyle &style = ImGui::GetStyle();
	float height = BottomBarHeight();

	if (!ImGui::BeginChild("##texbottom", ImVec2(0, height), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::EndChild();
		return;
	}

	if (ImGui::BeginTable("##texbottomtable", 2, ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("controls", ImGuiTableColumnFlags_WidthStretch, 0.55f);
		ImGui::TableSetupColumn("info", ImGuiTableColumnFlags_WidthStretch, 0.45f);
		ImGui::TableNextRow();

		// size and filter, with the labels lined up
		ImGui::TableSetColumnIndex(0);
		float labelW = ImGui::CalcTextSize("Filter:").x + style.ItemSpacing.x * 2;

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Size:");
		ImGui::SameLine(labelW);
		ImGui::SetNextItemWidth(-1);
		DrawSizeCombo();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Filter:");
		ImGui::SameLine(labelW);
		float clearW = ImGui::GetFrameHeight();
		ImGui::SetNextItemWidth(-(clearW + style.ItemSpacing.x));
		if (ImGui::InputTextWithHint("##filter", "name, or type over the textures", filterBuf, sizeof(filterBuf)))
			texv->SetFilter(filterBuf);
		ImGui::SameLine();
		ImGui::BeginDisabled(filterBuf[0] == 0);
		if (ImGui::Button("x##clearfilter", ImVec2(clearW, 0)))
		{
			filterBuf[0] = 0;
			texv->SetFilter(filterBuf);
		}
		ImGui::EndDisabled();

		// TODO: needs the map to know which textures are used
		ImGui::BeginDisabled();
		bool usedOnly = false;
		ImGui::Checkbox("Only used textures", &usedOnly);
		ImGui::EndDisabled();

		ImGui::TableSetColumnIndex(1);
		DrawTextureInfo();

		ImGui::EndTable();
	}

	ImGui::EndChild();
}


/*
========================================================================

	FAVOURITES

========================================================================
*/

/*
==============
WndTexture::DrawFolderMenuItems

one item per favourites folder plus "New Folder...", adds items to the one picked
==============
*/
bool WndTexture::DrawFolderMenuItems(std::set<std::string> &items)
{
	bool done = false;
	char label[256];

	std::function<void(const std::vector<favFolder_t> &, const std::string &, int)> walk =
		[&](const std::vector<favFolder_t> &list, const std::string &parent, int depth)
	{
		for (const favFolder_t &f : list)
		{
			std::string path = parent.empty() ? f.name : parent + "/" + f.name;
			snprintf(label, sizeof(label), "%*s%s##add_%s", depth * 2, "", f.name.c_str(), path.c_str());
			if (ImGui::MenuItem(label))
			{
				TextureFavorites::AddItems(path, items);
				RefreshSource();
				done = true;
			}
			walk(f.children, path, depth + 1);
		}
	};
	walk(TextureFavorites::folders, std::string(), 0);

	if (!TextureFavorites::folders.empty())
		ImGui::Separator();
	if (ImGui::MenuItem("New Folder..."))
	{
		OpenFolderDialog(false, std::string());
		folderAddItems = items;
		done = true;
	}
	return done;
}

/*
==============
WndTexture::DrawFolderDialog
==============
*/
void WndTexture::DrawFolderDialog()
{
	const char *popupName = "Favorites Folder##favname";

	if (wantFolderDialog)
	{
		ImGui::OpenPopup(popupName);
		wantFolderDialog = false;
	}

	if (!ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	if (folderRename)
		ImGui::Text("Rename \"%s\"", folderTarget.c_str());
	else if (folderTarget.empty())
		ImGui::TextUnformatted("New folder");
	else
		ImGui::Text("New folder in \"%s\"", folderTarget.c_str());

	if (ImGui::IsWindowAppearing())
		ImGui::SetKeyboardFocusHere();
	bool enter = ImGui::InputText("##foldername", folderBuf, sizeof(folderBuf), ImGuiInputTextFlags_EnterReturnsTrue);

	bool ok = TextureFavorites::CleanName(folderBuf).size() > 0;
	ImGui::BeginDisabled(!ok);
	bool pressed = ImGui::Button("OK", ImVec2(120, 0));
	ImGui::EndDisabled();
	ImGui::SameLine();
	bool cancel = ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape);

	if ((pressed || enter) && ok)
	{
		if (folderRename)
		{
			favFolder_t *f = TextureFavorites::Find(folderTarget);
			if (f && TextureFavorites::RenameFolder(folderTarget, folderBuf))
			{
				// renaming the folder being shown (or one above it) moves the shown path too
				size_t slash = folderTarget.rfind('/');
				std::string newPath = (slash == std::string::npos ? std::string() : folderTarget.substr(0, slash + 1)) + f->name;
				if (sourceKind == SRC_FAVORITES &&
					(sourceFav == folderTarget || sourceFav.compare(0, folderTarget.size() + 1, folderTarget + "/") == 0))
					sourceFav = newPath + sourceFav.substr(folderTarget.size());
			}
		}
		else
		{
			std::string path = TextureFavorites::AddFolder(folderTarget, folderBuf);
			if (!path.empty() && !folderAddItems.empty())
				TextureFavorites::AddItems(path, folderAddItems);
		}
		folderAddItems.clear();
		RefreshSource();
		ImGui::CloseCurrentPopup();
	}
	else if (cancel)
	{
		folderAddItems.clear();
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}


/*
========================================================================

	BROWSER

========================================================================
*/

/*
==============
WndTexture::BrowserMouseXY

like Window::GetMouseXY but for the browser area: y goes up from its bottom edge
==============
*/
void WndTexture::BrowserMouseXY(int &x, int &y)
{
	ImVec2 m = ImGui::GetIO().MousePos;
	x = (int)(m.x - browserMin.x);
	y = (int)(browserMax.y - 1 - m.y);
}

/*
==============
WndTexture::TypeToFilter

typing while the mouse is over the browser edits the filter, like hammertime's
==============
*/
void WndTexture::TypeToFilter()
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantTextInput || io.KeyCtrl || io.KeyAlt)
		return;

	size_t len = strlen(filterBuf);
	bool changed = false;

	for (ImWchar c : io.InputQueueCharacters)
	{
		if (c < 32 || c >= 127 || len + 1 >= sizeof(filterBuf))
			continue;
		filterBuf[len++] = (char)c;
		filterBuf[len] = 0;
		changed = true;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && len > 0)
	{
		filterBuf[--len] = 0;
		changed = true;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Escape) && len > 0)
	{
		filterBuf[0] = 0;
		changed = true;
	}

	if (changed)
	{
		io.InputQueueCharacters.resize(0);
		texv->SetFilter(filterBuf);
	}
}

/*
==============
WndTexture::Browser

stands in for both the old WM_* mouse handling and the GL render
==============
*/
void WndTexture::Browser(ImVec2 size)
{
	ImGuiIO &io = ImGui::GetIO();
	hoverTex = nullptr;

	// room for the scrollbar on the right, the textures get the rest
	float scrollbarW = ImGui::GetStyle().ScrollbarSize;
	size.x -= scrollbarW;
	if (size.x < 1 || size.y < 1)
		return;

	browserMin = ImGui::GetCursorScreenPos();
	browserMax = ImVec2(browserMin.x + size.x, browserMin.y + size.y);
	if (size.x != lastBrowserSize.x || size.y != lastBrowserSize.y)
	{
		lastBrowserSize = size;
		texv->Resize((int)size.x, (int)size.y);
	}

	// one big invisible button: owns the mouse while a button is held, like SetCapture did
	ImGui::InvisibleButton("##texbrowser", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	bool hovered = ImGui::IsItemHovered();
	bool active = ImGui::IsItemActive();

	int x, y;
	BrowserMouseXY(x, y);

	// the wheel works over the scrollbar too
	bool overScrollbar = ImGui::IsMouseHoveringRect(ImVec2(browserMax.x, browserMin.y), ImVec2(browserMax.x + scrollbarW, browserMax.y)) &&
		ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

	if ((hovered || overScrollbar) && io.MouseWheel != 0)
	{
		Focus();
		if (io.KeyCtrl)
		{
			if (texv->GetCellSize() > 0)
				StepCellSize(io.MouseWheel < 0 ? -1 : 1);
			else
				texv->Scale(io.MouseWheel < 0 ? 0.8f : 1.25f);
		}
		else
			texv->Scroll(io.MouseWheel < 0 ? -64 : 64, io.KeyShift);
		WndMain_UpdateWindows(W_TEXTURE);
	}

	if (hovered)
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			MouseDown(x, y, MOUSE_LEFT);
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			MouseDown(x, y, MOUSE_RIGHT);
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
			MouseDown(x, y, MOUSE_MIDDLE);
		TypeToFilter();
		hoverTex = texv->TexAtCursorPos(x, y);
	}
	if (active || hovered)
	{
		if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)
			MouseMoved(x, y, io.MouseDelta.y);
	}
	// not IsItemActive: ImGui already let go of the button on the release frame
	if (rPressed && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		rPressed = false;
		MouseUp(x, y, MOUSE_RIGHT);
	}

	// dragging the highlighted textures onto a favourites folder (left button only,
	// right drag scrolls)
	if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !texv->Highlighted().empty())
	{
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(TEXTURES_PAYLOAD, nullptr, 0);
			size_t n = texv->Highlighted().size();
			if (n == 1)
				ImGui::Text("%s", texv->Highlighted().begin()->c_str());
			else
				ImGui::Text("%d textures", (int)n);
			ImGui::TextDisabled("drop on a favorites folder");
			ImGui::EndDragDropSource();
		}
	}

	Render();
	Scrollbar(ImVec2(browserMax.x, browserMin.y), ImVec2(browserMax.x + scrollbarW, browserMax.y));
}

/*
==============
WndTexture::Scrollbar

TextureView scrolls downward into negative y (0 is the top), ImGui's scrollbar
counts up from 0, so the value is just flipped
==============
*/
void WndTexture::Scrollbar(ImVec2 min, ImVec2 max)
{
	ImS64 visible = (ImS64)texv->GetHeight();
	ImS64 content = (ImS64)texv->GetLayoutLength();
	if (content < visible)
		content = visible;

	ImS64 pos = -(ImS64)texv->GetScroll();
	if (ImGui::ScrollbarEx(ImRect(min, max), ImGui::GetID("##texscroll"), ImGuiAxis_Y, &pos, visible, content, ImDrawFlags_RoundCornersAll))
	{
		texv->SetScroll(-(int)pos);
		WndMain_UpdateWindows(W_TEXTURE);
	}
}

/*
==============
WndTexture::Render

port of TexBrowserRenderer::Draw
==============
*/
void WndTexture::Render()
{
	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 origin = browserMin;
	int tvw = texv->GetWidth();
	int tvh = texv->GetHeight();
	float fontSize = ImGui::GetFontSize();

	ImU32 colBackground = ImGui::GetColorU32(ImGuiCol_FrameBg);
	ImU32 colText = ImGui::GetColorU32(ImGuiCol_Text);
	ImU32 colAvg = ImGui::GetColorU32(ImGuiCol_TextDisabled);
	ImU32 colSelection = IM_COL32(255, 0, 0, 255);
	ImU32 colHighlight = IM_COL32(255, 200, 0, 255);

	texv->Arrange();
	int scroll = texv->GetScroll();

	// layout space -> screen: the top edge of the browser is y = scroll
	auto sx = [&](float lx) { return origin.x + lx; };
	auto sy = [&](float ly) { return origin.y + (scroll - ly); };

	dl->PushClipRect(origin, ImVec2(origin.x + tvw, origin.y + tvh), true);
	dl->AddRectFilled(origin, ImVec2(origin.x + tvw, origin.y + tvh), colBackground);

	if (texv->layout.empty())
	{
		const char *empty = texv->GetFilter().empty() ? "No textures" : "No textures match the filter";
		dl->AddText(ImVec2(origin.x + MARGIN_X, origin.y + MARGIN_X), colAvg, empty);
	}

	Texture *selected = texv->Selected();

	for (const TextureView::texWndItem_t &thumb : texv->layout)
	{
		// Is this texture visible?
		if (thumb.y < scroll - tvh)
			break;
		if (thumb.y - thumb.h - FONT_HEIGHT > scroll)
			continue;

		float yTop = (float)thumb.y;
		float x = (float)thumb.x;
		ImVec2 imgMin(sx(x), sy(yTop - FONT_HEIGHT));
		ImVec2 imgMax(sx(x + thumb.w), sy(yTop - FONT_HEIGHT - thumb.h));

		// if in use, draw a background
		if (thumb.tex->used)
			dl->AddRect(ImVec2(imgMin.x - 1, imgMin.y - 1), ImVec2(imgMax.x + 1, imgMax.y + 1), colAvg);

		// Draw the texture
		dl->AddImage((ImTextureID)(uintptr_t)thumb.tex->glTex.TexNum(), imgMin, imgMax);

		// highlighted (multi) and the work texture
		if (thumb.tex != selected && texv->IsHighlighted(thumb.tex))
			dl->AddRect(ImVec2(imgMin.x - 3, imgMin.y - 3), ImVec2(imgMax.x + 3, imgMax.y + 3), colHighlight, 0.0f, ImDrawFlags_None, 2.0f);
		if (thumb.tex == selected)
			dl->AddRect(ImVec2(imgMin.x - 4, imgMin.y - 4), ImVec2(imgMax.x + 4, imgMax.y + 4), colSelection, 0.0f, ImDrawFlags_None, 3.0f);

		// draw the texture name, without the directory
		const char *texname = thumb.tex->name.c_str();
		const char *slash = texname;
		for (const char *c = texname; *c; c++)
		{
			if (*c == '/' || *c == '\\')
				slash = c + 1;
		}
		// clipped to the thumbnail's slot so long names don't run into the next one
		float textY = sy(yTop - FONT_HEIGHT + 2) - fontSize;
		dl->PushClipRect(ImVec2(sx(x), textY), ImVec2(sx(x + thumb.slot + MARGIN_X - 2), textY + fontSize), true);
		dl->AddText(ImVec2(sx(x), textY), colText, slash);
		dl->PopClipRect();
	}

	dl->PopClipRect();
}

/*
==============
WndTexture::DoPopupMenu

quaked3's IDR_CONTEXT_TEXGRP: the default / wad / texture submenus are one
menu here, with the wad and texture items only when something is under the cursor
==============
*/
void WndTexture::DoPopupMenu()
{
	if (!ImGui::BeginPopup("##texcontext"))
		return;

	// TODO: hide unused needs the map, load/add wad needs a file dialog
	ImGui::MenuItem("Hide Unused", "U", false, false);
	ImGui::MenuItem("Load/Add Wad...", nullptr, false, false);
	if (popupGroup && popupGroup != &Textures::group_unknown)
	{
		if (ImGui::MenuItem("Reload Wad"))
			Textures::MenuReloadGroup(popupGroup);
	}

	std::set<std::string> highlighted = texv->Highlighted();
	if (!highlighted.empty())
	{
		ImGui::Separator();
		char label[64];
		snprintf(label, sizeof(label), "Add to Favorites (%d)", (int)highlighted.size());
		if (ImGui::BeginMenu(label))
		{
			DrawFolderMenuItems(highlighted);
			ImGui::EndMenu();
		}
		if (sourceKind == SRC_FAVORITES)
		{
			snprintf(label, sizeof(label), "Remove from Favorites (%d)", (int)highlighted.size());
			if (ImGui::MenuItem(label))
			{
				TextureFavorites::RemoveItems(sourceFav, highlighted);
				texv->ClearHighlight();
				RefreshSource();
			}
		}
	}

	if (popupTex)
	{
		ImGui::Separator();
		// TODO: Selection::MatchingTextures and the find/replace dialog once they're ported
		ImGui::MenuItem("Select Matching", nullptr, false, false);
		ImGui::MenuItem("Replace Matching...", nullptr, false, false);
	}

	ImGui::Separator();
	if (ImGui::MenuItem("Reset Window Scale"))
		WndMain_Command(ID_TEXTURES_RESETSCALE);

	ImGui::EndPopup();
}

/*
==============
WndTexture::UpdateHighlightStatus
==============
*/
void WndTexture::UpdateHighlightStatus()
{
	size_t n = texv->Highlighted().size();
	if (n < 2)
		return;

	char status[64];
	snprintf(status, sizeof(status), "%d textures highlighted", (int)n);
	WndMain_Status(status, 0);
}

/*
==============
WndTexture::MouseDown
==============
*/
void WndTexture::MouseDown(const int x, const int y, const int btndown)
{
	ImGuiIO &io = ImGui::GetIO();
	Focus();

	// Context Menu
	if (btndown == MOUSE_RIGHT)
	{
		rMoved = false;
		rPressed = true;
		mDownX = x;
		mDownY = y;
	}

	if (btndown == MOUSE_LEFT)
	{
		Texture *tex = texv->TexAtCursorPos(x, y);

		// hammertime's highlighting: ctrl toggles, shift extends, a plain click picks one
		if (io.KeyCtrl)
		{
			texv->ToggleHighlight(tex);
			UpdateHighlightStatus();
			return;
		}
		if (io.KeyShift)
		{
			texv->HighlightRange(tex);
			UpdateHighlightStatus();
			return;
		}

		texv->HighlightOnly(tex);

		// TODO: quaked3 does this in TextureTool, which also applies the texture to
		// the selected brushes/faces (shift/ctrl variants) once selection is ported
		if (tex)
			texv->ChooseTexture(tex);
	}
}

/*
==============
WndTexture::MouseMoved
==============
*/
void WndTexture::MouseMoved(const int x, const int y, const float dy)
{
	ImGuiIO &io = ImGui::GetIO();
	float scale;
	bool rDown = ImGui::IsMouseDown(ImGuiMouseButton_Right) && ImGui::IsItemActive();

	texv->Arrange();

	// sikk--->	Mouse Zoom Texture Window
	// rbutton+control = zoom texture view (real size only, cells go by the size combo)
	if (rDown && io.KeyCtrl)
	{
		rMoved = true;
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);
		if (dy != 0 && texv->GetCellSize() == 0)
		{
			if (dy > 0)
				scale = powf(1.01f, fabsf(dy));
			else
				scale = powf(0.99f, fabsf(dy));

			texv->Scale(scale);
			WndMain_UpdateWindows(W_TEXTURE);
		}
		return;
	}
	// <---sikk

	// rbutton = drag texture origin
	if (rDown)
	{
		// Context Menu
		if (!rMoved && (abs(mDownX - x) > 1 || abs(mDownY - y) > 1))
			rMoved = true;

		if (rMoved)
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
			if (dy != 0)
			{
				texv->Scroll((int)dy, io.KeyShift);
				WndMain_UpdateWindows(W_TEXTURE);
			}
		}
	}
	else
		MouseOver(x, y);
}

/*
============
WndTexture::MouseOver
============
*/
void WndTexture::MouseOver(const int x, const int y)
{
	char		texstring[256];
	Texture		*tex;

	tex = texv->TexAtCursorPos(x, y);
	if (tex)
	{
		snprintf(texstring, sizeof(texstring), "%s (%dx%d)", tex->name.c_str(), tex->width, tex->height);
		WndMain_Status(texstring, 0);
		return;
	}
	WndMain_Status("", 0);
}

/*
============
WndTexture::MouseUp
============
*/
void WndTexture::MouseUp(const int x, const int y, const int btnup)
{
	// Context Menu
	if (btnup == MOUSE_RIGHT && !rMoved)
	{
		popupTex = texv->TexAtCursorPos(mDownX, mDownY);
		// the texture's own wad, for "Reload Wad"
		popupGroup = popupTex ? texv->GroupOf(popupTex) : texv->GetSourceGroup();

		// right clicking outside the highlight works on just that texture, like a file browser
		if (popupTex && !texv->IsHighlighted(popupTex))
			texv->HighlightOnly(popupTex);

		wantPopup = true;
	}
	(void)x;
	(void)y;
}
