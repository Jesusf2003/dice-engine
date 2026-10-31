//==============================
//	texture.h
//
//	port of quaked3's Texture, GLTexture and TextureGroup
//==============================

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <string>

#define MAX_TEXNAME	16		// WAD2/WAD3 lump name limit

// showflags, how a texture name is treated by the view filters
#define BFL_LIQUID	0x0001
#define BFL_SKY		0x0002
#define BFL_CLIP	0x0004
#define BFL_HINT	0x0008
#define BFL_SKIP	0x0010
#define BFL_TRANS	0x0020

/*
==================
GLTexture
==================
*/
class GLTexture
{
public:
	GLTexture() {}
	~GLTexture();
	GLTexture(const GLTexture &) = delete;
	GLTexture &operator=(const GLTexture &) = delete;

	void Make(int w, int h, const unsigned char *rgba);
	void SetMode(int mode);

	void Bind();
	void Unbind();
	inline unsigned int TexNum() const { return n; }

private:
	unsigned int n = 0;
};

/*
==================
Texture
==================
*/
class Texture
{
public:
	Texture(int w, int h, const std::string &n, const float c[3], const unsigned char *rgba);
	~Texture() {};

	Texture		*next;
	std::string	name;		// must remain limited to the WAD2 spec (16)
	int			width, height;
	GLTexture	glTex;		// gl bind
	float		color[3];	// average color 0-255, for flat shade mode
	bool		used;		// true = is present on the level
	unsigned	showflags;

	void	Use();
private:
	void	SetFlags();
};

/*
==================
TextureGroup

one wad's worth of textures, a linked list sorted by name
==================
*/
class TextureGroup
{
public:
	TextureGroup();
	~TextureGroup();

	bool operator< (const TextureGroup &other) const { return name < other.name; }
	static bool sortcmp(const TextureGroup *a, const TextureGroup *b) { return *a < *b; }

	Texture		*first;
	int			numTextures;
	int			id;
	std::string	name;

	Texture *ForName(const std::string &name);
	Texture *ByColor(const float c[3]);
	void ClearUsed();
	void Flush();
	void Add(Texture *tx);
};

#endif
