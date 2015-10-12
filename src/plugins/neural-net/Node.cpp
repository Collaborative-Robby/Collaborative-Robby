#include <iostream>
#include <robby/neural-net.h>
#include <robby/neural-net-utils.h>

using namespace std;
/* Node */

Node::Node(unsigned long int id, int type, unsigned long int l_num, unsigned long int l_den) {
    this->type=type;
    this->id=id;
    this->value=0;
    this->active_in_genes = 0;
    this->level_numerator=l_num;
    this->level_denom=l_den;
}

Node::Node(Node* copy) {
    list<Gene*>::iterator g_it;

    this->type=copy->type;
    this->value=copy->value;
    this->id=copy->id;
    this->level_numerator=copy->level_numerator;
    this->level_denom=copy->level_denom;

    this->active_in_genes = copy->active_in_genes;
}

Node::~Node(void) {
    this->input_genes.clear();
    this->output_genes.clear();
}

void Node::print(void) {
    cout << "node " << this->id << " " << this->type << endl;
}

void Node::activate(double input) {
    list<Gene*>::iterator it;
    this->activate_count++;

    this->value=0;
    if(this->type==NODE_TYPE_INPUT) {
        this->value=input;
    } else {
        /* Check if we are over the relative max_activation */
        if(this->activate_count >= this->active_in_genes + MAX_REACTIVATIONS)
            return;

        for(it=this->input_genes.begin(); it!=this->input_genes.end(); it++) {
            value+=(*it)->weight*(*it)->value;
        }
        this->value=sigmoid(this->value);
    }
    for(it=this->output_genes.begin(); it!=this->output_genes.end(); it++) {
        (*it)->activate(this->value);
    }
}
