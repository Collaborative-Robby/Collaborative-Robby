#ifndef _ROBBY_STRUCT
#define _ROBBY_STRUCT

#include<robby/neural-net.h>

#define POSSIBLE_MOVES 5
#define NOP_MOVE (POSSIBLE_MOVES+1)

enum types {
	ROBBY,
};

struct world_map {
	long unsigned int sizex, sizey;
	long unsigned int n_robots;
	long unsigned int n_cans;
	void ***innermatrix;
	struct robby **rl;
};

/* Common structure to all elements */
/* Extendible, so we can declare beacons or some new elements */
struct melement {
	int type;
};

struct robby {
	int type;
	int id;
	long unsigned int x, y;
	long unsigned int original_x, original_y;
	char moved;
	int old_move;
	void *over;
	int (*move)(struct world_map *, struct robby *);

	int gathered_cans;
	int failed_moves;
	int last_gathered_can_time;
	long unsigned int num_moves;

	double fitness;

	unsigned long int viewradius;
	char **view;

	int learning_algorithm;
    
    long unsigned int m_sizex, m_sizey;
	char **known_map;

	class Genome *genome;
	char *dna;

	long unsigned int clock;
};

struct robby_msg {
	int id;
	char **view;
	int old_move;
    long unsigned int x,y;
};

#endif
