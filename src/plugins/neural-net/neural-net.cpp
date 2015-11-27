/* Sample module for collaborative robby */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <list>

#include <robby/struct.h>
#include <robby/module.h>
#include <robby/dismath.h>
#include <robby/neural-net.h>
#include <robby/neural-net-const.h>

#include <robby/neural-net-utils.h>

double pool_maxfitness = 0.0;

long unsigned int genome_count = 0;
long unsigned int global_innovation = 0;
unordered_map<GENE_KEY_TYPE, unsigned long int> last_modifications;

list <Species *> species_list;
vector <struct robby_msg> msg_list;

int update_known_map(struct robby_msg *msg, struct robby *r, struct world_map *m)
{
    int i,j;
    long int kmap_x, kmap_y;
    long unsigned int basex, basey;

    basex=msg->x;
    basey=msg->y;

    /* Map relative positions (message positions)
     * to absolute positions (map positions) and populate known map
     */
    kmap_x=-(VIEW_RADIUS-1);
    for (i = 0;
         i < SQUARE_SIDE;
         i++, kmap_x++) {

        if (basex + kmap_x >= m->sizex)
		continue;

	kmap_y = -(VIEW_RADIUS-1);

	for (j = 0;
	     j < SQUARE_SIDE;
	     j++, kmap_y++) {

		if (basey + kmap_y >= m->sizey)
			continue;
		if(msg->view[i][j]!=VIEW_ROBBY && msg->view[i][j]!=VIEW_TOO_FAR)
			r->known_map[basex+kmap_x][basey+kmap_y]=msg->view[i][j];
	}
    }

#ifdef DEBUG_KNOWN_MAP
	cout << "=====" << endl;
	cout << "Known map: " << r->id << endl;
	for (i=0;i<m->sizex;i++) {
		printf("[");
		for (j=0;j<m->sizey;j++)
			printf("%3d", r->known_map[i][j]);
		printf("]\n");
	}
	cout << "=====" << endl;
#endif
	return 0;
}

int update_view_and_send(struct world_map *m, struct robby *rl, long unsigned int robbynum)
{
	unsigned long int i;
	struct robby_msg newmsg;
	msg_list.clear();

	for (i = 0; i < robbynum; i++) {
		/* Update the state of the robby with the new view */
		update_view(&(rl[i]), m, false);
		newmsg.id = rl[i].id;
		newmsg.view = rl[i].view;
		newmsg.old_move = rl[i].old_move;
        newmsg.x=rl[i].x;
        newmsg.y=rl[i].y;

		msg_list.push_back(newmsg);

#ifdef KNOWN_MAP
        update_known_map(&newmsg, &rl[0],m);
#endif
	}
    
	return 0;
}

int move(struct world_map *m, struct robby *r)
{
    /* Dirnum = -1 means that the robby pulled up a can */
    int dirnum = -1;

    /* 1 -> The action was successfully done. */
    int success = 0;

    /* Prepare the state of the robby */
    PREPARE_STATE(r);

#ifdef DEBUG_MOVE
    cout << "position BEFORE " << r->x << "," << r->y << endl;
    cout << "over BEFORE " << r->over << "vs"<< (int)(r->over==CAN_DUMMY_PTR) << endl;
#endif
    
    /*done, terminate early*/
    if(m->gathered_cans == m->n_cans) {
        return 2;
    }
    
    dirnum=r->genome->activate(r, &msg_list);
    if(dirnum<4) {
        success=MOVE_NORMAL(r,m,dirnum);
    }
    else if(r->over==CAN_DUMMY_PTR) {
        r->over=NULL;
        r->gathered_cans++;
        m->gathered_cans++;
        success=1;
    }
    else 
        success=0;

    if(success==0)
        r->failed_moves++;
    r->num_moves++;

    r->clock++;

#ifdef DEBUG_MOVE
    /* Display some info on the current move */
    PRINT_MOVE_INFO(dirnum, r->genome->id, success);
    cout << "position AFTER" << r->x << " , " << r->y << endl;
    cout << "over AFTER " << r->over << "vs"<< (int) (r->over==CAN_DUMMY_PTR) << endl;
#endif
    r->old_move = dirnum;
    return success;
}

static int setup_generations(struct robby **rl, long unsigned int couplenum,
        long unsigned int robbynum)
{
    unsigned int coup, i;
    unsigned long int input_no;
    char **k_map;

#ifdef KNOWN_MAP
   input_no = rl[0][0].m_sizex*rl[0][0].m_sizey;
#else
    input_no = get_dis_circle_area(VIEW_RADIUS);
#endif

    /* Setup robby positions. */
    setup_positions(rl, robbynum);

    /* Create the genome for the robby */
    for (coup = 0; coup < couplenum; coup++) {

	/* Get genome from file or generate a new genome. */
        if (!exist_genome_file(DEFAULT_GENOME_DIR, coup))
            rl[coup][0].genome = new Genome(input_no,POSSIBLE_MOVES,robbynum);
        else
            rl[coup][0].genome = new Genome(DEFAULT_GENOME_DIR, coup);
        
        rl[coup][0].genome->specialize(&species_list);

#ifdef KNOWN_MAP
        k_map=(char**) calloc(rl[coup][0].m_sizex, sizeof(char*));
        if(!k_map) {
            perror("calloc known map");
            exit(-1);
        }

        for(i=0; i<rl[coup][0].m_sizex; i++) {
            k_map[i]=(char*) calloc(rl[coup][0].m_sizey, sizeof(char));
            if(!k_map[i]) {
                perror("calloc known map");
                exit(-1);
            }
	    memset(k_map[i], VIEW_TOO_FAR, rl[coup][0].m_sizey * sizeof(char));
        }
#endif

        for (i = 0; i < robbynum; i++) {
            /* Set the ID */
            rl[coup][i].id = i;

            rl[coup][i].viewradius = VIEW_RADIUS;
            rl[coup][i].known_map=k_map;

            /* Copy the genomes from the first of the couple (same genome for
	     * every robby on the map).
	     */
            if (i > 0)
                rl[coup][i].genome = new Genome(rl[coup][0].genome);
        }

    }
    return 0;
}

static int next_generation(struct robby **rl, unsigned long int couplenum,
        unsigned long int robbynum)
{
    unsigned long int coup,size, input_no;
    unsigned long int i;
    unsigned long int r1, r2;
    int r;
    unsigned long int j;
    double breed, tot_fitness;
    Genome *gen;
    Species *s;
    list <Species *>::iterator rs1, rs2;
    Genome *rg1, *rg2;
    list<Genome*> children;
    list<Genome*>::iterator g_it, end_git;
    vector<Genome*>::iterator vg_it, vend_git;
    list <Species *>::iterator s_it, end_sit;

    for (coup = 0; coup < couplenum; coup++) {
        rl[coup][0].genome->fitness = rl[coup][0].fitness;
#ifdef KNOWN_MAP
    for(i=0; i<rl[coup][0].m_sizex; i++) {
        memset(rl[coup][0].known_map[i], VIEW_TOO_FAR, rl[coup][0].m_sizey);
    }
#endif
    }
    
    /*clear last modification map for each generation*/
    last_modifications.clear();

    species_list.sort(species_desc_cmp);

    tot_fitness=0;
    size=0;
    
    remove_species(&species_list, couplenum, tot_fitness);
    
    /*cut the species in half*/
    for (s_it = species_list.begin(); s_it != species_list.end();) {
	if(!(*s_it)->cull(false)) {
	    delete (*s_it);    
	    s_it=species_list.erase(s_it);
	}
	else {
	    ++s_it;
	}
    }
    
    /*create children */
    for (s_it = species_list.begin(), end_sit = species_list.end(); s_it != end_sit; ++s_it) {
        /* Crossover children */
        breed=floor((((*s_it)->average_fitness / (double) tot_fitness))*(double) couplenum * .75)-1;
        for(i=0; i<breed; i++) {
            if (RANDOM_DOUBLE(1) >= INTERSPECIES_CROSSOVER_PROB) {
		    gen=new Genome(*s_it, false);
	    } else {
		    /* Crossover between different species */
		    r1 = (long unsigned int)round(RANDOM_DOUBLE(species_list.size() - 1));
		    do {
			    r2 = (long unsigned int)round(RANDOM_DOUBLE(species_list.size() - 1));
		    } while(r1 == r2 && species_list.size() > 1);

		    rs1 = species_list.begin();
		    advance(rs1, r1);

		    rs2 = species_list.begin();
		    advance(rs2, r2);

		    r1 = (long unsigned int)round(RANDOM_DOUBLE(((*rs1)->genomes.size() - 1)));
		    r2 = (long unsigned int)round(RANDOM_DOUBLE(((*rs2)->genomes.size() - 1)));

		    rg1 = (*rs1)->genomes[r1];
		    //rg1 = (*rs1)->genomes.begin();
		    //advance(rg1, r1);

		    rg2 = (*rs2)->genomes[r2];
		    //rg2 = (*rs2)->genomes.begin();
		    //advance(rg2, r2);

		    gen=new Genome(rg1, rg2);
            }
            children.push_back(gen);
            size++;
        }

        /* Mutate children */
        breed=floor((((*s_it)->average_fitness / (double) tot_fitness))*(double) couplenum * .25)-1;
        for(i=0; i<breed; i++) {
            gen=new Genome(*s_it, true);
            children.push_back(gen);
            size++;
        }
    }

    /*keep only 1 genome per species, delete empty ones*/
    for (s_it = species_list.begin(); s_it != species_list.end();) {
        if(!(*s_it)->cull(true)) {
            delete (*s_it);
            s_it=species_list.erase(s_it);
        } else {
            size+=(*s_it)->genomes.size();
            ++s_it;
        }
    }


    while(size<couplenum) {
        /* Add remaining children */
        r=(int)round(RANDOM_DOUBLE(species_list.size()-1));
#ifdef KNOWN_MAP
        input_no = rl[0][0].m_sizex*rl[0][0].m_sizey;
#else
        input_no = get_dis_circle_area(VIEW_RADIUS);
#endif
        s=LIST_GET(Species*, species_list, r);

	if(RANDOM_DOUBLE(1) < MISSING_GENOME_CROSSOVER_PROB)
		gen=new Genome(s, true);
	else
		gen=new Genome(input_no, POSSIBLE_MOVES,robbynum);
	
        children.push_back(gen);

        size++;
    }

    /*assign each children to a species*/
    for(g_it=children.begin(), end_git = children.end(); g_it!=end_git; ++g_it) {
        (*g_it)->specialize(&species_list);
    }

    i=0;

    /*assign genomes to robbies and clean up*/
    for(s_it=species_list.begin(), end_sit = species_list.end(); s_it!=end_sit; ++s_it){

        for(vg_it=(*s_it)->genomes.begin(), vend_git = (*s_it)->genomes.end(); vg_it!=vend_git; ++vg_it) {
            rl[i][0].genome=(*vg_it);

            for(j=1; j<robbynum; j++) {
                if(rl[i][j].genome)
                    delete rl[i][j].genome;
                rl[i][j].genome=new Genome(*vg_it);
            }

            i++;
        }

    }

    return 0;
}

/* Generate the robbies for the next generation (the list is sorted
 * from the best fitting to the worst).
 * You can choose fixed parameters for every robby.
 */
void generate_robbies(struct robby **rl, long unsigned int couplenum,
        long unsigned int robbynum,
        long unsigned int generation)
{
    unsigned long int i;

    if(generation>0)
        for (i = 0; i < couplenum; i++)
            rl[i][0].genome->fitness = rl[i][0].fitness;

#ifdef DEBUG_GENOMES
    list <Species *>::iterator s_it;
    vector <Genome *>::iterator g_it;

    i=0;
    for (s_it = species_list.begin(); s_it != species_list.end(); ++s_it) {
        cout << "Species BEFORE " << i++ << endl;
        for (g_it = (*s_it)->genomes.begin(); g_it != (*s_it)->genomes.end(); ++g_it) {
            cout <<"fitness is: "  <<(*g_it)->fitness <<" >> "<< (*g_it)->id << endl;
            cout << "----" << endl;
        }
    }
#endif

    if (generation == 0) {
        setup_generations(rl, couplenum, robbynum);
#ifdef DEBUG_SPECIES
        FILE *f;
        f=fopen("fitvalues", "w");
        fclose(f);
#endif
    } else {
#ifdef DEBUG_SPECIES
        FILE *f;
        f=fopen("fitvalues","a");
        for(i=0;i<couplenum;i++) {
            fprintf(f,"%lu %f %lu\n",i, rl[i][0].fitness,generation);
            fprintf(f,"0 %f %lu\n\n", rl[i][0].fitness, generation);
        }
        fprintf(f,"\n");
        fclose(f);
#endif
        next_generation(rl, couplenum, robbynum);
    }


#ifdef DEBUG_GENOMES
    i=0;
    for (s_it = species_list.begin(); s_it != species_list.end(); ++s_it) {
        cout << "Species AFTER " << i++ << endl;
        for (g_it = (*s_it)->genomes.begin(); g_it != (*s_it)->genomes.end(); ++g_it) {
            cout <<"fitness is: "  <<(*g_it)->fitness <<" >> "<< (*g_it)->id << endl;
            cout << "----" << endl;
        }
    }
#endif
}

static inline void free_kmap(struct robby *r) {
    long unsigned int i;
    for(i=0;i<r->m_sizex;i++)
        free(r->known_map[i]);
    free(r->known_map);
}

void cleanup(struct robby **rl, long unsigned int couplenum, long unsigned int robbynum)
{ 
    unsigned long int i, j;
    list<Species*>::iterator s_it;
    vector<Genome*>::iterator g_it;
    
    i=0;
    s_it=species_list.begin();
    while(s_it!=species_list.end()) {
        for(g_it=(*s_it)->genomes.begin(); g_it!=(*s_it)->genomes.end(); ++g_it) {
            (*g_it)->save_to_file(DEFAULT_GENOME_DIR,i);
            i++;
        }
        delete (*s_it);
        s_it=species_list.erase(s_it);
    }

    for(i=0; i<couplenum; i++) { 
        free_kmap(&rl[i][0]);
        for(j=1; j<robbynum; j++)
            delete(rl[i][j].genome);
    }
}
