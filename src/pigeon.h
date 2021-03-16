/** @file pigeon.h
 *  @brief Instance generator for pigeonhole problems and its variants.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#ifndef _PIGEON_H_
#define _PIGEON_H_

#include "graph.h"

/** @brief Defines a pigeonhole problem.
 *
 *  See pigeon.c for struct fields and motivation.
 */
typedef struct pigeonhole_problem pigeon_t;


/** Pigeonhole API */

/** Creation and free functions */
pigeon_t *pigeon_create(int n);
void pigeon_free(pigeon_t *p);

/** Getters */
int pigeon_get_n(pigeon_t *p);

/** Generators */
graph_t *pigeon_generate_graph(pigeon_t *p);

#endif /* _PIGEON_H_ */
