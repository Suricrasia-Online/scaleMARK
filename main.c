#define GL_GLEXT_PROTOTYPES

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stdint.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <linux/memfd.h>
#include <linux/fcntl.h>

#include <libspectre/spectre.h>
#include "postscript.h"

#include "shader.h"
const char* vshader = "#version 450\nvec2 y=vec2(1.,-1);\nvec4 x[4]={y.yyxx,y.xyxx,y.yxxx,y.xxxx};void main(){gl_Position=x[gl_VertexID];}";

#define CANVAS_WIDTH 1920
#define CANVAS_HEIGHT 1080
#define CHAR_BUFF_SIZE 64

#define DEBUG
#define KEY_HANDLING

inline void quit_asm() {
	asm volatile(".intel_syntax noprefix");
	asm volatile("xor edi, edi");
	asm volatile("push 231"); //exit_group
	asm volatile("pop rax");
	asm volatile("syscall");
	asm volatile(".att_syntax prefix");
	__builtin_unreachable();
}

GLuint vao;
GLuint p;
GLuint renderedTex;

GTimer* gtimer;

#ifdef KEY_HANDLING
static gboolean check_escape(GtkWidget *widget, GdkEventKey *event)
{
	(void)widget;
	if (event->keyval == GDK_KEY_Escape) {
		quit_asm();
	}

	return FALSE;
}
#endif

inline static int SYS_memfd_create(const char* name, unsigned int flags) {
	register int ret;
	asm volatile("mov $356, %%eax\nint $0x80":"=a"(ret):"b"(name),"c"(flags));
	return ret;
}

inline static ssize_t SYS_write(int fd, const void *buf, size_t c) {
  register ssize_t ret;
  asm volatile("mov $4, %%eax\nint $0x80":"=a"(ret):"b"(fd),"c"(buf),"d"(c));
  return ret;
}

void render_postscript(unsigned char* postscript, unsigned int length, unsigned char** data, int* row_length) {
	int fd = SYS_memfd_create("", 0);
	SYS_write(fd, postscript, length);

	char memfd_path[CHAR_BUFF_SIZE];
	if (snprintf(memfd_path, CHAR_BUFF_SIZE, "/proc/self/fd/%d", fd) >= CHAR_BUFF_SIZE) {
		quit_asm();
	}

	SpectreDocument* doc = spectre_document_new();
	spectre_document_load(doc, memfd_path);
	spectre_document_render(doc, data, row_length);
}

static gboolean
on_render (GtkGLArea *glarea, GdkGLContext *context)
{
	(void)context;
	(void)glarea;
	float itime = g_timer_elapsed(gtimer, NULL);

	glUseProgram(p);
	glBindVertexArray(vao);
	glVertexAttrib1f(0, 0);
	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, renderedTex);
	glUniform1f(0, itime);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	return TRUE;
}

static void on_realize(GtkGLArea *glarea)
{
	gtk_gl_area_make_current(glarea);

	// compile shader
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(f, 1, &shader_frag_min, NULL);
	glCompileShader(f);

#ifdef DEBUG
	GLint isCompiled = 0;
	glGetShaderiv(f, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetShaderInfoLog(f, maxLength, &maxLength, error);
		printf("%s\n", error);

		quit_asm();
	}
#endif

	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vshader, NULL);
	glCompileShader(v);

#ifdef DEBUG
	GLint isCompiled2 = 0;
	glGetShaderiv(v, GL_COMPILE_STATUS, &isCompiled2);
	if(isCompiled2 == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(v, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetShaderInfoLog(v, maxLength, &maxLength, error);
		printf("%s\n", error);

		quit_asm();
	}
#endif

	// link shader
	p = glCreateProgram();
	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);

#ifdef DEBUG
	GLint isLinked = 0;
	glGetProgramiv(p, GL_LINK_STATUS, (int *)&isLinked);
	if (isLinked == GL_FALSE) {
		GLint maxLength = 0;
		glGetProgramiv(p, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetProgramInfoLog(p, maxLength, &maxLength,error);
		printf("%s\n", error);

		quit_asm();
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
	render_postscript(postscript_ps, postscript_ps_len, &rendered_data, &row_length);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, row_length/4, row_length/4, 0, GL_BGRA, GL_UNSIGNED_BYTE, rendered_data);

	GdkGLContext *context = gtk_gl_area_get_context(glarea);
	GdkWindow *glwindow = gdk_gl_context_get_window(context);
	GdkFrameClock *frame_clock = gdk_window_get_frame_clock(glwindow);

	// Connect update signal:
	g_signal_connect_swapped(frame_clock, "update", G_CALLBACK(gtk_gl_area_queue_render), glarea);

	// Start updating:
	gdk_frame_clock_begin_updating(frame_clock);
}

void _start() {
	asm volatile("sub $8, %rsp\n");
	gtimer = g_timer_new();

	typedef void (*voidWithOneParam)(int*);
	voidWithOneParam gtk_init_one_param = (voidWithOneParam)gtk_init;
	(*gtk_init_one_param)(NULL);

	GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *glarea = gtk_gl_area_new();
	gtk_container_add(GTK_CONTAINER(win), glarea);

	g_signal_connect(win, "destroy", gtk_main_quit, NULL);
#ifdef KEY_HANDLING
	g_signal_connect(win, "key_press_event", G_CALLBACK(check_escape), NULL);
#endif
	g_signal_connect(glarea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glarea, "render", G_CALLBACK(on_render), NULL);

	gtk_widget_show_all (win);

	gtk_window_fullscreen((GtkWindow*)win);
	GdkWindow* window = gtk_widget_get_window(win);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	GdkCursor* Cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
#pragma GCC diagnostic pop
	gdk_window_set_cursor(window, Cursor);

	gtk_main();

	quit_asm();
}