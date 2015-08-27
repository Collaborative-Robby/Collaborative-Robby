#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include <list>

using namespace std;

class Gene {
    public:
        int innovation;
        class Node *in,*out;
        double weight;
        bool enabled;
        int point_mutate(void);
        void copy(Gene *g);
};

class Node {
    
    public:
        int type;
        int id;
        list<Gene*> input_genes, output_genes;
        Node(int id, int type);
        int activate(Gene* activator, double input);

};

class Genome {
    private:
        list<Node*> node_list;
        list<Gene*> gene_list;
        int node_count,global_innov;

    public:
        Genome(void);
        int mutate(void);
        int copy(Genome *gen);
        int node_mutate(void);
        int link_mutate(bool force_bias);
};

#endif
