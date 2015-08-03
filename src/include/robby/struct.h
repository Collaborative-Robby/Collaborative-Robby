#ifndef _ROBBY_STRUCT
#define _ROBBY_STRUCT

enum types {
	ROBBY,
};

struct map {
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
	long unsigned int id;
	long unsigned int x, y;
	void *over;
	void (*move)(struct map *, struct robby *);

	int gathered_cans;
	int last_gathered_can_time;

	int viewradius;
	char *view;

	int learning_algorithm;

	char **known_map;

/*	struct neural_net *nnet;*/
	char *dna;
};

#endif
