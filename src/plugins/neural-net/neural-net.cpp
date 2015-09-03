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

list<list <Genome*> > species_lists;

int move(struct world_map *m, struct robby *r)
{
	/* Dirnum = -1 means that the robby pulled up a can */
	int dirnum = -1;

	/* 1 -> The action was successfully done. */
	int success = 0;

	/* Prepare the state of the robby */
	PREPARE_STATE(r);


    r->genome->activate(r->view, r->viewradius);

	if (r->over == CAN_DUMMY_PTR) {
		/* Pick up a can */
		r->over = NULL;
		r->gathered_cans++;
		success = 1;
	} else {
		/* Move in a random direction */
		dirnum=rand() % 4;

		/* Move with wraparound */
		success = MOVE_WRAP(r, m, dirnum);
	}

	/* Update the state of the robby with the new view */
	update_view(r, m, false);

	/* Display some info on the current move */
	PRINT_MOVE_INFO(dirnum, r->id, success);

	return success;
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
		/* Create the genome for the robby */
		for (coup = 0; coup < couplenum; coup++) {

			rl[coup][0].genome= new Genome(SQUARE_AREA,POSSIBLE_MOVES);
			for (i = 0; i < robbynum; i++) {
				/* Set the ID */
				rl[coup][i].id = i;
				/* A radius of one for the view */
				rl[coup][i].viewradius = VIEW_RADIUS;

				if (i > 0)
					rl[coup][i].genome = new Genome(rl[coup][0].genome);
			}
		}
	} else {
		/* TODO Cross over */
	}

	for (coup = 0; coup < couplenum; coup++) {
		gen = rl[coup][0].genome;
		rl[coup][0].genome->mutate();

		rl[coup][0].genome->print();
    
		for(i=1; i<robbynum; i++) {
			/* delete/garbage collecting */
			delete rl[coup][i].genome;

			rl[coup][i].genome = new Genome(gen);
			rl[coup][i].genome->print();
		}
	}

	/* Do nothing for the next generations:
	 * keep the same robbies in random positions.
	 */
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
