#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include <unordered_map>
#include <list>
#include <vector>
#include <robby/Fraction.h>

#define NODE_TYPE_INPUT 0
#define NODE_TYPE_HIDDEN 1
#define NODE_TYPE_OUTPUT 2

#define MAX_REACTIVATIONS 0

#define DEFAULT_GENOME_DIR ((char *)"genomes")
#define GENOME_EXT ((char *)"genome")

#define LIST_GET(ltype, l ,nelem) ({list<ltype>::iterator it=(l).begin(); advance(it,nelem); *it;})
#define HASH_GET(ktype, etype, l ,nelem) ({unordered_map<ktype, etype>::iterator it=l.begin(); advance(it,nelem); it->second;})

#define NODE_KEY_TYPE unsigned long int
#define GENE_KEY_TYPE unsigned long long int

#define GENE_INSERT(cgene, refhash) ({refhash.insert(pair<GENE_KEY_TYPE, Gene *>(hash_ull_int_encode((cgene)->in->id, (cgene)->out->id), (cgene)));})

#define NODE_INSERT(cnode, refvector) (refvector.push_back(cnode))
#define HASH_INSERT(cnode, refhash) (refhash.insert(pair<NODE_KEY_TYPE, Node *>((cnode)->id, (cnode))))

using namespace std;


bool exist_genome_file(char *dir, int fileno);

long unsigned int next_innovation(unsigned long int id_node_in, unsigned long int id_node_out);

//int remove_stale_species(list <class Species *> *sl);
int remove_species(list <class Species *> *sl, long unsigned int couplenum, double &tot_fitness);

class Gene {
	public:
		unsigned long int innovation;
		class Node *in,*out;
		double weight;
		bool enabled;
		double value;
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
		unsigned long int active_in_genes;
		double value;
		Fraction level;
		list<Gene*> input_genes;
		list<Gene*> output_genes;
		list< vector<Node*> >::iterator level_it;

		Node(unsigned long id, int type, unsigned long int l_num, unsigned long int l_den);
		Node(Node *copy);
		~Node(void);
		void activate(double input);
		void print(void);

};

class Genome {

	public:
		//unordered_map<unsigned long int, Node*> node_map;
		vector <Node*> node_vector;
		unordered_map<unsigned long long int, Gene*> gene_map;
		unsigned long int max_innov;
		unsigned long int id;
		double fitness;
 	    list< vector <Node*> > level_list;

		Genome(unsigned long int input_no, unsigned long int output_no, unsigned long robbynum);
		Genome(Genome *gen);
		Genome(Genome *g1, Genome *g2);
		Genome(char *dir, int fileno);
		Genome(class Species *s, bool crossover);

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
		int activate(struct robby *r, vector <struct robby_msg> *ml);
		int save_to_file(char *dir, long unsigned int fno);
		int specialize(list <class Species *> *sl);
		void insert_level_list(Node *n);
};

class Species {	
	public:
		int staleness;
		long unsigned int age;
		double top_fitness;
		double average_fitness;
		vector <Genome *> genomes;

		Species(void);
		~Species(void);

		unsigned long int cull(bool onlyone);
		double calculate_avg_fitness(void);

		friend bool operator>  (Species &s1, Species &s2);
		friend bool operator<  (Species &s1, Species &s2);
		friend bool operator>= (Species &s1, Species &s2);
		friend bool operator<= (Species &s1, Species &s2);
};

#endif

