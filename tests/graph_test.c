/** @file graph_test.c
 *  @brief Tests the graph.c file.
 *  
 *  TODO Testing of edge IDs on more than bipartite graphs.
 *
 *  @author Cayden Codel (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <assert.h>

#include "../graph.h" // TODO think about making an inc/ and src/ directories

#define K 2
#define N 5

int main() {
  graph_t *g = graph_create(K, N);
  assert(graph_get_num_partitions(g) == K);

  for (int i = 0; i < N; i++) {
    assert(graph_is_edge_between(g, 0, i, 1, i) == 0);
    assert(graph_get_num_neighbors(g, 0, i, 1) == 0);
  }


  // Add some edges - straight across first
  for (int i = 0; i < N; i++) {
    graph_add_edge(g, 0, i, 1, i);
  }

  // Check that the number of neighbors is still correct
  for (int i = 0; i < N; i++) {
    assert(graph_is_edge_between(g, 0, i, 1, i) == 1);
    assert(graph_get_num_neighbors(g, 0, i, 1) == 1);
    assert(graph_get_edge_id(g, 0, i, 1, i) == (i + 1));
  }

  // Now fully connect node 1 to partition 2
  graph_fully_connect_node(g, 0, 0, 1);
  assert(graph_get_num_neighbors(g, 0, 0, 1) == N);
  for (int i = 0; i < N; i++) {
    assert(graph_is_edge_between(g, 0, 0, 1, i) == 1);
  }

  // Check other edges, updated IDs for other edges
  for (int i = 1; i < N; i++) {
    assert(graph_is_edge_between(g, 0, i, 1, i) == 1);
    assert(graph_get_num_neighbors(g, 0, i, 1) == 1);
    assert(graph_get_edge_id(g, 0, i, 1, i) == (i + N));
  }

  return 0;
}
