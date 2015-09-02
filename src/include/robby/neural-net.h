#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include <map>
#include <list>

#define NODE_TYPE_INPUT 0
#define NODE_TYPE_HIDDEN 1
#define NODE_TYPE_OUTPUT 2

#define MAX_ACTIVATIONS 20

using namespace std;


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
    int activate(char **view, int viewradius);
};

#endif

