/* Sample module for collaborative robby */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <robby/struct.h>
#include <robby/module.h>

/* Greedy procedural move, pick up a can if is possible, otherwise move in a
 * random direction.
 * MUST return 0 if the move failed or 1 if the move succeded.
 */
int move(struct world_map *m, struct robby *r)
{
	/* Dirnum = -1 means that the robby pulled up a can */
	int dirnum = -1;

	/* 1 -> The action was successfully done. */
	int success = 0;

	/* Prepare the state of the robby */
	PREPARE_STATE(r);

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
void generate_robbies(struct robby **rl, long unsigned int couplenum, long unsigned int robbynum,
		long unsigned int generation)
{
	long unsigned int i,j;

	/* initialize robbies for the next generations */
	if (generation == 0) {
		for (i = 0; i < robbynum; i++) {
			for (j = 0; j < robbynum; j++) {
				/* Set the ID */
				rl[i][j].id = (int) j;

				/* A radius of one for the view */
				rl[i][j].viewradius = 2;
			}
		}
	}

	/* Do nothing for the next generations:
	 * keep the same robbies in random positions.
	 */
}

void cleanup(struct robby **rl, int couplenum, int rnum)
{
}
