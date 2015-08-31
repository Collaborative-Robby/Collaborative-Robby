#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include <map>
#include <list>

using namespace std;

class Gene {
    public:
        int innovation;
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
        int id;
        list<Gene*> input_genes;
        list<Gene*> output_genes;
        Node(int id, int type);
        Node(Node *copy);
        ~Node(void);
        int activate(class Gene* activator, double input);
        void print(void);

};

class Genome {

    public:
        map<int, Node*> node_map;
        list<Gene*> gene_list;
        int node_count,global_innov;
        Genome(int input_no, int output_no);
        Genome(Genome *gen);
        ~Genome();
        int mutate(void);
        int node_mutate(void);
        int link_mutate(bool force_bias);
        int enable_disable_mutate(bool enable);
	int next_innovation(void);
        void print(void);
};

#endif
