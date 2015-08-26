#include <list>

using namespace std;

class Gene {
    public:
        int innovation;
        class Node *in,*out;
        double weight;
        bool enabled;
};

class Node {
    
    public:
        int type;
        int id;
        list<Gene> input_genes, output_genes;
        Node(int id, int type);
        int Activate(Gene* activator, double input);


};

class Genome {
    private:
        list<Node> node_list;
        list<Gene> gene_list;
        int node_count,global_innov;

    public:
        Genome(void);
};

