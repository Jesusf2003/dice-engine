//==============================
//	framebuffer.cpp
//==============================

#include "renderer/framebuffer.h"
#include "renderer/glFuncs.h"

#include <SDL.h>

Framebuffer::~Framebuffer()
{
	Destroy();
}

/*
==================
Framebuffer::Resize
==================
*/
bool Framebuffer::Resize(int w, int h)
{
	if (w < 1) w = 1;
	if (h < 1) h = 1;
	if (fbo && w == width && h == height)
		return true;

	Destroy();
	width = w;
	height = h;

	glGenTextures(1, &color);
	glBindTexture(GL_TEXTURE_2D, color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	qglGenRenderbuffers(1, &depth);
	qglBindRenderbuffer(GL_RENDERBUFFER, depth);
	qglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
	qglBindRenderbuffer(GL_RENDERBUFFER, 0);

	qglGenFramebuffers(1, &fbo);
	qglBindFramebuffer(GL_FRAMEBUFFER, fbo);
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
	qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
	GLenum status = qglCheckFramebufferStatus(GL_FRAMEBUFFER);
	qglBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		SDL_Log("framebuffer %dx%d incomplete (0x%x)", w, h, status);
		Destroy();
		return false;
	}
	return true;
}

void Framebuffer::Destroy()
{
	if (fbo) qglDeleteFramebuffers(1, &fbo);
	if (depth) qglDeleteRenderbuffers(1, &depth);
	if (color) glDeleteTextures(1, &color);
	fbo = color = depth = 0;
	width = height = 0;
}

void Framebuffer::Bind()
{
	qglBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
}

void Framebuffer::Unbind()
{
	qglBindFramebuffer(GL_FRAMEBUFFER, 0);
}
