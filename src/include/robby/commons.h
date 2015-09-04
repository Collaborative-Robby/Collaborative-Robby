#ifndef _ROBBY_COMMON_H
#define _ROBBY_COMMON_H

/* Directions as modification on the map */
int left[] =  { 0,-1};
int right[] = { 0, 1};
int up[] =    {-1, 0};
int down[] =  { 1, 0};

/* Array of directions and index to lookup */
int* dir[]= {left, right, up, down };

/* External handle */
int **directions = dir;

#define TEST_DEFAULT_DIR ((char*)"test_maps")
#define TRAINING_DEFAULT_DIR ((char*)"training_maps")

#endif
