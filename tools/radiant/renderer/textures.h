//==============================
//	textures.h
//
//	the texture library: every loaded wad as a TextureGroup, plus a name map
//	where later wads stomp earlier ones (same as qbsp). port of quaked3's
//	textures.cpp + WadReader + PaletteReader
//==============================

#ifndef __TEXTURES_H__
#define __TEXTURES_H__

#include <map>
#include <string>
#include <vector>

class TextureGroup;
class Texture;

namespace Textures
{
	extern std::vector<TextureGroup*> groups;
	extern std::map<std::string, Texture*> texMap;

	extern TextureGroup group_unknown;	// bucket for bad textures
	extern TextureGroup group_solid;	// bucket for silly solid color entity textures

	extern std::string wadPath;			// relative wad names are looked up here

	void Init(const std::string &wadDir);
	void Shutdown();					// before the GL context goes away
	void LibraryChanged();
	void HandleChange();				// once per frame
	void ReloadAll();
	void RefreshUsedStatus();
	void FlushAll();

	Texture *CreateSolid(const std::string &name);
	Texture *CreateBroken(const std::string &name);
	Texture *ForName(const std::string &name);

	void LoadWadsFromWadstring(const std::string &wadstring);
	void LoadWadDir(const std::string &dir);		// every *.wad in dir, sorted by name
	void LoadWad(const std::string &wadfile);
	void LoadPalette();
	void RemoveFromNameMap(TextureGroup *tg);
	void AddToNameMap(TextureGroup *tg);
	void SelectFirstTexture();
	void ReloadGroup(TextureGroup *tg);
	void ReloadGroup(std::vector<TextureGroup*>::iterator tgIt);
	void CleanUnknowns();
	void FixWorkTexDef();

	void MenuReloadAll();
	void MenuReloadGroup(TextureGroup *tg);

	void SetTextureMode(int mode);
};

#endif
