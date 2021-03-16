/** @file additionalgraphs.c
 *  @brief Instance generator for additional graphs.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#include "additionalgraphs.h"
#include "stdlib.h"
#include "xmalloc.h"
#include "stdio.h"
#include "stdbool.h"

/** @brief Defines graph variables.
 *
 *  n                 Number of nodes for one side.
 *  cardinality   Difference in # nodes between sides.
 *  density        Density of random graph.
 *  nedges        Edge count of random graph.
 *
 */
struct graph_variables {
  int n;
  int cardinality;
  float density;
  int nedges;
}; // graph_var_t

/** @brief Creates the graph variables data structure with default values set in bipargen.c.
 *  Width default if half the nodes so 2*width covers the entire partition.
 *
 *  @param n                           Number of nodes for one side.
 *  @param cardinality     Difference in # nodes between sides.
 *  @param density              Probability of an edge between any two nodes.
 *  @param nedges                Edge count of random graph.
 *  @return             Graph variables struct.
 */
graph_var_t *graph_var_create(int n, int cardinality, float density, int nedges) {
  graph_var_t *gt =xmalloc(sizeof(graph_var_t));
  gt->n = n;
  gt->cardinality = cardinality;
  gt->density = density;
  gt->nedges = nedges;
  return gt;
}

struct edge_struct {
  int n1;
  int n2;
};
typedef struct edge_struct edge_t;

/** @brief Shuffles elements of a list (of edges).
 *
 *  @param edges           list of edges to be shuffled.
 *  @param edgeCount  Number of edges in list.
 */
void shuffle_edges(edge_t *edges, int edgeCount) {
  edge_t temp;
  int p;
  for(int i = edgeCount-1; i > 0; i--) {
        p = rand()%i;
        temp = edges[p];
        edges[p] = edges[i];
        edges[i] = temp;
  }
}

/** @brief Generate a graph based on graph_variable parameters.
 *
 *  @param gv  Graph variables used to generate graph.
 *  @param seed Random number seed.
 *  @return   A bi-partite graph with edges based on graph variable values.
 */
graph_t *generate_random_graph(graph_var_t *gv, int seed) {
  graph_t* g;
  int sizes[2] = {gv->n+gv->cardinality, gv->n}, edgeLimit;
  int i, r, edgeShuffSize, edgeN = 0;
  float density = gv->density;
  edge_t edge;
  edge_t edgeShuff[sizes[0]*sizes[1]];
  bool byCount = false;
  if (gv->nedges > 0) byCount = true;
  
  // fill edgeShuff with all possible edges
  r = 0;
  for(int i = 0; i < sizes[0]; i++) {
    for(int j = 0; j < sizes[1]; j++) {
      edgeShuff[r].n1 = i;
      edgeShuff[r].n2 = j;
      r++;
    }
  }
  edgeShuffSize = r;
  // seed random number generator
  srand(seed);
  
  // create graph
  g = graph_create_with_sizes(2, sizes);
  
  edgeLimit = density * (sizes[0] * sizes[1]);

  // Build random spanning tree
  for(int i = 0; i < sizes[0]; i++) {
    if (i < sizes[1]) { // edge across
      edgeLimit--;
      edgeN++;
      graph_add_edge(g,0,i,1,i);
      if (i > 0) r = rand()%i;
      else r = 0;
    }
    else r = rand() % sizes[1];
    // edge to random node 0 <= r < i
    graph_add_edge(g,0,i,1,r);
    if (i>0) {edgeLimit--; edgeN++;}
  }
  
  // Randomly shuffly possible edges
  shuffle_edges(edgeShuff, edgeShuffSize);
  
  // Add edges randomly until edgeLimit is reached
  i = 0;
  
  while (true) {
    if (i >= edgeShuffSize) {
      printf("Number of edges too high for given size with density 1.\n");
      break;
    }
    if (!byCount && (edgeLimit <= 0)) break; // based on density
    else if (byCount && (edgeN >= gv->nedges)) break; // based on edge_count
    edge = edgeShuff[i++];
    if (!graph_is_edge_between(g, 0, edge.n1, 1, edge.n2)) {
      graph_add_edge(g, 0, edge.n1, 1, edge.n2);
      edgeLimit--;
      edgeN++;
    }
    
  }
  
  return g;
}
