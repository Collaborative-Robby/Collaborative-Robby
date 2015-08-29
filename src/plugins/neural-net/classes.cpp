/* Sample module for collaborative robby */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <iostream>
#include <list>
#include <robby/struct.h>
#include <robby/module.h>
#include <robby/neural-net.h>

#define MUTATION_RATE_NODE 2.5
#define MUTATION_RATE_CONNECTION 2.25
#define MUTATION_RATE_LINK 2.0
#define MUTATION_RATE_BIAS 2.4
#define MUTATION_RATE_ENABLE 2.2
#define MUTATION_RATE_DISABLE 2.4

#define PERTURB_CHANCE 0.9
#define PERTURB_STEP 0.1

#define RANDOM_DOUBLE(max) (((double) rand()/ (double) RAND_MAX)*((double) max))

#define LIST_GET(ltype, l ,nelem) ({list<ltype>::iterator it=l.begin(); advance(it,nelem); *it;})

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

void Node::print(void) {
    cout << "node " << this->id << " " << this->type << endl;
}

/* Gene */

void Gene::print(void) {
    cout<< "Gene " << this->innovation << " enabled: " << this->enabled << endl;
    cout << "\tweight: " << this->weight << endl;
    if(this->in)
        cout << "\tfrom: " <<  this->in->id << endl;
    if(this->out)
        cout << "\tto: " << this->out->id << endl;
}

Gene::Gene(Gene *gen, bool copy_ptrs) {
    this->innovation=gen->innovation;
    if (copy_ptrs) {
	    this->in=gen->in;
	    this->out=gen->out;
    }
    this->weight=gen->weight;
    this->enabled=gen->enabled;
}

/* Genome */

Genome::Genome(int input_no, int output_no) {
    int i;

    this->node_count=0;
    this->global_innov=0;

    for(i=0;i<input_no;i++) {
        this->node_list.push_back(new Node(i,0));
    }
    for(i=0;i<output_no;i++) {
        this->node_list.push_back(new Node(input_no+i, 2));
    }
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

    cout <<"mutated+bias" << endl;
    
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

int Genome::copy(Genome *gen) {
    Node* node;
    Gene* gene;
    list<Node*>::iterator n_it;
    list<Gene*>::iterator g_it;

    this->node_count=gen->node_count;
    this->global_innov=gen->global_innov;

    for(n_it=this->node_list.begin(); n_it!=this->node_list.end(); n_it++) {
        node=new Node((*n_it)->id, (*n_it)->type);
    }

}

void Genome::print() {
    list<Node*>::iterator node_it;
    list<Gene*>::iterator gene_it;

    for(node_it=this->node_list.begin(); node_it!=this->node_list.end(); node_it++)
        (*node_it)->print();

    for(gene_it=this->gene_list.begin(); gene_it!=this->gene_list.end(); gene_it++)
        (*gene_it)->print();

}

int Genome::node_mutate(void) {
    Node *neuron;
    int list_len;
    int gene_index;
    list<Node*>::iterator it;
    Gene *g_orig, *g1,*g2;
    
    //TODO get id, define types
    neuron=new Node(1,1);

    this->node_list.push_back(neuron);

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
    
    g1=new Gene(g_orig, true);
    g2=new Gene(g_orig, true);
    
    cout << "created" << endl;

    //g1->copy(g_orig);
    g1->out=neuron;
    g1->weight=1.0;
    //TODO innovation
    g1->innovation=666;
    g1->enabled=true;

    //g2->copy(g_orig);
    g2->in=neuron;
    g2->innovation=667;
    g2->enabled=true;
    
    neuron->input_genes.push_back(g1);
    neuron->output_genes.push_back(g2);

    g_orig->in->output_genes.push_back(g1);
    g_orig->out->input_genes.push_back(g2);

    this->gene_list.push_back(g1);
    this->gene_list.push_back(g2);

    return 0;
}

int Genome::link_mutate(bool force_bias) {
    int rand1,rand2;
    unsigned int node_size;
    Node *n1,*n2,*temp;
    Gene *new_gene;

    node_size=this->node_list.size();

    if(node_size==0) {
        return 1;
    }
    rand1=round(RANDOM_DOUBLE(node_size-1));
    rand2=round(RANDOM_DOUBLE(node_size-1));
   

    n1=LIST_GET(Node*, this->node_list, rand1);
    n2=LIST_GET(Node*, this->node_list, rand2);
   

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

    //new_gene->innovation=get_innovation();
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
        if((*it)->enabled== not enable)
            candidates.push_back(*it);
    }

    if(candidates.size()>0) {
        selected=LIST_GET(Gene*, candidates, round(RANDOM_DOUBLE(candidates.size())));
        selected->enabled=enable;
    }

}
