#include <stdio.h>
#include <stdlib.h>
#include <robby/struct.h>
#include <robby/module.h>

void move(struct map *m, struct robby *r) {
    int i,dirnum;
    int* dir;


    m->innermatrix[r->x][r->y]=r->over;
   
    dirnum=rand()%4; 
    dir=directions[dirnum];

    switch(dirnum) {
	case 0:
		printf("robby %d: left\n", r->id);
		break;
	case 1:
		printf("robby %d: right\n", r->id);
		break;
	case 2:
		printf("robby %d: up\n", r->id);
		break;
	case 3:
		printf("robby %d: down\n", r->id);
		break;
    }

    r->x=(r->x+dir[0])%(m->sizex);
    r->y=(r->y+dir[1])%(m->sizey);
    
    r->over=m->innermatrix[r->x][r->y];
    m->innermatrix[r->x][r->y]=r;
}
