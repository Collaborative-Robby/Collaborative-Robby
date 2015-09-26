#ifndef _ROBBY_STRUCT
#define _ROBBY_STRUCT

#include<robby/neural-net.h>

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
	char moved;
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

	char **known_map;

	class Genome *genome;
    char *dna;

    long unsigned int clock;
};

#endif
