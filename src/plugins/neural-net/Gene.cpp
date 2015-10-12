#include <iostream>
#include <robby/dismath.h>
#include <robby/neural-net.h>
#include <robby/neural-net-const.h>

using namespace std;

/* Gene */

Gene::Gene(void) {
    this->enabled = true;
    this->value=0;
    this->weight=RANDOM_DOUBLE(4)-2;
}

Gene::Gene(Gene *gen) {
    this->innovation=gen->innovation;
    this->in=gen->in;
    this->out=gen->out;
    this->value=gen->value;
    this->weight=gen->weight;
    this->enabled=gen->enabled;
}

void Gene::print(void) {
    cout<< "Gene " << this->innovation << " " << (this->enabled ? "en" : "dis" ) << "abled" << endl;
    cout << "\tweight: " << this->weight << endl;
    if(this->in)
        cout << "\tfrom: " <<  this->in->id << endl;
    if(this->out)
        cout << "\tto: " << this->out->id << endl;
}

void Gene::activate(double value) {
    if(this->enabled) {
        this->value=value;
#if 0
/* FIXME Non abbiamo piu' la dfs. */
        this->out->activate(value);
#endif
    }
}

int Gene::point_mutate(void) {
    if(RANDOM_DOUBLE(1)<PERTURB_CHANCE)
        this->weight+=(RANDOM_DOUBLE(1)*PERTURB_STEP*2-PERTURB_STEP);
    else
        this->weight=RANDOM_DOUBLE(1)*4-2;
    
    return 0;
}
