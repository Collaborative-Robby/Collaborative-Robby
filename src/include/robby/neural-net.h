#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include <map>
#include <list>

#define NODE_TYPE_INPUT 0
#define NODE_TYPE_HIDDEN 1
#define NODE_TYPE_OUTPUT 2

#define MAX_ACTIVATIONS 1

#define DEFAULT_GENOME_DIR ((char *)"genomes")
#define GENOME_EXT ((char *)"genome")

#define LIST_GET(ltype, l ,nelem) ({list<ltype>::iterator it=(l).begin(); advance(it,nelem); *it;})
#define HASH_GET(ktype, etype, l ,nelem) ({map<ktype, etype>::iterator it=l.begin(); advance(it,nelem); it->second;})

#define NODE_KEY_TYPE unsigned long int
#define GENE_KEY_TYPE unsigned long long int

#define GENE_INSERT(cgene, refhash) (refhash.insert(pair<GENE_KEY_TYPE, Gene *>(hash_ull_int_encode((cgene)->in->id, (cgene)->out->id), (cgene))))

#define NODE_INSERT(cnode, refhash) (refhash.insert(pair<NODE_KEY_TYPE, Node *>((cnode)->id, (cnode))))

using namespace std;

bool exist_genome_file(char *dir, int fileno);

long unsigned int next_innovation(void);

int remove_stale_species(list <class Species *> *sl);
int remove_weak_species(list <class Species *> *sl, long unsigned int couplenum);

class Gene {
	public:
		unsigned long int innovation;
		class Node *in,*out;
		double weight;
		bool enabled;
		double value;
        int activate_count;
		int point_mutate(void);
		void print(void);
		Gene(Gene *gen);
		Gene(void);
		void activate(double value);
};

class Node{

	public:
		int type;
		unsigned long int id;
		int activate_count;
		double value;
		list<Gene*> input_genes;
		list<Gene*> output_genes;
		Node(unsigned long id, int type);
		Node(Node *copy);
		~Node(void);
		void activate(double input);
		void print(void);

};

class Genome {

	public:
		map<unsigned long int, Node*> node_map;
		map<unsigned long long int, Gene*> gene_map;
		unsigned long int node_count;
		unsigned long int max_innov;
		double fitness;

		Genome(unsigned long int input_no, unsigned long int output_no);
		Genome(Genome *gen);
		Genome(Genome *g1, Genome *g2);
		Genome(char *dir, int fileno);
        Genome(class Species *s);

		~Genome(void);
        
        void copy(Genome *g);
        void crossover(Genome *g1, Genome *g2);
		int insert_gene(Gene *g1);
		int mutate(void);
		int node_mutate(void);
		int link_mutate(bool force_bias);
		int enable_disable_mutate(bool enable);
		void print(void);
		bool containslink(class Gene *g);
		int activate(char **view);
		int save_to_file(char *dir, int fno);
		int specialize(list <class Species *> *sl);
};

class Species {	
	public:
		int staleness;
		double top_fitness;
		double average_fitness;
		list <Genome *> genomes;

		Species(void);
        ~Species(void);

		unsigned long int cull(bool onlyone);
		double calculate_avg_fitness(void);
};

#endif

