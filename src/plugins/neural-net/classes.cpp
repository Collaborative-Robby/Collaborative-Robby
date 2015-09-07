/* Sample module for collaborative robby */
#include <map>
#include <list>
#include <iostream>
#include <fstream>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/types.h>

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

/* Utilities */

inline unsigned long long int hash_ull_int_encode(unsigned long from, unsigned long to)
{
    unsigned long long a, b;
    a = from;
    b = to;

    return (a >= b ? (a * a + a + b) : (a + b * b));
}

double sigmoid(double input) {
    //TODO: controlla coefficente sigmoid
#define BETA -4.9
    return 1/(1+pow(M_E, -input*BETA));
}

bool exist_genome_file(char *dir, int fileno) {
	char *path;
	int rval;
	FILE *f;

	rval = asprintf(&path, "%s/%d.%s", dir, fileno, GENOME_EXT);
	if (rval < 0) {
		perror("asprintf exist_genome_file");
		return false;
	}

	f = fopen(path, "r");
	if (!f) {
		free(path);
		return false;
	}
	free(path);
	return true;
}

/* Node */

Node::Node(unsigned long int id, int type) {
    this->type=type;
    this->id=id;
    this->value=0;
}

Node::Node(Node* copy) {
    Gene* current;
    list<Gene*>::iterator g_it;

    this->type=copy->type;
    this->value=copy->value;
    this->id=copy->id;
}

Node::~Node(void) {
    this->input_genes.clear();
    this->output_genes.clear();
}

void Node::print(void) {
    cout << "node " << this->id << " " << this->type << endl;
}

void Node::activate(double input) {
    int i;
    list<Gene*>::iterator it;
    this->value=0;
    this->activate_count++;
    if(this->activate_count>=MAX_ACTIVATIONS) {
        cout << "max activations" << endl;
        return;
    }
    if(this->type==NODE_TYPE_INPUT)
        this->value=input;
    else {
        for(it=this->input_genes.begin(); it!=this->input_genes.end(); it++) {
            value+=(*it)->weight*(*it)->value;
        }
    }
    this->value=sigmoid(this->value);
    for(it=this->output_genes.begin(); it!=this->output_genes.end(); it++) {
        (*it)->activate(this->value);
    }
}

/* Gene */

Gene::Gene(void) {
    this->enabled = true;
    this->value=0;
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
        this->out->activate(value);
    }
}

int Gene::point_mutate(void) {
    if(RANDOM_DOUBLE(1)<PERTURB_CHANCE)
        this->weight+=(RANDOM_DOUBLE(1)*PERTURB_STEP*2-PERTURB_STEP);
    else
        this->weight=RANDOM_DOUBLE(1)*4-2;
}

/* Genome */
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

Genome::Genome(unsigned long int input_no, unsigned long int output_no) {
    int i;
    Node *curr;

    this->node_count=0;
    this->global_innov=0;

    for(i=0;i<output_no;i++) {
        curr=new Node(i, NODE_TYPE_OUTPUT);
        NODE_INSERT(curr, this->node_map);
    }
    this->node_count += output_no;

    for(i=0;i<input_no;i++) {
        curr=new Node(output_no+i, NODE_TYPE_INPUT);
        NODE_INSERT(curr, this->node_map);
    }
    this->node_count += input_no;
}

Genome::Genome(char *dir, int fileno) {
	map <GENE_KEY_TYPE, Gene *>::iterator g_it;
	map <NODE_KEY_TYPE, Node *>::iterator n_it;
	char *path;
	FILE *f;
	int rval, i;
	int node_size, gene_size;
	int node_id, node_type;
	Node *cur_node;
	Gene *cur_gene;
	int idin = -1, idout = -1;

	rval = asprintf(&path, "%s/%d.%s", dir, fileno, GENOME_EXT);
	if (rval < 0) {
		perror("asprintf genome load");
		return;
	}

	f = fopen(path, "r");
	if (!f) {
		perror("genome load");
		return;
	}
	cout << "loading genome from " << path << endl;

	fscanf(f, "%d %d\n{\n", &node_size, &gene_size);

	for (i =0 ; i < node_size; i ++) {
		fscanf(f, "%d %d\n", &node_id, &node_type);
		cur_node = new Node(node_id, node_type);
		NODE_INSERT(cur_node, this->node_map);
	}
	this->node_count = node_size;

	fscanf(f, "}\n{\n");

	for (i =0 ; i < gene_size; i ++) {
		cur_gene = new Gene();
		fscanf(f, "%d -> %d: %d %d %lf\n", &idin, &idout,
			&cur_gene->innovation, &cur_gene->enabled,
			&cur_gene->weight);
		if (idin >= 0)
			cur_gene->in = this->node_map[idin];
		if (idout >= 0)
			cur_gene->out = this->node_map[idout];
		GENE_INSERT(cur_gene, this->gene_map);
	}
	this->global_innov= gene_size;
	fclose(f);

	free(path);
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

    list_len=this->gene_map.size();
    if(list_len==0)
        return 1;

    gene_index=(unsigned long int) round(RANDOM_DOUBLE(list_len-1)); 

    g_orig=HASH_GET(GENE_KEY_TYPE, Gene*, this->gene_map, gene_index);

    if(!g_orig->enabled)
        return -1;

    neuron=new Node(this->node_count,NODE_TYPE_HIDDEN);

    NODE_INSERT(neuron, this->node_map);
    this->node_count++;

    g_orig->enabled=false;

    g1=new Gene(g_orig);
    g2=new Gene(g_orig);

    g1->out=neuron;
    g1->weight=1.0;
    g1->innovation=this->next_innovation();
    g1->enabled=true;

    g2->in=neuron;
    g2->innovation=this->next_innovation();
    g2->enabled=true;

    cout << "new node " << neuron->id << " from " << g1->in->id << " to " << g2->out->id << endl;

    neuron->input_genes.push_back(g1);
    neuron->output_genes.push_back(g2);

    g_orig->in->output_genes.push_back(g1);
    g_orig->out->input_genes.push_back(g2);

    GENE_INSERT(g1, this->gene_map);
    GENE_INSERT(g2, this->gene_map);

    return 0;
}

int Genome::next_innovation() {
    return this->global_innov++;
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


    if((n1->type==n2->type && n1->type!=NODE_TYPE_HIDDEN) || n1->id == n2->id)
        return 1;

    if(n2->type==NODE_TYPE_INPUT) {
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
    if(this->containslink(new_gene)) {
        delete new_gene;
        return 1;
    }

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

int Genome::enable_disable_mutate(bool enable) {
    Gene *selected;
    list<Gene*> candidates;
    map<GENE_KEY_TYPE, Gene*>::iterator it;

    for(it=this->gene_map.begin(); it!=this->gene_map.end();it++) {
        if(it->second->enabled == not enable)
            candidates.push_back(it->second);
    }

    if(candidates.size()>0) {
        selected=LIST_GET(Gene*, candidates, round(RANDOM_DOUBLE(candidates.size()-1)));
        selected->enabled=enable;
    }

    candidates.clear();

}

int Genome::activate(char **view, int viewradius) {
    int square_size=SQUARE_AREA;
    int i,j;
    long unsigned int key;
    long unsigned int max_id;
    double max;
    Node* in_node;

    for(key=0; key<this->node_count; key++) {
        this->node_map[key]->activate_count=0;
    }

    for(i=0; i<SQUARE_SIDE; i++) {
        for(j=0; j<SQUARE_SIDE; j++) {
            key=i*(SQUARE_SIDE)+j+POSSIBLE_MOVES;
            in_node=this->node_map[key];
            in_node->activate(view[i][j]);
        }
    }

    max=(-DBL_MAX);
    for(key=0; key<POSSIBLE_MOVES; key++) {
        if(max<this->node_map[key]->value) {
            max=this->node_map[key]->value;
            max_id=key;
        }
    }

    cout << "best move is: " << max_id << " value " << max << endl;

    return max_id;

    //TODO prendi gli output
}

int Genome::save_to_file(char *dir, int fileno) {
	map <GENE_KEY_TYPE, Gene *>::iterator g_it;
	map <NODE_KEY_TYPE, Node *>::iterator n_it;
	int idin = -1, idout = -1;
	char *path;
	FILE *f;
	DIR *d;
	int rval;

	d = opendir(dir);
	if (!d) {
		mkdir(dir, 0755);
		d = opendir(dir);
		if (!d) {
			perror("mkdir");
			return -1;
		}
	}

	rval = asprintf(&path, "%s/%d.%s", dir, fileno, GENOME_EXT);
	if (rval < 0)
		return rval;

	f = fopen(path, "w");
	if (!f) {
		perror("genome save");
		return -1;
	}

	fprintf(f, "%d %d\n{\n", this->node_map.size(), this->gene_map.size());

	for (n_it=this->node_map.begin(); n_it != this->node_map.end(); n_it++)
		fprintf(f, "%d %d\n", n_it->second->id, n_it->second->type);

	fprintf(f, "}\n{\n");

	for (g_it=this->gene_map.begin(); g_it != this->gene_map.end(); g_it++) {
		if (g_it->second->in)
			idin = g_it->second->in->id;
		if (g_it->second->out)
			idout = g_it->second->out->id;

		fprintf(f, "%d -> %d: %d %d %f\n", idin, idout,
		        g_it->second->innovation, g_it->second->enabled,
			g_it->second->weight);
	}
	fprintf(f, "}\n");

	fclose(f);

	free(path);
}
