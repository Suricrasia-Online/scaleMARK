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

#include <opus/opus.h>

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

static SDL_Window* mainwindow;

static void quit() {
	SDL_DestroyWindow(mainwindow);
	SYS_exit_group(0);
	__builtin_unreachable();
}

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
	static Uint32 startTime=0;
	if (startTime == 0) {
		startTime = SDL_GetTicks();
		SDL_PauseAudio(0);
	}
	float itime = (SDL_GetTicks()-startTime)/1000.0;

	glUseProgram(p);
	glBindVertexArray(vao);
	glVertexAttrib1f(0, 0);
	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, renderedTex);
	glUniform1f(0, itime);

	// if (itime > 16) quit();

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

inline int abs(int x) {
	return x<0?-x:x;
}

int triangle(int x, int f, int a, int p) {
	return (a*(f/p-abs((x%(f*2))-f)))/f;
}

// int soft_triangle(int x, int f, int a, int p) {
// 	return triangle(x-f/3,f,a/8,p)+triangle(x-f/5,f,a/4,p)+triangle(x,f,a/4,p)+triangle(x+f/5,f,a/4,p)+triangle(x+f/3,f,a/8,p);
// }

#define PACKET_SIZE 125
static unsigned char PACKET[] = {
	[0] = 0x7b, [1] = 0x03, [2 ... PACKET_SIZE] = 0x00
};
// static unsigned char SILENCE_PACKET[] = { 0x58 };
#define DECODED_DATA_SIZE 2880
static int16_t DECODED_DATA[DECODED_DATA_SIZE];

static void decode_random_packet(uint32_t seed, OpusDecoder* opus_decoder) {
	uint32_t state = seed;
	for (int i = 2; i < PACKET_SIZE; i++) {
		state++;
		state = state ^ (state << 13u);
		state = state ^ (state >> 17u);
		state = state ^ (state << 5u);
		// state *= 1685821657u;
		PACKET[i] = state;
	}
	int length = opus_decode(opus_decoder, PACKET, PACKET_SIZE, DECODED_DATA, DECODED_DATA_SIZE, seed == 0);
	(void)length;
}

// SILENCE
#define ___ 0x00

// BEATS
#define BT1 0xc4
#define BT2 0x4f
#define BT3 0x95

// BEEPS
#define BP1 0xfc
#define BP2 0x50
#define BP3 0xe8
#define BP4 0x18
#define BP5 0x30

// GRUNTS
#define GR1 0x87
#define GR2 0xe6
#define GR3 0xea
#define GR4 0x10
#define GR5 0x2a
#define GR6 0x68
#define GR7 0x70
#define GR8 0x99
#define GR9 0xa4

// TICKS
#define TI1 0xdf
#define TI2 0x02
#define TI3 0x29
#define TI4 0x3d
#define TI5 0x94
#define TI6 0xa0
#define TI7 0xb6
#define TI8 0xd0
#define TI9 0xd6
#define TIA 0xdb
#define TIB 0xe1

// HITS
#define HT1 0x5b
#define HT2 0xad
#define HT3 0xd2

// OTHER
#define ML1 0xb1
#define ML2 0xc8

static const unsigned char MUSIC_ROLL[] = {
TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  BP2,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  BP2,BP2,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  BP2,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  BP2,BP2,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  GR7,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  GR7,BP2,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,


TI6,TIB,  ___,___,  ___,___,  ___,___,  GR7,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  GR7,BP2,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,


TI6,TIB,  ___,___,  ___,___,  ___,___,  GR7,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  GR7,BP2,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  TI9,___,  ___,___,  GR7,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  TI9,___,  ___,___,  GR7,___,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  BP2,BT1,  ___,___,  BT1,___,  ___,___,

TI6,TIB,  ___,___,  ___,___,  ___,___,  GR7,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  ___,___,  ___,___,  GR7,BP2,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,

TI6,TIB,  ___,___,  TI9,___,  ___,___,  GR7,___,  ___,___,  ___,___,  ___,___,
BP1,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,  ___,___,
TI6,___,  ___,___,  TI9,___,  ___,___,  GR7,___,  ___,___,  ___,___,  ___,___,
BP1,BP4,  ___,___,  ___,___,  ___,___,  BP2,BT1,  ___,___,  BT1,___,  ___,___,

};

#define SONG_LENGTH 120
#define SAMPLE_RATE 44100
#define MAX_SAMPLES SAMPLE_RATE*SONG_LENGTH
int16_t song_samples[MAX_SAMPLES];
static void generate_song() {

	OpusDecoder* opus_decoder_1 = opus_decoder_create(48000, 1, NULL);
	OpusDecoder* opus_decoder_2 = opus_decoder_create(48000, 1, NULL);
	for (int i = 0; i < sizeof(MUSIC_ROLL); i++) {
		decode_random_packet(MUSIC_ROLL[i], i%2==0 ? opus_decoder_1 : opus_decoder_2);
		// for (int j = 0; j < DECODED_DATA_SIZE; j++) {
		// 	int abs_d = abs(DECODED_DATA[j]);
		// 	max = max<abs_d?abs_d:max;
		// }
		// int mult = (6*4000)/max;
		for (int j = 0; j < DECODED_DATA_SIZE && i/2*DECODED_DATA_SIZE+j < MAX_SAMPLES; j++) {
			song_samples[i/2*DECODED_DATA_SIZE+j] += (int)DECODED_DATA[j]*4;
		}
	}
	for(int i = DECODED_DATA_SIZE*128; i < MAX_SAMPLES; i++) {
		int phased_i = i + triangle(i, 200, 300, 2);
		song_samples[i] += (int16_t)triangle(phased_i, 600, triangle(phased_i, DECODED_DATA_SIZE*32, 22000, 1), 2);
	}
}

static void audio_callback(void* userdata, Uint8* stream, int len) {
	(void)userdata;
	static int audiotime = 0;
	for(int i = 0; i < len; i++) {
		audiotime++;
		if(audiotime > MAX_SAMPLES*2) quit();
		stream[i] = ((Uint8*)song_samples)[audiotime^1];
	}
}

void _start() {
	asm volatile("sub $8, %rsp\n");

	generate_song();

	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	mainwindow = SDL_CreateWindow(
		"",
		0,
		0,
		CANVAS_WIDTH/8,
		CANVAS_HEIGHT/8,
		SDL_WINDOW_OPENGL
	);

	SDL_GL_CreateContext(mainwindow);

	static SDL_AudioSpec desired = {
		.freq = SAMPLE_RATE,
		.format = AUDIO_S16LSB,
		.channels = 1,
		.silence = 0,
		.samples = 4096,
		.size = 0,
		.callback = audio_callback,
		.userdata = 0
	};
	SDL_OpenAudio(&desired, NULL);
	SDL_ShowCursor(false);

	on_realize();

	while (true) {
		SDL_Event Event;
		while (SDL_PollEvent(&Event)) {
			if (Event.type == SDL_QUIT/* || (Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_ESCAPE)*/) {
				quit();
			}
		}
		on_render();
		SDL_GL_SwapWindow(mainwindow);
	}
	__builtin_unreachable();
}