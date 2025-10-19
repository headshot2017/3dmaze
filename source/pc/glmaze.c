#include "pch.h"
#pragma hdrstop

#include <GL/freeglut.h>
#include <time.h>
#include <assert.h>

MazeView vw;
MazeSolution sol;
bool player_mode = false;
float player_timer = 0;
float player_timer_start = 0;

SDL_Window* gSDLWindow = 0;
SDL_GLContext* gSDLGLContext = 0;

MazeOptions maze_options =
{
    true,          // depth_test
    RENDER_TEXTURED,    // render mode (WALLS)
    RENDER_TEXTURED,    // render mode (FLOOR)
    RENDER_TEXTURED,    // render mode (CEILING)
    false,           // frame_count
    false,          // top_view
    true,           // eye_view
    false,          // single_step
    false,          // all_alpha
    true,          // bDither
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
    
    glFlush();
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
    glNewList((int)maze_walls_list, GL_COMPILE);
    DrawMazeWalls();
    glEndList();
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
	player_timer = 0;

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
        special_obj[i].draw_arg = rand() % SPECIAL_ARG_COUNT;
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

static void AddTimer()
{
	player_timer = (float)(SDL_GetPerformanceCounter()) / SDL_GetPerformanceFrequency() - player_timer_start;
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

	const uint8_t* state = SDL_GetKeyboardState(NULL);
	static bool spacePressed = false;
	if (state[SDL_SCANCODE_RETURN] && solve_state != SST_MAZE_GROW)
	{
		solve_state = SST_NEW_MAZE;
		player_mode = !player_mode;
		printf("%s player mode\n", player_mode ? "Entering" : "Leaving");
		if (player_mode)
			printf("Use arrow keys to move, space to toggle map\n");
	}
	if (state[SDL_SCANCODE_SPACE] != spacePressed)
	{
		spacePressed = state[SDL_SCANCODE_SPACE];
		if (spacePressed)
			maze_options.top_view = !maze_options.top_view;
	}

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
				printf("GG! Took %.3f seconds\n", player_timer);
        }
        else if (goal != NULL)
        {
            solve_state = SST_ROTATE;
            found_goal = goal;
            rot_step = 0;
        }

		AddTimer();
        break;

    case SST_MAZE_GROW:
        maze_height += .025f;
        if (maze_height >= 1.0f)
        {
            solve_state = SST_SOLVING;
			player_timer_start = (float)(SDL_GetPerformanceCounter()) / SDL_GetPerformanceFrequency();
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
		AddTimer();
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
}

void UpdateModes(void)
{
    if (maze_options.depth_test)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
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

static void 
LoadTextures(void)
{
	for (int i=0; i<NUM_TEXTURES; i++)
	{
		glGenTextures(1, (GLuint*)(&gTextures[i].id));
		glBindTexture(GL_TEXTURE_2D, (GLuint)gTextures[i].id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gTextures[i].w, gTextures[i].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, gTextures[i].data);
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
	if (!pTex)
	{
		glDisable(GL_TEXTURE_2D);
		return;
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, (GLuint)pTex->id);

	/*
    static HTEXTURE hCurTex = (HTEXTURE) -1;
    HTEXTURE hTex = pTexEnv->pTex;

    // We cache the current texture for 'no texture object' case

    if( !ss_TextureObjectsEnabled() &&
        (hCurTex == hTex) )
        return; // same texture, no need to send it down

    // Make this texture current

    ss_SetTexture( hTex );

    // Set texture palette if necessary

    if( pTexEnv->bPalRot &&
        !ss_TextureObjectsEnabled() && 
        ss_PalettedTextureEnabled() ) {

        // We need to send down the (rotated) palette
        ss_SetTexturePalette( hTex, pTexEnv->iPalRot );
    }

    hCurTex = hTex;
	*/
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
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 1;

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );

    gSDLWindow = SDL_CreateWindow( 
        "maze",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!gSDLWindow)
		return 1;

    gSDLGLContext = SDL_GL_CreateContext(gSDLWindow);
    SDL_GL_SetSwapInterval(1);

    if (!gSDLGLContext)
		return 1;

	glewExperimental = GL_TRUE;
	const GLenum glewInitResult = glewInit();
	if (glewInitResult != GLEW_OK)
		return 1;

	glutInit(&argc, argv);

	getIniSettings();
    VerifyTextureFiles();

	srand(time(0));

	maze_Init();

	bool run = true;
	while (run)
	{
		SDL_Event event;

		while( SDL_PollEvent( &event ) )
		{
			switch( event.type )
			{
			case SDL_QUIT:
				run = false;
				break;

			case SDL_KEYDOWN:
				if( event.key.keysym.sym == SDLK_ESCAPE )
					run = false;

			case SDL_WINDOWEVENT:
				if( event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED )
				{
					int w = event.window.data1;
					int h = event.window.data2;
					glViewport( 0, 0, w, h );
					Reshape(w, h, 0);
				}
				break;
			}
		}

		Step();

		SDL_GL_SwapWindow(gSDLWindow);
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

    glShadeModel( GL_FLAT );
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearDepth(1);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    /*
    if( (giImageQual == IMAGEQUAL_DEFAULT) || gbTurboMode ) {
        maze_options.bDither = false;
        glDisable( GL_DITHER );
    } else {
    */
        maze_options.bDither = true;
        glEnable( GL_DITHER );
    //}

    // Load textures and set render modes
    LoadTextures();
    
    fv4[0] = MAZE_SIZE/2.0f;
    fv4[1] = MAZE_SIZE/2.0f;
    fv4[2] = 10.0f;
    fv4[3] = 1.0f;
    glLightfv(GL_LIGHT0, GL_POSITION, fv4);
    glEnable(GL_LIGHT0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Check which texture environment function to use for objects
    //if( ss_fOnGL11() )
        //gTexEnvMode = GL_REPLACE;
    //else
        gTexEnvMode = GL_MODULATE; 

    maze_walls_list = (void*)glGenLists(1);
    
    UpdateModes();

	printf("Press enter to play\n");
}
