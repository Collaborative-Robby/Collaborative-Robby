#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <robby/struct.h>

#include <robby/neural-net.h>
#include <robby/neural-net-const.h>

void setup_positions(struct robby **rl, long unsigned int robbynum)
{
    rl[0][0].original_x = 0;
    rl[0][0].original_y = 0;
    if (robbynum > 1) {
       /* original_ values contains sizex and sizey */
       rl[0][1].original_x--;
       rl[0][1].original_y--;
    }
    /* Random placing for the other robbies. */
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

static inline int calculate_species_values(list <Species *> *sl, long unsigned int couplenum, double &tot_fitness, double &max_fitness) 
{
	list <Species *>::iterator s_it;
    Genome* cgenome;

    /*calculate max fitness and total avg fitness*/
    for (s_it = sl->begin(); s_it != sl->end(); s_it++) {
        
		(*s_it)->genomes.sort(cmp_desc_genomes);
		
		cgenome = LIST_GET(Genome*, ((*s_it)->genomes), 0);
        
		if (cgenome->fitness > (*s_it)->top_fitness) {
			(*s_it)->top_fitness = cgenome->fitness;
			(*s_it)->staleness = 0;
        }
        else {
			(*s_it)->staleness++;
        }
        
        tot_fitness += (*s_it)->calculate_avg_fitness();

        if(max_fitness < (*s_it)->top_fitness)
            max_fitness=(*s_it)->top_fitness;
    }
    return 0;
}

/*remove species that have staled or have a low avg fitness*/
int remove_species(list <Species *> *sl, long unsigned int couplenum, double &tot_fitness)
{
	list <Species *>::iterator s_it;
	double breed = 0, max_fitness=0, local_tot;

    calculate_species_values(sl, couplenum, tot_fitness, max_fitness);
    local_tot=tot_fitness;
    
    /*remove species that are very weak*/
	for (s_it = sl->begin(); s_it != sl->end();) {
		breed = floor(((*s_it)->average_fitness / local_tot) *
		              (double) couplenum)-2;
        
		if (breed < 1 && sl->size()>1) {
            /*remove species that have a low average*/
            tot_fitness-=(*s_it)->average_fitness;
            delete (*s_it);
			s_it = sl->erase(s_it);
		} else if((*s_it)->staleness >= SPECIES_STALE_TRESHOLD &&
		    (*s_it)->top_fitness < max_fitness) {
            /*remove species that have staled*/
            tot_fitness-=(*s_it)->average_fitness;
            delete (*s_it);
			s_it = sl->erase(s_it);
        }
        else {
            s_it++;
        }
	}
    return 0;
}

