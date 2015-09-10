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

    dirnum=r->genome->activate(r->view, r->viewradius);
    if(dirnum<4) {
        success=MOVE_WRAP(r,m,dirnum);
    }
    else if(r->over==CAN_DUMMY_PTR) {
        r->over=NULL;
        r->gathered_cans++;
        success=1;
    }
    else
        success=0;

	/* Update the state of the robby with the new view */
	update_view(r, m, false);

	/* Display some info on the current move */
	PRINT_MOVE_INFO(dirnum, r->id, success);

	return success;
}

static int setup_generations(struct robby **rl, long unsigned int couplenum,
		long unsigned int robbynum)
{
	int coup, i;
	/* Create the genome for the robby */
	for (coup = 0; coup < couplenum; coup++) {

		if (!exist_genome_file(DEFAULT_GENOME_DIR, coup))
			rl[coup][0].genome = new Genome(SQUARE_AREA,POSSIBLE_MOVES);
		else
			rl[coup][0].genome = new Genome(DEFAULT_GENOME_DIR, coup);

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

static int next_generation(struct robby **rl, long unsigned int couplenum,
		long unsigned int robbynum)
{
	int coup;
	list <Species *>::iterator s_it;
	for (coup = 0; coup < couplenum; coup++)
		rl[coup][0].genome->fitness = rl[coup][0].fitness;
	for (s_it = species_list.begin(); s_it != species_list.end(); s_it++) {
		(*s_it)->cull(false);
	}
	/* TODO Cross over */
	/* TODO assign a genome to a robby */
}

/* Generate the robbies for the next generation (the list is sorted
 * from the best fitting to the worst).
 * You can choose fixed parameters for every robby.
 */
void generate_robbies(struct robby **rl, long unsigned int couplenum,
		long unsigned int robbynum,
		long unsigned int generation)
{
	int i, coup;
	Genome* gen;

	/* initialize robbies for the next generations */
	if (generation == 0) {
		setup_generations(rl, couplenum, robbynum);
	} else {
		next_generation(rl, couplenum, robbynum);
	}

	for (coup = 0; coup < couplenum; coup++) {
		cout << "Classificating genome " << coup << endl;
		rl[coup][0].genome->print();
		gen = rl[coup][0].genome;
		rl[coup][0].genome->mutate();

		rl[coup][0].genome->specialize(&species_list);

		for(i=1; i<robbynum; i++) {
			/* delete/garbage collecting */
			delete rl[coup][i].genome;

			rl[coup][i].genome = new Genome(gen);
		}

		rl[coup][0].genome->save_to_file(DEFAULT_GENOME_DIR, coup);
	}
	/* Do nothing for the next generations:
	 * keep the same robbies in random positions.
	 */
	list <Species *>::iterator s_it;
	list <Genome *>::iterator g_it;
	i = 0;
	for (s_it = species_list.begin(); s_it != species_list.end(); s_it++) {
		cout << "Species " << i++ << endl;
		for (g_it = (*s_it)->genomes.begin(); g_it != (*s_it)->genomes.end(); g_it++) {
			(*g_it)->print();
			cout << "----" << endl;
		}
	}
}

void cleanup(struct robby **rl, int couplenum, int robbynum)
{
	int i,j;
	for(i=0; i<couplenum; i++) {
		for(j=0; j<robbynum; j++) {
			if (rl[i][j].genome)
				delete rl[i][j].genome;
		}
	}
}
