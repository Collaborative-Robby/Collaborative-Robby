#include <iostream>

#include <float.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>

#include <robby/struct.h>
#include <robby/dismath.h>
#include <robby/module.h>

#include <robby/neural-net.h>
#include <robby/neural-net-const.h>
#include <robby/neural-net-utils.h>

#include <robby/Fraction.h>

using namespace std;

extern long unsigned int genome_count;
extern long unsigned int global_innovation;

Genome::Genome(Genome *gen) {
    this->id=genome_count++;
    this->copy(gen);
}

void Genome::copy(Genome *gen) {
    unsigned long int node_key;
    Node *node, *val;
    Gene* gene;
    unordered_map<unsigned long int,      Node*>::iterator n_it, end_nit;
    unordered_map<unsigned long long int, Gene*>::iterator g_it, end_git;

    this->node_count=gen->node_count;
    this->max_innov=gen->max_innov;
    
    for(n_it=gen->node_map.begin(), end_nit=gen->node_map.end(); n_it!=end_nit; ++n_it) {
        val = n_it->second;
        node_key = n_it->first;
        node=new Node(node_key, val->type, val->level.n, val->level.d);
        NODE_INSERT(node, this->node_map);
        
	    this->insert_level_list(node);
    }

    for(g_it=gen->gene_map.begin(), end_git=gen->gene_map.end() ;  g_it!=end_git; ++g_it) {
        gene = new Gene(g_it->second);
        gene->in  = node_map[gene->in->id];
        gene->out = node_map[gene->out->id];

        if(gene->enabled) {
            gene->in->output_genes.push_back(gene);
            gene->out->input_genes.push_back(gene);
        }
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
    list <Node*>::iterator inner_level_it, end_inner_level_it;
    list < list <Node*> >::iterator l_it, end_lit;

    this->id=genome_count++;
    this->node_count=0;
    this->max_innov=0;
    this->fitness=0.0;
    
    /*Output nodes*/
    for(i=0;i<output_no;i++) {
        curr=new Node(this->node_count, NODE_TYPE_OUTPUT,1,1);
        NODE_INSERT(curr, this->node_map);
        out_list.push_back(curr);
        this->node_count++;
    }

    /* Bias node */
    curr = new Node(this->node_count, NODE_TYPE_INPUT,0,1);
    NODE_INSERT(curr, this->node_map);
    in_list.push_back(curr);
    this->node_count++;
    
    /*This robby input nodes*/
    for(i=0;i<input_no;i++) {
        curr=new Node(this->node_count, NODE_TYPE_INPUT, 0,1);
        NODE_INSERT(curr, this->node_map);
        in_list.push_back(curr);
        this->node_count++;
    }
    
    /*This robby position*/
    if (ROBBY_NNET_POSITION) {
        curr=new Node(this->node_count, NODE_TYPE_INPUT,0,1);
        NODE_INSERT(curr, this->node_map);
        in_list.push_back(curr);
	    this->node_count++;

        curr=new Node(this->node_count, NODE_TYPE_INPUT,0,1);
        NODE_INSERT(curr, this->node_map);
        in_list.push_back(curr);
	    this->node_count++;
    }

    for(i=1; i<robbynum; i++) {
        /*Other robbies input nodes*/
#ifndef KNOWN_MAP
        for(j=0; j<input_no; j++) {
            curr=new Node(this->node_count, NODE_TYPE_INPUT,0,1);
            NODE_INSERT(curr, this->node_map);
            in_list.push_back(curr);
            this->node_count++;
        }
#endif
        /*Other robbies positions*/
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
#ifdef OLD_MOVE
        /*old move node*/
        curr=new Node(this->node_count, NODE_TYPE_INPUT,0,1);
        NODE_INSERT(curr, this->node_map);
        in_list.push_back(curr);
        this->node_count++;
#endif
    }
    
    /*Push inputs and outputs in level list*/
    this->level_list.push_back(in_list);
    this->level_list.push_back(out_list);
    
    /*Get iterator for each level*/
    for (l_it = this->level_list.begin(), end_lit=this->level_list.end(); l_it!=end_lit; ++l_it)
        for (inner_level_it = l_it->begin(),end_inner_level_it=l_it->end() ; inner_level_it != end_inner_level_it; ++inner_level_it)
             (*inner_level_it)->level_it = l_it;
    
    /*Connect inputs to outputs*/
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
    
    /*Mutate this genome*/
    this->mutate();
}

Genome::Genome(char *dir, int fileno) {
	unordered_map <GENE_KEY_TYPE, Gene *>::iterator g_it;
	unordered_map <NODE_KEY_TYPE, Node *>::iterator n_it;
	char *path;
	FILE *f;
	int rval, i;
	int node_size, gene_size;
	int node_id, node_type;
    long unsigned int node_num, node_den;
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
		fscanf(f, "%d %d, %lu/%lu\n", &node_id, &node_type, &node_num, &node_den);
		cur_node = new Node(node_id, node_type,node_num, node_den);
		NODE_INSERT(cur_node, this->node_map);
        this->insert_level_list(cur_node);
	}
	this->node_count = node_size;

	fscanf(f, "}\n{\n");

	for (i =0 ; i < gene_size; i ++) {
		cur_gene = new Gene();
		fscanf(f, "%d -> %d: %lu %d %lf\n", &idin, &idout,
			&cur_gene->innovation, &tmp,
			&cur_gene->weight);
        cur_gene->enabled=(bool)tmp;
		if (idin >= 0)
			cur_gene->in = this->node_map[idin];
		if (idout >= 0)
			cur_gene->out = this->node_map[idout];
		GENE_INSERT(cur_gene, this->gene_map);
        if(cur_gene->innovation>global_innovation)
            global_innovation=cur_gene->innovation+1;
        if(cur_gene->innovation>this->max_innov)
            this->max_innov=cur_gene->innovation;
        
        if(cur_gene->enabled) {
            cur_gene->in->output_genes.push_back(cur_gene);
            cur_gene->out->input_genes.push_back(cur_gene);
        }
	}
	fclose(f);

	free(path);
}

Genome::Genome(Species *s, bool crossover) {
    Genome *g1,*g2;
    long unsigned int r;

    this->id=genome_count++;
    this->max_innov=0;
    this->fitness=0;

    if(crossover) {
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

/*Insert gene and create adjacent nodes (used in crossover)*/
int Genome::insert_gene(Gene *g) {
    Gene *new_gene;
    long unsigned int id_in, id_out;
    Node *n1,*n2;

    id_in=g->in->id;
    id_out=g->out->id;

    /*If adjacent nodes don't exist, create them*/
    if(!this->node_map.count(id_in)) {
        n1=new Node(g->in);
        NODE_INSERT(n1, this->node_map);
	    this->insert_level_list(n1);
        if(n1->id>=this->node_count)
            this->node_count++;
    } else {
        n1=this->node_map[id_in];
    }

    if(!this->node_map.count(id_out)) {
        n2=new Node(g->out);
        NODE_INSERT(n2, this->node_map);
	    this->insert_level_list(n2);
        if(n2->id>=this->node_count)
            this->node_count++;
    } else {
        n2=this->node_map[id_out];
    }

    /*Avoid loops*/
    if(n1->level >= n2->level)
	    return 0;

    /*Create and push the gene*/
    new_gene=new Gene(g);
    new_gene->in=n1;
    new_gene->out=n2;
    if(new_gene->enabled) {
        n1->output_genes.push_back(new_gene);
        n2->input_genes.push_back(new_gene);
    }

    /* Update activation count */
    new_gene->out->active_in_genes++;

    GENE_INSERT(new_gene, this->gene_map);

    /*Update max innovation*/
    if(new_gene->innovation>this->max_innov)
        this->max_innov=new_gene->innovation;

    return 0;
}

Genome::Genome(Genome *g1, Genome *g2){
    this->id=genome_count++;
    this->crossover(g1, g2);
    this->mutate();
}

/*Insert a node to the appropriate level list*/
void Genome::insert_level_list(Node *n) {
	list <Node *> l;
	list < list <Node *> >::iterator l_it;
    
    /*If no list is present, create input and output lists*/
	if (this->level_list.size() == 0) {
		/* Input list */
		this->level_list.push_back(l);
		/* Output list */
		this->level_list.push_back(l);
	}

	if (n->type == NODE_TYPE_INPUT) {
        /*Push input node*/
		l_it = this->level_list.begin();
		n->level_it = l_it;
		l_it->push_back(n);
	} else if (n->type == NODE_TYPE_OUTPUT) {
        /*Push output node*/
		l_it = this->level_list.end();
		/*Get last element */
		l_it--;

		n->level_it = l_it;
		l_it->push_back(n);
	} else {
		l_it = this->level_list.begin();
		++l_it;
        
		/* Advance the list to our level */
		while(l_it->size()>0 && (*(l_it->begin()))->level < n->level) {
			++l_it;
        }

		if(l_it->size()>0 && (*(l_it->begin()))->level == n->level) {
			/* Push the node in the existent level */
			l_it->push_back(n);
		    n->level_it = l_it;
		} else {
			/* Create a new level and insert the node*/
			l.push_back(n);

			l_it=this->level_list.insert(l_it, l);
            
            n->level_it=l_it;
		}

	}
}

void Genome::crossover(Genome *rg1, Genome *rg2){
    unordered_map<unsigned long long, Gene*>::iterator g_it, end_git;
    unordered_map<NODE_KEY_TYPE, Node*>::iterator n_it, end_nit;
    long unsigned int id_in,id_out;
    Genome *g1, *g2;
    Node *new_n;
    
    /*We privilege one genome, over the other, randomize the order*/
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

    this->node_count=0; 
    
    /*Copy input and output genes*/
    for(n_it=g1->node_map.begin(), end_nit=g1->node_map.end() ; n_it!=end_nit; ++n_it) {
        if(n_it->second->type!=NODE_TYPE_HIDDEN) {
            new_n=new Node(n_it->second);

	        new_n->active_in_genes = 0;
            NODE_INSERT(new_n, this->node_map);
            this->node_count++;
	        this->insert_level_list(new_n);
        }
    }
    
    /*Crossover genes from the first genome*/
    for(g_it=g1->gene_map.begin(), end_git=g1->gene_map.end(); g_it!=end_git; ++g_it) {
        id_in=g_it->second->in->id;
        id_out=g_it->second->out->id;
        
        if(!g2->gene_map.count(g_it->first)){
            this->insert_gene(g_it->second);
        } else {
            /*If both genomes have the same gene, choose it randomly*/
            if(round(RANDOM_DOUBLE(1))) {
                this->insert_gene(g_it->second);
            } else {
                this->insert_gene(g2->gene_map[g_it->first]);
            }
        /*avoid problems when the gene is not really inserted because it creates a cycle*/
	    if ((!g_it->second->enabled ||
		    !g2->gene_map[g_it->first]->enabled) && this->gene_map.count(g_it->first)>0 ) {
		    if (RANDOM_DOUBLE(1) < DISABLE_INHERIT_GENE_RATIO) {
			    this->gene_map[g_it->first]->enabled = false;
                this->gene_map[g_it->first]->in->output_genes.remove(this->gene_map[g_it->first]);
                this->gene_map[g_it->first]->out->input_genes.remove(this->gene_map[g_it->first]);
            }
		    else if(!this->gene_map[g_it->first]->enabled){
			    this->gene_map[g_it->first]->enabled = true;
                this->gene_map[g_it->first]->in->output_genes.push_back(this->gene_map[g_it->first]);
                this->gene_map[g_it->first]->out->input_genes.push_back(this->gene_map[g_it->first]);
            }
            else {
			    this->gene_map[g_it->first]->enabled = true;
            }

	    }
        }
    }
    
    /*Add remaining genes from the second genome*/
    for(g_it=g2->gene_map.begin(), end_git=g2->gene_map.end(); g_it!=end_git; ++g_it) {
        id_in=g_it->second->in->id;
        id_out=g_it->second->out->id;
        
        if(!this->gene_map.count(hash_ull_int_encode(id_in, id_out))){
            this->insert_gene(g_it->second);
        }
    }

}

Genome::~Genome(void) {
    unordered_map<unsigned long long, Gene*>::iterator g_it;
    unordered_map<unsigned long int,  Node*>::iterator n_it;
    list< list <Node*> >::iterator lev_it, end_levit;

    for(n_it=this->node_map.begin(); n_it!=this->node_map.end(); ++n_it)
        delete n_it->second;

    for(g_it=this->gene_map.begin(); g_it!=this->gene_map.end(); ++g_it)
        delete g_it->second;

    this->gene_map.clear();
    this->node_map.clear();
    for(lev_it=this->level_list.begin(), end_levit=this->level_list.end(); lev_it!=end_levit; ++lev_it) {
        lev_it->clear();
    }
    this->level_list.clear();
}

int Genome::mutate(void) {
    double mrate_link;
    unordered_map<unsigned long long int, Gene*>::iterator gene_iter, end_geneiter;

    mrate_link=MUTATION_RATE_LINK;

    /*Mutate node, break a gene and add a node between its adjacent nodes*/
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_NODE) 
        this->node_mutate(); 

    /*Link mutate, add a random gene*/
    while(mrate_link>0) {
        if(RANDOM_DOUBLE(1)<mrate_link)
            this->link_mutate(false);
        mrate_link=mrate_link-1;
    }

    /*Add a link from the bias node to a random node*/
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_BIAS)
        this->link_mutate(true); 

    /*Change the gene weight*/
    for (gene_iter = this->gene_map.begin(), end_geneiter=this->gene_map.end() ; gene_iter != end_geneiter; gene_iter++)
        if(RANDOM_DOUBLE(1)<MUTATION_RATE_CONNECTION) 
            gene_iter->second->point_mutate();

    /*Enable a random gene*/
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_ENABLE)
        this->enable_disable_mutate(true);
    
    /*Disable a random gene*/
    if(RANDOM_DOUBLE(1)<MUTATION_RATE_DISABLE)
        this->enable_disable_mutate(false);
    
    return 0;
}

void Genome::print() {
    unordered_map<unsigned long int,      Node*>::iterator node_it, end_nodeit;
    unordered_map<unsigned long long int, Gene*>::iterator gene_it, end_geneit;

    for(node_it=this->node_map.begin(), end_nodeit=this->node_map.end(); node_it!=end_nodeit; ++node_it) {
        node_it->second->print();
    }

    for(gene_it=this->gene_map.begin(), end_geneit=this->gene_map.end(); gene_it!=end_geneit; ++gene_it) {
        gene_it->second->print();
    }

}


int Genome::node_mutate(void) {
    Node *neuron;
    Gene *g_orig, *g1,*g2;
    Node *node_preceding_output_level, *node_input_level;

    unsigned long int list_len;
    unsigned long int gene_index;

    list <Node*> new_l;
    list< list<Node*> >::iterator lv_it;
    unordered_map<unsigned long int, Node*>::iterator it;

    /* Empty fraction */
    Fraction f;

    /*Select a random gene*/
    list_len=this->gene_map.size();
    if(list_len==0)
        return 1;

    gene_index=(unsigned long int) round(RANDOM_DOUBLE(list_len-1)); 

    g_orig=HASH_GET(GENE_KEY_TYPE, Gene*, this->gene_map, gene_index);

    if(!g_orig->enabled)
        return -1;
    
    /*Get the levels for the input and outputs of the selected gene*/
    lv_it=g_orig->out->level_it;
    lv_it--;

    node_preceding_output_level = (*(*lv_it).begin());
    node_input_level = g_orig->in;

    if(node_preceding_output_level->level == node_input_level->level) {
        /*if the input level and the level preceding output are the same, create a new level*/
	    f = Fraction(&g_orig->in->level, &g_orig->out->level);

        neuron=new Node(this->node_count,NODE_TYPE_HIDDEN, f.n, f.d);
        new_l.push_back(neuron);
        this->level_list.insert(g_orig->out->level_it, new_l);

    } else {
        /*else choose the level preceding output*/
        neuron=new Node(this->node_count,NODE_TYPE_HIDDEN,
                        node_preceding_output_level->level.n,
                        node_preceding_output_level->level.d);
        lv_it->push_back(neuron);
    }

    NODE_INSERT(neuron, this->node_map);
    this->node_count++;
    
    /*Disable the old gene and remove it from the nodes*/
    g_orig->enabled=false;
    g_orig->in->output_genes.remove(g_orig);
    g_orig->out->input_genes.remove(g_orig);
    
    /*Create the new genes*/
    g1=new Gene(g_orig);
    g2=new Gene(g_orig);

    g1->out=neuron;
    g1->weight=1.0;
    g1->innovation=next_innovation();
    g1->enabled=true;

    g2->in=neuron;
    g2->innovation=next_innovation();
    g2->enabled=true;
    
    /*Push appropriate nodes*/
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

/*Get the next innovation number and increment it globally*/
long unsigned int next_innovation() {
    return global_innovation++;
}

/*Check if a link between nodes is present*/
bool Genome::containslink(Gene *g) {
    return this->gene_map.count(hash_ull_int_encode(g->in->id, g->out->id));
}

/*Create a random link*/
int Genome::link_mutate(bool force_bias) {
    unsigned long int node_size;
    unsigned long int rand1,rand2;
    Node *n1,*n2,*temp;
    Gene *new_gene;
    NODE_KEY_TYPE key;

    node_size=this->node_count;

    if(node_size==0) {
        return 1;
    }
    /*select two random nodes*/
    rand1=(unsigned long int) round(RANDOM_DOUBLE(node_size-1));
    rand2=(unsigned long int) round(RANDOM_DOUBLE(node_size-1));


    n1=this->node_map[rand1];
    n2=this->node_map[rand2];

    /*Check if the two nodes are on the same level*/
    if(n1->level == n2->level)
        return 1;

    /*If the first node is on a higher level, swap the nodes*/
    if(n1->level > n2->level) {
        temp=n2;
        n2=n1;
        n1=temp;
    }

    /*Create the new gene*/
    new_gene=new Gene();
    new_gene->in=n1;
    new_gene->out=n2;
    
    /*Use the bias gene as input*/
    if(force_bias) {
	    key = POSSIBLE_MOVES;
        new_gene->in = this->node_map[key];
    }
    
    /*Do not add the gene if it's already there*/
    if(this->containslink(new_gene)) {
        delete new_gene;
        return 1;
    }

    new_gene->innovation=next_innovation();
    
    /*Get a random weight*/
    new_gene->weight=RANDOM_DOUBLE(4)-2;

#ifdef LINK_MUTATE_DEBUG
    n1->print();
    n2->print();
#endif

    GENE_INSERT(new_gene, this->gene_map);

    n1->output_genes.push_back(new_gene);
    n2->input_genes.push_back(new_gene);
    
    /*Update genome innovation*/
    this->max_innov = new_gene->innovation;

    new_gene->out->active_in_genes++;

    return 0;
}

int Genome::enable_disable_mutate(bool enable) {
    Gene *selected;
    list<Gene*> candidates;
    unordered_map<GENE_KEY_TYPE, Gene*>::iterator it, end_it;
    
    /*Choose candidates that are disabled or enabled depending on the parameter*/
    for(it=this->gene_map.begin(), end_it=this->gene_map.end(); it!=end_it;++it) {
        if(it->second->enabled == not enable)
            candidates.push_back(it->second);
    }
    
    /*Select a random gene from the candidates, enable or disable it*/
    if(candidates.size()>0) {
        selected=LIST_GET(Gene*, candidates, round(RANDOM_DOUBLE(candidates.size()-1)));
        selected->enabled=enable;
        
        if(enable) {
            selected->in->output_genes.push_back(selected);
            selected->out->input_genes.push_back(selected);
        } else {
            selected->in->output_genes.remove(selected);
            selected->out->input_genes.remove(selected);
        }

	    /* Increment or decrement active genes */
	    selected->out->active_in_genes += ((2 * selected->enabled) - 1);
    }

    candidates.clear();
    return 0;
}

/* Activate the neural net*/
int Genome::activate(struct robby *r, list<struct robby_msg> *msg_list ) {
    unsigned long int i,j;
    unsigned long int key;
    unsigned long int max_id;
    double max;
    Node* in_node;
    unordered_map<unsigned long long int, Gene*>::iterator g_it, end_git;
    unordered_map<unsigned long int, Node*>::iterator n_it, end_nit;
    list<struct robby_msg>::iterator m_it, end_mit;
    list < list <Node *> >::iterator l_it, end_lit;
    list <Node *>::iterator level_it, end_levelit;

    #ifdef DEBUG_MSG
    for (m_it=msg_list->begin(); m_it!=msg_list->end(); ++m_it) {
          cout << "From: " << (*m_it).id << " Move: " << (*m_it).old_move << endl;
    }
    #endif
    
    /* Bias (input node with value 1.0) activation */
    key = POSSIBLE_MOVES;
    in_node = this->node_map[key];
    in_node->activate(1.0);
    key++;
    

#ifdef KNOWN_MAP
    /*Activate with known map*/
    for(i=0; i<r->m_sizex; i++) {
        for(j=0; j<r->m_sizey; j++) {
            in_node=this->node_map[key];
            in_node->activate(r->known_map[i][j]);
            key++;
        }
    }
#else
    /*Activate with robby view*/
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

    /* Activate with robby position, from 1 to map size*/
    if (ROBBY_NNET_POSITION) {
        this->node_map[key]->activate((double)r->x+1.0);
        key++;

        this->node_map[key]->activate((double)r->y+1.0);
        key++;
    }
    
    /*Read info from the message list*/
    for(m_it=msg_list->begin(), end_mit=msg_list->end(); m_it!=end_mit; ++m_it) {
        /*Get info from the other robbies*/
        if((*m_it).id!=r->id) {
#ifndef KNOWN_MAP
            /*Get other robbies view*/
            for(i=0; i<SQUARE_SIDE; i++) {
                for(j=0; j<SQUARE_SIDE;j++)
                    if((*m_it).view[i][j]!=-1) {
                        this->node_map[key]->activate((*m_it).view[i][j]);
                        key++;
                    }
            }
#endif      
            /*Get other robbies position, from 1 to map size*/
            if(ROBBY_NNET_POSITION) {
                this->node_map[key]->activate((double)m_it->x+1.0);
                key++;
                this->node_map[key]->activate((double)m_it->y+1.0);
                key++;
            }
#ifdef OLD_MOVE
            /*Get other robbies last move*/
            this->node_map[key]->activate((*m_it).old_move);
            key++;
#endif
        }
    }

    /* BFS activation */
    l_it = this->level_list.begin();
    /* We already done with the input layer */
    ++l_it;

    end_lit=this->level_list.end();
    
    /*Activate each layer*/
    for (; l_it != end_lit ; ++l_it) {
        for (level_it = l_it->begin(), end_levelit=l_it->end(); level_it != end_levelit; ++level_it) {
             (*level_it)->activate(0.0);
        }
    }
    
    /*Get the chosen ouput*/
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
    
    /*return the chosen move*/
    return (int)max_id;
}

/*save genomes to file*/
int Genome::save_to_file(char *dir, long unsigned int fileno) {
	unordered_map <GENE_KEY_TYPE, Gene *>::iterator g_it;
	long unsigned int idin = 0, idout = 0;
    list<Node*>::iterator n_it;
    list<list <Node*> >::iterator l_it,o_it;
	char *path;
	FILE *f;
	DIR *d;
	int rval;
    
    /*check if the dir exists, if not create it*/
	d = opendir(dir);
	if (!d) {
		mkdir(dir, 0755);
		d = opendir(dir);
		if (!d) {
			perror("mkdir");
			return -1;
		}
	}

    /*create the genome file*/
	rval = asprintf(&path, "%s/%lu.%s", dir, fileno, GENOME_EXT);
	if (rval < 0)
		return rval;
    
	f = fopen(path, "w");
	if (!f) {
		perror("genome save");
		return -1;
	}
    
    /*save node and genome size*/
	fprintf(f, "%lu %lu\n{\n", this->node_map.size(), this->gene_map.size());
    
    /*save input layer*/
    l_it=this->level_list.begin();
    for (n_it=l_it->begin(); n_it != l_it->end(); ++n_it)
        fprintf(f, "%lu %d, %lu/%lu \n", (*n_it)->id, (*n_it)->type, (*n_it)->level.n, (*n_it)->level.d);


    /*save output layer*/
    o_it=this->level_list.end();
    o_it--;
    for (n_it=o_it->begin(); n_it != o_it->end(); ++n_it)
        fprintf(f, "%lu %d, %lu/%lu \n", (*n_it)->id, (*n_it)->type, (*n_it)->level.n, (*n_it)->level.d);

    /*save node info*/
    l_it=this->level_list.begin();
    ++l_it;
    for(; l_it!=o_it; ++l_it) {
        for (n_it=l_it->begin(); n_it != l_it->end(); ++n_it)
            fprintf(f, "%lu %d, %lu/%lu \n", (*n_it)->id, (*n_it)->type, (*n_it)->level.n, (*n_it)->level.d);
    }
	fprintf(f, "}\n{\n");

    /*save genome info*/
	for (g_it=this->gene_map.begin(); g_it != this->gene_map.end(); ++g_it) {
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

/*Insert genome in the appropriate species*/
int Genome::specialize(list <Species *> *sl)
{
	Species *new_species;
	list <Species *>::iterator s_it, end_sit;
    double min_distance,curr_distance;
    Species *found_species;

    min_distance=SAME_SPECIES_TRESHOLD;
    found_species=NULL;
    
    /*For each species*/
	for (s_it=sl->begin(), end_sit=sl->end(); s_it !=end_sit;++s_it) {
        /*Get the best genome in the species and compare it with this*/
		Genome *oth_g = LIST_GET(Genome*, (*s_it)->genomes, 0);
        curr_distance=delta_species(this,oth_g);
        /*Get the most similar species below the treshold*/
		if (curr_distance<min_distance) {
            found_species=(*s_it);
            min_distance=curr_distance;
#ifdef SPECIALIZE_DEBUG
		    this->print();
#endif
		}
	}
    
    /*Create a new species if no close species was found*/ 
    if (!found_species) {
		new_species = new Species();
		sl->push_back(new_species);
		new_species->genomes.push_back(this);
#ifdef SPECIALIZE_DEBUG
		this->print();
#endif
	} else {
        /*Else push the genome to the closest species*/
        found_species->genomes.push_back(this);
    }

    return 0;
}

