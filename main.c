#define GL_GLEXT_PROTOTYPES

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stdint.h>

#include <SDL2/SDL.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "sys.h"

#include <libspectre/spectre.h>
#include "postscript.h"

#include "shader.h"
const char* vshader = "#version 420\nvec2 y=vec2(1.,-1);\nvec4 x[4]={y.yyxx,y.xyxx,y.yxxx,y.xxxx};void main(){gl_Position=x[gl_VertexID];}";

#define CANVAS_WIDTH 1920
#define CANVAS_HEIGHT 1080
#define CHAR_BUFF_SIZE 64

// #define DEBUG_VERTEX
#define DEBUG_FRAGMENT
// #define DEBUG_PROGRAM

static GLuint vao;
static GLuint p;
static GLuint renderedTex;

static Uint32 startTime=0;

static void render_postscript(const unsigned char* postscript, unsigned int length, unsigned char** data, int* row_length) {
	int fd = SYS_memfd_create("", 0);
	SYS_write(fd, postscript, length);

	char memfd_path[CHAR_BUFF_SIZE];
	sprintf(memfd_path, "/proc/self/fd/%d", fd);

	SpectreDocument* doc = spectre_document_new();
	spectre_document_load(doc, memfd_path);
	spectre_document_render(doc, data, row_length);
}

static void on_render()
{
	if (startTime == 0) startTime = SDL_GetTicks();
	float itime = (SDL_GetTicks()-startTime)/1000.0;

	glUseProgram(p);
	glBindVertexArray(vao);
	glVertexAttrib1f(0, 0);
	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, renderedTex);
	glUniform1f(0, itime);

	// if (itime > 16) SYS_exit_group(0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void on_realize()
{
	// compile shader
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(f, 1, &shader_frag_min, NULL);
	glCompileShader(f);

#ifdef DEBUG_VERTEX
	{
		GLint isCompiled = 0;
		glGetShaderiv(f, GL_COMPILE_STATUS, &isCompiled);
		if(isCompiled == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetShaderInfoLog(f, maxLength, &maxLength, error);
			SYS_write(0, error, maxLength);

			SYS_exit_group(-1);
		}
	}
#endif

	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vshader, NULL);
	glCompileShader(v);

#ifdef DEBUG_FRAGMENT
	{
		GLint isCompiled = 0;
		glGetShaderiv(f, GL_COMPILE_STATUS, &isCompiled);
		if(isCompiled == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetShaderInfoLog(f, maxLength, &maxLength, error);
			SYS_write(0, error, maxLength);

			SYS_exit_group(-1);
		}
	}
#endif

	// link shader
	p = glCreateProgram();
	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);

#ifdef DEBUG_PROGRAM
	{
		GLint isLinked = 0;
		glGetProgramiv(p, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv(p, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetProgramInfoLog(p, maxLength, &maxLength,error);
			SYS_write(0, error, maxLength);

			SYS_exit_group(-1);
		}
	}
#endif

	glGenVertexArrays(1, &vao);

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &renderedTex);
	glBindTexture(GL_TEXTURE_2D, renderedTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	unsigned char* rendered_data;
	int row_length;
	render_postscript(postscript_ps_min, postscript_ps_min_len, &rendered_data, &row_length);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, row_length/4, row_length/4, 0, GL_BGRA, GL_UNSIGNED_BYTE, rendered_data);
}

void _start() {
	asm volatile("sub $8, %rsp\n");

	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
	SDL_Window* mainwindow = SDL_CreateWindow(
		"", 
		0,
		0,
		CANVAS_WIDTH,
		CANVAS_HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
	);

	SDL_GLContext glcontext = SDL_GL_CreateContext(mainwindow);

	on_realize();

	while (true) {
		SDL_Event Event;
		while (SDL_PollEvent(&Event)) {
			if (Event.type == SDL_QUIT || (Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_ESCAPE)) {
				SYS_exit_group(0);
			}
		}
		on_render();
		SDL_GL_SwapWindow(mainwindow);
	}
	__builtin_unreachable();
}