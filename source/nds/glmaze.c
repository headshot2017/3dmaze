#include "pch.h"

#include <fat.h>
#include <time.h>
#include <assert.h>

MazeView vw;
MazeSolution sol;
bool player_mode = false;

MazeOptions maze_options =
{
    false,          // depth_test
    RENDER_TEXTURED,    // render mode (WALLS)
    RENDER_TEXTURED,    // render mode (FLOOR)
    RENDER_TEXTURED,    // render mode (CEILING)
    false,           // frame_count
    false,          // top_view
    true,           // eye_view
    false,          // single_step
    false,          // all_alpha
    false,          // bDither
    5               // nrats
};

float gfAspect = 1.0f;  // aspect ratio

#define SST_NEW_MAZE    0
#define SST_MAZE_GROW   1
#define SST_SOLVING     2
#define SST_MAZE_SHRINK 3
#define SST_ROTATE      4

int solve_state = SST_NEW_MAZE;

// Table of textures used.  The first entries contain 1 texture for each main
// surface, followed by textures used for various objects
//TEXTURE all_textures[NUM_TEXTURES];

// Environment associated with the texture (repetition, palette rotation,..) 
// For now, have one for each of the textures in all_textures.  But, this 
// could be a per-object thing.
//TEX_ENV gTexEnv[NUM_TEXTURES];

// texture environment mode
int gTexEnvMode;

// Texture resources for the main surfaces
/*
TEX_RES gTexResSurf[NUM_DEF_SURFACE_TEXTURES] = {
    {TEX_BMP, IDB_BRICK},       // default textures
    {TEX_BMP, IDB_WOOD},
    {TEX_BMP, IDB_CASTLE},
    {TEX_A8,  IDB_CURL4},       // mandelbrot textures
    {TEX_A8,  IDB_BHOLE4},
    {TEX_A8,  IDB_SNOWFLAK},
    {TEX_A8,  IDB_SWIRLX4}
};
*/

// Texture resources for objects
/*
TEX_RES gTexResObject[NUM_OBJECT_TEXTURES] = {
    {TEX_A8,  IDB_START},
    {TEX_A8,  IDB_END},
    {TEX_A8,  IDB_RAT},
    {TEX_A8,  IDB_AD},
    {TEX_BMP, IDB_COVER}
};
*/
//SSContext gssc;

void maze_Init();
void RotateTexturePalettes( TEX_ENV *pTexEnv, int count, int iRot );

enum {
    TIMER_START = 0,
    TIMER_STOP
};

void Draw(void)
{
	/*
    GLbitfield mask;

    mask = 0;
    if (maze_options.depth_test)
    {
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (maze_options.render[FLOOR] == RENDER_NONE ||
        maze_options.render[CEILING] == RENDER_NONE ||
        maze_options.render[WALLS] == RENDER_NONE ||
        maze_height < 1.0f)
    {
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (mask != 0)
    {
        glClear(mask);
    }
	*/
	if (maze_options.depth_test)
    {
		glClearDepth(GL_MAX_DEPTH);
	}
	if (maze_options.render[FLOOR] == RENDER_NONE ||
        maze_options.render[CEILING] == RENDER_NONE ||
        maze_options.render[WALLS] == RENDER_NONE ||
        maze_height < 1.0f)
    {
		glClearColor(0, 0, 0, 31);
	}

    // Rotate the palettes of any paletted texures by 1
    //RotateTexturePalettes( gTexEnv, NUM_TEXTURES, 1 );

    if (maze_options.eye_view)
    {
        DrawMaze(&vw);
    }

    if (maze_options.top_view)
    {
        DrawTopView(&vw);
    }
}

MazeGoal maze_goals[MAX_GOALS];
int ngoals;

Object special_obj[MAX_SPECIALS];

#define MAX_LETTERS 3
Object letters_obj[MAX_LETTERS];

typedef struct _Rat
{
    Object obj;
    MazeView vw;
    MazeSolution sol;
} Rat;
Rat rats[MAX_RATS];

void NewMazeList(void)
{
	/*
    glNewList(maze_walls_list, GL_COMPILE);
    DrawMazeWalls();
    glEndList();
	*/

	uint32_t* l = (uint32_t*)maze_walls_list;
	l[0] = DrawMazeWallsList(maze_walls_list);
}

void UpdateRatPosition(Rat *rat)
{
    MoveObject(&rat->obj, rat->vw.pos.x, rat->vw.pos.y);
    // Invert the angle because view angles move opposite to
    // object angles
    rat->obj.ang = FaAdd(0, -rat->vw.ang);
}

// Simple routines to pick cells at random while guaranteeing
// that those cells are unoccupied

static int rnd_cells[MAZE_CELLS];
static int nrnd_cells;

void InitRandomCells(void)
{
    int i;

    nrnd_cells = MAZE_CELLS;
    for (i = 0; i < nrnd_cells; i++)
    {
        rnd_cells[i] = i;
    }
}

void PickRandomCell(IntPt2 *pos)
{
    int idx, t;

#if DBG
    if (nrnd_cells == 0)
    {
        MessageBox(GetDesktopWindow(), TEXT("Out of random cells"),
                   NULL, MB_OK);
        pos->x = 0;
        pos->y = 0;
        return;
    }
#endif
    
    idx = rand() % nrnd_cells;
    nrnd_cells--;
    t = rnd_cells[idx];
    rnd_cells[idx] = rnd_cells[nrnd_cells];
    pos->x = t % MAZE_GRID;
    pos->y = t / MAZE_GRID;
}

void RemoveRandomCell(IntPt2 *pos)
{
    int i, idx;

    for (i = 0; i < nrnd_cells; i++)
    {
        idx = rnd_cells[i];
        if ((idx % MAZE_GRID) == pos->x &&
            (idx / MAZE_GRID) == pos->y)
        {
            nrnd_cells--;
            rnd_cells[i] = rnd_cells[nrnd_cells];
            return;
        }
    }
}

void NewMaze(void)
{
    IntPt2 cell, start_cell;
    int i, nspecials, nads;
    static bool firstMaze = true;

    // If not in full screen mode, move the maze window around after it's solved
    //if( !gbTurboMode && !firstMaze )
        //ss_RandomWindowPos();

    InitRandomCells();
    
    if (!InitMaze(&start_cell, maze_goals, &ngoals))
    {
        printf("InitMaze failed\n");
        exit(1);
    }

    RemoveRandomCell(&start_cell);
    cell.x = maze_goals[GOAL_END].clx;
    cell.y = maze_goals[GOAL_END].cly;
    RemoveRandomCell(&cell);

    nspecials = rand() % MAX_SPECIALS;
    for (i = 0; i < nspecials; i++)
    {
        PickRandomCell(&cell);
        maze_goals[ngoals].clx = cell.x;
        maze_goals[ngoals].cly = cell.y;
        maze_goals[ngoals].user = &special_obj[i];
        
        special_obj[i].w = FMAZE_CELL_SIZE/4;
        special_obj[i].h = FxFltVal(.25);
        special_obj[i].z = special_obj[i].h;
        special_obj[i].col = 15;
        special_obj[i].draw_style = DRAW_SPECIAL;
        special_obj[i].draw_arg = 0;
        special_obj[i].ang = FaDeg(0);
        special_obj[i].user1 = (rand() % 6)+2;
        special_obj[i].user2 = rand() % 5;
        special_obj[i].user3 = 0;
        PlaceObject(&special_obj[i],
                    CellToMfx(maze_goals[ngoals].clx)+FMAZE_CELL_SIZE/2,
                    CellToMfx(maze_goals[ngoals].cly)+FMAZE_CELL_SIZE/2);
        ngoals++;
    }
    while (i < MAX_SPECIALS)
    {
        special_obj[i].cell = NULL;
        i++;
    }

    nads = (rand() % (MAX_LETTERS*2))-MAX_LETTERS+1;
    for (i = 0; i < nads; i++)
    {
        PickRandomCell(&cell);
        
        letters_obj[i].w = FMAZE_CELL_SIZE/3;
        letters_obj[i].h = FxFltVal(.33);
        letters_obj[i].z = FxFltVal(.5);
        letters_obj[i].col = 15;
        letters_obj[i].draw_style = DRAW_POLYGON;
        letters_obj[i].pTex = &gTextures[ TEX_AD ];
        letters_obj[i].ang = FaDeg(0);
        PlaceObject(&letters_obj[i],
                    CellToMfx(cell.x)+FMAZE_CELL_SIZE/2,
                    CellToMfx(cell.y)+FMAZE_CELL_SIZE/2);
    }
    while (i < MAX_LETTERS)
    {
        letters_obj[i].cell = NULL;
        i++;
    }

    for (i = 0; i < maze_options.nrats; i++)
    {
        PickRandomCell(&cell);
        
        rats[i].obj.w = FMAZE_CELL_SIZE/4;
        rats[i].obj.h = FxFltVal(.125);
        rats[i].obj.z = rats[i].obj.h;
        rats[i].obj.col = 16;
        rats[i].obj.draw_style = DRAW_POLYGON;
        rats[i].obj.pTex = &gTextures[ TEX_RAT ];
        SolveMazeStart(&rats[i].vw, &maze_cells[0][0], MAZE_GRID, MAZE_GRID,
                       &cell, SOL_DIR_LEFT,
                       NULL, 0,
                       (rand() % 1000) > 500 ? SOL_TURN_LEFT : SOL_TURN_RIGHT,
                       &rats[i].sol);
        // Need to force this to NULL when a new maze is generated so
        // that moving doesn't try to use old maze data
        rats[i].obj.cell = NULL;
        UpdateRatPosition(&rats[i]);
    }
    
    NewMazeList();
    
    SolveMazeStart(&vw, &maze_cells[0][0], MAZE_GRID, MAZE_GRID,
                   &start_cell, SOL_DIR_RIGHT,
                   maze_goals, ngoals,
                   (rand() % 1000) > 500 ? SOL_TURN_LEFT : SOL_TURN_RIGHT,
                   &sol);

    solve_state = SST_MAZE_GROW;
    firstMaze = false;
}

/**************************************************************************\
* Step
*
* Main draw proc
*
\**************************************************************************/

static MazeGoal *found_goal = NULL;
static int rot_step;

void Step()
{
    int i;
    MazeGoal *goal;

	if (keysDown() & KEY_START && solve_state != SST_MAZE_GROW)
	{
		solve_state = SST_NEW_MAZE;
		player_mode = !player_mode;
		printf("%s player mode\n", player_mode ? "Entering" : "Leaving");
		if (player_mode)
			printf("D-Pad to move, A to toggle map\n");
	}
	if (keysDown() & KEY_A)
		maze_options.top_view = !maze_options.top_view;

    switch(solve_state)
    {
    case SST_NEW_MAZE:
        view_rot = 0;
        maze_height = 0.0f;
        NewMaze();
		if (player_mode)
			printf("New level started\n");
        break;

    case SST_SOLVING:
        for (i = 0; i < maze_options.nrats; i++)
        {
            SolveMazeStep(&rats[i].vw, &rats[i].sol);
            UpdateRatPosition(&rats[i]);
        }
        
        goal = (player_mode) ? SolveMazeStepPlayer(&vw, &sol) : SolveMazeStep(&vw, &sol);
        if (goal == &maze_goals[GOAL_END])
        {
            solve_state = SST_MAZE_SHRINK;
			if (player_mode)
				printf("GG!\n");
        }
        else if (goal != NULL)
        {
            solve_state = SST_ROTATE;
            found_goal = goal;
            rot_step = 0;
        }
        break;

    case SST_MAZE_GROW:
        maze_height += .025f;
        if (maze_height >= 1.0f)
        {
            solve_state = SST_SOLVING;
        }
        break;
        
    case SST_MAZE_SHRINK:
        maze_height -= .025f;
        if (maze_height <= 0.0f)
        {
            solve_state = SST_NEW_MAZE;
        }
        break;

    case SST_ROTATE:
        view_rot += 10;
        if (++rot_step == 18)
        {
            Object *sp_obj;

            sp_obj = (Object *)found_goal->user;
            RemoveObject(sp_obj);
            
            solve_state = SST_SOLVING;
            ngoals--;
            if (found_goal < maze_goals+ngoals)
            {
                memmove(found_goal, found_goal+1,
                        sizeof(MazeGoal)*(ngoals-(found_goal-maze_goals)));
            }
            SolveMazeSetGoals(&sol, maze_goals, ngoals);
            found_goal = NULL;

            if (view_rot >= 360)
            {
                view_rot = 0;
            }
            else
            {
                view_rot = 180;
            }
        }
        break;
    }

    Draw();

    for (i = 0; i < MAX_SPECIALS; i++)
    {
        // Increment rotations of any specials still present in the maze
        if (special_obj[i].cell != NULL)
        {
            special_obj[i].ang = FaAdd(special_obj[i].ang,
                                       FaDeg(special_obj[i].user1));
            special_obj[i].user3 += special_obj[i].user2;
        }
    }
	
	glFlush(0);
}

void UpdateModes(void)
{
    if (maze_options.depth_test)
    {
        //glEnable(GL_DEPTH_TEST);
    }
    else
    {
        //glDisable(GL_DEPTH_TEST);
    }
}

/**************************************************************************\
* Reshape
*
*       - called on resize, expose                                      
*       - always called on app startup                                  
*
\**************************************************************************/

void 
Reshape(int width, int height, void *data )
{
    gfAspect = height == 0 ? 1.0f: (float) width / (float) height;
}


/******************************Public*Routine******************************\
* VerifyTextureFiles
*
* Check that any user specified textures are valid
* - MUST be called at ss_Init, or any error msgs will not display properly
*
\**************************************************************************/

void VerifyTextureFiles(void)
{
    //ss_DisableTextureErrorMsgs();

	/*
    for (int i = 0; i < NUM_SURFACES; i++) {
        if( gTexInfo[i].bTex && !gTexInfo[i].bDefTex ) {
            if( !ss_VerifyTextureFile( gTexInfo[i].texFile ) )
                // use default texture
                gTexInfo[i].bDefTex = true;
        }
    }
	*/
}

/******************************Public*Routine******************************\
* CalcRep
*
* Figure out repetion based on size.
*
* - Use 128 pixels as a unit repetition reference
* - Size is always a power of 2
*
\**************************************************************************/

static int
CalcRep( int size )
{
    double pow2;
    int pow2ref = 8;
    int rep;

    pow2 = log((double)size) / log((double)2.0);
    rep = 1 + pow2ref - (int)pow2;
    return rep; 
}

// tradeoff
#define MAX_TEX_DIM 128
//#define MAX_TEX_DIM 256

/******************************Public*Routine******************************\
* CalcTexRep
*
* Figure out texture repetition based on texture size.
*
* - mf: ? double for walls/ceiling ?
*
\**************************************************************************/

/*
static void
CalcTexRep( TEXTURE *pTex, IPOINT2D *pTexRep )
{
    if( pTex->width >= MAX_TEX_DIM )
        pTexRep->x = 1;
    else
        pTexRep->x = CalcRep( pTex->width );

    if( pTex->height >= MAX_TEX_DIM )
        pTexRep->y = 1;
    else
        pTexRep->y = CalcRep( pTex->height );
}
*/

/******************************Public*Routine******************************\
* LoadTextures
*
* Load textures from .bmp files or from embedded resources, using
* global texture info structure.
*
* For now, this also sets render modes, since if texturing is off for a
* surface, we always use RENDER_FLAT
*
\**************************************************************************/

#define RGB_to_DS(src) \
	((src[0] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[2] & 0xF8) << 7) | ((255 & 0x80) << 8)

#define RGBA8_to_DS(src) \
	((src[0] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[2] & 0xF8) << 7) | ((src[3] & 0x80) << 8)

static int FindColorInPalette(u16* pal, int pal_size, u16 col)
{
	if ((col >> 15) == 0) return 0;
	
	for (int i = 1; i < pal_size; i++) {
		if(pal[i] == col) return i;
	}
	
	return -1;
}

static GL_TEXTURE_TYPE_ENUM Palettize(int w, int h, int n, unsigned char* data, unsigned short* tmp, unsigned short* tmp_palette)
{
	// transfer to tmp
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			u8* src = data + ((y * w + x) * n);
			u16* dst = tmp + (y * w + x);

			if (n == 3)
				*dst = RGB_to_DS(src);
			else
				*dst = RGBA8_to_DS(src);
		}
	}

	tmp_palette[0] = 0;
	int pal_size = 1;
	for (int i = 0; i < w * h; i++)
	{
		u16 col = tmp[i];
	
		int idx = FindColorInPalette(tmp_palette, pal_size, col);
		
		if (idx == -1) {
			pal_size++;
			if (pal_size > 256) break;
			tmp_palette[pal_size - 1] = col;
		}
	}

	GL_TEXTURE_TYPE_ENUM GLformat = (n == 3) ? GL_RGB : GL_RGBA;
	if(pal_size <= 4) GLformat = GL_RGB4;
	else if(pal_size <= 16) GLformat = GL_RGB16;
	else if(pal_size <= 256) GLformat = GL_RGB256;

	if(GLformat != GL_RGBA && GLformat != GL_RGB)
	{
		char* tmp_chr = (char*) tmp;
		
		for (int i = 0; i < w * h; i++)
		{
			u16 col = tmp[i];
			int idx = FindColorInPalette(tmp_palette, pal_size, col);
			
			if(GLformat == GL_RGB256) {
				tmp_chr[i] = idx;
			} else if(GLformat == GL_RGB16) {
				if((i & 1) == 0) {
					tmp_chr[i >> 1] = idx;
				} else {
					tmp_chr[i >> 1] |= idx << 4;
				}
			} else {
				if((i & 3) == 0) {
					tmp_chr[i >> 2] = idx;
				} else {
					tmp_chr[i >> 2] |= idx << (2 * (i & 3));
				}
			}
		}
	}

	return GLformat;
}

static void 
LoadTextures(void)
{
	for (int i=0; i<NUM_TEXTURES; i++)
	{
		Texture* pTex = &gTextures[i];

		// attempt to palettize texture
		u16* tmp = (u16*)malloc(pTex->w * pTex->h * sizeof(u16));
		if (!tmp)
		{
			printf("FAILED %d: tmp\n", i);
			continue;
		}
		u16* tmp_palette = (u16*)malloc(256*sizeof(u16));
		if (!tmp_palette)
		{
			printf("FAILED %d: tmp_palette\n", i);
			free(tmp);
			continue;
		}

		GL_TEXTURE_TYPE_ENUM GLformat = Palettize(pTex->w, pTex->h, pTex->n, pTex->data, tmp, tmp_palette);

		glGenTextures(1, (int*)(&pTex->id));
		glBindTexture(0, (int)pTex->id);

		if (glTexImage2D(GL_TEXTURE_2D, 0, GLformat, pTex->w, pTex->h, 0, 0, tmp) == 0)
		{
			glDeleteTextures(1, (int*)(&pTex->id));
			pTex->id = 0;
			free(tmp);
			free(tmp_palette);
			printf("glTexImage2D failed %d: %d %d %d\n", i, pTex->w, pTex->h, pTex->n);
			continue;
		}

		switch(GLformat)
		{
			case GL_RGB4:
				printf("%d: GL_RGB4\n", i);
				break;

			case GL_RGB16:
				printf("%d: GL_RGB16\n", i);
				break;

			case GL_RGB256:
				printf("%d: GL_RGB256\n", i);
				break;

			case GL_RGBA:
				printf("%d: GL_RGBA\n", i);
				break;

			case GL_RGB:
				printf("%d: GL_RGB\n", i);
				break;

			default: break;
		}

		if (GLformat != GL_RGBA && GLformat != GL_RGB)
		{
			int glPalSize;
			if(GLformat == GL_RGB4) glPalSize = 4;
			else if(GLformat == GL_RGB16) glPalSize = 16;
			else glPalSize = 256;

			glColorTableEXT(0, 0, glPalSize, 0, 0, tmp_palette);
		}
		
		glTexParameter(0, TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);
	}

	/*
    int i;
    TEXTURE *pTex = all_textures;
    TEX_ENV *pTexEnv = gTexEnv;
    TEX_RES *pTexRes;

    assert( (NUM_SURFACES + NUM_OBJECT_TEXTURES) == NUM_TEXTURES );

    // load surface textures (wall, floor, ceiling)

    for (i = 0; i < NUM_SURFACES; i++, pTex++, pTexEnv++) {
        maze_options.render[i] = RENDER_FLAT;
        if( gTexInfo[i].bTex ) {
            pTexEnv->pTex = pTex;
            pTexEnv->bTransp = false;

            // Load user or default texture for the surface

			ss_LoadTextureFile( gTexInfo[i].texFile, pTex );
            if( !gTexInfo[i].bDefTex && 
                ss_LoadTextureFile( gTexInfo[i].texFile, pTex )) {
            } else
            {
                // Load default resource texture

                pTexRes = &gTexResSurf[ gTexInfo[i].iDefTex ];
                if( !ss_LoadTextureResource( pTexRes, pTex ) )
                    continue; 
    
                if( ss_PalettedTextureEnabled() &&
                    (pTexRes->type == TEX_A8) )
                {
                    pTexEnv->bPalRot = true;
                    pTexEnv->iPalRot = ss_iRand( 0xff );
                } else
                    pTexEnv->bPalRot = false;
            }

            maze_options.render[i] = RENDER_TEXTURED;

            // Figure out texture repetition
            CalcTexRep( pTex, &pTexEnv->texRep );
            // We would have to set texture parameters per object here,
            // but we just use default ones already set by ss_LoadTexture*
        }
    }

    // load object textures

    pTexRes = gTexResObject;
    for( i = 0; i < NUM_OBJECT_TEXTURES; i++, pTex++, pTexEnv++, pTexRes++ ) {
        if( ss_LoadTextureResource( pTexRes, pTex ) )
            pTexEnv->pTex = pTex;
        else
            pTexEnv->pTex = NULL;

        pTexEnv->bTransp = false;

        // For now we don't do palrot's on any of the object textures
        pTexEnv->bPalRot = false;
        // No repetition
        pTexEnv->texRep.x = pTexEnv->texRep.y = 1;
    }

    // Set transparency for some of the textures
    ss_SetTextureTransparency( gTexEnv[TEX_START].pTex, 0.42f, false );
    ss_SetTextureTransparency( gTexEnv[TEX_END].pTex, 0.4f, false );
    ss_SetTextureTransparency( gTexEnv[TEX_AD].pTex, 0.4f, false );

    // Enable transparency for some of the texture environments
    gTexEnv[TEX_START].bTransp = true;
    gTexEnv[TEX_END].bTransp = true;
    gTexEnv[TEX_AD].bTransp = true;
	*/
}

void UseTexture( Texture *pTex )
{
	glBindTexture(0, (!pTex) ? 0 : (int)pTex->id);
}

/******************************Public*Routine******************************\
* RotateTexturePalettes
*
* If paletted texturing is enabled, go through the supplied list of texture
* environments, and if any have a palette, increment its rotation by the
* supplied iRot value.
*
\**************************************************************************/

void
RotateTexturePalettes( TEX_ENV *pTexEnv, int count, int iRot )
{
	/*
    if( !ss_PalettedTextureEnabled() || !pTexEnv )
        return;


    for( ; count; count--, pTexEnv++ ) {

        if( !pTexEnv->pTex || !pTexEnv->pTex->pal )
            continue;

        if( pTexEnv->bPalRot ) {
            // increment palette rotation
            pTexEnv->iPalRot += iRot;
            if( pTexEnv->iPalRot >= pTexEnv->pTex->pal_size )
                pTexEnv->iPalRot = 0;
            // Only send down the new palette if texture objects are enabled,
            // since otherwise it will be sent down when texture is 
            // 'made current'
            if( ss_TextureObjectsEnabled() ) {
                ss_SetTexturePalette( pTexEnv->pTex, pTexEnv->iPalRot );
            }
        }
    }
	*/
}

/******************************Public*Routine******************************\
* InitStretchInfo
*
\**************************************************************************/

/*
static void
InitStretchInfo( STRETCH_INFO *pStretch )
{
    pStretch->baseWidth = 320;
    pStretch->baseHeight = 200;
    pStretch->bRatioMode = false;
}
*/

/******************************Public*Routine******************************\
* SetFloaterInfo
*
* Set the size and position of the floating child window
*
\**************************************************************************/


/*
static void 
SetFloaterInfo( ISIZE *pParentSize, CHILD_INFO *pChild )
{
    float hdtvAspect = 9.0f / 16.0f;
    ISIZE *pChildSize = &pChild->size;

    // Set width according to user-specified size
    // (giSize range is 0..100)
    // set min size as 1/3 parent width
    pChildSize->width = 
            (int) ((0.333f + 2.0f*giSize/300.0f) * pParentSize->width);

    // Scale height for hdtv aspect ratio
    pChildSize->height = (int) (hdtvAspect * pChildSize->width + 0.5f);
    // Ensure height not too big
    SS_CLAMP_TO_RANGE2( pChildSize->height, 0, pParentSize->height );

    pChild->pos.x = (pParentSize->width - pChildSize->width) / 2;
    pChild->pos.y = (pParentSize->height - pChildSize->height) / 2;
}
*/

/******************************Public*Routine******************************\
* ss_Init
*
* Initialize - called on first entry into ss.
* Called BEFORE gl is initialized!
* Just do basic stuff here, like set up callbacks, verify dialog stuff, etc.
*
* Fills global SSContext structure with required data, and returns ptr
* to it.
*
\**************************************************************************/

int main(int argc, char** argv)
{
	defaultExceptionHandler();

	videoSetMode(MODE_0_3D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankB(VRAM_B_TEXTURE);
	vramSetBankC(VRAM_C_TEXTURE);
	vramSetBankD(VRAM_D_TEXTURE);
	vramSetBankE(VRAM_E_TEX_PALETTE);
	vramSetBankF(VRAM_F_TEX_PALETTE);
	vramSetBankG(VRAM_G_TEX_PALETTE);
	vramSetBankH(VRAM_H_SUB_BG);

	// Setup sub screen for the text console
	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 0, 1, false, true);

	if (!fatInitDefault())
	{
		printf("Failed to init FAT\nPlease check your SD card\n");
		while (1) swiWaitForVBlank();
	}

	glInit();
	glViewport(0, 0, 255, 191);
	glEnable(GL_TEXTURE_2D);

	getIniSettings();
	VerifyTextureFiles();

	maze_Init();

	bool run = true;
	while (run)
	{
		scanKeys();

		uint32_t held = keysHeld();
		if (held & KEY_START && held & KEY_SELECT)
			run = false;

		Step();
	}

	return 0;
}

/******************************Public*Routine******************************\
* maze_Init
*
* Initializes OpenGL state
*
\**************************************************************************/
void maze_Init()
{
    float fv4[4];

    if (!FxInitialize(FA_TABLE_SIZE, 0))
    {
        printf("FxInit failed\n");
        exit(1);
    }

    //glShadeModel( GL_FLAT );
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPolyFmt(POLY_CULL_NONE | POLY_ALPHA(31));

    glClearDepth(GL_MAX_DEPTH);

	/*
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	*/

    /*
    if( (giImageQual == IMAGEQUAL_DEFAULT) || gbTurboMode ) {
        maze_options.bDither = false;
        glDisable( GL_DITHER );
    } else {
    */
        maze_options.bDither = true;
        //glEnable( GL_DITHER );
    //}

    // Load textures and set render modes
    LoadTextures();
    
    fv4[0] = MAZE_SIZE/2.0f;
    fv4[1] = MAZE_SIZE/2.0f;
    fv4[2] = 10.0f;
    fv4[3] = 1.0f;
    //glLightfv(GL_LIGHT0, GL_POSITION, fv4);
    //glEnable(GL_LIGHT0);

    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Check which texture environment function to use for objects
    //if( ss_fOnGL11() )
        //gTexEnvMode = GL_REPLACE;
    //else
        //gTexEnvMode = GL_MODULATE;

    maze_walls_list = (void*)malloc(sizeof(uint32_t) * 8192);
    
    UpdateModes();

	printf("Press START to play\n");
}
