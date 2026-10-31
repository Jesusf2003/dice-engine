//==============================
//	textureFavorites.h
//
//	favourite texture folders, idea from hammertime's TextureBrowser: nested
//	folders holding texture names, saved in radiant.ini
//==============================

#ifndef __TEXTURE_FAVORITES_H__
#define __TEXTURE_FAVORITES_H__

#include <set>
#include <string>
#include <vector>

struct favFolder_t
{
	std::string					name;
	std::vector<std::string>	items;		// texture names, lowercase
	std::vector<favFolder_t>	children;
};

namespace TextureFavorites
{
	extern std::vector<favFolder_t> folders;

	void	RegisterSettings();		// before the first ImGui::NewFrame, so radiant.ini fills it

	// folders are addressed by path, "parent/child"
	favFolder_t	*Find(const std::string &path);
	std::string	AddFolder(const std::string &parentPath, const std::string &name);	// returns the new path
	bool	RenameFolder(const std::string &path, const std::string &name);			// keeps the folder in place
	bool	DeleteFolder(const std::string &path);

	void	AddItems(const std::string &path, const std::set<std::string> &names);
	void	RemoveItems(const std::string &path, const std::set<std::string> &names);	// from the folder and its subfolders

	// every texture in the folder and its subfolders (an empty path is all favourites)
	void	Collect(const std::string &path, std::set<std::string> &out);

	std::string	CleanName(const std::string &name);	// '/' and '|' are path/ini separators
};

#endif
