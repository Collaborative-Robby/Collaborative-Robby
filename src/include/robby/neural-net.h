#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include <map>
#include <list>

using namespace std;

class Gene {
    public:
        unsigned long int innovation;
        class Node *in,*out;
        double weight;
        bool enabled;
        int point_mutate(void);
        void print(void);
	Gene(Gene *gen);
	Gene(void);
};

class Node{

    public:
        int type;
        unsigned long int id;
        list<Gene*> input_genes;
        list<Gene*> output_genes;
        Node(unsigned long id, int type);
        Node(Node *copy);
        ~Node(void);
        int activate(class Gene* activator, double input);
        void print(void);

};

class Genome {

    public:
        map<unsigned long int, Node*> node_map;
        map<unsigned long long int, Gene*> gene_map;
        unsigned long int node_count, global_innov;
        Genome(unsigned long int input_no, unsigned long int output_no);
        Genome(Genome *gen);
        ~Genome();
        int mutate(void);
        int node_mutate(void);
        int link_mutate(bool force_bias);
        int enable_disable_mutate(bool enable);
	int next_innovation(void);
        void print(void);
	bool containslink(class Gene *g);
};

#endif

