#ifndef ku_gl_shader_h
#define ku_gl_shader_h

#include <GLES2/gl2.h>

typedef struct _glsha_t
{
    GLuint name;
    GLint  uni_loc[13];
} glsha_t;

glsha_t ku_gl_shader_create(
    const char*  vertex_source,
    const char*  fragment_source,
    int          attribute_locations_length,
    const char** attribute_structure,
    int          uniform_locations_length,
    const char** uniform_structure);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <stdio.h>

GLuint ku_gl_shader_compile(GLenum type, const GLchar* source)
{
    GLint  status, logLength, realLength;
    GLuint shader = 0;

    status = 0;
    shader = glCreateShader(type);

    if (shader > 0)
    {
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

	if (logLength > 0)
	{
	    GLchar log[logLength];

	    glGetShaderInfoLog(shader, logLength, &realLength, log);

	    printf("Shader compile log: %s\n", log);
	}

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE)
	    return 0;
    }
    else
	printf("Cannot create shader\n");

    return shader;
}

int ku_gl_shader_link(GLuint program)
{
    GLint status, logLength, realLength;

    glLinkProgram(program);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength > 0)
    {
	GLchar log[logLength];
	glGetProgramInfoLog(program, logLength, &realLength, log);
	printf("Program link log : %i %s\n", realLength, log);
    }

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE)
	return 1;
    return 0;
}

glsha_t ku_gl_shader_create(
    const char*  vertex_source,
    const char*  fragment_source,
    int          attribute_locations_length,
    const char** attribute_structure,
    int          uniform_locations_length,
    const char** uniform_structure)
{
    glsha_t sh = {0};

    sh.name = glCreateProgram();

    GLuint vertex_shader = ku_gl_shader_compile(GL_VERTEX_SHADER, vertex_source);
    if (vertex_shader == 0) printf("Failed to compile vertex shader : %s\n", vertex_source);

    GLuint fragment_shader = ku_gl_shader_compile(GL_FRAGMENT_SHADER, fragment_source);
    if (fragment_shader == 0) printf("Failed to compile fragment shader : %s\n", fragment_source);

    glAttachShader(sh.name, vertex_shader);
    glAttachShader(sh.name, fragment_shader);

    for (int index = 0; index < attribute_locations_length; index++)
    {
	const GLchar* name = attribute_structure[index];
	glBindAttribLocation(sh.name, index, name);
    }

    int success = ku_gl_shader_link(sh.name);

    if (success == 1)
    {
	for (int index = 0; index < uniform_locations_length; index++)
	{
	    const GLchar* name     = uniform_structure[index];
	    GLint         location = glGetUniformLocation(sh.name, name);
	    sh.uni_loc[index]      = location;
	}
    }
    else
	printf("Failed to link shader program\n");

    if (vertex_shader > 0)
    {
	glDetachShader(sh.name, vertex_shader);
	glDeleteShader(vertex_shader);
    }

    if (fragment_shader > 0)
    {
	glDetachShader(sh.name, fragment_shader);
	glDeleteShader(fragment_shader);
    }

    return sh;
}

#endif
