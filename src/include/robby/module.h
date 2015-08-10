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

/* Directions as modification on the map */
int left[] =  {-1, 0};
int right[] = { 1, 0};
int up[] =    { 0,-1};
int down[] =  { 0, 1};

/* Array of directions and index to lookup */
int* directions[]= {left, right, up, down };
#define RIGHT directions[0]
#define LEFT directions[3]
#define UP directions[1]
#define DOWN directions[2]

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
		out = m->sizex+dir[0];\
	else\
		out =(r->x+dir[0]) % m->sizex;\
	out;\
})

/* Wrap in the two directions the y */
#define __WRAP_Y(r,m,dir) ({\
	long unsigned int out;\
	if ((int)r->y+(int)dir[1] < 0)\
		out = m->sizey+dir[1];\
	else\
		out = (r->y+dir[1]) % m->sizey;\
	out;\
})

/* Prepare the robby for the moving phase (avoid pre-move looping)*/
#define PREPARE_STATE(r) (r->moved = 1)

/* Update the robby state (update the view with the new values) */
#define UPDATE_STATE(r, m) ({\
	/*update_view(r, m);*/\
})

/* Move in a direction */
#define MOVE_DIR(r, m, dirnum, impact, wraparound) ({\
	int* dir;\
	int success = 0, oob = 0;\
	long unsigned int newx, newy;\
	struct robby *new_over = NULL;\
\
	dir = directions[dirnum];\
\
	if (wraparound) {\
		/* If we have wraparound get the correct values */\
		newx = __WRAP_X(r,m,dir);\
		newy = __WRAP_Y(r,m,dir);\
	} else {\
		/* Else specify that we can't go out of the matrix bounds */\
		newx = r->x+dir[0];\
		newy = r->y+dir[1];\
		oob = OUT_OF_BOUNDS(newx, newy, m);\
	}\
\
	if (!oob && !impact && OVER_ROBBY(m, newx, newy)) {\
		/* Give control to the robby that is over the destination cell */\
		new_over = m->innermatrix[newx][newy];\
		printf("robby %d: impact avoidance started giving control to robby %d\n",\
		       r->id, new_over->id);\
		if (!new_over->moved)\
			new_over->move(m, new_over);\
	}\
\
	if (!oob && !OVER_ROBBY(m, newx, newy)) {\
		/* Restore the old over */\
		m->innermatrix[r->x][r->y]=r->over;\
		/* Set the new over */\
		r->over=m->innermatrix[newx][newy];\
\
		/* Move the robby */\
		m->innermatrix[newx][newy]=r;\
		r->x = newx;\
		r->y = newy;\
\
		success=1;\
	}\
	success;\
})

/* Moving possibilities */

/* NO wrap, NO impact (pre-give control) */
#define MOVE_NORMAL(r, m, dirnum)      MOVE_DIR(r,m, dirnum, 0, 0)

/* wrap, NO impact (pre-give control) */
#define MOVE_WRAP(r, m, dirnum)        MOVE_DIR(r,m, dirnum, 0, 1)

/* NO wrap, impact (return failure if the cell is occuped) */
#define MOVE_IMPACT(r, m, dirnum)      MOVE_DIR(r,m, dirnum, 1, 0)

/* wrap, impact (return failure if the cell is occupied) */
#define MOVE_WRAP_IMPACT(r, m, dirnum) MOVE_DIR(r,m, dirnum, 1, 1)

#endif
