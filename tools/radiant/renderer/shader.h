//==============================
//	shader.h
//
//	a linked GLSL program and its uniforms
//==============================

#ifndef __SHADER_H__
#define __SHADER_H__

#include "mathlib.h"

#include <string>

class Shader
{
public:
	Shader() {}
	~Shader();
	Shader(const Shader &) = delete;
	Shader &operator=(const Shader &) = delete;

	// attribute 0 is bound to "position" before linking
	bool	Build(const char *name, const char *vertexSrc, const char *fragmentSrc);
	void	Destroy();
	inline bool Valid() const	{ return program != 0; }

	void	Use();
	void	Set(const char *uniform, int v);
	void	Set(const char *uniform, float v);
	void	Set(const char *uniform, const vec3 &v);
	void	Set(const char *uniform, float r, float g, float b, float a);
	void	Set(const char *uniform, const mat4 &m);

private:
	unsigned int	program = 0;
	std::string		name;
};

#endif
