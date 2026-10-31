//==============================
//	textureFavorites.cpp
//==============================

#include "ui/favourites/textureFavorites.h"

#include <imgui.h>
#include <imgui_internal.h>	// ImGuiSettingsHandler

#include <algorithm>
#include <cstring>

std::vector<favFolder_t> TextureFavorites::folders;

/*
==================
TextureFavorites::CleanName
==================
*/
std::string TextureFavorites::CleanName(const std::string &name)
{
	std::string out = name;
	std::replace(out.begin(), out.end(), '/', '_');
	std::replace(out.begin(), out.end(), '|', '_');
	out.erase(0, out.find_first_not_of(" \t"));
	out.erase(out.find_last_not_of(" \t") + 1);
	return out;
}

/*
==================
FindIn

walks a "a/b/c" path; also hands back the vector the folder lives in
==================
*/
static favFolder_t *FindIn(std::vector<favFolder_t> &list, const std::string &path, std::vector<favFolder_t> **owner)
{
	size_t slash = path.find('/');
	std::string head = path.substr(0, slash);

	for (favFolder_t &f : list)
	{
		if (f.name != head)
			continue;
		if (slash == std::string::npos)
		{
			if (owner)
				*owner = &list;
			return &f;
		}
		return FindIn(f.children, path.substr(slash + 1), owner);
	}
	return nullptr;
}

favFolder_t *TextureFavorites::Find(const std::string &path)
{
	if (path.empty())
		return nullptr;
	return FindIn(folders, path, nullptr);
}

/*
==================
UniqueName

same as hammertime: name, name_1, name_2...
==================
*/
static std::string UniqueName(const std::vector<favFolder_t> &siblings, const std::string &name)
{
	std::string uniq = name;
	int counter = 1;
	auto taken = [&](const std::string &n) {
		return std::any_of(siblings.begin(), siblings.end(), [&](const favFolder_t &f) { return f.name == n; });
	};
	while (taken(uniq))
		uniq = name + "_" + std::to_string(counter++);
	return uniq;
}

/*
==================
TextureFavorites::AddFolder
==================
*/
std::string TextureFavorites::AddFolder(const std::string &parentPath, const std::string &name)
{
	std::string clean = CleanName(name);
	if (clean.empty())
		return std::string();

	favFolder_t *parent = Find(parentPath);
	std::vector<favFolder_t> &siblings = parent ? parent->children : folders;

	favFolder_t f;
	f.name = UniqueName(siblings, clean);
	siblings.push_back(f);
	ImGui::MarkIniSettingsDirty();

	return parent ? parentPath + "/" + f.name : f.name;
}

/*
==================
TextureFavorites::RenameFolder
==================
*/
bool TextureFavorites::RenameFolder(const std::string &path, const std::string &name)
{
	std::vector<favFolder_t> *owner = nullptr;
	favFolder_t *f = path.empty() ? nullptr : FindIn(folders, path, &owner);
	std::string clean = CleanName(name);
	if (!f || clean.empty() || clean == f->name)
		return false;

	f->name = UniqueName(*owner, clean);
	ImGui::MarkIniSettingsDirty();
	return true;
}

/*
==================
TextureFavorites::DeleteFolder
==================
*/
bool TextureFavorites::DeleteFolder(const std::string &path)
{
	std::vector<favFolder_t> *owner = nullptr;
	favFolder_t *f = path.empty() ? nullptr : FindIn(folders, path, &owner);
	if (!f)
		return false;

	owner->erase(owner->begin() + (f - owner->data()));
	ImGui::MarkIniSettingsDirty();
	return true;
}

/*
==================
TextureFavorites::AddItems
==================
*/
void TextureFavorites::AddItems(const std::string &path, const std::set<std::string> &names)
{
	favFolder_t *f = Find(path);
	if (!f)
		return;

	for (const std::string &n : names)
	{
		if (std::find(f->items.begin(), f->items.end(), n) == f->items.end())
			f->items.push_back(n);
	}
	ImGui::MarkIniSettingsDirty();
}

/*
==================
TextureFavorites::RemoveItems
==================
*/
static void RemoveFrom(favFolder_t &f, const std::set<std::string> &names)
{
	f.items.erase(std::remove_if(f.items.begin(), f.items.end(),
		[&](const std::string &n) { return names.count(n) > 0; }), f.items.end());
	for (favFolder_t &c : f.children)
		RemoveFrom(c, names);
}

void TextureFavorites::RemoveItems(const std::string &path, const std::set<std::string> &names)
{
	if (path.empty())
	{
		for (favFolder_t &f : folders)
			RemoveFrom(f, names);
	}
	else if (favFolder_t *f = Find(path))
		RemoveFrom(*f, names);

	ImGui::MarkIniSettingsDirty();
}

/*
==================
TextureFavorites::Collect
==================
*/
static void CollectFrom(const favFolder_t &f, std::set<std::string> &out)
{
	out.insert(f.items.begin(), f.items.end());
	for (const favFolder_t &c : f.children)
		CollectFrom(c, out);
}

void TextureFavorites::Collect(const std::string &path, std::set<std::string> &out)
{
	if (path.empty())
	{
		for (const favFolder_t &f : folders)
			CollectFrom(f, out);
	}
	else if (const favFolder_t *f = Find(path))
		CollectFrom(*f, out);
}


/*
========================================================================

	RADIANT.INI

	[RadiantFavorites][Data]
	Folder=walls
	Folder=walls/brick
	Item=walls/brick|brick1_1

========================================================================
*/

/*
==================
EnsurePath

creates every folder along the path while reading, keeping the saved names as they are
==================
*/
static favFolder_t *EnsurePath(const std::string &path)
{
	std::vector<favFolder_t> *list = &TextureFavorites::folders;
	favFolder_t *f = nullptr;
	size_t start = 0;

	while (start <= path.size())
	{
		size_t slash = path.find('/', start);
		std::string part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
		if (part.empty())
			break;

		auto it = std::find_if(list->begin(), list->end(), [&](const favFolder_t &x) { return x.name == part; });
		if (it == list->end())
		{
			list->push_back(favFolder_t());
			list->back().name = part;
			it = list->end() - 1;
		}
		f = &*it;
		list = &f->children;

		if (slash == std::string::npos)
			break;
		start = slash + 1;
	}
	return f;
}

static void *Fav_ReadOpen(ImGuiContext *, ImGuiSettingsHandler *, const char *name)
{
	if (strcmp(name, "Data") != 0)
		return nullptr;
	TextureFavorites::folders.clear();
	return (void *)1;
}

static void Fav_ReadLine(ImGuiContext *, ImGuiSettingsHandler *, void *, const char *line)
{
	if (!strncmp(line, "Folder=", 7))
	{
		EnsurePath(line + 7);
	}
	else if (!strncmp(line, "Item=", 5))
	{
		const char *bar = strchr(line + 5, '|');
		if (!bar || !bar[1])
			return;
		favFolder_t *f = EnsurePath(std::string(line + 5, bar));
		if (f)
			f->items.push_back(bar + 1);
	}
}

static void WriteFolder(ImGuiTextBuffer *buf, const favFolder_t &f, const std::string &parent)
{
	std::string path = parent.empty() ? f.name : parent + "/" + f.name;
	buf->appendf("Folder=%s\n", path.c_str());
	for (const std::string &item : f.items)
		buf->appendf("Item=%s|%s\n", path.c_str(), item.c_str());
	for (const favFolder_t &c : f.children)
		WriteFolder(buf, c, path);
}

static void Fav_WriteAll(ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
{
	buf->appendf("[%s][Data]\n", handler->TypeName);
	for (const favFolder_t &f : TextureFavorites::folders)
		WriteFolder(buf, f, std::string());
	buf->append("\n");
}

/*
==================
TextureFavorites::RegisterSettings
==================
*/
void TextureFavorites::RegisterSettings()
{
	ImGuiSettingsHandler handler;
	handler.TypeName = "RadiantFavorites";
	handler.TypeHash = ImHashStr("RadiantFavorites");
	handler.ReadOpenFn = Fav_ReadOpen;
	handler.ReadLineFn = Fav_ReadLine;
	handler.WriteAllFn = Fav_WriteAll;
	ImGui::AddSettingsHandler(&handler);
}
