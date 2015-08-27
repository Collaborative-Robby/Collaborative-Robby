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

#define MUTATION_RATE_NODE 0.5
#define MUTATION_RATE_CONNECTION 0.25

#define PERTURB_CHANCE 0.9
#define PERTURB_STEP 0.1

/* Greedy procedural move, pick up a can if is possible, otherwise move in a
 * random direction.
 * MUST return 0 if the move failed or 1 if the move succeded.
 */

#define RANDOM_DOUBLE(max) (((double) rand()/ (double) RAND_MAX)*(double) max)

#define LIST_GET(ltype, l ,nelem) ({list<ltype>::iterator it=l.begin(); advance(it,nelem); *it;})

list<list <Genome*> > species_lists;

Genome::Genome(void) {
    this->node_count=0;
    this->global_innov=0;
}

int Genome::mutate(void) {
    list<Gene*>::iterator gene_iter;
    int mrate_node;

    //mutate node, spezza un arco e aggiunge un nodo
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_NODE) 
        this->node_mutate(); 
    

    //mutate link, aggiunge un link fra due nodi random
    
    //mutate link+bias, aggiungi link con input da tutti i nodi di input

    //mutate point, cambia i pesi
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_CONNECTION) {
        for (gene_iter = this->gene_list.begin(); gene_iter != this->gene_list.end(); gene_iter++)
            (*gene_iter)->point_mutate();
    } 
}

int Genome::copy(Genome *gen) {
}

Node::Node(int id, int type) {
   this->type=type;
   this->id=id;
}

int Genome::node_mutate(void) {
    Node *neuron;
    unsigned int list_len;
    int gene_index;
    list<Node*>::iterator it;
    Gene *g_orig, *g1,*g2;
    
    //TODO get id, define types
    neuron=new Node(1,1);
    this->node_list.push_back(neuron);

    list_len=this->gene_list.size();
    gene_index=round(RANDOM_DOUBLE(list_len)); 
    
    g_orig=LIST_GET(Gene*, this->gene_list, gene_index);
    
    if(!g_orig->enabled)
        return -1;
    
    g_orig->enabled=false;
    
    g1=new Gene();
    g2=new Gene();

    g1->copy(g_orig);
    g1->out=neuron;
    g1->weight=1.0;
    //TODO innovation
    g1->innovation=666;
    g1->enabled=true;

    g2->copy(g_orig);
    g2->in=neuron;
    g2->innovation=667;
    g2->enabled=true;
    
    neuron->input_genes.push_back(g1);
    neuron->output_genes.push_back(g2);

    g_orig->in->output_genes.push_back(g1);
    g_orig->out->input_genes.push_back(g2);

    this->gene_list.push_back(g1);
    this->gene_list.push_back(g2);

    return 0;
}

int Gene::point_mutate(void) {
    if(RANDOM_DOUBLE(1)<PERTURB_CHANCE)
        this->weight+=(RANDOM_DOUBLE(1)*PERTURB_STEP*2-PERTURB_STEP);
    else
        this->weight=RANDOM_DOUBLE(1)*4-2;
}


int move(struct map *m, struct robby *r)
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
void generate_robbies(struct robby *rl, long unsigned int robbynum,
		long unsigned int generation)
{
	int i;
    Genome* gen;

	/* initialize robbies for the next generations */
	if (generation == 0) {
		for (i = 0; i < robbynum; i++) {
			/* Set the ID */
			rl[i].id = i;

			/* A radius of one for the view */
			rl[i].viewradius = 2;

            rl[i].genome=new Genome();
		}
	}
    
    gen=rl[0].genome;
    rl[0].genome->mutate();

    for(i=0; i<robbynum; i++) {
       rl[i].genome->copy(gen);
    }

	/* Do nothing for the next generations:
	 * keep the same robbies in random positions.
	 */
}
