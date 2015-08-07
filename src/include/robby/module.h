#ifndef _ROBBY_MODULE_H
#define _ROBBY_MODULE_H

extern char __can_const;

#define CAN_DUMMY_PTR (void *)&__can_const

int left[] = {-1,0};
int right[] = {1,0};
int up[] = {0,-1};
int down[] = {0,1};

int* directions[]= {left, right, up, down };

#define RIGHT directions[0]
#define LEFT directions[3]
#define UP directions[1]
#define DOWN directions[2]

#define PRINT_MOVE_INFO(move, id) ({\
	switch(dirnum) {\
		case 0:\
			printf("robby %d: left\n", id);\
			break;\
		case 1:\
			printf("robby %d: right\n", id);\
			break;\
		case 2:\
			printf("robby %d: up\n", id);\
			break;\
		case 3:\
			printf("robby %d: down\n", id);\
			break;\
		case -1:\
			printf("robby %d: picked up a can\n", id);\
			break;\
		default:\
			printf("Move unknown to the print engine\n");\
	}\
})

#endif
