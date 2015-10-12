#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <robby/neural-net.h>
#include <robby/neural-net-const.h>

/* Compare the level (in the Q domain) of two nodes
 * Return -1 or 1 if, respectively the first or the second node is at an
 * inferior level.
 */
int compare_level(Node* n1, Node *n2)
{
    unsigned long long int v1,v2;

    v1 = n1->level_numerator*n2->level_denom;
    v2 = n2->level_numerator*n1->level_denom;

    if (v1<v2)
        return -1;
    if (v1>v2)
        return 1;

    return 0;
}

/* Hash function to encode two unsigned long int */
 unsigned long long int hash_ull_int_encode(unsigned long from, unsigned long to)
{
    unsigned long long a, b;
    a = from;
    b = to;

    return (a >= b ? (a * a + a + b) : (a + b * b));
}

/* Sigmoid smooth activation function */
double sigmoid(double input) {
    return 1/(1+pow(M_E, -input*SIGMOID_BETA));
}

/* Check if a genome file exist. */
bool exist_genome_file(char *dir, int fileno) {
	char *path;
	int rval;
	FILE *f;

	rval = asprintf(&path, "%s/%d.%s", dir, fileno, GENOME_EXT);
	if (rval < 0) {
		perror("asprintf exist_genome_file");
		return false;
	}

	f = fopen(path, "r");
	if (!f) {
		free(path);
		return false;
	}
	free(path);
	return true;
}

/* Compare genome in descending order (used to sort). */
bool cmp_desc_genomes(Genome *g1, Genome *g2)
{
	return (g1->fitness > g2->fitness);
}

 double delta_species(Genome *g1, Genome *g2)
{
	int found;
	double delta_excess = 0, delta_disjoint = 0, delta_weight = 0;

    long unsigned int gene_count,equal_genes_count,excess_genes,disjoint_genes;
	map<GENE_KEY_TYPE, Gene*>::iterator g_it;
	Gene *equal_gene;
	Genome *g, *oth_g;

    gene_count=0;
    excess_genes=0;
    disjoint_genes=0;
    equal_genes_count=0;
    found=0;

	g = g1;
	oth_g = g2;

	if (g1->max_innov < g2->max_innov) {
		g = g2;
		oth_g = g1;
	}

	gene_count = g1->gene_map.size();

	if (gene_count < g2->gene_map.size())
		gene_count = g2->gene_map.size();

	for (g_it=g->gene_map.begin(); g_it!=g->gene_map.end();g_it++) {
		found = oth_g->gene_map.count(g_it->first) &&
		(oth_g->gene_map[g_it->first]->innovation ==
		g_it->second->innovation);

		if (!found && g_it->second->innovation > oth_g->max_innov)
			excess_genes++;

		else if (!found)
			disjoint_genes++;

		/* XXX check the same innovation number */
		else {
			equal_gene = oth_g->gene_map[g_it->first];

			equal_genes_count++;

			delta_weight += fabs(g_it->second->weight - equal_gene->weight);
		}
	}

	for (g_it=oth_g->gene_map.begin(); g_it!=oth_g->gene_map.end();g_it++) {
		/* Excesses genes are already counted. */
		found = g->gene_map.count(g_it->first) &&
		        (g->gene_map[g_it->first]->innovation == g_it->second->innovation);

		if (!found)
			disjoint_genes++;
	}


	delta_excess = COEFFICIENT_EXCESS_GENES * (double) excess_genes / (double) gene_count;
	delta_disjoint = COEFFICIENT_DISJOINT_GENES * (double) disjoint_genes / (double) gene_count;
	if (equal_genes_count)
		delta_weight = COEFFICIENT_DELTA_WEIGHT * delta_weight / (double) equal_genes_count;

	if (!gene_count)
		delta_excess = delta_disjoint = 0;

	double x = (delta_excess + delta_disjoint + delta_weight);
	return x;
}

 void get_level_num(long unsigned int n1, long unsigned int d1, long unsigned int n2, long unsigned int d2, long unsigned int *new_n, long unsigned int *new_d) {
   if(d1>=d2) {
        *new_d=d1*2;
        *new_n=(n2*(d1/d2))+n1;
   }
   else {
        *new_d=d2*2;
        *new_n=(n1*(d2/d1))+n2;
   }
}

int remove_weak_species(list <Species *> *sl, long unsigned int couplenum)
{
	list <Species *>::iterator s_it;
    Genome* cgenome;
	double tot_fitness = 0, breed = 0, max_fitness=0;

    for (s_it = sl->begin(); s_it != sl->end(); s_it++) {
        
		(*s_it)->genomes.sort(cmp_desc_genomes);
		
		cgenome = LIST_GET(Genome*, ((*s_it)->genomes), 0);
        
		if (cgenome->fitness > (*s_it)->top_fitness)
			(*s_it)->top_fitness = cgenome->fitness;
        
        tot_fitness += (*s_it)->calculate_avg_fitness();

        if(max_fitness < (*s_it)->top_fitness)
            max_fitness=(*s_it)->top_fitness;
    }

	for (s_it = sl->begin(); s_it != sl->end();) {
		breed = floor(((*s_it)->average_fitness / tot_fitness) *
		              (double) couplenum)-1;

		if (breed < 1 && sl->size()>1) {
            delete (*s_it);
			s_it = sl->erase(s_it);
		}
        else {
            s_it++;
        }
	}
    return 0;
}

int remove_stale_species(list <Species *> *sl)
{
	Genome *cgenome;
	double max_fitness = 0;
	list <Species *>::iterator s_it;

	for (s_it = sl->begin(); s_it != sl->end(); s_it++) {
		(*s_it)->genomes.sort(cmp_desc_genomes);

		cgenome = LIST_GET(Genome*, ((*s_it)->genomes), 0);
		if (cgenome->fitness > (*s_it)->top_fitness) {
			(*s_it)->top_fitness = cgenome->fitness;

			(*s_it)->staleness = 0;
		} else {
			(*s_it)->staleness++;
		}

		if (max_fitness < (*s_it)->top_fitness)
			max_fitness = (*s_it)->top_fitness;

	}

	for (s_it = sl->begin(); s_it != sl->end();) {
		if ((*s_it)->staleness >= SPECIES_STALE_TRESHOLD &&
		    (*s_it)->top_fitness < max_fitness) {
            delete (*s_it);
			s_it = sl->erase(s_it);
		}
        else {
            s_it++;
        }
	}   
    return 0;
}

