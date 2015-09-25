/* Sample module for collaborative robby */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <iostream>
#include <list>
#include <robby/struct.h>
#include <robby/module.h>
#include <robby/neural-net.h>
#include <robby/dismath.h>

double pool_maxfitness = 0.0;
list<Species *> species_list;

extern long unsigned int global_innovation;

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
    dirnum=r->genome->activate(r);
    if(dirnum<4) {
        success=MOVE_NORMAL(r,m,dirnum);
    }
    else if(r->over==CAN_DUMMY_PTR) {
        r->over=NULL;
        r->gathered_cans++;
        success=1;
    }
    else 
        success=0;

    if(success==0)
        r->failed_moves++;

    /* Update the state of the robby with the new view */
    update_view(r, m, false);

    r->clock++;

#ifdef DEBUG_MOVE
    /* Display some info on the current move */
    PRINT_MOVE_INFO(dirnum, r->genome->id, success);
    cout << "position AFTER" << r->x << " , " << r->y << endl;
    cout << "over AFTER " << r->over << "vs"<< (int) (r->over==CAN_DUMMY_PTR) << endl;
#endif
    
    return success;
}

static int setup_generations(struct robby **rl, long unsigned int couplenum,
        long unsigned int robbynum)
{
    unsigned int coup, i;
    unsigned long int real_view;

    real_view = get_dis_circle_area(VIEW_RADIUS);

    /* Create the genome for the robby */
    for (coup = 0; coup < couplenum; coup++) {

        if (true ||  !exist_genome_file(DEFAULT_GENOME_DIR, coup))
            rl[coup][0].genome = new Genome(real_view,POSSIBLE_MOVES);
        else
            rl[coup][0].genome = new Genome(DEFAULT_GENOME_DIR, coup);

        rl[coup][0].genome->specialize(&species_list);

        for (i = 0; i < robbynum; i++) {
            /* Set the ID */
            rl[coup][i].id = i;
            /* A radius of one for the view */
            rl[coup][i].viewradius = VIEW_RADIUS;

            if (i > 0)
                rl[coup][i].genome = new Genome(rl[coup][0].genome);
        }

    }
    return 0;
}

bool cmp_average_fitness(Species *s1, Species *s2) 
{
    return s1->average_fitness > s2->average_fitness;
}

static int next_generation(struct robby **rl, unsigned long int couplenum,
        unsigned long int robbynum)
{
    unsigned long int coup;
    int i,r;
    unsigned long int j;
    double breed, tot_fitness;
    Genome* gen;
    Species *s;
    list<Genome*> children;
    list<Genome*>::iterator g_it;
    list <Species *>::iterator s_it;

    for (coup = 0; coup < couplenum; coup++)
        rl[coup][0].genome->fitness = rl[coup][0].fitness;

    for (s_it = species_list.begin(); s_it != species_list.end();) {
        if(!(*s_it)->cull(false)) {
            delete (*s_it);    
            s_it=species_list.erase(s_it);
        }
        else {
            s_it++;
        }
    }
    
    remove_stale_species(&species_list);
    //forse rank globally???
    remove_weak_species(&species_list, couplenum);
    
    tot_fitness=0;
    for (s_it = species_list.begin(); s_it != species_list.end(); s_it++)
        tot_fitness += (*s_it)->calculate_avg_fitness();

    species_list.sort(cmp_average_fitness);

    for (s_it = species_list.begin(); s_it != species_list.end(); s_it++) {
        breed=floor((((*s_it)->average_fitness / (double) tot_fitness))*(double) couplenum)-1;
        for(i=0; i<breed; i++) {
            gen=new Genome(*s_it);
            children.push_back(gen);
        }
    }

    for (s_it = species_list.begin(); s_it != species_list.end();) {
        if(!(*s_it)->cull(true)) {
            delete (*s_it);
            s_it=species_list.erase(s_it);
        }
        else {
            s_it++;
        }
    }

    while(children.size()+species_list.size()<couplenum) {
        r=(int)round(RANDOM_DOUBLE(species_list.size()-1));
        s=LIST_GET(Species*, species_list, r);
        gen=new Genome(s);
        children.push_back(gen);
    }

    for(g_it=children.begin(); g_it!=children.end(); g_it++) {
        (*g_it)->specialize(&species_list);
    }

    i=0;
    /* TODO assign a genome to a robby */
    for(s_it=species_list.begin(); s_it!=species_list.end(); s_it++){
        for(g_it=(*s_it)->genomes.begin(); g_it!=(*s_it)->genomes.end(); g_it++) {

            /*for(j=0; j<robbynum; j++) {
              if(rl[i][j].genome)
              delete rl[i][j].genome;
              }*/

            rl[i][0].genome=(*g_it);
            for(j=1; j<robbynum; j++)
                rl[i][j].genome=new Genome(*g_it);
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
    FILE *f;
    unsigned long int i, j;

    for (i = 0; i < couplenum; i++)
        for (j = 0; j < robbynum; j++)
            rl[i][j].clock = 0;
   
    //FIXME roba stampe brutta cacca
    if(generation>0)
        for (i = 0; i < couplenum; i++)
            rl[i][0].genome->fitness = rl[i][0].fitness;

#ifdef DEBUG_GENOMES
    list <Species *>::iterator s_it;
    list <Genome *>::iterator g_it;
    
    i=0;
    for (s_it = species_list.begin(); s_it != species_list.end(); s_it++) {
        cout << "Species BEFORE " << i++ << endl;
        for (g_it = (*s_it)->genomes.begin(); g_it != (*s_it)->genomes.end(); g_it++) {
            //(*g_it)->print();
            cout <<"fitness is: "  <<(*g_it)->fitness <<" >> "<< (*g_it)->id << endl;
            cout << "----" << endl;
        }
    }
#endif
    
    if (generation == 0) {
        setup_generations(rl, couplenum, robbynum);
        f=fopen("fitvalues", "w");
        fclose(f);
    } else {
    #ifdef DEBUG_SPECIES
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

    cout << "species list size: " << species_list.size() << endl;

#ifdef DEBUG_GENOMES
    
    i=0;
    for (s_it = species_list.begin(); s_it != species_list.end(); s_it++) {
        cout << "Species AFTER " << i++ << endl;
        for (g_it = (*s_it)->genomes.begin(); g_it != (*s_it)->genomes.end(); g_it++) {
            //(*g_it)->print();
            cout <<"fitness is: "  <<(*g_it)->fitness <<" >> "<< (*g_it)->id << endl;
            cout << "----" << endl;
        }
    }
#endif
}

void cleanup(struct robby **rl, long unsigned int couplenum, long unsigned int robbynum)
{
    list<Species*>::iterator s_it;

    s_it=species_list.begin();
    while(s_it!=species_list.end()) {
        delete (*s_it);
        s_it=species_list.erase(s_it);
    }
}
