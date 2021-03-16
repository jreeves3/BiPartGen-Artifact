/** @file additionalgraphs.h
 *  @brief Instance generator for additional graphs.
 *
 *  See additionalgraphs.c for documentation and implementation details.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#ifndef _ADDITIONALGRAPHS_H_
#define _ADDITIONALGRAPHS_H_

#include "graph.h"

/** @brief Defines graph variables.
 *
 *  Graph parameters used in generators in additionalgraphs.c.
 */
typedef struct graph_variables graph_var_t;

/** Graph Generator API*/

/* Creation Function*/
graph_var_t *graph_var_create(int n, int card, float density, int nedges);

/* Generators */
graph_t *generate_random_graph(graph_var_t *gv, int seed);

#endif /* _ADDITIONALGRAPHS_H */
