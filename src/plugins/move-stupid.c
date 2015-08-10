#include <stdio.h>
#include <stdlib.h>
#include <robby/struct.h>
#include <robby/module.h>

int move(struct map *m, struct robby *r)
{
	int i,dirnum = -1;
	int success = 0; 
	PREPARE_STATE(r);

	if (r->over == CAN_DUMMY_PTR) {
		/* pick up can */
		r->over = NULL;
		r->gathered_cans++;
		success = 1;
	} else {
		/* Move in a random direction */
		dirnum=rand() % 4;

		success = MOVE_WRAP(r, m, dirnum);
	}

	UPDATE_STATE(r, m);

	PRINT_MOVE_INFO(dirnum, r->id, success);

	return success;
}

void generate_robbies(struct robby *rl, long unsigned int robbynum,
		long unsigned int generation)
{
	int i;
	/* do nothing */
	if (generation == 0)
		for (i = 0; i < robbynum; i++)
			rl[i].id = i;
}
