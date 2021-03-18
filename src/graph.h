/**********************************************************************************
 Copyright (c) 2021 Joseph Reeves and Cayden Codel, Carnegie Mellon University
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/


/** @file graph.h
 *  @brief A graph representation for k-partite graphs.
 *
 *  See graph.c for implementation details.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#ifndef _GRAPH_H_
#define _GRAPH_H_

/** @brief Defines a graph struct.
 *
 *  See graph.c for struct fields and motivation.
 */
typedef struct k_partite_graph graph_t;

/** @brief Defines a perfect matching.
 *
 *  Generated with graph_generate_perfect_matchings().
 */
typedef struct perfect_matching matching_t;

/** Graph API */

/** Creation and free functions */
graph_t *graph_create(int partitions, int nodes);
graph_t *graph_create_with_sizes(int partitions, int *sizes);
void graph_free(graph_t *g);

/** Getters */
int graph_get_num_partitions(graph_t *g);
const int *graph_get_partition_sizes(graph_t *g);
int graph_is_edge_between(graph_t *g, int p1, int n1, int p2, int n2);
int graph_get_num_neighbors(graph_t *g, int p1, int n1, int p2);
int *graph_get_neighbors(graph_t *g, int p1, int n1, int p2, int *size);

int graph_get_edge_id(graph_t *g, int p1, int n1, int p2, int n2);

/** Modification functions */
void graph_add_edge(graph_t *g, int p1, int n1, int p2, int n2);
void graph_remove_edge(graph_t *g, int p1, int n1, int p2, int n2);
void graph_fully_connect_node(graph_t *g, int p1, int n1, int p2);
void graph_fully_connect_partition(graph_t *g, int p1, int p2);

/** Functions for interfacing with perfect matchings */
void graph_generate_perfect_matchings(graph_t *g, int up_to_size);
matching_t *graph_get_first_matching(graph_t *g, int p1, int n1, int p2);
matching_t *graph_get_prev_matching(matching_t *m);
matching_t *graph_get_next_matching(matching_t *m);
matching_t *graph_get_next_set(matching_t *m);
matching_t *graph_get_prev_set(matching_t *m);

/** Getters for matchings */
int graph_get_num_matchings(graph_t *g, int p1, int n1, int p2);
int graph_get_matching_size(matching_t *m);
int graph_get_num_similar_matchings(matching_t *m);
const int *graph_get_matching_left_nodes(matching_t *m);
const int *graph_get_matching_right_nodes(matching_t *m);
const int *graph_get_matching_ordered_right_nodes(matching_t *m);
void graph_remove_matching(graph_t *g, int p1, int n1, int p2, matching_t *m);

// TODO remove
void graph_print_perfect_matching(matching_t *m);

#endif /* _GRAPH_H_ */
