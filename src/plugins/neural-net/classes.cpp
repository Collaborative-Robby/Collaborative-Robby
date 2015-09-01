/* Sample module for collaborative robby */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <iostream>
#include <map>
#include <list>
#include <robby/struct.h>
#include <robby/module.h>
#include <robby/dismath.h>
#include <robby/neural-net.h>

#define MUTATION_RATE_NODE 0.5
#define MUTATION_RATE_CONNECTION 0.25
#define MUTATION_RATE_LINK 2.0
#define MUTATION_RATE_BIAS 0.4
#define MUTATION_RATE_ENABLE 0.2
#define MUTATION_RATE_DISABLE 0.4

#define PERTURB_CHANCE 0.9
#define PERTURB_STEP 0.1

unsigned long long int hash_ull_int_encode(unsigned long int a, unsigned long int b);

#define HASH_GET(ktype, etype, l ,nelem) ({map<ktype, etype>::iterator it=l.begin(); advance(it,nelem); it->second;})

#define LIST_GET(ltype, l ,nelem) ({list<ltype>::iterator it=l.begin(); advance(it,nelem); *it;})

#define NODE_KEY_TYPE unsigned long int
#define GENE_KEY_TYPE unsigned long long int

#define GENE_INSERT(cgene, refhash) (refhash.insert(pair<GENE_KEY_TYPE, Gene *>(hash_ull_int_encode((cgene)->in->id, (cgene)->out->id), (cgene))))

#define NODE_INSERT(cnode, refhash) (refhash.insert(pair<NODE_KEY_TYPE, Node *>((cnode)->id, (cnode))))

using namespace std;

/* Node */

Node::Node(unsigned long int id, int type) {
   this->type=type;
   this->id=id;
}

Node::Node(Node* copy) {
    Gene* current;
    list<Gene*>::iterator g_it;

    this->type=copy->type;
    this->id=copy->id;
}

Node::~Node(void) {
	this->input_genes.clear();
	this->output_genes.clear();
}

void Node::print(void) {
    cout << "node " << this->id << " " << this->type << endl;
}

/* Gene */

Gene::Gene(void) {
	this->enabled = true;
}

void Gene::print(void) {
    cout<< "Gene " << this->innovation << " " << (this->enabled ? "en" : "dis" ) << "abled" << endl;
    cout << "\tweight: " << this->weight << endl;
    if(this->in)
        cout << "\tfrom: " <<  this->in->id << endl;
    if(this->out)
        cout << "\tto: " << this->out->id << endl;
}

Gene::Gene(Gene *gen) {
    this->innovation=gen->innovation;
    this->in=gen->in;
    this->out=gen->out;
    this->weight=gen->weight;
    this->enabled=gen->enabled;
}

/* Genome */

Genome::Genome(unsigned long int input_no, unsigned long int output_no) {
    int i;

    this->node_count=0;
    this->global_innov=0;

    for(i=0;i<output_no;i++) {
        NODE_INSERT(new Node(i,0), this->node_map);
    }
    this->node_count += output_no;

    for(i=0;i<input_no;i++) {
        NODE_INSERT(new Node(output_no + i, 2), this->node_map);
    }
    this->node_count += input_no;
}

Genome::~Genome(void) {
    map<unsigned long long, Gene*>::iterator g_it;
    map<unsigned long int,  Node*>::iterator n_it;

    for(n_it=this->node_map.begin(); n_it!=this->node_map.end(); n_it++)
        delete n_it->second;

    for(g_it=this->gene_map.begin(); g_it!=this->gene_map.end(); g_it++)
        delete g_it->second;

    this->gene_map.clear();
    this->node_map.clear();
}

int Genome::mutate(void) {
    double mrate_link;
    map<unsigned long long int, Gene*>::iterator gene_iter;

    mrate_link=MUTATION_RATE_LINK;

    cout << "begin mutation" <<endl;

    //mutate node, spezza un arco e aggiunge un nodo
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_NODE) 
        this->node_mutate(); 

    cout << "node mutation" << endl;

    //mutate link, aggiunge un link fra due nodi random
    while(mrate_link>0) {
        if(RANDOM_DOUBLE(1)<mrate_link)
            this->link_mutate(false);
        mrate_link=mrate_link-1;
    }

    cout << "mutate link" << endl;

    //mutate link+bias, aggiungi link con input da tutti i nodi di input
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_BIAS)
        this->link_mutate(true); 

    cout << "mutated+bias" << endl;
    
    //mutate point, cambia i pesi
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_CONNECTION) {
        for (gene_iter = this->gene_map.begin(); gene_iter != this->gene_map.end(); gene_iter++)
            gene_iter->second->point_mutate();
    } 
    
    cout << "mutate connection" << endl;

    //enable/disable mutate
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_ENABLE)
        this->enable_disable_mutate(true);

    cout << "enable connection" << endl;

    if(RANDOM_DOUBLE(1)<MUTATION_RATE_DISABLE)
        this->enable_disable_mutate(false);
    cout << "disable connection" << endl;
}

Genome::Genome(Genome *gen) {
    unsigned long int node_key;
    unsigned long long int gene_key;
    Node *node, *val;
    Gene* gene;
    map<unsigned long int,      Node*>::iterator n_it;
    map<unsigned long long int, Gene*>::iterator g_it;

    this->node_count=gen->node_count;
    this->global_innov=gen->global_innov;

    for(n_it=gen->node_map.begin(); n_it!=gen->node_map.end(); n_it++) {
	val = n_it->second;
	node_key = n_it->first;
        node=new Node(node_key, val->type);
        NODE_INSERT(node, this->node_map);
    }

    for(g_it=gen->gene_map.begin(); g_it!=gen->gene_map.end(); g_it++) {
         gene = new Gene(g_it->second);
         gene->in  = node_map[gene->in->id];
         gene->out = node_map[gene->out->id];
         gene_key  = hash_ull_int_encode(gene->in->id, gene->out->id);
	 GENE_INSERT(gene, this->gene_map);
    }
}

void Genome::print() {
    map<unsigned long int,      Node*>::iterator node_it;
    map<unsigned long long int, Gene*>::iterator gene_it;

    for(node_it=this->node_map.begin(); node_it!=this->node_map.end(); node_it++) {
        node_it->second->print();
    }

    for(gene_it=this->gene_map.begin(); gene_it!=this->gene_map.end(); gene_it++) {
        gene_it->second->print();
    }

}

int Genome::node_mutate(void) {
    Node *neuron;
    unsigned long int list_len;
    unsigned long int gene_index;
    map<unsigned long int, Node*>::iterator it;
    Gene *g_orig, *g1,*g2;
    
    //TODO get id, define types
    neuron=new Node(this->node_count,1);

    NODE_INSERT(neuron, this->node_map);
    this->node_count++;

    list_len=this->gene_map.size();
    if(list_len==0)
        return 1;

    gene_index=(unsigned long int) round(RANDOM_DOUBLE(list_len-1)); 
    
    g_orig=HASH_GET(GENE_KEY_TYPE, Gene*, this->gene_map, gene_index);
    
    if(!g_orig->enabled)
        return -1;
    
    g_orig->enabled=false;
    
    g1=new Gene(g_orig);
    g2=new Gene(g_orig);
    
    //g1->copy(g_orig);
    g1->out=neuron;
    g1->weight=1.0;
    //TODO innovation
    g1->innovation=this->next_innovation();
    g1->enabled=true;

    //g2->copy(g_orig);
    g2->in=neuron;
    g2->innovation=this->next_innovation();
    g2->enabled=true;
    
    neuron->input_genes.push_back(g1);
    neuron->output_genes.push_back(g2);

    g_orig->in->output_genes.push_back(g1);
    g_orig->out->input_genes.push_back(g2);

    GENE_INSERT(g1, this->gene_map);
    GENE_INSERT(g1, this->gene_map);

    return 0;
}

int Genome::next_innovation() {
	return this->global_innov++;
}

inline unsigned long long int hash_ull_int_encode(unsigned long from, unsigned long to)
{
	unsigned long long a, b;
	a = from;
	b = to;

	return (a >= b ? (a * a + a + b) : (a + b * b));
}

bool Genome::containslink(Gene *g) {
	return this->gene_map.count(hash_ull_int_encode(g->in->id, g->out->id));
}

int Genome::link_mutate(bool force_bias) {
    unsigned int node_size;
    unsigned long int rand1,rand2;
    Node *n1,*n2,*temp;
    Gene *new_gene;

    node_size=this->node_map.size();

    if(node_size==0) {
        return 1;
    }
    rand1=round(RANDOM_DOUBLE(node_size-1));
    rand2=round(RANDOM_DOUBLE(node_size-1));
   

    n1=this->node_map[rand1];
    n2=this->node_map[rand2];
   

    if((n1->type==n2->type && n1->type!=1) || n1->id == n2->id)
        return 1;
    
    if(n2->type==0) {
        temp=n2;
        n2=n1;
        n1=temp;
    }
    

    new_gene=new Gene();
    new_gene->in=n1;
    new_gene->out=n2;

    if(force_bias) {
        //DO SOMETHING
    }
    
    //todo contains link
    if(this->containslink(new_gene)) return 1;

    new_gene->innovation=this->next_innovation();

    new_gene->weight=RANDOM_DOUBLE(4)-2;

    
    n1->print();
    n2->print();

    GENE_INSERT(new_gene, this->gene_map);

    //this->gene_list.push_back(new_gene);

    n1->output_genes.push_back(new_gene);
    n2->input_genes.push_back(new_gene);


    return 0;
}

int Gene::point_mutate(void) {
    if(RANDOM_DOUBLE(1)<PERTURB_CHANCE)
        this->weight+=(RANDOM_DOUBLE(1)*PERTURB_STEP*2-PERTURB_STEP);
    else
        this->weight=RANDOM_DOUBLE(1)*4-2;
}

int Genome::enable_disable_mutate(bool enable) {
    Gene *selected;
    list<Gene*> candidates;
    map<GENE_KEY_TYPE, Gene*>::iterator it;

    for(it=this->gene_map.begin(); it!=this->gene_map.end();it++) {
        if(it->second->enabled == not enable)
            candidates.push_back(it->second);
    }

    if(candidates.size()>0) {
        selected=LIST_GET(Gene*, candidates, round(RANDOM_DOUBLE(candidates.size())));
        selected->enabled=enable;
    }

}
