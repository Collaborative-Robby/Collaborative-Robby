#include <stdio.h>
#include <stdlib.h>
#include <robby/struct.h>
#include <robby/module.h>

void move(struct map *m, struct robby *r)
{
	int i,dirnum = -1;
	int* dir;

	m->innermatrix[r->x][r->y]=r->over;

	if (r->over == CAN_DUMMY_PTR) {
		/* pick up can */
		r->over = NULL;
		r->gathered_cans++;
	} else {
		/* Move in a random direction */
		dirnum=rand()%4; 
		dir=directions[dirnum];

		r->x=(r->x+dir[0])%(m->sizex);
		r->y=(r->y+dir[1])%(m->sizey);

		r->over=m->innermatrix[r->x][r->y];
		m->innermatrix[r->x][r->y]=r;
	}

	PRINT_MOVE_INFO(dirnum, r->id);
}

void generate_robbies(struct robby *rl, long unsigned int robbynum,
		long unsigned int generation)
{
	int i;
	/* do nothing */
	if (generation == 0)
		for (i = 0; i < robbynum; i++)
			rl[i].id = i;
}
