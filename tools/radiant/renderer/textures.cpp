//==============================
//	textures.cpp
//==============================

#include "renderer/textures.h"
#include "renderer/texture.h"
#include "ui/textures/textureView.h"
#include "ui/wndMain.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

extern int g_nTextureMode;

static bool g_bTexturesChanged = false;

std::vector<TextureGroup*>		Textures::groups;
std::map<std::string, Texture*>	Textures::texMap;
TextureGroup					Textures::group_solid;
TextureGroup					Textures::group_unknown;
std::string						Textures::wadPath;


/*
========================================================================

	PALETTE

	quake's palette.lmp, used for WAD2. WAD3 textures carry their own
========================================================================
*/

static unsigned char s_palette[768] = {
	0, 0, 0, 15, 15, 15, 31, 31, 31, 47, 47, 47, 63, 63, 63, 75, 75, 75, 91, 91, 91, 107, 107, 107,
	123, 123, 123, 139, 139, 139, 155, 155, 155, 171, 171, 171, 187, 187, 187, 203, 203, 203, 219, 219, 219, 235, 235, 235,
	15, 11, 7, 23, 15, 11, 31, 23, 11, 39, 27, 15, 47, 35, 19, 55, 43, 23, 63, 47, 23, 75, 55, 27,
	83, 59, 27, 91, 67, 31, 99, 75, 31, 107, 83, 31, 115, 87, 31, 123, 95, 35, 131, 103, 35, 143, 111, 35,
	11, 11, 15, 19, 19, 27, 27, 27, 39, 39, 39, 51, 47, 47, 63, 55, 55, 75, 63, 63, 87, 71, 71, 103,
	79, 79, 115, 91, 91, 127, 99, 99, 139, 107, 107, 151, 115, 115, 163, 123, 123, 175, 131, 131, 187, 139, 139, 203,
	0, 0, 0, 7, 7, 0, 11, 11, 0, 19, 19, 0, 27, 27, 0, 35, 35, 0, 43, 43, 7, 47, 47, 7,
	55, 55, 7, 63, 63, 7, 71, 71, 7, 75, 75, 11, 83, 83, 11, 91, 91, 11, 99, 99, 11, 107, 107, 15,
	7, 0, 0, 15, 0, 0, 23, 0, 0, 31, 0, 0, 39, 0, 0, 47, 0, 0, 55, 0, 0, 63, 0, 0,
	71, 0, 0, 79, 0, 0, 87, 0, 0, 95, 0, 0, 103, 0, 0, 111, 0, 0, 119, 0, 0, 127, 0, 0,
	19, 19, 0, 27, 27, 0, 35, 35, 0, 47, 43, 0, 55, 47, 0, 67, 55, 0, 75, 59, 7, 87, 67, 7,
	95, 71, 7, 107, 75, 11, 119, 83, 15, 131, 87, 19, 139, 91, 19, 151, 95, 27, 163, 99, 31, 175, 103, 35,
	35, 19, 7, 47, 23, 11, 59, 31, 15, 75, 35, 19, 87, 43, 23, 99, 47, 31, 115, 55, 35, 127, 59, 43,
	143, 67, 51, 159, 79, 51, 175, 99, 47, 191, 119, 47, 207, 143, 43, 223, 171, 39, 239, 203, 31, 255, 243, 27,
	11, 7, 0, 27, 19, 0, 43, 35, 15, 55, 43, 19, 71, 51, 27, 83, 55, 35, 99, 63, 43, 111, 71, 51,
	127, 83, 63, 139, 95, 71, 155, 107, 83, 167, 123, 95, 183, 135, 107, 195, 147, 123, 211, 163, 139, 227, 179, 151,
	171, 139, 163, 159, 127, 151, 147, 115, 135, 139, 103, 123, 127, 91, 111, 119, 83, 99, 107, 75, 87, 95, 63, 75,
	87, 55, 67, 75, 47, 55, 67, 39, 47, 55, 31, 35, 43, 23, 27, 35, 19, 19, 23, 11, 11, 15, 7, 7,
	187, 115, 159, 175, 107, 143, 163, 95, 131, 151, 87, 119, 139, 79, 107, 127, 75, 95, 115, 67, 83, 107, 59, 75,
	95, 51, 63, 83, 43, 55, 71, 35, 43, 59, 31, 35, 47, 23, 27, 35, 19, 19, 23, 11, 11, 15, 7, 7,
	219, 195, 187, 203, 179, 167, 191, 163, 155, 175, 151, 139, 163, 135, 123, 151, 123, 111, 135, 111, 95, 123, 99, 83,
	107, 87, 71, 95, 75, 59, 83, 63, 51, 67, 51, 39, 55, 43, 31, 39, 31, 23, 27, 19, 15, 15, 11, 7,
	111, 131, 123, 103, 123, 111, 95, 115, 103, 87, 107, 95, 79, 99, 87, 71, 91, 79, 63, 83, 71, 55, 75, 63,
	47, 67, 55, 43, 59, 47, 35, 51, 39, 31, 43, 31, 23, 35, 23, 15, 27, 19, 11, 19, 11, 7, 11, 7,
	255, 243, 27, 239, 223, 23, 219, 203, 19, 203, 183, 15, 187, 167, 15, 171, 151, 11, 155, 131, 7, 139, 115, 7,
	123, 99, 7, 107, 83, 0, 91, 71, 0, 75, 55, 0, 59, 43, 0, 43, 31, 0, 27, 15, 0, 11, 7, 0,
	0, 0, 255, 11, 11, 239, 19, 19, 223, 27, 27, 207, 35, 35, 191, 43, 43, 175, 47, 47, 159, 47, 47, 143,
	47, 47, 127, 47, 47, 111, 47, 47, 95, 43, 43, 79, 35, 35, 63, 27, 27, 47, 19, 19, 31, 11, 11, 15,
	43, 0, 0, 59, 0, 0, 75, 7, 0, 95, 7, 0, 111, 15, 0, 127, 23, 7, 147, 31, 7, 163, 39, 11,
	183, 51, 15, 195, 75, 27, 207, 99, 43, 219, 127, 59, 227, 151, 79, 231, 171, 95, 239, 191, 119, 247, 211, 139,
	167, 123, 59, 183, 155, 55, 199, 195, 55, 231, 227, 87, 127, 191, 255, 171, 231, 255, 215, 255, 255, 103, 0, 0,
	139, 0, 0, 179, 0, 0, 215, 0, 0, 255, 0, 0, 255, 243, 147, 255, 247, 199, 255, 255, 255, 159, 91, 83,
};

/*
==================
Textures::LoadPalette

<wadPath>/gfx/palette.lmp if there is one, otherwise the built in quake palette
TODO: gamma/brightness once there are editor settings
==================
*/
void Textures::LoadPalette()
{
	fs::path pal = fs::path(wadPath) / "gfx" / "palette.lmp";
	std::ifstream f(pal, std::ios::binary);
	if (!f)
		return;

	unsigned char buf[768];
	if (!f.read((char *)buf, sizeof(buf)))
	{
		SDL_Log("Problem loading palette %s (bad length), using the default", pal.string().c_str());
		return;
	}
	memcpy(s_palette, buf, sizeof(buf));
	SDL_Log("Palette loaded from %s", pal.string().c_str());
}


/*
========================================================================

	WAD READER

========================================================================
*/

#define	TYP_MIPTEX2	68		// 'D', WAD2 (quake)
#define	TYP_MIPTEX3	67		// 'C', WAD3 (half-life)
#define	MIPLEVELS	4

#pragma pack(push, 1)
struct miptex_t
{
	char		name[MAX_TEXNAME];
	uint32_t	width, height;
	uint32_t	offsets[MIPLEVELS];	// four mip maps stored
};

struct wadinfo_t
{
	char		identification[4];	// WAD2 or WAD3
	int32_t		numlumps;
	int32_t		infotableofs;
};

struct lumpinfo_t
{
	int32_t		filepos;
	int32_t		disksize;
	int32_t		size;				// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];			// must be null terminated
};
#pragma pack(pop)

/*
==================
ReadWad
==================
*/
static TextureGroup *ReadWad(const std::string &filename)
{
	std::ifstream f(fs::u8path(filename), std::ios::binary);
	if (!f)
	{
		SDL_Log("Couldn't load wad %s", filename.c_str());
		return nullptr;
	}
	std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	size_t size = buf.size();

	if (size < sizeof(wadinfo_t))
	{
		SDL_Log("Couldn't load wad %s (not a valid wadfile)", filename.c_str());
		return nullptr;
	}

	wadinfo_t *wadinfo = (wadinfo_t *)buf.data();
	bool wad3;
	if (!strncmp(wadinfo->identification, "WAD2", 4))
		wad3 = false;
	else if (!strncmp(wadinfo->identification, "WAD3", 4))
		wad3 = true;
	else
	{
		SDL_Log("Couldn't load wad %s (not a valid wadfile)", filename.c_str());
		return nullptr;
	}

	if (wadinfo->infotableofs < 0 || wadinfo->numlumps < 0 ||
		(size_t)wadinfo->infotableofs + (size_t)wadinfo->numlumps * sizeof(lumpinfo_t) > size)
	{
		SDL_Log("Couldn't load wad %s (bad lump table)", filename.c_str());
		return nullptr;
	}

	lumpinfo_t *lumps = (lumpinfo_t *)(buf.data() + wadinfo->infotableofs);
	TextureGroup *wad = new TextureGroup();
	wad->name = filename;

	std::vector<unsigned char> rgba;

	for (int i = 0; i < wadinfo->numlumps; ++i)
	{
		char lumpname[17] = {};
		memcpy(lumpname, lumps[i].name, 16);

		if (lumps[i].type != (wad3 ? TYP_MIPTEX3 : TYP_MIPTEX2) || lumps[i].compression)
			continue;	// palettes, conchars, qpics... not textures

		if (lumps[i].filepos < 0 || (size_t)lumps[i].filepos + sizeof(miptex_t) > size)
		{
			SDL_Log("%s: %s is incomplete, ignoring", filename.c_str(), lumpname);
			continue;
		}

		miptex_t *qmip = (miptex_t *)(buf.data() + lumps[i].filepos);
		if (qmip->width > 1024 || qmip->width < 1 || qmip->height > 1024 || qmip->height < 1)
		{
			SDL_Log("%s: %s has bad size, ignoring", filename.c_str(), lumpname);
			continue;
		}

		size_t count = qmip->width * qmip->height;
		size_t base = (size_t)lumps[i].filepos;
		if (!qmip->offsets[0] || base + qmip->offsets[0] + count > size)
		{
			SDL_Log("%s: %s is incomplete, ignoring", filename.c_str(), lumpname);
			continue;
		}

		const unsigned char *src = buf.data() + base + qmip->offsets[0];
		const unsigned char *pal = s_palette;

		if (wad3)
		{
			// after the 4th mip: short color count, then the 256 color palette
			size_t palofs = base + qmip->offsets[3] + (qmip->width / 8) * (qmip->height / 8) + 2;
			if (!qmip->offsets[3] || palofs + 768 > size)
			{
				SDL_Log("%s: %s has no palette, ignoring", filename.c_str(), lumpname);
				continue;
			}
			pal = buf.data() + palofs;
		}

		// '{' textures are cutouts: the last palette color is see-through
		bool cutout = (lumpname[0] == '{');
		float color[3] = { 0, 0, 0 };
		rgba.resize(count * 4);

		// rows are stored top first, which is what the browser wants
		for (size_t j = 0; j < count; ++j)
		{
			const unsigned char *c = pal + src[j] * 3;
			rgba[j * 4 + 0] = c[0];
			rgba[j * 4 + 1] = c[1];
			rgba[j * 4 + 2] = c[2];
			rgba[j * 4 + 3] = (cutout && src[j] == 255) ? 0 : 255;
			color[0] += c[0];
			color[1] += c[1];
			color[2] += c[2];
		}
		color[0] /= count;
		color[1] /= count;
		color[2] /= count;

		// the lump name is the reliable one, some tools leave junk in the miptex name
		wad->Add(new Texture(qmip->width, qmip->height, lumpname, color, rgba.data()));
	}

	return wad;
}


/*
========================================================================

	LIBRARY

========================================================================
*/

/*
==================
Textures::Init
==================
*/
void Textures::Init(const std::string &wadDir)
{
	SDL_Log("Initializing textures");

	wadPath = wadDir;
	if (!wadPath.empty() && wadPath.back() != '/' && wadPath.back() != '\\')
		wadPath += '/';

	LoadPalette();

	// create solid color group
	group_solid.name = "*solid";
	// * is invalid as a filename character, so a theoretical wad named "solid" can't collide
	group_unknown.name = "*unknown";
}

/*
==================
Textures::Shutdown
==================
*/
void Textures::Shutdown()
{
	FlushAll();
	group_solid.Flush();
}

/*
==================
Textures::LibraryChanged
==================
*/
void Textures::LibraryChanged()
{
	g_bTexturesChanged = true;
	g_vTexture.Refresh();
}

/*
==================
Textures::HandleChange

some texture housekeeping is best done post-frame so it isn't repeated
wastefully, ie after each wad loaded in sequence
==================
*/
void Textures::HandleChange()
{
	if (!g_bTexturesChanged) return;

	// 1) remove dead unknown textures that may have been replaced by real textures
	Textures::CleanUnknowns();
	// 2) remove any dead icons in the texture browser
	g_vTexture.Refresh();
	// 3) sort out what texture is selected/bound
	Textures::FixWorkTexDef();
	// 2 comes before 3 because FWTD may select the first texture in the browser as a
	// default, but if the map is new and all textures are flushed, that first texture
	// is a null reference

	WndMain_UpdateWindows(W_CAMERA | W_TEXTURE | W_SURF);

	g_bTexturesChanged = false;
}

/*
==================
Textures::CleanUnknowns

clean up old unknown textures since they're not owned by a wad, and thus not
cleaned up the normal way
==================
*/
void Textures::CleanUnknowns()
{
	Texture **tex, *temp;

	if (group_unknown.numTextures > 0)
	{
		for (tex = &group_unknown.first; *tex; )
		{
			if (*tex != texMap[(*tex)->name])
			{
				temp = *tex;
				*tex = (*tex)->next;
				delete temp;
				group_unknown.numTextures--;
			}
			else
			{
				tex = &(*tex)->next;
			}
		}
	}
}

/*
==================
Textures::FixWorkTexDef

keep the selected texture in the browser valid after the library changes
==================
*/
void Textures::FixWorkTexDef()
{
	Texture *sel = g_vTexture.Selected();

	// the selected texture may have been deleted with its wad: look it up again by name
	if (sel)
	{
		auto it = texMap.find(g_vTexture.SelectedName());
		sel = (it != texMap.end()) ? it->second : nullptr;
	}

	g_vTexture.ChooseTexture(sel);
}

/*
==================
Textures::FlushAll
==================
*/
void Textures::FlushAll()
{
	// delete all texture groups
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		delete *tgIt;
	groups.clear();
	group_unknown.Flush();
	g_vTexture.ChooseTexture(nullptr);
	Textures::texMap.clear();
	LibraryChanged();
}

/*
==================
Textures::RefreshUsedStatus
==================
*/
void Textures::RefreshUsedStatus()
{
	// call clearused on all texture groups
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		(*tgIt)->ClearUsed();
	group_unknown.ClearUsed();	// don't forget meee

	// TODO: refresh the texdefs of every brush (selected, active, regioned) once
	// the map is ported, that's what marks textures used again

	g_vTexture.Refresh();
}

/*
==================
HexToRGB

"#rrggbb" -> 0-255 floats
==================
*/
static void HexToRGB(const std::string &name, float color[3])
{
	std::string hex = name.substr(name[0] == '#' ? 1 : 0, 6);
	unsigned long rgb = strtoul(hex.c_str(), nullptr, 16);
	color[0] = (float)((rgb >> 16) & 255);
	color[1] = (float)((rgb >> 8) & 255);
	color[2] = (float)(rgb & 255);
}

/*
==================
Textures::CreateSolid
==================
*/
Texture *Textures::CreateSolid(const std::string &name)
{
	float			color[3];
	unsigned char	data[4];
	Texture			*solidtex;

	// make solid texture from name
	HexToRGB(name, color);

	data[0] = (unsigned char)color[0];
	data[1] = (unsigned char)color[1];
	data[2] = (unsigned char)color[2];
	data[3] = 255;

	solidtex = new Texture(1, 1, name, color, data);

	// stash in the solid color group
	group_solid.Add(solidtex);
	return solidtex;
}

/*
==================
Textures::CreateBroken

universal stand in for all broken texture entries
==================
*/
Texture *Textures::CreateBroken(const std::string &name)
{
	static unsigned char data[64 * 64 * 4];
	float	color[3] = { 128, 0, 128 };
	int		x, y, ofs;

	memset(data, 0, sizeof(data));

	// hot programmer pink checkerboard
	for (x = 0; x < 32; x++) for (y = 0; y < 32; y++)
	{
		ofs = 4 * (x * 64 + y);
		data[ofs] = 255;
		data[ofs + 2] = 255;
		data[ofs + 3] = 255;

		ofs = 4 * ((x + 32) * 64 + y + 32);
		data[ofs] = 255;
		data[ofs + 2] = 255;
		data[ofs + 3] = 255;
	}
	// and opaque black squares in between
	for (x = 0; x < 64; x++) for (y = 0; y < 64; y++)
		data[4 * (x * 64 + y) + 3] = 255;

	return new Texture(64, 64, name, color, data);
}

/*
==================
Textures::ForName

keeping every wad in a separate group means textures with matching names
never clash, which would be great if not for the fact that it's inconsistent with
the way qbsp behaves (where textures loaded later override those loaded earlier).
the name map speeds up lookups, and also provides late-stomps-early as a
convenient side effect.
==================
*/
Texture *Textures::ForName(const std::string &name)
{
	Texture *tx;
	std::string namelc = name;
	std::transform(namelc.begin(), namelc.end(), namelc.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });

	// keep the solid textures out of the main texture space
	if (!name.empty() && name[0] == '#')
	{
		float color[3];
		HexToRGB(name, color);
		tx = group_solid.ByColor(color);
		if (!tx)
			tx = CreateSolid(name);
		return tx;
	}

	// check the global texture name map
	tx = texMap[namelc];
	if (tx)
	{
		tx->Use();
		return tx;
	}

	// still not found, make nulltexture and put it in the unknown wad
	tx = CreateBroken(namelc);
	tx->next = nullptr;

	group_unknown.Add(tx);
	texMap[namelc] = tx;

	LibraryChanged();
	return tx;
}

/*
==================
Textures::RemoveFromNameMap

do before deleting a texturegroup that is potentially used

the null indexes are okay because they'll turn into notextures in
the unknown group if they're used again
==================
*/
void Textures::RemoveFromNameMap(TextureGroup *tg)
{
	for (Texture *q = tg->first; q; q = q->next)
	{
		auto it = texMap.find(q->name);
		if (it != texMap.end() && it->second == q)
			it->second = nullptr;
	}

	LibraryChanged();
}

/*
==================
Textures::AddToNameMap
==================
*/
void Textures::AddToNameMap(TextureGroup *tg)
{
	for (Texture *q = tg->first; q; q = q->next)
		texMap[q->name] = q;

	LibraryChanged();
}

/*
==================
Textures::SelectFirstTexture
==================
*/
void Textures::SelectFirstTexture()
{
	if (groups.empty())
		return;
	if (!groups.front()->numTextures)
		return;

	g_vTexture.ChooseTexture(groups.front()->first);
}

/*
==================
WadFilePath
==================
*/
static std::string WadFilePath(const std::string &wadfile)
{
	if (fs::u8path(wadfile).is_absolute())
		return wadfile;
	return Textures::wadPath + wadfile;
}

/*
==================
Textures::ReloadGroup
==================
*/
void Textures::ReloadGroup(TextureGroup *tg)
{
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
	{
		if (*tgIt == tg)
		{
			ReloadGroup(tgIt);
			return;
		}
	}
}

void Textures::ReloadGroup(std::vector<TextureGroup*>::iterator tgIt)
{
	TextureGroup *oldwad, *newwad;
	Texture *temp;

	oldwad = *tgIt;
	newwad = ReadWad(WadFilePath(oldwad->name));
	if (!newwad)
		return;	// errors will already be in the log

	newwad->id = oldwad->id;

	// copy used status so we don't have to rebuild the whole map to restore it
	for (Texture *tex = newwad->first; tex; tex = tex->next)
	{
		Texture *otex = oldwad->ForName(tex->name);
		if (otex)	// already flushed if not
			tex->used = otex->used;
	}

	Textures::RemoveFromNameMap(oldwad);

	// swap new wad data into old TextureGroup so its position doesn't have to change
	// this lets the texture browser keep persistent state about texture groups
	temp = oldwad->first;
	oldwad->first = newwad->first;
	newwad->first = temp;
	oldwad->numTextures = newwad->numTextures;
	delete newwad;	// will now take oldwad's textures with it

	Textures::AddToNameMap(oldwad);

	LibraryChanged();
}

/*
==================
Textures::ReloadAll
==================
*/
void Textures::ReloadAll()
{
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		ReloadGroup(tgIt);

	LibraryChanged();
}

/*
==================
Textures::LoadWadsFromWadstring

the worldspawn "wad" key: names separated by ';'
==================
*/
void Textures::LoadWadsFromWadstring(const std::string &wadstring)
{
	size_t start = 0;

	while (start <= wadstring.size())
	{
		size_t end = wadstring.find(';', start);
		if (end == std::string::npos)
			end = wadstring.size();

		std::string w = wadstring.substr(start, end - start);
		w.erase(0, w.find_first_not_of(" \t"));
		w.erase(w.find_last_not_of(" \t") + 1);
		if (!w.empty())
			Textures::LoadWad(w);

		start = end + 1;
	}

	LibraryChanged();
}

/*
==================
Textures::LoadWadDir
==================
*/
void Textures::LoadWadDir(const std::string &dir)
{
	std::vector<std::string> wads;
	std::error_code ec;

	for (const fs::directory_entry &e : fs::directory_iterator(fs::u8path(dir), ec))
	{
		std::string ext = e.path().extension().u8string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });
		if (e.is_regular_file() && ext == ".wad")
			wads.push_back(e.path().filename().u8string());
	}
	if (ec)
	{
		SDL_Log("Couldn't list wads in %s: %s", dir.c_str(), ec.message().c_str());
		return;
	}

	std::sort(wads.begin(), wads.end());
	for (const std::string &w : wads)
		LoadWad(w);
}

/*
==================
Textures::LoadWad
==================
*/
void Textures::LoadWad(const std::string &wadfile)
{
	TextureGroup *oldwad, *newwad;
	std::string wadpath = WadFilePath(wadfile);

	SDL_Log("Loading all textures from %s", wadpath.c_str());

	// check if the wad is already loaded and replace it
	auto tgIt = groups.begin();
	for (; tgIt != groups.end(); tgIt++)
	{
		if ((*tgIt)->name == wadfile)
			break;
	}
	if (tgIt != groups.end())
	{
		oldwad = *tgIt;

		newwad = ReadWad(wadpath);
		if (!newwad)
			return;	// errors will already be in the log
		newwad->name = wadfile;
		Textures::RemoveFromNameMap(oldwad);
		*tgIt = newwad;
		delete oldwad;
	}
	else
	{
		newwad = ReadWad(wadpath);
		if (!newwad)
			return;

		newwad->name = wadfile;
		groups.push_back(newwad);
	}

	Textures::AddToNameMap(newwad);
	LibraryChanged();
}


//=====================================================


/*
==================
Textures::MenuReloadAll
==================
*/
void Textures::MenuReloadAll()
{
	ReloadAll();
	// TODO: g_map.BuildBrushData() once the map is ported
}

/*
==================
Textures::MenuReloadGroup
==================
*/
void Textures::MenuReloadGroup(TextureGroup *tg)
{
	ReloadGroup(tg);
	// TODO: g_map.BuildBrushData() once the map is ported
}

/*
==================
Textures::SetTextureMode
==================
*/
void Textures::SetTextureMode(int mode)
{
	g_nTextureMode = std::min(std::max(0, mode), 5);

	for (auto tmIt = texMap.begin(); tmIt != texMap.end(); ++tmIt)
	{
		if (!tmIt->second)
			continue;
		tmIt->second->glTex.Bind();
		tmIt->second->glTex.SetMode(g_nTextureMode);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}
