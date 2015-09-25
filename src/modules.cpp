#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <robby/struct.h>
#include <robby/module.h>


#define VIEW_TOO_FAR -1
#define VIEW_WALL 0
#define VIEW_ROBBY 1
#define VIEW_EMPTY 2
#define VIEW_CAN 3

unsigned long int get_dis_circle_area(unsigned long int viewradius) {
    unsigned long int i;
    unsigned long int retval;
    retval = 0;

    for(i=1; i<=viewradius; i++)
        retval += DISSIN(i,viewradius) - 1;

    return retval * 4 + 1;
}

int get_circle_view (unsigned long int viewradius, char **view) {
    unsigned long int i,j;
    unsigned long int x,y; 
    for(i=1; i<=viewradius; i++) {
        for(j=0; j<DISSIN(i,viewradius); j++) {
            
            x=viewradius+viewradius-i-1;
            y=viewradius-j-1;
            view[x][y]=VIEW_EMPTY;

            x=viewradius+viewradius-i-1;
            y=viewradius+j-1;
            view[x][y]=VIEW_EMPTY;

            x=i-1;
            y=viewradius+j-1;
            view[x][y]=VIEW_EMPTY;

            x=i-1;
            y=viewradius-j-1;
            view[x][y]=VIEW_EMPTY;
        }
    }

    return 0;
}

int get_view(struct robby *r, struct world_map *m, int wraparound) {
    long unsigned int i,j;
    long unsigned int x,y;
    for(i=0; i<SQUARE_SIDE;i++) {
        for(j=0; j<SQUARE_SIDE;j++) {
            x=r->x+(i-(r->viewradius-1)); 
            y=r->y+(j-(r->viewradius-1));
            if(r->view[i][j]==VIEW_TOO_FAR)
                continue;
            
            if(wraparound) {
            //TODO wraparound
            }
            if(x>=m->sizex || y>=m->sizey)
                r->view[i][j]=VIEW_WALL;
            else if(i==r->viewradius-1 && j==r->viewradius-1) {
                if(r->over==CAN_DUMMY_PTR)
                    r->view[i][j]=VIEW_CAN;
                else
                    r->view[i][j]=VIEW_EMPTY;
            }
            else if(m->innermatrix[x][y]==CAN_DUMMY_PTR)
                r->view[i][j]=VIEW_CAN;
            else if(m->innermatrix[x][y]==NULL)
                r->view[i][j]=VIEW_EMPTY;
            else
                r->view[i][j]=VIEW_ROBBY;

        }
    }

    return 0;
}

int update_view(struct robby *r, struct world_map *m, int wraparound)
{
	long unsigned int i;
	if (!r->view) {
		r->view = (char **)calloc(SQUARE_SIDE, sizeof(char*));
		if (!r->view) {
			fprintf(stderr, "robby %d error on malloc of view\n", r->id);
			return -1;
		}
        for(i=0; i<2*(r->viewradius-1)+1;i++) {
            r->view[i] = (char*) calloc(SQUARE_SIDE, sizeof(char));
            if (!r->view[i]) {
			    fprintf(stderr, "robby %d error on malloc of view\n", r->id);
			    return -1;
		    }
            memset(r->view[i], VIEW_TOO_FAR,SQUARE_SIDE);
        }

        get_circle_view(r->viewradius, r->view);
	}

	get_view(r, m, wraparound);
    
    #ifdef DEBUG_VIEW
    unsigned long int j;
    printf("robby %d:\n", r->id);
    
    for(i=0; i<SQUARE_SIDE;i++) {
        printf("{");
        for(j=0; j<SQUARE_SIDE;j++) {
            printf(" %d ", r->view[i][j]);
        }
        printf("}\n");
    }
    #endif 
	return 0;
}

/* Move in a direction */
int move_dir(struct robby *r, struct world_map *m, int dirnum,  int impact,  int wraparound)
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
		newx = (long unsigned int)((int)r->x+dir[0]);
		newy = (long unsigned int)((int)r->y+dir[1]);
		oob = OUT_OF_BOUNDS(newx, newy, m);
	}

	if (!oob && !impact && OVER_ROBBY(m, newx, newy)) {
		/* Give control to the robby that is over the destination cell */
		new_over = (struct robby *)m->innermatrix[newx][newy];
		printf("robby %d: impact avoidance started giving control to robby %d\n",
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
