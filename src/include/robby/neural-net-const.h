#ifndef ROBBY_NEURAL_NET_CONST_H
#define ROBBY_NEURAL_NET_CONST_H

/* Neural net mutation rates */
#define MUTATION_RATE_NODE 0.03
#define MUTATION_RATE_CONNECTION 0.75
#define MUTATION_RATE_LINK 0.3
#define MUTATION_RATE_BIAS 0.0
#define MUTATION_RATE_ENABLE 0.0
#define MUTATION_RATE_DISABLE 0.375

#define CROSSOVER_CHANCE 0.75
#define INTERSPECIES_CROSSOVER_PROB 0.001
#define PERTURB_CHANCE 0.95
#define PERTURB_STEP 0.1

/* Species comparison parameters */
#define SAME_SPECIES_TRESHOLD 2.0
#define COEFFICIENT_DELTA_WEIGHT 3.0
#define COEFFICIENT_EXCESS_GENES 1.0
#define COEFFICIENT_DISJOINT_GENES 1.0
#define DISABLE_INHERIT_GENE_RATIO 0.75

#define MISSING_GENOME_CROSSOVER_PROB 0.95

/* Number of turns of species survival. */
#define SPECIES_STALE_TRESHOLD 1000

/* Parameter of the sigmoid activation function */
#define SIGMOID_BETA -4.9

/* Simulation features */
#define ROBBY_NNET_POSITION true

#endif /* ROBBY_NEURAL_NET_CONST_H */
