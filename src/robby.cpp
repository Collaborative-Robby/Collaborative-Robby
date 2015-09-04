#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <robby/struct.h>
#include <robby/dismath.h>
#include <robby/commons.h>

/* callbacks functions */
int (*move_callback)(struct world_map *, struct robby *);
void (*generate_robbies_callback)(struct robby *, long unsigned int, long unsigned int);
int (*update_view_callback) (struct robby *, struct world_map *, int);
void (*plugin_cleanup_callback) (struct robby *, int rnum);

void *callbacks = NULL;

char __can_const;

#define CAN_DUMMY_PTR (void *)&__can_const

int map_constructor(struct world_map *m, long unsigned int x, long unsigned int y,
        long unsigned int robbynum,
        long unsigned int cannum)
{
    int i;

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
            nx = (long unsigned int) random() % x;
            ny = (long unsigned int) random() % y;
        } while (m->innermatrix[nx][ny]);

        m->innermatrix[nx][ny] = CAN_DUMMY_PTR;
    }

    return 0;
}

void map_destructor(struct world_map *m)
{
    int i;
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

struct robby *add_robby(struct world_map *m, struct robby *r)
{
    if (!r)
        return r;

    r->type = ROBBY;

    /* TODO fixed position */
    do {
        r->x = (long unsigned int) RANDOM_DOUBLE(m->sizex);
        r->y = (long unsigned int) RANDOM_DOUBLE(m->sizey);
    } while (m->innermatrix[r->x][r->y] && !(m->innermatrix[r->x][r->y]==CAN_DUMMY_PTR));

    if(m->innermatrix[r->x][r->y]) {
        r->over=m->innermatrix[r->x][r->y];
    }

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

#define MOVE_ALL_ROBBIES(m) ({\
        int i;\
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

    if (!generate_robbies_callback || !move_callback || !update_view_callback) {
        exit(EXIT_FAILURE);
    }
}

/* TODO change this thing, this is only for test... */
float eval(struct robby *r, long unsigned int totalcans)
{
    if (!r)
        return -1;

    r->fitness = ((float)r->gathered_cans / (float)totalcans);

    return r->fitness;
}

void destroy_robbies(struct robby *rl, int robbynum)
{
    int i,j;

    if (plugin_cleanup_callback) {
        plugin_cleanup_callback(rl, robbynum);
    }

    if (!rl)
        return;

    /* View free */
    for (i = 0; i < robbynum; i++) {
        if (!rl[i].view)
            continue;

        for (j = 0; j < 2 * (rl[i].viewradius - 1) + 1; j++)
            if (rl[i].view[j])
                free(rl[i].view[j]);

        free(rl[i].view);
    }

    free(rl);

    if (callbacks)
        dlclose(callbacks);
}

int compare_eval(const void *a, const void *b)
{
    float fitnessa;
    float fitnessb;

    if (b)
        fitnessb = ((struct robby *)b)->fitness;
    if (a)
        fitnessa = ((struct robby *)a)->fitness;

    /* return 1 if the first is the major, -1 if the second is the major */
    return (fitnessa > fitnessb) * 2 - 1;
}


#define sort_by_best_eval(rl, length) qsort(rl, length, sizeof(struct robby), compare_eval);

#define print_in_generation_header(g) printf("===> Generation %lu\n", g)
#define print_end_generation_header(g, rl, rnum)\
    printf("===> End of Generation %lu Best fitness: %f\n", g,\
            (rnum > 0 ? rl[0].fitness : 0))

int generational_step(long unsigned int sizex, long unsigned int sizey,
        long unsigned int robbynum, long unsigned int cannum,
        long unsigned int totalrounds,
        struct robby *rl)
{
    long unsigned int round;
    int i;
    struct world_map m;

    round = 0;

    if (map_constructor(&m, sizex, sizey, robbynum, cannum) != 0) {
        perror("map construction");
        return EXIT_FAILURE;
    }

    for (i = 0; i < robbynum; i++)
        add_robby(&m, &rl[i]);

    for (round = 0; round < totalrounds; round++) {
        print_status(m, round);
        print_map(&m);
        MOVE_ALL_ROBBIES(m);
    }

    /* last turn print */
    print_status(m, round);
    print_map(&m);

    for (i = 0; i < robbynum; i++)
        eval(&rl[i], totalrounds);

    map_destructor(&m);
}

void zero_fitness(struct robby *rl, long unsigned int robbynum)
{
    long unsigned int i;
    for (i = 0; i < robbynum; i++) {
        rl[i].gathered_cans = 0;
        rl[i].last_gathered_can_time = 0;
        rl[i].fitness = 0.0;
    }
}

void write_map(struct world_map *m, char *filename, char *dir) {
    char* path;
    FILE* f;
    int x,y;
    asprintf(&path, "%s/%s", dir, filename);
    f=fopen(path, "w");
    fprintf(f, "%d %d\n", m->sizex, m->sizey);
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

int generate_maps(int nmaps,char* dir, long unsigned int sizex, long unsigned int sizey,long unsigned int cannum) {
    int ret,fcount,i;
    struct world_map m;
    char *map_file_name;
    errno=0;

    if(!dir)
        return -1;

    printf("generating %d maps in %s\n", nmaps, dir);

    ret=mkdir(dir, 0755);
    if(ret<0 && errno==EEXIST) {
        printf("directory %s exists\n", dir);
        fcount=count_files(dir);
        if(fcount<nmaps) {
            fprintf(stderr, "not enough maps in %s (%d of %d)\n", dir, fcount, nmaps);
            return -1;
        }
        return fcount;
    }    
    else if(ret<0) {
        perror("dir: ");
        return -1;
    }
    for(i=0;i<nmaps;i++) {
        map_constructor(&m,sizex,sizey, 0, cannum);
        asprintf(&map_file_name, "%d", i);
        write_map(&m, map_file_name , dir);
        free(map_file_name);
        map_destructor(&m);
    }

}

#define USAGE "<plugin> [-h| -x <sizex>| -y <size y> | -r <#ofrobbies> | -c <#ofcans> | -R <# of rounds> | -g <# of generations> | -m <# of test_maps[,# of training maps]> | -d <dir for test maps[,dir for training maps]> ]"
int main(int argc, char **argv)
{
    long unsigned int sizex, sizey, robbynum, cannum, totalrounds,
         totalgenerations, generation, training_map_num, test_map_num;
    struct robby *rl;
    int opt;
    int i,nargs;
    char *test_dir,*train_dir, *tmp;

    sizex = 10;
    sizey = 10;
    robbynum = 1;
    cannum = 1;
    totalrounds = 10;
    totalgenerations = 10;
    training_map_num=10;
    test_map_num=10;
    test_dir=train_dir=tmp=NULL;


    if (argc < 2) {
        fprintf(stderr, "%s %s\n", argv[0], USAGE);
        return EXIT_FAILURE;
    }

    RANDOM_SEED();
    load_plugin(argv[1]);

    while ((opt = getopt(argc - 1, argv + 1, "hx:y:r:c:R:g:d:m:")) != -1) {
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
            case 'h':
            default:
                fprintf(stderr, "usage: %s %s\n", argv[0], USAGE);
                exit(0);
        }
    }

    if(!test_dir) {
        generate_maps(test_map_num, TEST_DEFAULT_DIR, sizex, sizey, cannum);
        generate_maps(training_map_num, TRAINING_DEFAULT_DIR,sizex, sizey, cannum);
    }


    rl = (struct robby *)calloc(robbynum, sizeof(struct robby));

    generation = 0;
    for (generation = 0; generation < totalgenerations; generation++) {
        print_in_generation_header(generation);

        /* Random placing */
        for (i = 0; i < robbynum; i++) {
            rl[i].x = sizex;
            rl[i].y = sizey;
        }

        generate_robbies_callback(rl, robbynum, generation);

        zero_fitness(rl, robbynum);

        generational_step(sizex, sizey, robbynum, cannum, totalrounds, rl);

        sort_by_best_eval(rl, robbynum);

        print_end_generation_header(generation, rl, robbynum);
    }

    destroy_robbies(rl, robbynum);
}
