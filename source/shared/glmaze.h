#ifndef __GLMAZE_H__
#define __GLMAZE_H__

#include "maze_std.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TEX_WALL  = 0,
    TEX_FLOOR,
    TEX_CEILING,
    TEX_START,
    TEX_END,
    TEX_RAT,
    TEX_AD,
    TEX_COVER,
    NUM_TEXTURES
};

void UseTexture( Texture *pTex );

#define MAX_RATS 10

extern MazeOptions maze_options;

#ifdef __cplusplus
}
#endif

#endif
