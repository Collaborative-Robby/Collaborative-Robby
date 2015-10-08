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

#define MUTATION_RATE_NODE 0.75
#define MUTATION_RATE_CONNECTION 0.75
#define MUTATION_RATE_LINK 2.0
#define MUTATION_RATE_BIAS 0.6
#define MUTATION_RATE_ENABLE 0.4
#define MUTATION_RATE_DISABLE 0.6

#define SAME_SPECIES_TRESHOLD 1.0
#define COEFFICIENT_DELTA_WEIGHT 0.4
#define COEFFICIENT_EXCESS_GENES 2.0
#define COEFFICIENT_DISJOINT_GENES 2.0

#define SPECIES_STALE_TRESHOLD 60

#define CROSSOVER_CHANCE 0.75

#define PERTURB_CHANCE 0.9
#define PERTURB_STEP 0.05

#define ROBBY_NNET_POSITION false

unsigned long long int hash_ull_int_encode(unsigned long int a, unsigned long int b);


long unsigned int global_innovation=0;

using namespace std;

/* Utilities */

static inline int compare_level(Node* n1, Node *n2) {
    unsigned long long int v1,v2;
    v1=n1->level_numerator*n2->level_denom;
    v2=n2->level_numerator*n1->level_denom;
    if(v1<v2)
        return -1;
    if(v1>v2)
        return 1;
        
    return 0;
}

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

bool cmp_desc_genomes(Genome *g1, Genome *g2)
{
//	return (g1->fitness < g2->fitness);
	return (g1->fitness > g2->fitness);
}

inline double delta_species(Genome *g1, Genome *g2)
{
	int found;
	double delta_excess = 0, delta_disjoint = 0, delta_weight = 0;

    long unsigned int gene_count,equal_genes_count,excess_genes,disjoint_genes;
	map<GENE_KEY_TYPE, Gene*>::iterator g_it;
	Gene *equal_gene;
	Genome *g, *oth_g;

    gene_count=0;
    excess_genes=0;
    disjoint_genes=0;
    equal_genes_count=0;
    found=0;

	g = g1;
	oth_g = g2;

	if (g1->max_innov < g2->max_innov) {
		g = g2;
		oth_g = g1;
	}

	gene_count = g1->gene_map.size();

	if (gene_count < g2->gene_map.size())
		gene_count = g2->gene_map.size();

	for (g_it=g->gene_map.begin(); g_it!=g->gene_map.end();g_it++) {
		found = oth_g->gene_map.count(g_it->first) &&
		(oth_g->gene_map[g_it->first]->innovation ==
		g_it->second->innovation);

		if (!found && g_it->second->innovation > oth_g->max_innov)
			excess_genes++;

		else if (!found)
			disjoint_genes++;

		/* XXX check the same innovation number */
		else {
			equal_gene = oth_g->gene_map[g_it->first];

			equal_genes_count++;

			delta_weight += fabs(g_it->second->weight - equal_gene->weight);
		}
	}

	for (g_it=oth_g->gene_map.begin(); g_it!=oth_g->gene_map.end();g_it++) {
		/* Excesses genes are already counted. */
		found = g->gene_map.count(g_it->first) &&
		        (g->gene_map[g_it->first]->innovation == g_it->second->innovation);

		if (!found)
			disjoint_genes++;
	}


	delta_excess = COEFFICIENT_EXCESS_GENES * (double) excess_genes / (double) gene_count;
	delta_disjoint = COEFFICIENT_DISJOINT_GENES * (double) disjoint_genes / (double) gene_count;
	if (equal_genes_count)
		delta_weight = COEFFICIENT_DELTA_WEIGHT * delta_weight / (double) equal_genes_count;

	if (!gene_count)
		delta_excess = delta_disjoint = 0;

	double x = (delta_excess + delta_disjoint + delta_weight);
	return x;
}

int remove_weak_species(list <Species *> *sl, long unsigned int couplenum)
{
	list <Species *>::iterator s_it;
    Genome* cgenome;
	double tot_fitness = 0, breed = 0, max_fitness=0;

    for (s_it = sl->begin(); s_it != sl->end(); s_it++) {
        
		(*s_it)->genomes.sort(cmp_desc_genomes);
		
		cgenome = LIST_GET(Genome*, ((*s_it)->genomes), 0);
        
		if (cgenome->fitness > (*s_it)->top_fitness)
			(*s_it)->top_fitness = cgenome->fitness;
        
        tot_fitness += (*s_it)->calculate_avg_fitness();

        if(max_fitness < (*s_it)->top_fitness)
            max_fitness=(*s_it)->top_fitness;
    }

	for (s_it = sl->begin(); s_it != sl->end();) {
		breed = floor(((*s_it)->average_fitness / tot_fitness) *
		              (double) couplenum)-1;

		if (breed < 1 && sl->size()>1) {
            delete (*s_it);
			s_it = sl->erase(s_it);
		}
        else {
            s_it++;
        }
	}
    return 0;
}

int remove_stale_species(list <Species *> *sl)
{
	Genome *cgenome;
	double max_fitness = 0;
	list <Species *>::iterator s_it;

	for (s_it = sl->begin(); s_it != sl->end(); s_it++) {
		(*s_it)->genomes.sort(cmp_desc_genomes);

		cgenome = LIST_GET(Genome*, ((*s_it)->genomes), 0);
		if (cgenome->fitness > (*s_it)->top_fitness) {
			(*s_it)->top_fitness = cgenome->fitness;

			(*s_it)->staleness = 0;
		} else {
			(*s_it)->staleness++;
		}

		if (max_fitness < (*s_it)->top_fitness)
			max_fitness = (*s_it)->top_fitness;

	}

	for (s_it = sl->begin(); s_it != sl->end();) {
		if ((*s_it)->staleness >= SPECIES_STALE_TRESHOLD &&
		    (*s_it)->top_fitness < max_fitness) {
            delete (*s_it);
			s_it = sl->erase(s_it);
		}
        else {
            s_it++;
        }
	}   
    return 0;
}

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

/* Genome */
long unsigned int genome_count=0;

Genome::Genome(Genome *gen) {
    this->id=genome_count++;
    this->copy(gen);
}

void Genome::copy(Genome *gen) {
    unsigned long int node_key;
    Node *node, *val;
    Gene* gene;
    map<unsigned long int,      Node*>::iterator n_it;
    map<unsigned long long int, Gene*>::iterator g_it;

    this->node_count=gen->node_count;
    this->max_innov=gen->max_innov;

    for(n_it=gen->node_map.begin(); n_it!=gen->node_map.end(); n_it++) {
        val = n_it->second;
        node_key = n_it->first;
        node=new Node(node_key, val->type, val->level_numerator, val->level_denom);
        NODE_INSERT(node, this->node_map);

	this->insert_level_list(node);
    }

    for(g_it=gen->gene_map.begin(); g_it!=gen->gene_map.end(); g_it++) {
        gene = new Gene(g_it->second);
        gene->in  = node_map[gene->in->id];
        gene->out = node_map[gene->out->id];

        gene->in->output_genes.push_back(gene);
        gene->out->input_genes.push_back(gene);

        GENE_INSERT(gene, this->gene_map);

	if (gene->enabled)
		gene->out->active_in_genes++;
    }
}

Genome::Genome(unsigned long int input_no, unsigned long int output_no, unsigned long int robbynum) {
    unsigned long int i,j;
    Node *curr;
    Gene *cgene;
    list<Node*> in_list, out_list;
    list <Node*>::iterator inner_level_it;
    list < list <Node*> >::iterator l_it;

    this->id=genome_count++;
    this->node_count=0;
    this->max_innov=0;
    this->fitness=0.0;

    for(i=0;i<output_no;i++) {
        curr=new Node(i, NODE_TYPE_OUTPUT,1,1);
        NODE_INSERT(curr, this->node_map);
        out_list.push_back(curr);
    }
    this->node_count += output_no;

    for(i=0;i<input_no;i++) {
        curr=new Node(output_no+i, NODE_TYPE_INPUT, 0,1);
        NODE_INSERT(curr, this->node_map);
        in_list.push_back(curr);
    }
    this->node_count += input_no;

    if (ROBBY_NNET_POSITION) {
        curr=new Node(output_no+input_no, NODE_TYPE_INPUT,0,1);
        NODE_INSERT(curr, this->node_map);
        in_list.push_back(curr);

        curr=new Node(output_no+input_no+1, NODE_TYPE_INPUT,0,1);
        NODE_INSERT(curr, this->node_map);
        in_list.push_back(curr);

	this->node_count+=2;
    }

    /* Bias node */
    curr = new Node(this->node_count, NODE_TYPE_INPUT,0,1);
    NODE_INSERT(curr, this->node_map);
    in_list.push_back(curr);
    this->node_count++;
    
    for(i=1; i<robbynum; i++) {
    #ifndef KNOWN_MAP
        for(j=0; j<input_no; j++) {
            curr=new Node(this->node_count, NODE_TYPE_INPUT,0,1);
            NODE_INSERT(curr, this->node_map);
            in_list.push_back(curr);
            this->node_count++;
        }
    #endif
        if(ROBBY_NNET_POSITION) {
            curr=new Node(this->node_count, NODE_TYPE_INPUT,0,1);
            NODE_INSERT(curr, this->node_map);
            in_list.push_back(curr);
            this->node_count++;
            
            curr=new Node(this->node_count, NODE_TYPE_INPUT,0,1);
            NODE_INSERT(curr, this->node_map);
            in_list.push_back(curr);
            this->node_count++;
        }
    #ifndef KNOWN_MAP
        /*old move node*/
        curr=new Node(this->node_count, NODE_TYPE_INPUT,0,1);
        NODE_INSERT(curr, this->node_map);
        in_list.push_back(curr);
        this->node_count++;
    #endif
    }

    this->level_list.push_back(in_list);
    this->level_list.push_back(out_list);

    for (l_it = this->level_list.begin(); l_it!=this->level_list.end(); l_it++)
        for (inner_level_it = l_it->begin(); inner_level_it != l_it->end(); inner_level_it++)
             (*inner_level_it)->level_it = l_it;

    for(i=output_no;i<node_count;i++) {
	    for (j = 0; j < output_no; j++) {
		    cgene = new Gene();
            cgene->innovation=0;

		    cgene->in  = this->node_map[i];
		    cgene->out = this->node_map[j];

		    this->node_map[i]->output_genes.push_back(cgene);
		    this->node_map[j]->input_genes.push_back(cgene);

		    cgene->out->active_in_genes++;

		    GENE_INSERT(cgene, this->gene_map);
	    }
    }

    this->mutate();
}

//TODO sistema
Genome::Genome(char *dir, int fileno) {
	map <GENE_KEY_TYPE, Gene *>::iterator g_it;
	map <NODE_KEY_TYPE, Node *>::iterator n_it;
	char *path;
	FILE *f;
	int rval, i;
	int node_size, gene_size;
	int node_id, node_type;
    int tmp;
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
        //FIXME
		cur_node = new Node(node_id, node_type,1,0);
		NODE_INSERT(cur_node, this->node_map);
	}
	this->node_count = node_size;

	fscanf(f, "}\n{\n");

	for (i =0 ; i < gene_size; i ++) {
		cur_gene = new Gene();
		fscanf(f, "%d -> %d: %lu %d %lf\n", &idin, &idout,
			&cur_gene->innovation, &tmp,
			&cur_gene->weight);
        cur_gene->enabled=tmp;
		if (idin >= 0)
			cur_gene->in = this->node_map[idin];
		if (idout >= 0)
			cur_gene->out = this->node_map[idout];
		GENE_INSERT(cur_gene, this->gene_map);
        if(cur_gene->innovation>global_innovation)
            global_innovation=cur_gene->innovation+1;
        if(cur_gene->innovation>this->max_innov)
            this->max_innov=cur_gene->innovation;
	}
	fclose(f);

	free(path);

	/* XXX active_in_count update */
}

Genome::Genome(Species *s) {
    Genome *g1,*g2;
    long unsigned int r;

    this->id=genome_count++;
    this->max_innov=0;
    this->fitness=0;

    if(RANDOM_DOUBLE(1)<CROSSOVER_CHANCE) {
        r=(long unsigned int) round(RANDOM_DOUBLE(s->genomes.size()-1));
        g1=LIST_GET(Genome*, s->genomes, r);
        r=(long unsigned int) round(RANDOM_DOUBLE(s->genomes.size()-1));
        g2=LIST_GET(Genome*, s->genomes, r);
        this->crossover(g1,g2);
    }
    else {
        r=(long unsigned int) round(RANDOM_DOUBLE(s->genomes.size()-1));
        g1=LIST_GET(Genome*, s->genomes, r);
        this->copy(g1);
    }
    
    this->mutate();
}

int Genome::insert_gene(Gene *g) {
    Gene *new_gene;
    long unsigned int id_in, id_out;
    Node *n1,*n2;
    
    id_in=g->in->id;
    id_out=g->out->id;
    
    if(!this->node_map.count(id_in)) {
        n1=new Node(g->in);
        NODE_INSERT(n1, this->node_map);

	this->insert_level_list(n1);
    }
    else {
        n1=this->node_map[id_in];
    }
    
    if(!this->node_map.count(id_out)) {
        n2=new Node(g->out);
        NODE_INSERT(n2, this->node_map);
	this->insert_level_list(n2);
    }
    else {
        n2=this->node_map[id_out];
    }
    
    if(compare_level(n1,n2)>=0) {
	    return 0;
    }

    new_gene=new Gene(g);
    new_gene->in=n1;
    new_gene->out=n2;
    n1->output_genes.push_back(new_gene);
    n2->input_genes.push_back(new_gene);

    /* Update activation count */
    new_gene->out->active_in_genes++;

    GENE_INSERT(new_gene, this->gene_map);
    if(new_gene->innovation>this->max_innov)
        this->max_innov=new_gene->innovation;

    return 0;
}

Genome::Genome(Genome *g1, Genome *g2){
    this->id=genome_count++;
    this->crossover(g1, g2);
    this->mutate();
}

void Genome::insert_level_list(Node *n) {
	list <Node *> l;
	list < list <Node *> >::iterator l_it;

	if (this->level_list.size() < 2) {
		/* Input list */
		this->level_list.push_back(l);
		/* Output list */
		this->level_list.push_back(l);
	}
	if (n->type == NODE_TYPE_INPUT) {
		l_it = this->level_list.begin();
		n->level_it = l_it;
		l_it->push_back(n);
	}

	else if (n->type == NODE_TYPE_OUTPUT) {
		l_it = this->level_list.end();
		/* Last element */
		l_it--;

		n->level_it = l_it;
		l_it->push_back(n);
	}
	else {
		l_it = this->level_list.begin();
		l_it++;

		/* Advance the list to our level */
		while(compare_level(*(l_it->begin()), n) < 0)
			l_it++;

		n->level_it = l_it;
		if(compare_level(*(l_it->begin()), n) == 0) {
			/* Push the node in the existent level */
			l_it->push_back(n);
		} else {
			/* Create a new level and insert the node in that */
			l.push_back(n);
			this->level_list.insert(l_it, l);
		}
	}
}

void Genome::crossover(Genome *rg1, Genome *rg2){
    map<unsigned long long, Gene*>::iterator g_it;
    map<NODE_KEY_TYPE, Node*>::iterator n_it;
    long unsigned int id_in,id_out;
    Genome *g1, *g2;
    Node *new_n;

	if (round(RANDOM_DOUBLE(1))) {
		g1 = rg2;
		g2 = rg1;
	} else {
		g1 = rg1;
		g2 = rg2;
	}
    
    if(g1==g2) {
        this->copy(g1);
        return;
    }
    
    for(n_it=g1->node_map.begin(); n_it!=g1->node_map.end(); n_it++) {
        if(n_it->second->type!=NODE_TYPE_HIDDEN) {
            new_n=new Node(n_it->second);

	    new_n->active_in_genes = 0;
            NODE_INSERT(new_n, this->node_map);

	    this->insert_level_list(new_n);
        }
    }

    for(g_it=g1->gene_map.begin(); g_it!=g1->gene_map.end(); g_it++) {
        id_in=g_it->second->in->id;
        id_out=g_it->second->out->id;
        
        if(!g2->gene_map.count(hash_ull_int_encode(id_in, id_out))){
            this->insert_gene(g_it->second);
        }
        else {
            if(round(RANDOM_DOUBLE(1))) {
                this->insert_gene(g_it->second);
            }
            else {
                this->insert_gene(g2->gene_map[hash_ull_int_encode(id_in, id_out)]);
            }
        }
    }
    
    for(g_it=g2->gene_map.begin(); g_it!=g2->gene_map.end(); g_it++) {
        id_in=g_it->second->in->id;
        id_out=g_it->second->out->id;
        
        if(!this->gene_map.count(hash_ull_int_encode(id_in, id_out))){
            this->insert_gene(g_it->second);
        }
    }

    this->node_count=this->node_map.size();
}

Genome::~Genome(void) {
    map<unsigned long long, Gene*>::iterator g_it;
    map<unsigned long int,  Node*>::iterator n_it;
    list< list <Node*> >::iterator lev_it;

    for(n_it=this->node_map.begin(); n_it!=this->node_map.end(); n_it++)
        delete n_it->second;

    for(g_it=this->gene_map.begin(); g_it!=this->gene_map.end(); g_it++)
        delete g_it->second;

    this->gene_map.clear();
    this->node_map.clear();
    for(lev_it=this->level_list.begin(); lev_it!=this->level_list.end(); lev_it++) {
        lev_it->clear();
    }
    this->level_list.clear();
}

int Genome::mutate(void) {
    double mrate_link;
    map<unsigned long long int, Gene*>::iterator gene_iter;

    mrate_link=MUTATION_RATE_LINK;

    //mutate node, spezza un arco e aggiunge un nodo
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_NODE) 
        this->node_mutate(); 

    //mutate link, aggiunge un link fra due nodi random
    while(mrate_link>0) {
        if(RANDOM_DOUBLE(1)<mrate_link)
            this->link_mutate(false);
        mrate_link=mrate_link-1;
    }

    //mutate link+bias, aggiungi link con input da tutti i nodi di input
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_BIAS)
        this->link_mutate(true); 

    //mutate point, cambia i pesi
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_CONNECTION) {
        for (gene_iter = this->gene_map.begin(); gene_iter != this->gene_map.end(); gene_iter++)
            gene_iter->second->point_mutate();
    } 

    //enable/disable mutate
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_ENABLE)
        this->enable_disable_mutate(true);

    if(RANDOM_DOUBLE(1)<MUTATION_RATE_DISABLE)
        this->enable_disable_mutate(false);
    
    return 0;
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

static inline void get_level_num(long unsigned int n1, long unsigned int d1, long unsigned int n2, long unsigned int d2, long unsigned int *new_n, long unsigned int *new_d) {
   if(d1>=d2) {
        *new_d=d1*2;
        *new_n=(n2*(d1/d2))+n1;
   }
   else {
        *new_d=d2*2;
        *new_n=(n1*(d2/d1))+n2;
   }
}

int Genome::node_mutate(void) {
    Node *neuron;
    unsigned long int list_len;
    unsigned long int gene_index;
    unsigned long int n1,n2,d1,d2,new_n,new_d;
    list <Node*> new_l;
    map<unsigned long int, Node*>::iterator it;
    Node *tmp1,*tmp2;
    list< list<Node*> >::iterator lv_it;
    Gene *g_orig, *g1,*g2;

    list_len=this->gene_map.size();
    if(list_len==0)
        return 1;

    gene_index=(unsigned long int) round(RANDOM_DOUBLE(list_len-1)); 

    g_orig=HASH_GET(GENE_KEY_TYPE, Gene*, this->gene_map, gene_index);

    if(!g_orig->enabled)
        return -1;

    //TODO sistema livello e lista
    n1=g_orig->in->level_numerator;
    d1=g_orig->in->level_denom;

    n2=g_orig->out->level_numerator;
    d2=g_orig->out->level_denom;
    
    lv_it=g_orig->out->level_it;
    lv_it--;

    tmp1=(*(*lv_it).begin());
    tmp2=g_orig->in;
    //if the input level and the level preceding output are the same, create a new level
    if(compare_level(tmp1,tmp2)==0) {
    
        get_level_num(n1,d1,n2,d2,&new_n, &new_d); 

        neuron=new Node(this->node_count,NODE_TYPE_HIDDEN, new_n, new_d);
        new_l.push_back(neuron);
        this->level_list.push_back(new_l);
    }
    //else choose the level preceding output
    else {
        new_n=tmp1->level_numerator;
        new_d=tmp1->level_denom;
        neuron=new Node(this->node_count,NODE_TYPE_HIDDEN,new_n, new_d);
        lv_it->push_back(neuron);
    }
    NODE_INSERT(neuron, this->node_map);
    this->node_count++;

    g_orig->enabled=false;
    g_orig->in->output_genes.remove(g_orig);
    g_orig->out->input_genes.remove(g_orig);

    g1=new Gene(g_orig);
    g2=new Gene(g_orig);

    g1->out=neuron;
    g1->weight=1.0;
    g1->innovation=next_innovation();
    g1->enabled=true;

    g2->in=neuron;
    g2->innovation=next_innovation();
    g2->enabled=true;
    
    neuron->input_genes.push_back(g1);
    neuron->output_genes.push_back(g2);

    g_orig->in->output_genes.push_back(g1);
    g_orig->out->input_genes.push_back(g2);

    GENE_INSERT(g1, this->gene_map);
    GENE_INSERT(g2, this->gene_map);

    this->max_innov = g2->innovation;

    /* Update the active genes count */
    neuron->active_in_genes = 1;

    return 0;
}

long unsigned int next_innovation() {
    return global_innovation++;
}

bool Genome::containslink(Gene *g) {
    return this->gene_map.count(hash_ull_int_encode(g->in->id, g->out->id));
}

int Genome::link_mutate(bool force_bias) {
    unsigned long int node_size;
    unsigned long int input_size;
    unsigned long int rand1,rand2;
    Node *n1,*n2,*temp;
    Gene *new_gene;
    NODE_KEY_TYPE key;

    node_size=this->node_map.size();

    if(node_size==0) {
        return 1;
    }
    rand1=(unsigned long int) round(RANDOM_DOUBLE(node_size-1));
    rand2=(unsigned long int) round(RANDOM_DOUBLE(node_size-1));


    n1=this->node_map[rand1];
    n2=this->node_map[rand2];


    if((n1->type==n2->type && n1->type!=NODE_TYPE_HIDDEN) || n1->id == n2->id || compare_level(n1,n2)==0)
        return 1;

    if(n2->type==NODE_TYPE_INPUT || compare_level(n1,n2)==1) {
        temp=n2;
        n2=n1;
        n1=temp;
    }


    new_gene=new Gene();
    new_gene->in=n1;
    new_gene->out=n2;

    if(force_bias) {
	input_size = get_dis_circle_area(VIEW_RADIUS);
	key = POSSIBLE_MOVES + input_size + (ROBBY_NNET_POSITION * 2);
        new_gene->in = this->node_map[key];
    }

    if(this->containslink(new_gene)) {
        delete new_gene;
        return 1;
    }

    new_gene->innovation=next_innovation();

    new_gene->weight=RANDOM_DOUBLE(4)-2;

    #ifdef LINK_MUTATE_DEBUG
    n1->print();
    n2->print();
    #endif

    GENE_INSERT(new_gene, this->gene_map);

    n1->output_genes.push_back(new_gene);
    n2->input_genes.push_back(new_gene);

    this->max_innov = new_gene->innovation;

    new_gene->out->active_in_genes++;

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
        
        if(enable) {
            selected->in->output_genes.push_back(selected);
            selected->out->input_genes.push_back(selected);
        }
        else {
            selected->in->output_genes.remove(selected);
            selected->out->input_genes.remove(selected);
        }

	/* Increment or decrement active genes */
	selected->out->active_in_genes += ((2 * selected->enabled) - 1);
    }

    candidates.clear();
    return 0;
}

/* FIXME BFS */
int Genome::activate(struct robby *r, list<struct robby_msg> *msg_list ) {
    int i,j;
    unsigned long int key;
    unsigned long int max_id;
    double max;
    Node* in_node;
    map<unsigned long long int, Gene*>::iterator g_it;
    map<unsigned long int, Node*>::iterator n_it;
    list<struct robby_msg>::iterator m_it;
    list < list <Node *> >::iterator l_it;
    list <Node *>::iterator level_it;

    #ifdef DEBUG_MSG
    for (m_it=msg_list->begin(); m_it!=msg_list->end(); m_it++) {
          cout << "From: " << (*m_it).id << " Move: " << (*m_it).old_move << endl;
    }
    #endif

    for(n_it=this->node_map.begin(); n_it!=this->node_map.end(); n_it++) {
        n_it->second->activate_count=0;
        n_it->second->value=0;
    }

    for(g_it=this->gene_map.begin(); g_it!=this->gene_map.end(); g_it++) {
        g_it->second->value=0;
    }

    key = POSSIBLE_MOVES;

#ifdef KNOWN_MAP
    for(i=0; i<r->m_sizex; i++) {
        for(j=0; j<r->m_sizey; j++) {
            in_node=this->node_map[key];
            in_node->activate(r->known_map[i][j]);
            key++;
        }
    }
#else
    for(i=0; i<SQUARE_SIDE; i++) {
        for(j=0; j<SQUARE_SIDE; j++) {
            if (r->view[i][j] != -1) {
                  in_node=this->node_map[key];
                  in_node->activate(r->view[i][j]);
                  key++;
            }
        }
    }
#endif


    /* ALERT check this thing. if key is reassigned it explodes. */
    if (ROBBY_NNET_POSITION) {
        this->node_map[key]->activate((double)r->x);
	key++;

        this->node_map[key]->activate((double)r->y);
	key++;
    }

    /* Bias (input node with value 1.0) activation */
    /* ALERT check this thing. if key is reassigned it explodes. */
    in_node = this->node_map[key];
    in_node->activate(1.0);

    for(m_it=msg_list->begin(); m_it!=msg_list->end(); m_it++) {
        if((*m_it).id!=r->id) {
            #ifndef KNOWN_MAP
            for(i=0; i<SQUARE_SIDE; i++) {
                for(j=0; j<SQUARE_SIDE;j++)
                    if((*m_it).view[i][j]!=-1) {
                        this->node_map[key]->activate((*m_it).view[i][j]);
                        key++;
                    }
            }
            #endif
            if(ROBBY_NNET_POSITION) {
                this->node_map[key]->activate(m_it->x);
                key++;
                this->node_map[key]->activate(m_it->y);
                key++;
            }
            this->node_map[key]->activate((*m_it).old_move);
            key++;
        }
    }

    /* BFS activation */
    l_it = this->level_list.begin();
    /* We already done with the input layer */
    l_it++;

    for (; l_it != this->level_list.end(); l_it++) {
        for (level_it = l_it->begin(); level_it != l_it->end(); level_it++) {
             (*level_it)->activate(0.0);
	}
    }

    max=(-DBL_MAX);
    for(key=0; key<POSSIBLE_MOVES; key++) {
        if(max<this->node_map[key]->value) {
            max=this->node_map[key]->value;
            max_id=key;
        }
    }
    
    #ifdef DEBUG_NNET
    cout << "best move is: " << max_id << " value " << max << endl;
    #endif

    return (int)max_id;
}

int Genome::save_to_file(char *dir, int fileno) {
	map <GENE_KEY_TYPE, Gene *>::iterator g_it;
	map <NODE_KEY_TYPE, Node *>::iterator n_it;
	long unsigned int idin = 0, idout = 0;
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

	fprintf(f, "%lu %lu\n{\n", this->node_map.size(), this->gene_map.size());

	for (n_it=this->node_map.begin(); n_it != this->node_map.end(); n_it++)
		fprintf(f, "%lu %d\n", n_it->second->id, n_it->second->type);

	fprintf(f, "}\n{\n");

	for (g_it=this->gene_map.begin(); g_it != this->gene_map.end(); g_it++) {
		if (g_it->second->in)
			idin = g_it->second->in->id;
		if (g_it->second->out)
			idout = g_it->second->out->id;

		fprintf(f, "%lu -> %lu: %lu %d %f\n", idin, idout,
		        g_it->second->innovation, g_it->second->enabled,
			g_it->second->weight);
	}
	fprintf(f, "}\n");

	fclose(f);

	free(path);

    return 0;
}

int Genome::specialize(list <Species *> *sl)
{
	Species *new_species;
	list <Species *>::iterator s_it;
    double min_distance,curr_distance;
    Species *found_species;

    min_distance=SAME_SPECIES_TRESHOLD;
    found_species=NULL;

	for (s_it=sl->begin(); s_it !=sl->end();s_it++) {
		Genome *oth_g = LIST_GET(Genome*, (*s_it)->genomes, 0);
        curr_distance=delta_species(this,oth_g);
		if (curr_distance<min_distance) {
            found_species=(*s_it);
            min_distance=curr_distance;
            #ifdef SPECIALIZE_DEBUG
		    this->print();
            #endif
		}
	}

	if (!found_species) {
		new_species = new Species();
		sl->push_back(new_species);
		new_species->genomes.push_back(this);
        #ifdef SPECIALIZE_DEBUG
		this->print();
        #endif
	}
    else
        found_species->genomes.push_back(this);

    return 0;
}

/* Species */
Species::Species(void)
{
	this->staleness = 0;
	this->top_fitness = 0.0;
	this->average_fitness = 0.0;
}

Species::~Species(void)
{
    list<Genome*>::iterator it;

    it=this->genomes.begin();
    while(it!=this->genomes.end()) {
        delete(*it);
        it=this->genomes.erase(it);
    }
}

unsigned long int Species::cull(bool top_only)
{
	int cutoff = 0;
	unsigned long int size = 0;
	list <Genome *>::iterator it;

	this->genomes.sort(cmp_desc_genomes);
	size = this->genomes.size();

	if (top_only && size>=2)
		cutoff = 2;
	else if (top_only)
        cutoff=1;
    else 
		cutoff = (int) round((double)this->genomes.size() / 2.0);
    
    if(size>0) {
        it=this->genomes.begin();
        advance(it,cutoff);
        while(it!=this->genomes.end()) {
            delete (*it);
            it=this->genomes.erase(it);
        }
    }

	return this->genomes.size();
}

double Species::calculate_avg_fitness(void)
{
	double avg = 0.0;
	unsigned long int size = 0;
	list <Genome *>::iterator g_it;

	size = this->genomes.size();

	for (g_it=this->genomes.begin(); g_it != this->genomes.end(); g_it++)
		avg += (*g_it)->fitness;

	this->average_fitness = (avg/(double) size);
	return this->average_fitness;
}
