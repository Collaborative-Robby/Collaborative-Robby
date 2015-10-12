#include <math.h>

#include <robby/neural-net.h>
#include <robby/neural-net-utils.h>

bool species_desc_cmp(Species *s1, Species *s2)
{
	return *s1 > *s2;
}

/* Species */
Species::Species(void)
{
	this->staleness = 0;
	this->top_fitness = 0.0;
	this->average_fitness = 0.0;
}

Species::~Species(void)
{
    list<Genome*>::iterator it;

    it=this->genomes.begin();
    while(it!=this->genomes.end()) {
        delete(*it);
        it=this->genomes.erase(it);
    }
}

unsigned long int Species::cull(bool top_only)
{
	int cutoff = 0;
	unsigned long int size = 0;
	list <Genome *>::iterator it;

	this->genomes.sort(cmp_desc_genomes);
	size = this->genomes.size();

	if (top_only && size>=2)
		cutoff = 2;
	else if (top_only)
        cutoff=1;
    else 
		cutoff = (int) round((double)this->genomes.size() / 2.0);
    
    if(size>0) {
        it=this->genomes.begin();
        advance(it,cutoff);
        while(it!=this->genomes.end()) {
            delete (*it);
            it=this->genomes.erase(it);
        }
    }

	return this->genomes.size();
}

double Species::calculate_avg_fitness(void)
{
	double avg = 0.0;
	unsigned long int size = 0;
	list <Genome *>::iterator g_it;

	size = this->genomes.size();

	for (g_it=this->genomes.begin(); g_it != this->genomes.end(); g_it++)
		avg += (*g_it)->fitness;

	this->average_fitness = (avg/(double) size);
	return this->average_fitness;
}

bool operator< (Species &s1, Species &s2)
{
	return s1.average_fitness < s2.average_fitness;
}

bool operator> (Species &s1, Species &s2)
{
	return s1.average_fitness < s2.average_fitness;
}

bool operator<= (Species &s1, Species &s2)
{
	return s1.average_fitness <= s2.average_fitness;
}

bool operator>= (Species &s1, Species &s2)
{
	return s1.average_fitness <= s2.average_fitness;
}
