//==============================
//	framebuffer.h
//
//	an offscreen color + depth target. the views render into one and hand the
//	color texture to ImGui::Image
//==============================

#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

class Framebuffer
{
public:
	Framebuffer() {}
	~Framebuffer();
	Framebuffer(const Framebuffer &) = delete;
	Framebuffer &operator=(const Framebuffer &) = delete;

	bool	Resize(int w, int h);	// (re)creates when the size changes
	void	Destroy();

	void	Bind();					// also sets the viewport
	static void Unbind();			// back to the window

	inline unsigned int ColorTexture() const	{ return color; }
	inline int Width() const					{ return width; }
	inline int Height() const					{ return height; }

private:
	unsigned int	fbo = 0, color = 0, depth = 0;
	int				width = 0, height = 0;
};

#endif
