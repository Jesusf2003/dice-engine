//==============================
//	shader.cpp
//==============================

#include "renderer/shader.h"
#include "renderer/glFuncs.h"

#include <SDL.h>

#include <vector>

/*
==================
CompileStage
==================
*/
static GLuint CompileStage(GLenum stage, const char *src, const char *name)
{
	GLuint s = qglCreateShader(stage);
	qglShaderSource(s, 1, &src, nullptr);
	qglCompileShader(s);

	GLint ok = 0;
	qglGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		GLint len = 0;
		qglGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
		std::vector<char> log(len > 1 ? len : 1);
		qglGetShaderInfoLog(s, (GLsizei)log.size(), nullptr, log.data());
		SDL_Log("shader %s (%s): %s", name, stage == GL_VERTEX_SHADER ? "vertex" : "fragment", log.data());
		qglDeleteShader(s);
		return 0;
	}
	return s;
}

Shader::~Shader()
{
	Destroy();
}

/*
==================
Shader::Build
==================
*/
bool Shader::Build(const char *shaderName, const char *vertexSrc, const char *fragmentSrc)
{
	Destroy();
	name = shaderName;

	GLuint vs = CompileStage(GL_VERTEX_SHADER, vertexSrc, shaderName);
	GLuint fs = CompileStage(GL_FRAGMENT_SHADER, fragmentSrc, shaderName);
	if (!vs || !fs)
	{
		if (vs) qglDeleteShader(vs);
		if (fs) qglDeleteShader(fs);
		return false;
	}

	program = qglCreateProgram();
	qglAttachShader(program, vs);
	qglAttachShader(program, fs);
	qglBindAttribLocation(program, 0, "position");
	qglLinkProgram(program);
	qglDeleteShader(vs);
	qglDeleteShader(fs);

	GLint ok = 0;
	qglGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok)
	{
		GLint len = 0;
		qglGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		std::vector<char> log(len > 1 ? len : 1);
		qglGetProgramInfoLog(program, (GLsizei)log.size(), nullptr, log.data());
		SDL_Log("shader %s (link): %s", shaderName, log.data());
		Destroy();
		return false;
	}
	return true;
}

void Shader::Destroy()
{
	if (program)
		qglDeleteProgram(program);
	program = 0;
}

void Shader::Use()
{
	qglUseProgram(program);
}

void Shader::Set(const char *uniform, int v)
{
	qglUniform1i(qglGetUniformLocation(program, uniform), v);
}

void Shader::Set(const char *uniform, float v)
{
	qglUniform1f(qglGetUniformLocation(program, uniform), v);
}

void Shader::Set(const char *uniform, const vec3 &v)
{
	qglUniform3f(qglGetUniformLocation(program, uniform), v.x, v.y, v.z);
}

void Shader::Set(const char *uniform, float r, float g, float b, float a)
{
	qglUniform4f(qglGetUniformLocation(program, uniform), r, g, b, a);
}

void Shader::Set(const char *uniform, const mat4 &m)
{
	qglUniformMatrix4fv(qglGetUniformLocation(program, uniform), 1, GL_FALSE, m.m);
}
