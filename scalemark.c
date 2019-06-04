#define GL_GLEXT_PROTOTYPES

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stdint.h>
#include<limits.h>

#include <SDL2/SDL.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "sys.h"

#include <libspectre/spectre.h>
#include "postscript.h"

#include <opus/opus.h>

#if defined(FULLSIZE)
#include "shader_full.h"
#define SHADER_SOURCE shader_full_frag_min
#else
#include "shader_small.h"
#define SHADER_SOURCE shader_small_frag_min
#endif

#define START_OFFSET_SECONDS 0

// #define CANVAS_WIDTH 1920
// #define CANVAS_HEIGHT 1080
#define CHAR_BUFF_SIZE 64

#define DEBUG_FRAGMENT
// #define DEBUG_PROGRAM
#define KEY_HANDLING

static SDL_Window* mainwindow;

static void quit() {
	SDL_DestroyWindow(mainwindow);
	SYS_exit_group(0);
	__builtin_unreachable();
}

static void render_postscript(const unsigned char* postscript, unsigned int length, unsigned char** data) {
	int fd = SYS_memfd_create("", 0);
	SYS_write(fd, postscript, length);

	static char memfd_path[CHAR_BUFF_SIZE];
	sprintf(memfd_path, "/proc/self/fd/%d", fd);

	SpectreDocument* doc = spectre_document_new();
	spectre_document_load(doc, memfd_path);
	int row_length;
	spectre_document_render(doc, data, &row_length);
}

static void on_render()
{
	static int startTime=0;
	if (startTime == 0) {
		startTime = SDL_GetTicks();
		SDL_PauseAudio(0);
	}
	float itime = ((int)SDL_GetTicks()-startTime-520 + START_OFFSET_SECONDS*1000)/1000.0;

	glUniform1f(0, itime);

	glRecti(-1,-1,1,1);
}

static void on_realize()
{
	// compile shader
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(f, 1, &SHADER_SOURCE, NULL);
	glCompileShader(f);

#ifdef DEBUG_FRAGMENT
	{
		GLint isCompiled = 0;
		glGetShaderiv(f, GL_COMPILE_STATUS, &isCompiled);
		if(isCompiled == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

			char error[maxLength];
			glGetShaderInfoLog(f, maxLength, &maxLength, error);
			SYS_write(0, error, maxLength);

			quit();
		}
	}
#endif

	GLuint p = glCreateProgram();
	glAttachShader(p,f);
	glLinkProgram(p);

#ifdef DEBUG_PROGRAM
	{
		GLint isLinked = 0;
		glGetProgramiv(p, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv(p, GL_INFO_LOG_LENGTH, &maxLength);

			char error[maxLength];
			glGetProgramInfoLog(p, maxLength, &maxLength,error);
			SYS_write(0, error, maxLength);

			quit();
		}
	}
#endif

	glUseProgram(p);

	GLuint renderedTex;
	glGenTextures(1, &renderedTex);
	glBindTexture(GL_TEXTURE_2D, renderedTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	static unsigned char* rendered_data;
	render_postscript(postscript_ps_min, postscript_ps_min_len, &rendered_data);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4096, 4096, 0, GL_BGRA, GL_UNSIGNED_BYTE, rendered_data);
}

inline int abs(int x) {
	return x<0?-x:x;
}

inline int triangle(int x, int f, int a, int p) {
	return (a*(f/p-abs((x%(f*2))-f)))/f;
}

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

#define ll ___,___,

static const unsigned char MUSIC_ROLL[] = {
	//small silence thing at start
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// OPENING
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //

	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	// WRINKLE 1
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	// WRINKLE 2
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,___, ll ___,___, ll BP2,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,BP4, ll ___,___, ll BP2,BP2, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,___, ll ___,___, ll BP2,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,BP4, ll ___,___, ll BP2,BP2, ll ___,___, ll
	// ----------------------------------------- //
	// WRINKLE 1
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	// WRINKLE 3
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll GR7,___, ll ___,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll GR7,BP2, ll ___,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll GR7,___, ll ___,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll GR7,BP2, ll ___,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	// WRINKLE 1 (with beat)
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,BP4, ll ___,___, ll ___,BT1, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	BP1,BP4, ll ___,___, ll ___,BT1, ll BT1,___, ll
	// ----------------------------------------- //
	// WRINKLE 3 (remixed)
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll GR7,___, ll ___,___, ll
	BP1,___, ll ___,___, ll BP2,___, ll ___,___, ll
	TI6,___, ll ___,___, ll GR7,BP2, ll ___,___, ll
	BP1,BP4, ll ___,___, ll BP1,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll GR7,___, ll ___,___, ll
	BP1,___, ll ___,___, ll BP2,___, ll ___,___, ll
	TI6,___, ll ___,___, ll GR7,BP2, ll ___,___, ll
	BP1,BP4, ll ___,___, ll BP1,BT1, ll BT1,___, ll
	// ----------------------------------------- //

	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll GR7,___, ll ___,___, ll
	BP1,___, ll ___,___, ll BP2,___, ll ___,___, ll
	TI6,___, ll ___,___, ll GR7,BP2, ll ___,___, ll
	BP1,BP4, ll ___,___, ll BP1,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll GR7,___, ll ___,___, ll
	BP1,___, ll ___,___, ll BP2,___, ll ___,___, ll
	TI6,___, ll ___,___, ll GR7,BP2, ll ___,___, ll
	BP1,BP4, ll ___,___, ll BP1,BT1, ll BT1,___, ll
	// ----------------------------------------- //
	// SILENCE BEFORE WILD SHIT
	// ----------------------------------------- //
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	// WRINKLE 4 + WRINKLE 3
	// ----------------------------------------- //
	BT1,TIB, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	BT1,TIB, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	// WRINKLE 5 (???)
	// ----------------------------------------- //
	BT1,___, ll ___,ML1, ll GR7,ML1, ll ML1,___, ll
	BP4,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ML1,___, ll GR7,BP2, ll ___,___, ll
	___,BP4, ll ___,___, ll ___,___, ll ML1,___, ll
	// ----------------------------------------- //
	BT1,___, ll ___,ML1, ll GR7,ML1, ll ML1,___, ll
	BP4,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ML1,___, ll GR7,BP2, ll ___,___, ll
	___,BP4, ll ___,___, ll ___,___, ll ML1,___, ll
	// ----------------------------------------- //
	// WRINKLE 4 + WRINKLE 3 (with beat)
	// ----------------------------------------- //
	BT1,TIB, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,BP4, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	BT1,TIB, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,BP4, ll ___,___, ll BT1,___, ll ___,BT1, ll
	// ----------------------------------------- //
	// TOTAL BREAKDOWN
	// ----------------------------------------- //
	ML2,TIB, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP4,___, ll ___,___, ll BP4,___, ll ___,___, ll
	TI6,___, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,BP4, ll ___,___, ll BP5,___, ll ___,___, ll
	// ----------------------------------------- //
	ML2,TIB, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP4,___, ll ___,___, ll BP4,___, ll ___,___, ll
	TI6,___, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,BP4, ll ___,___, ll BP5,___, ll ___,___, ll
	// ----------------------------------------- //

	// ----------------------------------------- //
	ML2,TIB, ll ML1,BP5, ll GR7,ML1, ll ML1,___, ll
	BP4,___, ll ___,___, ll BP4,___, ll ___,___, ll
	ML2,___, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,BP4, ll ___,___, ll BP5,___, ll ___,___, ll
	// ----------------------------------------- //
	ML2,TIB, ll ML1,BP5, ll GR7,ML1, ll ML1,___, ll
	BP4,___, ll ___,___, ll BP4,___, ll ___,___, ll
	ML2,___, ll ML1,___, ll GR7,ML1, ll ML1,___, ll
	BP1,BP4, ll ___,___, ll BP5,___, ll ___,___, ll
	// ----------------------------------------- //
	// WRAPUP
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
	TI6,TIB, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	TI6,___, ll ___,___, ll ___,___, ll ___,___, ll
	___,___, ll ___,___, ll ___,___, ll ___,___, ll
	// ----------------------------------------- //
};

#define SONG_LENGTH 75
#define SAMPLE_RATE 44100
#define MAX_SAMPLES SAMPLE_RATE*SONG_LENGTH
#define BPM (SAMPLE_RATE*60)/(DECODED_DATA_SIZE*8)
int16_t song_samples[MAX_SAMPLES];
static void generate_song() {

	for (int k = 0; k < 2; k++) {
		OpusDecoder* opus_decoder = opus_decoder_create(48000, 1, NULL);
		for (int i = 0; i < (int)sizeof(MUSIC_ROLL)/2; i++) {
			decode_random_packet(MUSIC_ROLL[i*2+k], opus_decoder);

			for (int j = 0; j < DECODED_DATA_SIZE && i*DECODED_DATA_SIZE+j < MAX_SAMPLES; j++) {
				song_samples[i*DECODED_DATA_SIZE+j] += DECODED_DATA[j]*3;
			}
		}
	}

	for(int i = DECODED_DATA_SIZE*128; i < MAX_SAMPLES; i++) {
		int phased_i = i + triangle(i, 200, 300, 2) + DECODED_DATA_SIZE;
		song_samples[i] += triangle(phased_i, 600, triangle(phased_i, DECODED_DATA_SIZE*32, 20000, 1), 2);
	}

	for(int i = DECODED_DATA_SIZE*640; i < DECODED_DATA_SIZE*(640+8); i++) {
		int phased_i = i + triangle(i, 20, 60, 2) + DECODED_DATA_SIZE;
		song_samples[i] += triangle(phased_i, 120, triangle(phased_i, DECODED_DATA_SIZE*16, 10000, 1), 2);
	}
}

static void audio_callback(void* userdata, Uint8* stream, int len) {
	(void)userdata;
	static int audiotime = START_OFFSET_SECONDS*SAMPLE_RATE*2;
	for(int i = 0; i < len; i++) {
		audiotime++;
		if(audiotime > MAX_SAMPLES*2) quit();
		stream[i] = ((Uint8*)song_samples)[audiotime];
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
		CANVAS_WIDTH,
		CANVAS_HEIGHT,
		SDL_WINDOW_OPENGL
#if defined(FULLSIZE)
			| SDL_WINDOW_FULLSCREEN
#endif
	);

	SDL_GL_CreateContext(mainwindow);

	static SDL_AudioSpec desired = {
		.freq = SAMPLE_RATE,
		.format = AUDIO_S16MSB,
		.channels = 1,
		.silence = 0,
		.samples = 1024,
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
			if (Event.type == SDL_QUIT
#if defined(KEY_HANDLING)
				|| (Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_ESCAPE)
#endif
			) {
				quit();
			}
		}
		on_render();
		SDL_GL_SwapWindow(mainwindow);
	}
	__builtin_unreachable();
}