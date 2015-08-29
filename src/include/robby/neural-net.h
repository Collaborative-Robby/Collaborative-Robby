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
        virtual int point_mutate(void);
        virtual void copy(Gene *g);
        virtual void print(void);
        Gene(void){
        }
};

class Node{

    public:
        int type;
        int id;
        list<Gene*> input_genes;
        list<Gene*> output_genes;
        Node(int id, int type);
        Node(Node *copy);
        int activate(class Gene* activator, double input);
        virtual void print(void);

};


class Genome {

    public:
        list<Node*> node_list;
        list<Gene*> gene_list;
        int node_count,global_innov;
        Genome(int input_no, int output_no);
        virtual int mutate(void);
        virtual int copy(Genome *gen);
        int node_mutate(void);
        virtual int link_mutate(bool force_bias);
        virtual int enable_disable_mutate(bool enable);
        virtual void print(void);
};

#endif

