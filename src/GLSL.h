//
//    Many useful helper functions for GLSL shaders - gleaned from various sources including orange book
//    Created by zwood on 2/21/10.
//    Modified by sueda 10/15/15.
//

#pragma once
#ifndef __GLSL__
#define __GLSL__

#define GLEW_STATIC
#include <GL/glew.h>

namespace GLSL {

	int printError();
	void printProgramInfoLog(GLuint program);
	void printShaderInfoLog(GLuint shader);
	void checkVersion();
	int textFileWrite(const char *filename, char *s);
	char *textFileRead(const char *filename);
	GLint getAttribLocation(const GLuint program, const char varname[], bool verbose = true);
	GLint getUniformLocation(const GLuint program, const char varname[], bool verbose = true);
	void enableVertexAttribArray(const GLint handle);
	void disableVertexAttribArray(const GLint handle);
	void vertexAttribPointer(const GLint handle, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
}

#endif
