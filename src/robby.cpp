#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <robby/struct.h>
#include <robby/dismath.h>
#include <robby/commons.h>

/* callbacks functions */
int (*move_callback)(struct world_map *, struct robby *);
void (*generate_robbies_callback)(struct robby **, long unsigned int, long unsigned int, long unsigned int);
int (*update_view_callback) (struct robby *, struct world_map *, int);
void (*plugin_cleanup_callback) (struct robby **, unsigned long int, unsigned long int);
int (*update_view_and_send_callback) (struct world_map *, struct robby *, long unsigned int);

void *callbacks = NULL;

char __can_const;

#define CAN_DUMMY_PTR (void *)&__can_const

int map_constructor(struct world_map *m, unsigned long int x, unsigned long int y,
        unsigned long int robbynum,
        unsigned long int cannum)
{
    unsigned long int i;

    m->innermatrix = (void ***) calloc(x, sizeof(void **));

    if (!m->innermatrix)
        return -1;

    m->rl = (struct robby **) calloc(robbynum, sizeof(struct robby *));
    if (!m->rl)
        return -1;

    for (i = 0; i < x; i++) {
        m->innermatrix[i] = (void **)calloc(y, sizeof(void *));
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
            nx = (long unsigned int) round(RANDOM_DOUBLE(x-1));
            ny = (long unsigned int) round(RANDOM_DOUBLE(y-1));
        } while (m->innermatrix[nx][ny]);

        m->innermatrix[nx][ny] = CAN_DUMMY_PTR;
    }

    return 0;
}

void map_copy(struct world_map *src, struct world_map *dst, unsigned long int robbynum)
{
    unsigned long int i, j;
    memcpy(dst, src, sizeof(*src));

    dst->innermatrix = (void ***) calloc(src->sizex, sizeof(void **));

    if (!dst->innermatrix)
        return;

    dst->rl = (struct robby **) calloc(robbynum, sizeof(struct robby *));
    if (!dst->rl)
        return;

    for (i = 0; i < src->sizex; i++) {
        dst->innermatrix[i] = (void **)calloc(src->sizey, sizeof(void *));
        if (!dst->innermatrix[i])
            return;
        memset(dst->innermatrix[i], 0, src->sizey * sizeof(void *));
    }

    /* deep copy */

    for (i = 0; i < src->sizex; i++)
        for (j = 0; j < src->sizey; j++)
            dst->innermatrix[i][j] = src->innermatrix[i][j];
}

void map_destructor(struct world_map *m)
{
    unsigned long int i;
    if (!m)
        return;
    if (m->rl)
        free(m->rl);
    if (!m->innermatrix)
        return;
    for (i = 0; i < m->sizex; i++)
        free(m->innermatrix[i]);
    free(m->innermatrix);

}

#define print_dispatcher(m, i, j) ({\
        struct melement *e;\
        if (!m->innermatrix[j][i])\
        printf("  ");\
        else{\
        e = (struct melement *) m->innermatrix[j][i];\
        if (e==CAN_DUMMY_PTR)\
        printf(" c");\
        else if (e->type == ROBBY)\
        printf("R%d",((struct robby *)e)->id);\
        }\
        })

static inline void print_map(struct world_map *m)
{
    unsigned long int i, j;
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

struct robby *add_robby(struct world_map *m, struct robby *r)
{
    if (!r)
        return r;

    r->type = ROBBY;

    r->over=m->innermatrix[r->x][r->y];

    m->n_robots++;

    m->innermatrix[r->x][r->y] = r;
    m->rl[m->n_robots - 1] = r;

    r->move = move_callback;

    update_view_callback(r, m, false);

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

#define MOVE_ALL_ROBBIES(m,train) ({\
	unsigned long int i;\
	for (i=0; i < m.n_robots; i++) {\
		if (!m.rl[i]->moved) m.rl[i]->move(&m, m.rl[i]);\
	}\
	for (i=0; i < m.n_robots; i++) {\
		m.rl[i]->moved = 0;\
	}\
})

void load_function(void **f, void *callback, const char *name)
{
    *f = dlsym(callback, name);

    if (!*f) {
        fprintf(stderr, "%s loading %s\n", dlerror(), name);
    }
}

void load_plugin(char *path)
{
    callbacks = dlopen(path, RTLD_LAZY);
    if (!callbacks) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    load_function((void **) &move_callback, callbacks, "move");
    load_function((void **) &generate_robbies_callback, callbacks, "generate_robbies");
    load_function((void **) &update_view_callback, callbacks, "update_view");
    load_function((void **) &plugin_cleanup_callback, callbacks, "cleanup");
    load_function((void **) &update_view_and_send_callback, callbacks, "update_view_and_send");

    if (!generate_robbies_callback || !move_callback || !update_view_callback ||
	    !update_view_and_send_callback) {
        exit(EXIT_FAILURE);
    }
}

/* TODO change this thing, this is only for test... */
double eval(struct robby *r, long unsigned int totalcans)
{
    if (!r)
        return -1;

    r->fitness = ((double)r->gathered_cans / (double)totalcans);

    return r->fitness;
}

double eval_couple(struct robby *r, long unsigned int robbynum, long unsigned
int totalcans, long unsigned int roundnum, long unsigned int map_num)
{
    unsigned long int i;
    double sum = 0;
    for (i=0; i < robbynum; i++)
        sum += (((double) r[i].gathered_cans / (double) (totalcans*map_num)));
               //((double) (roundnum-r[i].failed_moves)/(double) (roundnum)));
    


    //for (i=0; i < robbynum; i++)
    //    sum+= (double)r[i].gathered_cans/(double)r[i].num_moves;
    //printf("total cans %lu, number rounds: %lu\n", totalcans, roundnum);

    /*if(r[0].failed_moves>20)
        r[0].fitness=0;
    else*/
    r[0].fitness = (sum);

    return r[0].fitness;
}

void destroy_robbies(struct robby **rl, long unsigned int couplenum, long unsigned int robbynum)
{
    long unsigned int i,j,k;

    if (plugin_cleanup_callback) {
        plugin_cleanup_callback(rl, couplenum, robbynum);
    }

    if (!rl)
        return;

    /* View free */
    for (k = 0; k < couplenum; k++) {
        if (!rl[k])
            continue;

        for (i = 0; i < robbynum; i++) {
            if (!rl[k][i].view)
                continue;

            for (j = 0; j < 2 * (rl[k][i].viewradius - 1) + 1; j++)
                if (rl[k][i].view[j])
                    free(rl[k][i].view[j]);

            free(rl[k][i].view);
        }
        free(rl[k]);
    }

    free(rl);

    //if (callbacks)
    //    dlclose(callbacks);
}

int compare_eval(const void *a, const void *b)
{
    double fitnessa;
    double fitnessb;

    if (b)
        fitnessb = ((struct robby **)b)[0]->fitness;
    if (a)
        fitnessa = ((struct robby **)a)[0]->fitness;

    /* return 1 if the first is the major, -1 if the second is the major */
    return (fitnessa < fitnessb) * 2 - 1;
}

#define sort_by_best_eval(rl, length) (qsort(rl, length, sizeof(struct robby **), compare_eval))

#define print_in_generation_header(g) printf("===> Generation %lu\n", g)
#define print_end_generation_header(g, r, rnum)\
    printf("===> End of Generation %lu Best fitness: %f\n",g,(rnum>0 ? r[0].fitness:0)); \
    for(long unsigned int i=0; i<rnum; i++) printf("failed: %d gathered: %d\n", r[i].failed_moves, r[i].gathered_cans);

#define print_test_generation_header(g) printf("===> Test Generation\n")
#define print_end_test_generation_header(r, rnum)\
    printf("===> End of Test Generation fitness: %f\n", (rnum>0 ? r[0].fitness:0)); \
    for(long unsigned int i=0; i<rnum; i++) printf("failed: %d gathered: %d\n", r[i].failed_moves, r[i].gathered_cans);


void choose_position(struct world_map *m, struct robby **rl,
        long unsigned int current_couple,
        long unsigned int robby_num)
{
    unsigned long int i;

    for (i = 0; i < robby_num; i++) {
        rl[0][i].x = rl[0][i].original_x;
        rl[0][i].y = rl[0][i].original_y;
    }

    for (i = 0; i < robby_num; i++) {
        if (current_couple == 0 && (rl[0][i].x==m->sizex|| rl[0][i].y==m->sizey)) {
            do {
                rl[0][i].x = (long unsigned int) round(RANDOM_DOUBLE(m->sizex - 1));
                rl[0][i].y = (long unsigned int) round(RANDOM_DOUBLE(m->sizey - 1));

                rl[0][i].original_x = rl[0][i].x;
                rl[0][i].original_y = rl[0][i].y;
            } while (m->innermatrix[rl[0][i].x][rl[0][i].y] && !(m->innermatrix[rl[0][i].x][rl[0][i].y]==CAN_DUMMY_PTR));
        } else {
            rl[current_couple][i].x = rl[0][i].x;
            rl[current_couple][i].y = rl[0][i].y;
            rl[current_couple][i].original_x = rl[0][i].original_x;
            rl[current_couple][i].original_y = rl[0][i].original_y;
        }
    }
    
}

int map_fetch_from_file(struct world_map *m, char* filename, long unsigned int robbynum) {
    FILE* mfile;
    int sizex, sizey,x,y,ret,cval;

    fprintf(stderr, "Fetching map %s\n", filename);

    mfile=fopen(filename, "r");
    if(!mfile) {
        return -1;
    }
    ret=fscanf(mfile, "%d %d\n", &sizex, &sizey);
    if(ret!=2) {
        return -1;
    }

    ret=map_constructor(m, sizex, sizey, robbynum,0);
    if(ret<0) {
        return -1;
    }

    for(x=0;x<sizex;x++) {
        fscanf(mfile,"[");
        for(y=0;y<sizey-1; y++) {
            ret=fscanf(mfile,"%d,", &cval);
            if(ret && cval) {
                m->innermatrix[x][y]=CAN_DUMMY_PTR;
                m->n_cans++;
            }
        }       
        ret=fscanf(mfile, "%d]\n", &cval);
        if(ret && cval) {
            m->innermatrix[x][y]=CAN_DUMMY_PTR;
            m->n_cans++;
        }
    }
    fclose(mfile);

    return 0;
}

int map_fetch_from_int(struct world_map *m, unsigned long int current_num, char *dir, long unsigned int robbynum) {
    char *filename;
    int ret;
    asprintf(&filename, "%s/%lu", dir, current_num);
    ret=map_fetch_from_file(m,filename,robbynum);
    free(filename);
    return ret;
}

int generational_step(long unsigned int sizex, long unsigned int sizey,
        long unsigned int robbynum, long unsigned int cannum,
        long unsigned int totalrounds, long unsigned int couple_num,
        struct robby **rl,
        char *map_dir,
        long unsigned int map_set_size,
        bool training, bool verbose)
{
	long unsigned int round;
	unsigned long int i, current_pool;
	struct world_map morig, m;
	unsigned long int current_map=0;
	round = 0;

	//TODO pensa al test
	for(current_map=0; current_map < map_set_size; current_map++) {

		/* Get the training map */
		if(map_fetch_from_int(&morig, current_map, map_dir, robbynum)) {
			perror("map fetch");
			return EXIT_FAILURE;
		}
		//} else if (map_constructor(&morig, sizex, sizey, robbynum, cannum) != 0) {
		//[> Create a new random map <]
		//perror("map construction");
		//return EXIT_FAILURE;

	for (current_pool = 0; current_pool < couple_num; current_pool++) {
		map_copy(&morig, &m, robbynum);

		choose_position(&m, rl, current_pool, robbynum);

		for (i = 0; i < robbynum; i++) 
			add_robby(&m, &rl[current_pool][i]);

		for (round = 0; round < totalrounds; round++) {
			if(verbose) {
				print_status(m, round);
				print_map(&m);
			}
			update_view_and_send_callback(&m, rl[current_pool], robbynum);
			MOVE_ALL_ROBBIES(m,training);
		}
		/* last turn print */
		if(verbose) {
			print_status(m, round);
			print_map(&m);
		}
		map_destructor(&m);
	}

	map_destructor(&morig);
}
for(current_pool=0; current_pool<couple_num; current_pool++)
eval_couple(rl[current_pool], robbynum, cannum, totalrounds,
		map_set_size);

return 0;
}

void zero_fitness(struct robby *rl, long unsigned int robbynum)
{
    long unsigned int i;
    for (i = 0; i < robbynum; i++) {
        rl[i].gathered_cans = 0;
        rl[i].failed_moves=0;
        rl[i].last_gathered_can_time = 0;
        rl[i].fitness = 0.0;
        rl[i].num_moves=0;
        rl[i].old_move=NOP_MOVE;
    }
}

void write_map(struct world_map *m, char *filename, char *dir) {
    char* path;
    FILE* f;
    unsigned long int x,y;
    asprintf(&path, "%s/%s", dir, filename);
    f=fopen(path, "w");
    fprintf(f, "%lu %lu\n", m->sizex, m->sizey);
    for(x=0; x<m->sizex; x++) {
        fprintf(f,"[");
        for(y=0; y<m->sizey; y++) {
            fprintf(f, "%d%s",(m->innermatrix[x][y]==CAN_DUMMY_PTR), (y==m->sizey-1 ?  "]\n" : ","));
        }
    }
    fclose(f);
    free(path);
}

int count_files(char *dir) {
    int count=0;
    DIR* d;
    struct dirent *dent;

    if ((d = opendir(dir)) != NULL) {
        while ((dent = readdir (d)) != NULL) {
            if(strncmp(dent->d_name, ".",1) && strncmp(dent->d_name, "..", 2))
                count++;
        }
        closedir(d);
    } else {
        perror("count files");
        return -1;
    }
    return count;
}

unsigned long int generate_maps(unsigned long int nmaps,char* dir, long unsigned int sizex, long unsigned int sizey,long unsigned int cannum) {
    int ret;
    unsigned long int fcount,i;
    struct world_map m;
    char *map_file_name;
    errno=0;

    if(!dir)
        return -1;

    printf("generating %lu maps in %s\n", nmaps, dir);

    ret=mkdir(dir, 0755);
    if(ret<0 && errno==EEXIST) {
        printf("directory %s exists\n", dir);
        fcount=count_files(dir);
        if(fcount<nmaps) {
            fprintf(stderr, "not enough maps in %s (%lu of %lu)\n", dir, fcount, nmaps);
            return 0;
        }
        return fcount;
    }    
    else if(ret<0) {
        perror("dir: ");
        return 0;
    }
    for(i=0;i<nmaps;i++) {
        map_constructor(&m,sizex,sizey, 0, cannum);
        asprintf(&map_file_name, "%lu", i);
        write_map(&m, map_file_name , dir);
        free(map_file_name);
        map_destructor(&m);
    }

    return nmaps;
}

#define USAGE "<plugin> [-h| -x <sizex>| -y <size y> | -r <#ofrobbies> | -c <#ofcans> | -R <# of rounds> | -g <# of generations> | -m <# of test_maps[,# of training maps]> | -d <dir for test maps[,dir for training maps]> ]"
int main(int argc, char **argv)
{
	long unsigned int sizex, sizey, robbynum, cannum, totalrounds,
	     totalgenerations, generation, training_map_num, test_map_num,couplenum,i,j;
	struct robby **rl;
	int opt;
	int nargs;
	char *test_dir,*train_dir, *tmp;
	bool verbose;

	sizex = 10;
	sizey = 10;
	robbynum = 1;
	cannum = 1;
	couplenum = 1;
	totalrounds = 10;
	totalgenerations = 10;
	training_map_num=10;
	test_map_num=10;
	test_dir=train_dir=tmp=NULL;
	verbose=false;

	if (argc < 2) {
		fprintf(stderr, "%s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	RANDOM_SEED();
	load_plugin(argv[1]);

	while ((opt = getopt(argc - 1, argv + 1, "hx:y:r:c:R:g:d:m:C:v")) != -1) {
		switch (opt) {
			case 'x':
				sizex = strtoul(optarg, NULL, 10);
				break;
			case 'y':
				sizey = strtoul(optarg, NULL, 10);
				break;
			case 'r':
				robbynum = strtoul(optarg, NULL, 10);
				break;
			case 'C':
				couplenum = strtoul(optarg, NULL, 10);
				break;
			case 'R':
				totalrounds = strtoul(optarg, NULL, 10);
				break;
			case 'c':
				cannum = strtoul(optarg, NULL, 10);
				break;
			case 'g':
				totalgenerations = strtoul(optarg, NULL, 10);
				break;
			case 'm':
				nargs=sscanf(optarg, "%lu,%lu",&test_map_num, &training_map_num);
				if(nargs==1)
					training_map_num=0;
				break;
			case 'd':
				asprintf(&test_dir, "%s", strtok(optarg, ","));
				tmp=strtok(NULL, ",");
				if(tmp)
					asprintf(&train_dir, "%s", tmp);
				break;
			case 'v':
				verbose=true;
				break;
			case 'h':
			default:
				fprintf(stderr, "usage: %s %s\n", argv[0], USAGE);
				exit(0);
		}
	}

	if(!train_dir) {
		generate_maps(training_map_num, TRAINING_DEFAULT_DIR,sizex, sizey, cannum);
		asprintf(&train_dir, TRAINING_DEFAULT_DIR);
	}
	if(!test_dir) {
		generate_maps(test_map_num, TEST_DEFAULT_DIR, sizex, sizey, cannum);
		asprintf(&test_dir, TEST_DEFAULT_DIR);
	}

	rl = (struct robby **)calloc(couplenum, sizeof(struct robby *));
	if (rl==NULL){
		fprintf(stderr, "robby list malloc %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < couplenum; i++) {
		rl[i] = (struct robby *)calloc(robbynum, sizeof(struct robby));
		if (rl[i]==NULL){
			fprintf(stderr, "robby list malloc %s on step %lu\n", strerror(errno), i);
			exit(EXIT_FAILURE);
		}
	}

	generation = 0;
	/* Random placing */
	for (i = 0; i < robbynum; i++) {
		for (j = 0; j < couplenum; j++ ) {
			rl[j][i].original_x = sizex;
			rl[j][i].original_y = sizey;

            rl[j][i].m_sizex=sizex;
            rl[j][i].m_sizey=sizey;
		}
	}

	for (generation = 0; generation < totalgenerations; generation++) {
		print_in_generation_header(generation);

		generate_robbies_callback(rl, couplenum, robbynum, generation);

		for (i = 0; i < couplenum; i++)
			zero_fitness(rl[i], robbynum);

		generational_step(sizex, sizey, robbynum, cannum, totalrounds,
				couplenum, rl, train_dir, training_map_num,
				true, verbose);

		sort_by_best_eval(rl, couplenum);

		print_end_generation_header(generation, rl[0], robbynum);
	}

	/* Test generation */
	print_test_generation_header();
	for (i = 0; i < couplenum; i++)
		zero_fitness(rl[0], robbynum);

	generational_step(sizex, sizey, robbynum, cannum, totalrounds,
			couplenum, rl, test_dir, test_map_num,
			false, verbose);

	print_end_test_generation_header(rl[0], robbynum);

	destroy_robbies(rl, couplenum, robbynum);
	free(test_dir);
	free(train_dir);
}
