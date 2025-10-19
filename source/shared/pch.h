#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef PLATFORM_SDL
	#define SDL_MAIN_HANDLED
	#include <GL/glew.h>
	#include <SDL2/SDL.h>
	extern SDL_Window* gSDLWindow;
	extern SDL_GLContext* gSDLGLContext;
#elif defined(PLATFORM_NDS)
	#define SCALE_VERTICES 4
	#include <nds.h>
#elif defined(PLATFORM_DC)
	#include <kos.h>
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <GL/glkos.h>
#endif

#include "fixed.h"

typedef unsigned short WallFlags;

typedef struct _Texture
{
	void* id;
	int w;
	int h;
	int n;
	uint8_t* data;
} Texture;

typedef struct _Object
{
    FxPt2 p;
    FxValue z;
    FaAngle ang;
    FxValue w, h;
    int col;
    int draw_style;
    int draw_arg;
    Texture *pTex;  // ptr to texture
    int user1, user2, user3;
    struct _Object *next;
    struct _Cell *cell;
} Object;

typedef struct _Cell
{
    WallFlags can_see;
    WallFlags unseen;
    Object *contents;
    struct _Wall *walls[4];
} Cell;

extern Texture gTextures[];

#include "maze_std.h"
#include "glmaze.h"
#include "slvmaze.h"
#include "maze.h"

void getIniSettings();
