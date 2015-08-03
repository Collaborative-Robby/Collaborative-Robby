#ifndef _ROBBY_MODULE_H
#define _ROBBY_MODULE_H

extern char __can_const;

int left[] = {-1,0};
int right[] = {1,0};
int up[] = {0,-1};
int down[] = {0,1};

int* directions[]= {left, right, up, down };

#define RIGHT directions[0]
#define LEFT directions[3]
#define UP directions[1]
#define DOWN directions[2]

#endif
