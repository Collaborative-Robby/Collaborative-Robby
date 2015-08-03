#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <robby/struct.h>

/* callbacks functions */
void (*move_callback)(struct map *, struct robby *);

void *callbacks = NULL;

char __can_const;

#define CAN_DUMMY_PTR &__can_const

int map_constructor(struct map *m, long unsigned int x, long unsigned int y,
                   long unsigned int robbynum,
                   long unsigned int cannum)
{
	int i;

	m->innermatrix = calloc(x, sizeof(void **));

	if (!m->innermatrix)
		return -1;

	m->rl = calloc(robbynum, sizeof(struct robby));
	if (!m->rl)
		return -1;

	for (i = 0; i < x; i++) {
		m->innermatrix[i] = calloc(y, sizeof(void *));
		if (!m->innermatrix[i])
			return -1;
		memset(m->innermatrix[i], 0, y * sizeof(void *));
	}

	m->sizex = x;
	m->sizey = y;

	m->n_robots = 0;
	m->n_cans = cannum;

	for (i = 0; i < cannum; i++) {
		long unsigned int nx, ny;
		
        do {
			nx = (long unsigned int) random() % x;
			ny = (long unsigned int) random() % y;
		} while (m->innermatrix[nx][ny]);

		m->innermatrix[nx][ny] = CAN_DUMMY_PTR;
	}

	return 0;
}

void map_destructor(struct map *m)
{
	int i;
	if (!m)
		return;
	for (i = 0; i < m->sizey; i++)
		free(m->innermatrix[i]);
	free(m->innermatrix);

    if (callbacks)
	dlclose(callbacks);
}

#define print_dispatcher(m, i, j) ({\
	struct melement *e;\
	if (!m->innermatrix[i][j])\
		printf("  ");\
	else{\
		e = (struct melement *) m->innermatrix[i][j];\
        if (e==CAN_DUMMY_PTR)\
            printf(" c");\
        else if (e->type == ROBBY)\
			printf("R%lu",((struct robby *)e)->id);\
	}\
})

static inline void print_map(struct map *m)
{
	int i, j;
	if (!m || !m->innermatrix)
		return;

	for (i = 0; i < m->sizex; i++) {
		printf("[ ");
		for (j = 0; j < m->sizey; j++) {
			printf(" ");
			print_dispatcher(m, j, i);
			printf(" ");
		}
		printf(" ]\n");
	}
}

struct robby *robby_constructor(struct map *m)
{
	struct robby *r = malloc(sizeof(struct robby));
	if (!r)
		return r;

	r->type = ROBBY;
	r->id = m->n_robots++;

	do {
		r->x = (long unsigned int) random() % m->sizex;
		r->y = (long unsigned int) random() % m->sizey;
	} while (m->innermatrix[r->x][r->y] && !(m->innermatrix[r->x][r->y]==CAN_DUMMY_PTR));

    if(m->innermatrix[r->x][r->y]) {
        r->over=m->innermatrix[r->x][r->y];
    }
	m->innermatrix[r->x][r->y] = r;
	m->rl[m->n_robots - 1] = r;

	r->move = move_callback;

	return r;
}

/* TODO windows version */
#define RANDOM_SEED() ({\
	unsigned int seed;\
	int sf = open("/dev/urandom", O_RDONLY);\
	if (read(sf, &seed, sizeof(seed)) < 0)\
		fprintf(stderr, "error in reading random seed");\
	srand(seed);\
})

#define print_status(m, r) ({\
	printf("--> %lu %lu | round %lu: %lu robby %lu cans\n",\
               m.sizex, m.sizey, r, m.n_robots, m.n_cans);\
})

#define MOVE_ALL_ROBBIES(m) ({\
	int i;\
	for (i=0; i < m.n_robots; i++) {\
		m.rl[i]->move(&m, m.rl[i]);\
	}\
})

void load_plugin(char *path)
{
	callbacks = dlopen(path, RTLD_LAZY);
	if (!callbacks) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}
	move_callback = dlsym(callbacks, "move");
	if (!move_callback) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	long unsigned int sizex, sizey, robbynum, cannum, totalrounds, round;
	struct map m;
	int i;

	if (argc < 3) {
		return EXIT_FAILURE;
	}

	RANDOM_SEED();

	/* Usage / getargs? */
	load_plugin(argv[1]);

	sizex = strtoul(argv[2], NULL, 10);
	sizey = strtoul(argv[3], NULL, 10);
	robbynum = strtoul(argv[4], NULL, 10);

	cannum = strtoul(argv[5], NULL, 10);
	totalrounds = strtoul(argv[6], NULL, 10);

	round = 0;

	if (map_constructor(&m, sizex, sizey, robbynum, cannum) != 0) {
		perror("map construction");
		return EXIT_FAILURE;
	}

	for (i = 0; i < robbynum; i++) {
		robby_constructor(&m);
	}

	for (round = 0; round < totalrounds; round++) {
		print_status(m, round);
		print_map(&m);
		MOVE_ALL_ROBBIES(m);
	}

	map_destructor(&m);

}
