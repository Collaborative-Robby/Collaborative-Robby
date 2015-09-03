/*****************************************************************************/
/* Module headers for the multi-robby simulator                              */
/* This library is a part of the Collaborative-Robby software.               */
/*                                                                           */
/* Copyright (C) 2015                                                        */
/* Daniele Baschieri, Davide Berardi, Giulio Biagini, Michele Corazza        */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or any later version.                                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,*/
/* USA.                                                                      */
/*****************************************************************************/
#ifndef _ROBBY_MODULE_H
#define _ROBBY_MODULE_H

#include <stdio.h>
#include <robby/struct.h>

/* Extern (got from the main engine) dummy variable to recognize
   the cans on the map */
extern char __can_const;
#define CAN_DUMMY_PTR (void *)&__can_const

/* Array of directions and index to lookup */
extern int** directions;
#define RIGHT directions[0]
#define LEFT directions[3]
#define UP directions[1]
#define DOWN directions[2]


#define DISSIN(n, r) ((round((sin((M_PI / (2.0 * (float) r)) * (float) n) * r))))

//fixed view radius for every robby
#define VIEW_RADIUS 2

#define SQUARE_SIDE ((VIEW_RADIUS-1)*2+1)

#define SQUARE_AREA (SQUARE_SIDE*SQUARE_SIDE)

#define POSSIBLE_MOVES 5

/* Print the last move status */
#define PRINT_MOVE_INFO(move, id, s) ({\
	switch(dirnum) {\
		case 0:\
			printf("robby %d: left", id);\
			break;\
		case 1:\
			printf("robby %d: right", id);\
			break;\
		case 2:\
			printf("robby %d: up", id);\
			break;\
		case 3:\
			printf("robby %d: down", id);\
			break;\
		case -1:\
			printf("robby %d: picked up a can", id);\
			break;\
		default:\
			printf("Move unknown to the print engine");\
	}\
	/* Check if the move failed */\
	if (!s)\
		printf(" FAIL");\
	printf("\n");\
})

/* The indexes are out of bound on the map */
#define OUT_OF_BOUNDS(x, y, m) ((x >= m->sizex || y >= m->sizey))
/* The move will be out of bound on the map */
#define OUT_OF_BOUNDS_MOVE(r, m, d) (OUT_OF_BOUNDS(r->x+d[0], r->y+d[1], m))

/* The coordinates refer to a cell with a robby on it */
#define OVER_ROBBY(m, x, y) ({\
	void *newover = NULL;\
	if (!OUT_OF_BOUNDS(x, y, m)) newover = m->innermatrix[x][y];\
	(newover && (newover != CAN_DUMMY_PTR));\
})

/* Wrap in the two directions the x */
#define __WRAP_X(r,m,dir) ({\
	long unsigned int out;\
	if ((int)r->x+(int)dir[0] < 0)\
		out = (long unsigned int)((int)m->sizex+dir[0]);\
	else\
		out = (long unsigned int)((int)r->x+dir[0]) % m->sizex;\
	out;\
})

/* Wrap in the two directions the y */
#define __WRAP_Y(r,m,dir) ({\
	long unsigned int out;\
	if ((int)r->y+(int)dir[1] < 0)\
		out = (long unsigned int)((int)m->sizey+dir[1]);\
	else\
		out = (long unsigned int)((int)r->y+dir[1]) % m->sizey;\
	out;\
})

/* Prepare the robby for the moving phase (avoid pre-move looping)*/
#define PREPARE_STATE(r) (r->moved = 1)

extern "C" {
/* Move in a direction */
int move_dir(struct robby *r, struct world_map *m, int dirnum, int impact, int waround);

/* Update the robby view */
int update_view(struct robby *r, struct world_map *m, int wraparound);


void generate_robbies(struct robby **rl, long unsigned int couplenum,
		long unsigned int robbynum,
		long unsigned int generation);


int move(struct world_map *m, struct robby *r);

void cleanup(struct robby **rl, int couplenum, int rnum);
}

/* Moving possibilities */

/* NO wrap, NO impact (pre-give control) */
#define MOVE_NORMAL(r, m, dirnum)      move_dir(r,m, dirnum, 0, 0)

/* wrap, NO impact (pre-give control) */
#define MOVE_WRAP(r, m, dirnum)        move_dir(r,m, dirnum, 0, 1)

/* NO wrap, impact (return failure if the cell is occuped) */
#define MOVE_IMPACT(r, m, dirnum)      move_dir(r,m, dirnum, 1, 0)

/* wrap, impact (return failure if the cell is occupied) */
#define MOVE_WRAP_IMPACT(r, m, dirnum) move_dir(r,m, dirnum, 1, 1)

#endif
