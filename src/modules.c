#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <robby/struct.h>
#include <robby/module.h>

#define ROUND(x) ((x > floor(x) + 0.5? ceil(x) : floor(x)))

#define UNFOLD_VIEWSIZE(rad) (ROUND(M_PI * rad * rad))
#define GET_VIEW_RADIUS(rad, vptr, map, wround) ({})

int update_view(struct robby *r, struct map *m, int wraparound)
{
	if (!r->view) {
		r->view = (char *)calloc(UNFOLD_VIEWSIZE(r->viewradius), sizeof(char));
		if (!r->view) {
			fprintf(stderr, "robby %d error on malloc of view\n", r->id);
			return -1;
		}
	}

	GET_VIEW_RADIUS(r->viewradius, r->view, m, wraparound);
}

/* Move in a direction */
int move_dir(struct robby *r, struct map *m, int dirnum,  int impact,  int wraparound)
{
	int* dir;
	int success = 0, oob = 0;
	long unsigned int newx, newy;
	struct robby *new_over = NULL;

	dir = directions[dirnum];

	if (wraparound) {
		/* If we have wraparound get the correct values */
		newx = __WRAP_X(r,m,dir);
		newy = __WRAP_Y(r,m,dir);
	} else {
		/* Else specify that we can't go out of the matrix bounds */
		newx = r->x+dir[0];
		newy = r->y+dir[1];
		oob = OUT_OF_BOUNDS(newx, newy, m);
	}

	if (!oob && !impact && OVER_ROBBY(m, newx, newy)) {
		/* Give control to the robby that is over the destination cell */
		new_over = m->innermatrix[newx][newy];
		printf("robby %d: impact avoidance started giving control to robby %dn",
		       r->id, new_over->id);
		if (!new_over->moved)
			new_over->move(m, new_over);
	}

	if (!oob && !OVER_ROBBY(m, newx, newy)) {
		/* Restore the old over */
		m->innermatrix[r->x][r->y]=r->over;
		/* Set the new over */
		r->over=m->innermatrix[newx][newy];

		/* Move the robby */
		m->innermatrix[newx][newy]=r;
		r->x = newx;
		r->y = newy;

		success=1;
	}
	return success;
}
