#ifndef __SLVMAZE_H__
#define __SLVMAZE_H__

#define ANI_STATE_NONE          0
#define ANI_STATE_TURN_TO       1
#define ANI_STATE_TURN_AWAY     2
#define ANI_STATE_FORWARD       3
#define ANI_STATE_REVERSE       4

#define SOL_DIR_UNKNOWN -1
#define SOL_DIR_LEFT    0
#define SOL_DIR_UP      1
#define SOL_DIR_RIGHT   2
#define SOL_DIR_DOWN    3
#define SOL_DIRS        4

#define SOL_TURN_LEFT   0
#define SOL_TURN_RIGHT  1

extern int left_turn[SOL_DIRS];
extern int right_turn[SOL_DIRS];
extern uint8_t dir_wall[SOL_DIRS];
extern int dir_cloff[SOL_DIRS][2];
extern FaAngle dir_ang[SOL_DIRS];
extern FxPt2 dir_off[SOL_DIRS];

/* We want to traverse one quarter of a circle in the given number of
   steps.  The distance is the arc length which is r*pi/2.  Divide that
   by the number of steps to get the distance each step should travel */
#define ARC_STEP 5
#define ARC_STEPS (90/ARC_STEP)

#define REVERSE_STEP (2*ARC_STEP)
#define REVERSE_STEPS (180/REVERSE_STEP)

typedef struct _MazeGoal
{
    int clx, cly;
    void *user;
} MazeGoal;

typedef struct _MazeSolution
{
    int clx, cly;
    int dir;
    Cell *maze;
    int w, h;
    MazeGoal *goals;
    int ngoals;
    int ani_state;
    int ani_count;
    int *turn_to;
    int *turn_away;
    int dir_sign;
} MazeSolution;

extern void SetView(MazeSolution *sol, MazeView *vw);

void SolveMazeStart(MazeView *vw,
                    Cell *maze, int w, int h,
                    IntPt2 *start, int start_dir,
                    MazeGoal *goals, int ngoals,
                    int turn_to,
                    MazeSolution *sol);
void SolveMazeSetGoals(MazeSolution *sol, MazeGoal *goals, int ngoals);
MazeGoal *SolveMazeStep(MazeView *vw, MazeSolution *sol);
MazeGoal *SolveMazeStepPlayer(MazeView *vw, MazeSolution *sol);

#endif
