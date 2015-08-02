#include <stdio.h>
#include <stdlib.h>
#include "include/struct.h"
#include "include/module.h"

void move(struct map *m, struct robby *r) {
    int i,dirnum;
    int* dir;

    printf("diobove\n");

    m->innermatrix[r->x][r->y]=r->over;
   
    dirnum=rand()%4; 
    dir=directions[dirnum];

    r->x=(r->x+dir[0])%(m->sizex);
    r->y=(r->y+dir[1])%(m->sizey);
    
    r->over=m->innermatrix[r->x][r->y];
    m->innermatrix[r->x][r->y]=r;
}
