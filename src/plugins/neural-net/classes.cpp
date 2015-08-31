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

#define LIST_GET(ltype, l ,nelem) ({list<ltype>::iterator it=l.begin(); advance(it,nelem); *it;})

using namespace std;

/* Node */

Node::Node(int id, int type) {
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

Genome::Genome(int input_no, int output_no) {
    int i;

    this->node_count=0;
    this->global_innov=0;

    for(i=0;i<input_no;i++) {
        this->node_map.insert(std::pair<int, Node *>(i, new Node(i,0)));
    }
    this->node_count += input_no;

    for(i=0;i<output_no;i++) {
        this->node_map.insert(std::pair<int, Node *>(input_no+i, new Node(input_no + i,2)));
    }
    this->node_count += output_no;
}

Genome::~Genome(void) {
    list<Gene*>::iterator g_it;
    map<int, Node*>::iterator n_it;

    for(n_it=this->node_map.begin(); n_it!=this->node_map.end(); n_it++)
        delete n_it->second;

    for(g_it=this->gene_list.begin(); g_it!=this->gene_list.end(); g_it++)
        delete *g_it;

    this->gene_list.clear();
    this->node_map.clear();
}

int Genome::mutate(void) {
    list<Gene*>::iterator gene_iter;
    double mrate_link;

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
        for (gene_iter = this->gene_list.begin(); gene_iter != this->gene_list.end(); gene_iter++)
            (*gene_iter)->point_mutate();
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
    int key;
    Node *node, *val;
    Gene* gene;
    map<int, Node*>::iterator n_it;
    list<Gene*>::iterator g_it;

    this->node_count=gen->node_count;
    this->global_innov=gen->global_innov;

    for(n_it=gen->node_map.begin(); n_it!=gen->node_map.end(); n_it++) {
	key = n_it->first;
	val = n_it->second;
        node=new Node(key, val->type);
	this->node_map.insert(pair<int, Node *>(key, node));
    }

    for(g_it=gen->gene_list.begin(); g_it!=gen->gene_list.end(); g_it++) {
         gene = new Gene(*g_it);
         gene->in  = node_map[gene->in->id];
         gene->out = node_map[gene->out->id];
         this->gene_list.push_back(gene);
    }
}

void Genome::print() {
    map<int, Node*>::iterator node_it;
    list<Gene*>::iterator gene_it;

    for(node_it=this->node_map.begin(); node_it!=this->node_map.end(); node_it++) {
        node_it->second->print();
    }

    for(gene_it=this->gene_list.begin(); gene_it!=this->gene_list.end(); gene_it++) {
        (*gene_it)->print();
    }

}

int Genome::node_mutate(void) {
    Node *neuron;
    int list_len;
    int gene_index;
    map<int, Node*>::iterator it;
    Gene *g_orig, *g1,*g2;
    
    //TODO get id, define types
    neuron=new Node(this->node_count,1);

    this->node_map.insert(pair<int, Node*>(node_count, neuron));
    this->node_count++;

    list_len=this->gene_list.size();
    if(list_len==0)
        return 1;

    gene_index=(int) round(RANDOM_DOUBLE(list_len-1)); 
    
    cout << "d1 " << gene_index << endl;

    g_orig=LIST_GET(Gene*, this->gene_list, gene_index);
    cout << "d2\n";
    
    if(!g_orig->enabled)
        return -1;
    
    g_orig->enabled=false;
    
    g1=new Gene(g_orig);
    g2=new Gene(g_orig);
    
    cout << "created" << endl;

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

    this->gene_list.push_back(g1);
    this->gene_list.push_back(g2);

    return 0;
}

int Genome::next_innovation() {
	return this->global_innov++;
}

int Genome::link_mutate(bool force_bias) {
    int rand1,rand2;
    unsigned int node_size;
    Node *n1,*n2,*temp;
    Gene *new_gene;

    node_size=this->node_map.size();

    if(node_size==0) {
        return 1;
    }
    rand1=round(RANDOM_DOUBLE(node_size-1));
    rand2=round(RANDOM_DOUBLE(node_size-1));
   

    //n1=LIST_GET(Node*, this->node_list, rand1);
    n1=this->node_map[rand1];
    //n2=LIST_GET(Node*, this->node_list, rand2);
    n2=this->node_map[rand2];
   

    if(n1->type==n2->type && n1->type==0)
        return 1;
    
    if(n2->type==1) {
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
    //if(containslink) return 

    new_gene->innovation=this->next_innovation();

    new_gene->weight=RANDOM_DOUBLE(4)-2;

    
    n1->print();
    n2->print();

    this->gene_list.push_back(new_gene);

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
    list<Gene*>::iterator it;

    for(it=this->gene_list.begin(); it!=this->gene_list.end();it++) {
        if((*it)->enabled == not enable)
            candidates.push_back(*it);
    }

    if(candidates.size()>0) {
        selected=LIST_GET(Gene*, candidates, round(RANDOM_DOUBLE(candidates.size())));
        selected->enabled=enable;
    }

}
