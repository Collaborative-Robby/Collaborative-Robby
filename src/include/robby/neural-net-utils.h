#ifndef ROBBY_NEURAL_NET_UTILS_H
#define ROBBY_NEURAL_NET_UTILS_H

double sigmoid(double input);
double delta_species(Genome *g1, Genome *g2);
unsigned long long int hash_ull_int_encode(unsigned long from, unsigned long to);
int compare_level(Node* n1, Node *n2);
void get_level_num(long unsigned int n1, long unsigned int d1, long unsigned int n2, long unsigned int d2, long unsigned int *new_n, long unsigned int *new_d);
bool cmp_desc_genomes(Genome *g1, Genome *g2);

#endif /* ROBBY_NEURAL_NET_UTILS_H */
