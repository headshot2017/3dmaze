#include "pch.h"

#define MazeAt(x, y) (sol->maze+(x)+(y)*(sol->w))

MazeGoal *SolveMazeStepPlayer(MazeView *vw, MazeSolution *sol)
{
	Cell *cell;
	int i, dir, turn_to;

	if (sol->ani_state != ANI_STATE_NONE)
	{
		if (--sol->ani_count == 0)
		{
			sol->ani_state = ANI_STATE_NONE;
			SetView(sol, vw);
		}
	}
	
	switch(sol->ani_state)
	{
	case ANI_STATE_TURN_TO:
		vw->pos.x += FxMulDiv(FaCos(vw->ang),
							  FxFltVal(PI*MAZE_CELL_SIZE/2),
							  FxVal(ARC_STEPS*2));
		vw->pos.y += FxMulDiv(FaSin(vw->ang),
							  FxFltVal(PI*MAZE_CELL_SIZE/2),
							  FxVal(ARC_STEPS*2));
		vw->ang = FaAdd(vw->ang, sol->dir_sign*FaDeg(ARC_STEP));
		return NULL;

	case ANI_STATE_TURN_AWAY:
		vw->pos.x += FxMulDiv(FaCos(vw->ang),
							  FxFltVal(PI*MAZE_CELL_SIZE/2),
							  FxVal(ARC_STEPS*2));
		vw->pos.y += FxMulDiv(FaSin(vw->ang),
							  FxFltVal(PI*MAZE_CELL_SIZE/2),
							  FxVal(ARC_STEPS*2));
		vw->ang = FaAdd(vw->ang, sol->dir_sign * -FaDeg(ARC_STEP));
		return NULL;

	case ANI_STATE_FORWARD:
		vw->pos.x += FxMulDiv(FaCos(vw->ang), MAZE_CELL_SIZE, ARC_STEPS);
		vw->pos.y += FxMulDiv(FaSin(vw->ang), MAZE_CELL_SIZE, ARC_STEPS);
		return NULL;
		
	case ANI_STATE_REVERSE:
		vw->ang = FaAdd(vw->ang, sol->dir_sign*FaDeg(REVERSE_STEP));
		return NULL;
	}

	for (i = 0; i < sol->ngoals; i++)
	{
		if (sol->clx == sol->goals[i].clx &&
			sol->cly == sol->goals[i].cly)
		{
			return &sol->goals[i];
		}
	}

	cell = MazeAt(sol->clx, sol->cly);

	dir = sol->dir;

	uint16_t keys = keysHeld();
	uint16_t left = keys & KEY_LEFT;
	uint16_t right = keys & KEY_RIGHT;
	if ((sol->dir_sign == 1 && !view_rot) || (sol->dir_sign != 1 && view_rot))
	{
		uint16_t temp = right;
		right = left;
		left = temp;
	}

	if (left)
	{
		turn_to = sol->turn_to[dir];
		if ((dir_wall[turn_to] & cell->can_see) == 0)
		{
			// No wall present when turned, so turn that way
			sol->clx += dir_cloff[turn_to][0];
			sol->cly += dir_cloff[turn_to][1];
			sol->dir = turn_to;

			sol->ani_state = ANI_STATE_TURN_TO;
			sol->ani_count = ARC_STEPS;
			return NULL;
		}
	}
	if (right)
	{
		turn_to = sol->turn_away[sol->turn_away[sol->turn_to[dir]]];
		if ((dir_wall[turn_to] & cell->can_see) == 0)
		{
			// No wall present when turned, so turn that way
			sol->clx += dir_cloff[turn_to][0];
			sol->cly += dir_cloff[turn_to][1];
			sol->dir = turn_to;

			sol->ani_state = ANI_STATE_TURN_AWAY;
			sol->ani_count = ARC_STEPS;
			return NULL;
		}
	}
	if (keys & KEY_UP)
	{
		turn_to = sol->turn_away[sol->turn_to[dir]];
		if ((dir_wall[turn_to] & cell->can_see) == 0)
		{
			// No wall present when turned, so turn that way
			sol->clx += dir_cloff[turn_to][0];
			sol->cly += dir_cloff[turn_to][1];
			sol->dir = turn_to;

			sol->ani_state = ANI_STATE_FORWARD;
			sol->ani_count = ARC_STEPS;
			return NULL;
		}
	}
	if (keys & KEY_DOWN)
	{
		turn_to = sol->turn_to[sol->turn_to[sol->dir]];
		if ((dir_wall[turn_to] & cell->can_see) == 0)
		{
			// No wall present when turned, so turn that way
			dir = turn_to;
			sol->clx += dir_cloff[dir][0];
			sol->cly += dir_cloff[dir][1];
			sol->dir = dir;

			sol->ani_state = ANI_STATE_REVERSE;
			sol->ani_count = REVERSE_STEPS;
			return NULL;
		}
	}

	/*
	for (i = 0; i < SOL_DIRS-1; i++)
	{
		turn_to = sol->turn_to[dir];
		if ((dir_wall[turn_to] & cell->can_see) == 0)
		{
			// No wall present when turned, so turn that way
			sol->clx += dir_cloff[turn_to][0];
			sol->cly += dir_cloff[turn_to][1];
			sol->dir = turn_to;

			sol->ani_count = ARC_STEPS;
			switch(i)
			{
			case 0:
				sol->ani_state = ANI_STATE_TURN_TO;
				break;

			case 1:
				sol->ani_state = ANI_STATE_FORWARD;
				break;

			case 2:
				sol->ani_state = ANI_STATE_TURN_AWAY;
				break;
			}
			break;
		}
		else
		{
			// Wall present, turn away and check again
			dir = sol->turn_away[dir];
		}
	}

	if (i == SOL_DIRS-1)
	{
		// Dead end.  Turn around
		dir = sol->turn_to[sol->turn_to[sol->dir]];
		sol->clx += dir_cloff[dir][0];
		sol->cly += dir_cloff[dir][1];
		sol->dir = dir;

		sol->ani_state = ANI_STATE_REVERSE;
		sol->ani_count = REVERSE_STEPS;
	}
	*/

	return NULL;
}