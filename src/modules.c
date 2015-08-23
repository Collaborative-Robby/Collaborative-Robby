#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <robby/struct.h>
#include <robby/module.h>

#define DISSIN(n, r) ((round((sin((M_PI / (2.0 * (float) r)) * (float) n) * r))))

#define SQUARE_SIZE(r) ((r-1)*2+1)

#define VIEW_TOO_FAR -1
#define VIEW_EMPTY 0
#define VIEW_CAN 1
#define VIEW_ROBBY 2
#define VIEW_WALL 3 

int get_circle_view (int viewradius, char **view) {
    int i,j;
    int x,y; 
    for(i=1; i<=viewradius; i++) {
        for(j=0; j<DISSIN(i,viewradius); j++) {
            
            x=viewradius+viewradius-i-1;
            y=viewradius-j-1;
            printf("%d %d\n", x, y);
            view[x][y]=VIEW_EMPTY;

            x=viewradius+viewradius-i-1;
            y=viewradius+j-1;
            printf("%d %d\n", x, y);
            view[x][y]=VIEW_EMPTY;

            x=i-1;
            y=viewradius+j-1;
            printf("%d %d\n", x, y);
            view[x][y]=VIEW_EMPTY;

            x=i-1;
            y=viewradius-j-1;
            printf("%d %d\n", x, y);
            view[x][y]=VIEW_EMPTY;
        }
    }
}

int get_view(struct robby *r, struct map *m, int wraparound) {
    int i,j;
    int x,y;
    for(i=0; i<SQUARE_SIZE(r->viewradius);i++) {
        for(j=0; j<SQUARE_SIZE(r->viewradius);j++) {
            x=r->x+(i-(r->viewradius-1)); 
            y=r->y+(j-(r->viewradius-1));
            if(r->view[i][j]==VIEW_TOO_FAR)
                continue;
            
            //TODO wraparound
            if(x>=m->sizex || x<0 || y>=m->sizey || y<0)
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
}


int update_view(struct robby *r, struct map *m, int wraparound)
{
	int items,i,j;
	if (!r->view) {
		r->view = (char **)calloc(SQUARE_SIZE(r->viewradius), sizeof(char*));
        printf("%d\n", 2*(r->viewradius-1)+1);
		if (!r->view) {
			fprintf(stderr, "robby %d error on malloc of view\n", r->id);
			return -1;
		}
        for(i=0; i<2*(r->viewradius-1)+1;i++) {
            r->view[i] = (char*) calloc(SQUARE_SIZE(r->viewradius), sizeof(char));
            if (!r->view[i]) {
			    fprintf(stderr, "robby %d error on malloc of view\n", r->id);
			    return -1;
		    }
            memset(r->view[i], VIEW_TOO_FAR,SQUARE_SIZE(r->viewradius));
        }

        get_circle_view(r->viewradius, r->view);
	}

	items = get_view(r, m, wraparound);
    
    printf("%d\n", SQUARE_SIZE(r->viewradius));

    for(i=0; i<SQUARE_SIZE(r->viewradius);i++) {
        printf("{");
        for(j=0; j<SQUARE_SIZE(r->viewradius);j++) {
            printf(" %d ", r->view[i][j]);
        }
        printf("}\n");
    }

	return items;
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
		newx = (long unsigned int)((int)r->x+dir[0]);
		newy = (long unsigned int)((int)r->y+dir[1]);
		oob = OUT_OF_BOUNDS(newx, newy, m);
	}

	if (!oob && !impact && OVER_ROBBY(m, newx, newy)) {
		/* Give control to the robby that is over the destination cell */
		new_over = m->innermatrix[newx][newy];
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
