//==============================
//	texture.cpp
//==============================

#include "renderer/texture.h"
#include "ui/textures/textureView.h"
#include "renderer/glFuncs.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>

/*
========================================================================

	GLTEXTURE

========================================================================
*/

static const int g_textureModes[] =
{
	GL_NEAREST,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR
};

int g_nTextureMode = 5;	// GL_LINEAR_MIPMAP_LINEAR

void GLTexture::Make(int w, int h, const unsigned char *rgba)
{
	glGenTextures(1, &n);
	Bind();
	SetMode(g_nTextureMode);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	if (qglGenerateMipmap)
		qglGenerateMipmap(GL_TEXTURE_2D);
	Unbind();
}

void GLTexture::SetMode(int mode)
{
	mode = std::min(std::max(0, mode), 5);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_textureModes[mode]);

	switch (g_textureModes[mode])
	{
	case GL_NEAREST:
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	default:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}
}

GLTexture::~GLTexture()
{
	if (n)
		glDeleteTextures(1, &n);
}

void GLTexture::Bind()
{
	glBindTexture(GL_TEXTURE_2D, n);
}

void GLTexture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}


/*
========================================================================

	TEXTURE

========================================================================
*/

/*
==================
Texture::Texture
==================
*/
Texture::Texture(int w, int h, const std::string &n, const float c[3], const unsigned char *rgba) :
	next(nullptr), name(n), width(w), height(h), used(false)
{
	color[0] = c[0];
	color[1] = c[1];
	color[2] = c[2];

	glTex.Make(w, h, rgba);
	if (name.size() > MAX_TEXNAME)
		name.resize(MAX_TEXNAME);
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });
	SetFlags();
}

void Texture::Use()
{
	// TODO: refresh only if hide unused is on, once that setting exists
	if (!used)
		g_vTexture.Refresh();

	used = true;
}

/*
==================
Texture::SetFlags
==================
*/
void Texture::SetFlags()
{
	auto startsWith = [this](const char *s) { return name.compare(0, strlen(s), s) == 0; };

	showflags = 0;

	if (startsWith("*"))
		showflags |= BFL_LIQUID | BFL_TRANS;
	else if (startsWith("sky"))
		showflags |= BFL_SKY;
	else if (startsWith("clip"))
		showflags |= BFL_CLIP | BFL_TRANS;
	else if (startsWith("hint"))
		showflags |= BFL_HINT | BFL_TRANS;
	else if (startsWith("skip"))
		showflags |= BFL_SKIP;
	else if (startsWith("trigger"))
		showflags |= BFL_TRANS;
}


/*
========================================================================

	TEXTUREGROUP

========================================================================
*/

/*
==================
TextureGroup::TextureGroup
==================
*/
TextureGroup::TextureGroup() : first(nullptr), numTextures(0), id(rand()), name("?")
{
}

/*
==================
TextureGroup::~TextureGroup
==================
*/
TextureGroup::~TextureGroup()
{
	Flush();
}

/*
==================
TextureGroup::Flush
==================
*/
void TextureGroup::Flush()
{
	Texture *tex, *nexttex;
	numTextures = 0;
	for (tex = first; tex; tex = nexttex)
	{
		nexttex = tex->next;
		delete tex;
	}
	first = nullptr;
}

/*
==================
TextureGroup::ByColor

a quick way to find entity solid color textures
==================
*/
Texture *TextureGroup::ByColor(const float c[3])
{
	for (Texture *tex = first; tex; tex = tex->next)
	{
		if (tex->color[0] == c[0] && tex->color[1] == c[1] && tex->color[2] == c[2])
			return tex;
	}
	return nullptr;
}

/*
==================
TextureGroup::ForName
==================
*/
Texture *TextureGroup::ForName(const std::string &texname)
{
	for (Texture *tex = first; tex; tex = tex->next)
	{
		if (texname == tex->name)
			return tex;
	}
	return nullptr;
}

/*
==================
TextureGroup::ClearUsed
==================
*/
void TextureGroup::ClearUsed()
{
	for (Texture *tex = first; tex; tex = tex->next)
		tex->used = false;
}

/*
==================
TextureGroup::Add
==================
*/
void TextureGroup::Add(Texture *tx)
{
	numTextures++;

	// add sorted by name
	if (!first)
	{
		first = tx;
		return;
	}
	if (tx->name < first->name)
	{
		tx->next = first;
		first = tx;
		return;
	}

	Texture *qtex = first;
	while (qtex->next)
	{
		if (tx->name < qtex->next->name)
		{
			tx->next = qtex->next;
			qtex->next = tx;
			return;
		}
		qtex = qtex->next;
	}
	qtex->next = tx;
}
