#ifndef ROBBY_NEURAL_NET_CONST_H
#define ROBBY_NEURAL_NET_CONST_H

/* Neural net mutation rates */
#define MUTATION_RATE_NODE 0.75
#define MUTATION_RATE_CONNECTION 0.75
#define MUTATION_RATE_LINK 2.0
#define MUTATION_RATE_BIAS 0.6
#define MUTATION_RATE_ENABLE 0.4
#define MUTATION_RATE_DISABLE 0.6

#define CROSSOVER_CHANCE 0.75
#define PERTURB_CHANCE 0.9
#define PERTURB_STEP 0.05

/* Species comparison parameters */
#define SAME_SPECIES_TRESHOLD 1.0
#define COEFFICIENT_DELTA_WEIGHT 0.4
#define COEFFICIENT_EXCESS_GENES 2.0
#define COEFFICIENT_DISJOINT_GENES 2.0

/* Number of turns of species survival. */
#define SPECIES_STALE_TRESHOLD 100

/* Parameter of the sigmoid activation function */
#define SIGMOID_BETA -4.9

/* Simulation features */
#define ROBBY_NNET_POSITION false

#endif /* ROBBY_NEURAL_NET_CONST_H */
