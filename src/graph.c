/** @file graph.c
 *  @brief A graph representation for k-partite graphs.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @todos Optimize removing an edge with perfect matchings.
 *
 *  @bug No known bugs.
 */

#include <assert.h>
#include <string.h>

#include <stdio.h> // For printf
#include <limits.h> // For INT_MAX

#include "graph.h"
#include "xmalloc.h"

#ifndef BITS_IN_BYTE
#define BITS_IN_BYTE     8
#endif

#ifndef BYTE_MASK
#define BYTE_MASK        (BITS_IN_BYTE - 1)
#endif

/** @brief Rounds x up to the next multiple of y.
 *
 *  @param x  The number to be rounded.
 *  @param y  The multiple.
 *  @return   Ceiling(x / y) * y
 */
#ifndef ROUND_UP
#define ROUND_UP(x, y)    ((((x) + (y) - 1) / (y)) * (y))
#endif

/** @brief Stores ordering information for perfect matchings */
typedef struct node_ordering {
  int *ordering;
  struct node_ordering *prev;
  struct node_ordering *next;
} node_ordering_t;


/** @brief A structure to store a perfect matching between two partitions.
 *
 *  When translating a graph into an encoding, sometimes additional clauses
 *  are added to the final CNF to block particular perfect matchings. This
 *  sort of clause blocking is done to "break symmetries" and to ensure that
 *  when there is freedom of choice between two sets of nodes, only one is
 *  taken.
 *
 *  For example, consider n1 and n2 in p1, each connected to n3 and n4 in p2.
 *  The subgraph has four edges. In the final solution, either n1 and n3 can
 *  share an edge, or n1 and n4 can share an edge, with n2 taking the other
 *  edge. Because both are equivalent, we can WLOG take n1 and n3 to be
 *  joined by an edge, and so can eliminate the other perfect matching.
 *
 *  The perfect matchings are indexed into in the graph structure (below)
 *  by partition, then by node in the partition, and are then recorded in
 *  lexicographic ordering with no repeats. Taking the above example, the
 *  perfect matching blocked would be n1 <-> n4 and n2 <-> n3 and recorded
 *  under n1. No perfect matching would be recorded under n2, even though
 *  n2 was involved in the perfect matching with n1.
 *
 *  Because the perfect matchings are found by indexing by partition, then
 *  by node, that information is omitted in the struct fields below.
 *
 *  "size" is the number of nodes on one side of the perfect matching. In the
 *  example used above, size would be 2.
 *
 *  "p1_nodes" is an array of size nodes, containing ordered indexes of nodes
 *  from partition 1 involved in the perfect matching.
 *
 *  "p2_nodes" is an array of size nodes, containing indexes of nodes from
 *  partition 2 involved in the perfect matching, such that p1_nodes[i]
 *  and p2_nodes[i] are in the matching, with an edge between them.
 *
 *  The ordering of the p2_nodes is stored in the p2_ordered linked list.
 *  Because several perfect matchings can be made on the same subset of
 *  p1_nodes and p2_nodes, storing just the orderings can save on memory.
 *
 *  "iterator" is an iterator pointer that is internal to the structure. It
 *  is used to keep track of how many perfect matchings on the given p1_nodes
 *  and p2_nodes have been accessed, going through the linked list.
 *
 *  The perfect matchings are stored as linked lists. They are chained together
 *  as they are built.
 */
struct perfect_matching {
  int size;       // Number of nodes on one side of the perfect matching
  int *p1_nodes;  // Node indexes in the first partition, including "n1"
  int *p2_nodes;  // Node indexes in the second partition, sorted
  int num_orderings; // Number of orderings in the linked-list
  node_ordering_t *head; // Orderings, linked list
  node_ordering_t *tail; // Orderings, linked list
  node_ordering_t *iterator;   // Used to iterate through the PMs
  struct perfect_matching *prev;  // Linked list
  struct perfect_matching *next;  // Linked list
}; // matching_t;


/** @brief Header of a perfect matching list. */
typedef struct perfect_matching_header {
  int matchings;     // Number of elements in the list
  matching_t *head;  // Pointer to first element in the list
  matching_t *tail;  // Pointer to the last element in the list
} matching_header_t;


/** @brief The structure that represents a k-partite graph.
 *
 *  k-partite graphs are graphs where the nodes may be parititoned into
 *  k disjoint subsets such that, within each subset, no two nodes share
 *  an edge. Therefore, all edges span between partitions. The struct below
 *  represents a k-partite graph.
 *
 *  "partitions" represents k, the number of partitions in the graph.
 *  partitions will define how large the partition_sizes array is, as well
 *  as how large the first two dimensions of the num_neighbors and edges arrays 
 *  are. "partitions" must be at least 2.
 *
 *  "partition_sizes" is a "partitions"-sized array containing the number
 *  of nodes per partition. Each entry in partition_sizes must be at least 1.
 *
 *  "partition_edges" is a "partitions"-sized array that counts the number
 *  of edges emanating from partition i to partition j, with j > i. NOTE
 *  THE ASSYMETRY OF THIS ARRAY, as it is used to calculate the edge ID
 *  of edges, assuming i < j.
 *
 *  "num_neighbors" is a 3-D array, num_neighbors[i][j][k], with dimensions
 *   
 *    (partitions) * (partitions) * p_sizes[i]
 *
 *  such that
 *
 *    - [i] Index into the first partition. Ranges from [0, partitions).
 *    - [j] Index into the second partition. Ranges from [0, partitions).
 *            num_neighbors[i][i] for valid i contains no information.
 *    - [k] The node in the first partition. The number stored at that
 *            index is the number of neighbors in partition j that node
 *            k is connected to.
 *
 *  "edges" is a 4-D bitvector array, edges[i][j][k][l], with dimensions
 *
 *    (partitions) * (partitions) * p_sizes[i] * (p_sizes[j]/ 8)
 *
 *  such that
 *
 *    - [i] Index into the first partition. Ranges from [0, partitions).
 *    - [j] Index into the second partition. Ranges from [0, partitions).
 *            edges[i][i] for valid i contains no information.
 *    - [k] Index into the nodes of the first partition.
 *            Ranges from [0, size[i]).
 *    - [l] Index into the bitvector containing edges from partition i to
 *            partition j. Ranges from [0, size[j] / 8). Indexing along all
 *            four dimensions returns a "slice" of the bitvector, and so some
 *            bitwise operations are required to extract the bit corresponding
 *            to node l in partition j.
 *
 *  Note that size[j] / 8 is rounded up, such that 15 -> 2.
 *
 *  Also note that the presence of edges is symmetric, so edges[i][j][k][l]
 *  and edges[j][i][l][k] will be "the same."
 *
 *  "matchngs" is a 3D array by partition 1, partition 2, and node 1 in the
 *  way defined above, that stores a linked list of perfect matchings "rooted"
 *  at that node to partition 2. See above for information.
 */
struct k_partite_graph {
  int partitions;
  int *partition_sizes;
  int *partition_edges;
  int ***num_neighbors;
  char ****edges;
  matching_header_t ***matchings;
}; // graph_t;


/** @brief Static helper array pointers to simplify function calls */
static int helper_size = 0;
static int hp1 = 0;
static int hp2 = 0;
static int *hp1s = NULL;
static int *hp2s = NULL;
static int *hp2os = NULL;
static graph_t *helper_g = NULL;


/** Helper functions */

/** @brief Performs common invariant checks on a graph.
 *
 *  Ensures that the partitions and node indexes are in range, given
 *  the provided graph.
 *
 *  @param g  A pointer to the graph structure to check.
 *  @param p1 The index of the first partition.
 *  @param n1 The node number in the first partition.
 *  @param p2 The index of the second partition.
 *  @param n2 The node number in the second partition.
 */
/*
static void check_graph_args(graph_t *g, int p1, int n1, int p2, int n2) {
  assert(g != NULL);
  assert(p1 != p2);
  assert(0 <= p1 && p1 < g->partitions);
  assert(0 <= p2 && p2 < g->partitions);
  assert(0 <= n1 && n1 < g->partition_sizes[p1]);
  assert(0 <= n2 && n2 < g->partition_sizes[p2]);
}
*/


/** Graph API */

/** @brief Creates an empty k-partite with n nodes in each partition.
 *
 *  Creates a new graph_t with k equally-sized partitions of size n.
 *
 *  Calls exit() on memory allocation failure.
 *
 *  @param partitions  The number of partitions in the graph, k.
 *  @param nodes       The number of nodes in each partition, n.
 *  @return            A k-partite graph with no edges.
 */
graph_t *graph_create(int partitions, int nodes) {
  int *sizes = xmalloc(nodes * sizeof(int));
  for (int i = 0; i < nodes; i++) {
    sizes[i] = nodes;
  }

  graph_t *g = graph_create_with_sizes(partitions, sizes);
  xfree(sizes);
  return g;
}


/** @brief Creates an empty k-partite graph with specified partition sizes.
 *
 *  Creates a new graph_t with k partitions, each with a number of nodes
 *  equal to the number indicated in the sizes array. The sizes each must
 *  be at least 1.
 *
 *  Calls exit() on memory allocation failure.
 *
 *  @param partitions  The number of partitions in the graph, k.
 *  @param sizes       The number of nodes in each partition. A k-sized array.
 *  @return            A k-partite graph with no edges.
 */
graph_t *graph_create_with_sizes(int partitions, int *sizes) {
  graph_t *g = xmalloc(sizeof(graph_t));
  g->partitions = partitions;

  // Allocate the partition sizes array and fill in with provided value
  g->partition_sizes = xmalloc(partitions * sizeof(int));
  memcpy(g->partition_sizes, sizes, partitions * sizeof(int));

  // Allocate edge counts for partitions
  //   Technically, the allocations can be made smaller, but for brevity,
  //   is given full memory here
  g->partition_edges = xcalloc(partitions, sizeof(int));

  // For each pair of partitions, allocate the num_neighbors arrays
  g->num_neighbors = xmalloc(partitions * sizeof(int **));
  for (int i = 0; i < partitions; i++) {
    g->num_neighbors[i] = xmalloc(partitions * sizeof(int *));
    for (int j = 0; j < partitions; j++) {
      if (i == j)
        continue;

      // Zero out a count array of that partition's size for each node
      g->num_neighbors[i][j] = xcalloc(sizes[i], sizeof(int));
    }
  }

  // For each pair of partitions, allocate the 2D edge arrays
  g->edges = xmalloc(partitions * sizeof(char **));
  for (int i = 0; i < partitions; i++) {
    g->edges[i] = xmalloc(partitions * sizeof(char *));
    for (int j = 0; j < partitions; j++) {
      if (i == j) // Skip on identity edges
        continue;

      // Allocate an edge bitvector for each node in this partition
      const int size = sizes[i];
      g->edges[i][j] = xmalloc(size * sizeof(char *));
      for (int k = 0; k < size; k++) {
        const int bitvec_size = ROUND_UP(sizes[j], BITS_IN_BYTE) / BITS_IN_BYTE;
        g->edges[i][j][k] = xcalloc(bitvec_size, sizeof(char));
      }
    }
  }

  // For each pair of partitions, have a NULL linked list for the nodes in p1
  g->matchings = xmalloc(partitions * sizeof(matching_t **));
  for (int i = 0; i < partitions; i++) {
    g->matchings[i] = xmalloc(partitions * sizeof(matching_t *));
    for (int j = 0; j < partitions; j++) {
      if (i == j) // Skip on identity edges/partitions
        continue;

      // Allocate a linked-list header for each node in the first partition
      const int size = sizes[i];
      g->matchings[i][j] = xcalloc(size, sizeof(matching_t)); // Set to 0/NULL
    }
  }

  return g;
}


/** @brief Frees the memory allocated for a graph.
 *
 *  @param g  A pointer to a graph to free.
 */
void graph_free(graph_t *g) {
  const int partitions = g->partitions;

  // Free inner dimensions of the num_neighbors array
  for (int i = 0; i < partitions; i++) {
    for (int j = 0; j < partitions; j++) {
      if (i == j)
        continue;
      xfree(g->num_neighbors[i][j]);
    }

    xfree(g->num_neighbors[i]);
  }

  // Free inner dimensions of the edges array
  for (int i = 0; i < partitions; i++) {
    for (int j = 0; j < partitions; j++) {
      if (i == j)
        continue;

      const int p1_size = g->partition_sizes[i];
      for (int k = 0; k < p1_size; k++) {
        xfree(g->edges[i][j][k]);
      }

      xfree(g->edges[i][j]);
    }

    xfree(g->edges[i]);
  }

  // Free the matchings
  for (int i = 0; i < partitions; i++) {
    for (int j = 0; j < partitions; j++) {
      if (i == j)
        continue;

      const int p1_size = g->partition_sizes[i];
      for (int k = 0; k < p1_size; k++) {
        matching_header_t *h = &g->matchings[i][j][k];
        while (h->head != NULL) {
          graph_remove_matching(g, i, k, j, h->head);
        }

        xfree(h);
      }

      xfree(g->matchings[i][j]);
    }

    xfree(g->matchings[i]);
  }

  // Free remaining fields
  xfree(g->partition_sizes);
  xfree(g->num_neighbors);
  xfree(g->edges);
  xfree(g->matchings);
  xfree(g);
}


/** @brief Gets the number of partitions in a graph.
 *
 *  @param g  A pointer to a graph.
 *  @return   The number of partitions in the graph.
 */
int graph_get_num_partitions(graph_t *g) {
  return g->partitions;
}


/** @brief Returns a pointer into the array of partition sizes.
 *
 *  Note that, in the interest of implementation brevity, a direct pointer
 *  into the struct is returned, and a copy of the partition sizes is
 *  NOT MADE. Therefore, this pointer DOES NOT NEED TO BE FREED.
 *
 *  The pointer returned is a const, to discourage editing of the underlying
 *  graph structure.
 *
 *  @param g  A pointer to a graph.
 *  @return   A pointer into the partition_sizes array.
 */
const int *graph_get_partition_sizes(graph_t *g) {
  return g->partition_sizes;
}


/** @brief Returns whether an edge exists between two nodes in a graph.
 *
 *  @param g   A pointer to a graph.
 *  @param p1  The index of the first partition.
 *  @param n1  The node number of the first node.
 *  @param p2  The index of the second partition.
 *  @param n2  The node number of the second node.
 *  @return    1 if an edge exists between the two nodes, 0 otherwise.
 */
int graph_is_edge_between(graph_t *g, int p1, int n1, int p2, int n2) {
  //check_graph_args(g, p1, n1, p2, n2);

  int n2_shift = n2 & BYTE_MASK;
  return (g->edges[p1][p2][n1][(n2 / BITS_IN_BYTE)] >> n2_shift) & 0x1;
}


/** @brief Get number of neighbors of one node in another partition.
 *
 *  @param g   A pointer to a graph.
 *  @param p1  The index of the partition the node is in.
 *  @param n1  The node number of the node.
 *  @param p2  The index of the partition the node is "querying."
 *  @return    The number of neighbors the node is connected to 
 *               in partition p2.
 */
int graph_get_num_neighbors(graph_t *g, int p1, int n1, int p2) {
  //check_graph_args(g, p1, n1, p2, 0);
  return g->num_neighbors[p1][p2][n1];
}


/** @brief Returns an array of integer indexes which a node is connected to
 *         in a specified partition.
 *
 *  The array indexes that are returned are n2s such that
 *
 *    graph_is_edge_between(g, p1, n1, p2, n2)
 *
 *  will return 1 for every n2.
 *
 *  The array returned will have size "size," and will need to be freed
 *  when finished.
 *
 *  @param g        A pointer to a graph.
 *  @param p1       The index of the partition the node is in.
 *  @param n1       The node number of the node.
 *  @param p2       The index of the partition the node is "querying."
 *  param size[out] A pointer to write the size of the returned array. 
 *  @return         An array containing those nodes n1 is connected to in p2.
 *                  Exit() is called on memory allocation failure, so a NULL
 *                  return value indicates that the node is connected to no
 *                  nodes in p2.
 */
int *graph_get_neighbors(graph_t *g, int p1, int n1, int p2, int *size) {
  const int num_neighbors = g->num_neighbors[p1][p2][n1];
  *size = num_neighbors;

  if (num_neighbors == 0) {
    return NULL;
  }

  const int p2_size = g->partition_sizes[p2];
  int *neighbors = xmalloc(num_neighbors * sizeof(int));
  int idx = 0;
  for (int n2 = 0; n2 < p2_size; n2++) {
    if (graph_is_edge_between(g, p1, n1, p2, n2)) {
      neighbors[idx++] = n2;
    }
  }

  return neighbors;
}


/** @brief Returns the ID of an edge between two nodes. The ID will
 *         be 1-indexed.
 *
 *  @return    Returns a unique ID for the edge, placing all edges into
 *             lexicographical ordering. If the edge is not present, 0
 *             is returned instead.
 */
int graph_get_edge_id(graph_t *g, int p1, int n1, int p2, int n2) {
  if (p1 > p2) {
    return graph_get_edge_id(g, p2, n2, p1, n1);
  }

  //check_graph_args(g, p1, n1, p2, n2);
  if (!graph_is_edge_between(g, p1, n1, p2, n2)) {
    return 0;
  }

  // Add all edges below to place into lexicographic ordering
  const int partitions = g->partitions;
  int edges = 0;
  for (int i = 0; i < p1; i++) {
    edges += g->partition_edges[i];
  }

  // Then, in current partition, loop over all nodes "lower" in lex. ordering
  for (int n = 0; n < n1; n++) {
    // Loop through every partition "greater" than p1
    for (int p = p1 + 1; p < partitions; p++) {
      edges += graph_get_num_neighbors(g, p1, n, p);
    }
  }

  // Now for the current node, add those edges "lower" in lex. ordering
  for (int n = 0; n < n2; n++) {
    if (graph_is_edge_between(g, p1, n1, p2, n)) {
      edges++;
    }
  }

  return edges + 1; // Add 1 to 1-index
}

/** @brief Adds an edge from one node to another.
 *
 *  All array (partition, node) accesses are 0-indexed. Adds the edge in
 *  both directions, to ensure symmetry of edges is maintained.
 *
 *  @param g   A pointer to a graph.
 *  @param p1  The index of the first partition.
 *  @param n1  The node number in the first partition.
 *  @param p2  The index of the second partition.
 *  @param n2  The node number in the second partition.
 */
void graph_add_edge(graph_t *g, int p1, int n1, int p2, int n2) {
  //check_graph_args(g, p1, n1, p2, n2);

  if (graph_is_edge_between(g, p1, n1, p2, n2)) {
    return;
  }

  // Add 1 to the number of neighbors for n1 and n2
  g->num_neighbors[p1][p2][n1]++;
  g->num_neighbors[p2][p1][n2]++;

  // Add 1 to the number of partition edges for the smaller partition
  if (p1 < p2) {
    g->partition_edges[p1]++; 
  } else {
    g->partition_edges[p2]++;
  }

  // Check invariant - there should be at least one edge for each node
  assert(g->num_neighbors[p1][p2][n1] > 0);
  assert(g->num_neighbors[p2][p1][n2] > 0);

  // Set the bit in the bitvector to 1
  int n1_shift = n1 & BYTE_MASK;
  int n2_shift = n2 & BYTE_MASK;
  g->edges[p1][p2][n1][(n2 / BITS_IN_BYTE)] |= (1 << n2_shift);
  g->edges[p2][p1][n2][(n1 / BITS_IN_BYTE)] |= (1 << n1_shift);
}


/** @brief Removes an edge from one node to another.
 *
 *  All array (partition, node) accesses are 0-indexed. Removes the edge
 *  from both directions, to ensure symmetry of edges is maintained.
 *
 *  @param g   A pointer to a graph.
 *  @param p1  The index of the first partition.
 *  @param n1  The node number in the first partition.
 *  @param p2  The index of the second partition.
 *  @param n2  The node number in the second partition.
 */
void graph_remove_edge(graph_t *g, int p1, int n1, int p2, int n2) {
  //check_graph_args(g, p1, n1, p2, n2);

  if (!graph_is_edge_between(g, p1, n1, p2, n2)) {
    return;
  }

  // Subtract 1 from the number of neighbors for n1 and n2
  g->num_neighbors[p1][p2][n1]--;
  g->num_neighbors[p2][p1][n2]--;

  // Subtract 1 from the number of partition edges for the smaller partition
  if (p1 < p2) {
    g->partition_edges[p1]--; 
  } else {
    g->partition_edges[p2]--;
  }

  // Check invariant - there should never be a negative number of nodes
  assert(g->num_neighbors[p1][p2][n1] >= 0);
  assert(g->num_neighbors[p2][p1][n2] >= 0);

  // Set the bit in the bitvector to 1
  int n1_shift = n1 & BYTE_MASK;
  int n2_shift = n2 & BYTE_MASK;
  g->edges[p1][p2][n1][(n2 / BITS_IN_BYTE)] &= ~(1 << n2_shift);
  g->edges[p2][p1][n2][(n1 / BITS_IN_BYTE)] &= ~(1 << n1_shift);
}


/** @brief Fully connects a single node to an entire partition.
 *
 *  @param g   A pointer to a graph.
 *  @param p1  The index of the first partition.
 *  @param n1  The node number of the partition the node comes from.
 *  @param p2  The index of the partition to fully connect to.
 */
void graph_fully_connect_node(graph_t *g, int p1, int n1, int p2) {
  const int p2_size = g->partition_sizes[p2];
  for (int n2 = 0; n2 < p2_size; n2++) {
    graph_add_edge(g, p1, n1, p2, n2);
  }
}


/** @brief Fully connects a partition to another partition.
 *
 *  @param g   A pointer to the graph structure.
 *  @param p1  The index of the first partition.
 *  @param p2  The index of the second partition.
 */
void graph_fully_connect_partition(graph_t *g, int p1, int p2) {
  const int p1_size = g->partition_sizes[p1];
  for (int n1 = 0; n1 < p1_size; n1++) {
    graph_fully_connect_node(g, p1, n1, p2);
  }
}


/** @brief Returns the size of the shared neighborhood between the vertices.
 *
 *  TODO hard-coded from partition 0 to 1.
 */
static int get_shared_neighborhood_size(int *verts, int num) {
  int size;
  int *neighs = graph_get_neighbors(helper_g, 0, verts[0], 1, &size);
  int left = size;

  for (int i = 1; i < num; i++) {
    int s;
    int *ns = graph_get_neighbors(helper_g, 0, verts[i], 1, &s);
    for (int j = 0; j < left; j++) {
      int val = neighs[j];
      int found = 0;
      for (int s_idx = 0; s_idx < s; s_idx++) {
        if (ns[s_idx] == val) {
          found = 1;
          break;
        }
      }

      if (!found) {
        memmove(neighs + j, neighs + j + 1, ((left - j) - 1) * sizeof(int));
        left--;
        j--;
      }
    }

    xfree(ns);
  }

  xfree(neighs);
  return left;
}


/** @brief Returns the minimum size of the pairwise shared neighborhoods
 *         between the vertices.
 */
static int get_pairwise_neighborhood_min_size(int *verts, int num) {
  int min = INT_MAX;
  for (int i = 0; i < num; i++) {
    for (int j = i + 1; j < num; j++) {
      int arr[2] = {verts[i], verts[j]};
      int res = get_shared_neighborhood_size(arr, 2);
      if (res < min) {
        min = res;
      }
    }
  }

  return min;
}


/** @brief Generates all permutations of the right node subset.
 *
 *  @param lo  The low index
 *  @return 1 if a perfect matching is found, 0 otherwise
 */
static void generate_subset_permutations(int lo) { 
  if (lo == helper_size - 1) {
    // Check the final edge for existence
    int map = hp2os[lo];
    if (graph_is_edge_between(helper_g, hp1, hp1s[lo], hp2, hp2s[map])) {
      // Have a perfect matching, add to the list
      // But only add it if there are no edges in common with any
      // of the previous perfect matchings of these two sets
      matching_header_t *h = &helper_g->matchings[hp1][hp2][hp1s[0]];
      if (h->tail != NULL) {
        // Assume that we take the matchings in lexicographic order - only end
        // TODO remove
        matching_t *curr = h->tail;
        if (curr->size == helper_size &&
            memcmp(curr->p1_nodes, hp1s, helper_size * sizeof(int)) == 0 &&
            memcmp(curr->p2_nodes, hp2s, helper_size * sizeof(int)) == 0) {
          // Vertex sets are the same - check for a shared edge in any found
          node_ordering_t *iter = curr->head;
          while (iter != NULL) {
            for (int i = 0; i < helper_size; i++) {
              if (iter->ordering[i] == hp2os[i]) {
                 return;
              }
            }

            iter = iter->next;
          }

          // Doesn't share a matching, so add an ordering
          node_ordering_t *new_ordering = xmalloc(sizeof(node_ordering_t));
          new_ordering->ordering = xmalloc(helper_size * sizeof(int));
          memcpy(new_ordering->ordering, hp2os, helper_size * sizeof(int));

          // Add to the linked list - at the tail
          new_ordering->prev = curr->tail;
          new_ordering->next = NULL;
          curr->tail->next = new_ordering;
          curr->num_orderings++;
          curr->tail = new_ordering;
          h->matchings++;
          return;
        }
      }

      // New matching struct is necessary
      matching_t *m = xmalloc(sizeof(matching_t));
      node_ordering_t *ordering = xmalloc(sizeof(node_ordering_t));
      ordering->ordering = xmalloc(helper_size * sizeof(int));
      m->size = helper_size;
      m->p1_nodes = xmalloc(helper_size * sizeof(int));
      m->p2_nodes = xmalloc(helper_size * sizeof(int));
      m->num_orderings = 1;
      m->head = ordering;
      m->tail = ordering;
      m->iterator = ordering;
      memcpy(m->p1_nodes, hp1s, helper_size * sizeof(int));
      memcpy(m->p2_nodes, hp2s, helper_size * sizeof(int));
      memcpy(ordering->ordering, hp2os, helper_size * sizeof(int));
      ordering->next = NULL;
      ordering->prev = NULL;

      // Insert into the back of the list
      m->next = NULL;
      if (h->tail != NULL) {
        h->tail->next = m;
        m->prev = h->tail;
      } else {
        h->head = m;
        m->prev = NULL;
      }

      h->tail = m;
      h->matchings++;
    }
  } else {
    for (int i = lo; i < helper_size; i++) {
      int temp = hp2os[lo];
      hp2os[lo] = hp2os[i];
      hp2os[i] = temp;

      // Check if edge exists - if not, no need to recurse
      int map = hp2os[lo];
      if (graph_is_edge_between(helper_g, hp1, hp1s[lo], hp2, hp2s[map])) {
        // Check that we are not sharing an edge with a previously found p.m.
        // that has the same vertex sets on left and right
        // Since we are adding matchings to the tail, only check last one
        matching_header_t *h = &helper_g->matchings[hp1][hp2][hp1s[0]];
        if (h->tail != NULL) {
          matching_t *curr = h->tail;
          // Examine left and right for match, then check for shared edge
          if (curr->size == helper_size &&
              memcmp(curr->p1_nodes, hp1s, helper_size * sizeof(int)) == 0 &&
              memcmp(curr->p2_nodes, hp2s, helper_size * sizeof(int)) == 0) {
            node_ordering_t *iter = curr->head;
            while (iter != NULL) {
              int end = (lo == helper_size - 2) ? helper_size - 1 : lo;
              for (int i = 0; i <= end; i++) {
                if (iter->ordering[i] == hp2os[i]) {
                  goto swap;
                }
              }

              iter = iter->next;
            }
          }
        }

        // Nothing found on shared edge check - keep going!
        generate_subset_permutations(lo + 1);
      }

swap:
      temp = hp2os[lo];
      hp2os[lo] = hp2os[i];
      hp2os[i] = temp;
    }
  }
}


static void generate_permutations(const int p1_size, const int p2_size) {
  // Reset the p1s array
  for (int i = 0; i < helper_size; i++) {
    hp1s[i] = i;
  }

  // Run through all subsets of p1_size for hp1s
  while (hp1s[0] < p1_size - helper_size + 1) {
    // Analysis on neighbors of hp1s for intersection.
    // Based on hard-coded
    if (helper_size == 2) {
      // We are looking for K_{2, 2}s, so any pair of starting nodes need
      // a shared neighborhood of at least two
      if (get_shared_neighborhood_size(hp1s, 2) < 2) {
        goto permute_p1;
      }
    } else if (helper_size == 3) {
      // If the shared neighborhood is less than 3, then look for C6s
      if (get_shared_neighborhood_size(hp1s, 3) < 3 &&
          get_pairwise_neighborhood_min_size(hp1s, 3) < 1) {
        goto permute_p1;
      }
    }
    

    // Reset p2 array
    for (int i = 0; i < helper_size; i++) {
      hp2s[i] = i;
    }

    // Run through all subsets of p2_size for hp2s
    while (hp2s[0] < p2_size - helper_size + 1) {
      // Reset the ordering array
      for (int i = 0; i < helper_size; i++) {
        hp2os[i] = i;
      }

      // Run through all permutations of subsets of p2 and check for perf. mat.
      generate_subset_permutations(0);

      // Increment the end of p2s, modulo by size
      hp2s[helper_size - 1]++;
      while (hp2s[helper_size - 1] >= p2_size) {
        int idx = helper_size - 1;
        do {
          idx--;
          hp2s[idx]++;
        } while (hp2s[idx] >= p2_size && idx > 0);

        // Now that we stopped, we found an element that is not above n
        // Propagate this value monotonically down the line
        for (int l = idx + 1; l < helper_size; l++) {
          hp2s[l] = hp2s[l - 1] + 1;
        }

        if (hp2s[0] >= p2_size - helper_size + 1) 
          break;
      }
    }
    
permute_p1:
    // Increment the end of p1s, modulo by size
    hp1s[helper_size - 1]++;
    while (hp1s[helper_size - 1] >= p1_size) {
      int idx = helper_size - 1;
      do {
        idx--;
        hp1s[idx]++;
      } while (hp1s[idx] >= p1_size && idx > 0);

      // Now that we stopped, we found an element that is not above n
      // Propagate this value monotonically down the line
      for (int l = idx + 1; l < helper_size; l++) {
        hp1s[l] = hp1s[l - 1] + 1;
      }

      if (hp1s[0] >= p1_size - helper_size + 1) 
        break;
    }
  }
}


/** @brief Generate perfect matchings.
 *
 *
 *  @param g           A pointer to a graph.
 *  @param up_to_size  Generates all perfect matchings of size up to this
 *                     variable. Note that the runtime of this goes up
 *                     exponentially in the magnitude of up_to_size.
 *                     Must be at least 2.
 */
void graph_generate_perfect_matchings(graph_t *g, int up_to_size) {
  assert(up_to_size >= 2);

  helper_g = g;
  hp1s = xmalloc(up_to_size * sizeof(int));
  hp2s = xmalloc(up_to_size * sizeof(int));
  hp2os = xmalloc(up_to_size * sizeof(int));

  const int partitions = g->partitions;
  for (helper_size = 2; helper_size <= up_to_size; helper_size++) {
    for (hp1 = 0; hp1 < partitions; hp1++) {
      for (hp2 = hp1 + 1; hp2 < partitions; hp2++) {
        const int p1_size = g->partition_sizes[hp1];
        const int p2_size = g->partition_sizes[hp2];
        generate_permutations(p1_size, p2_size);
      }
    }
  }

  xfree(hp1s);
  xfree(hp2s);
  xfree(hp2os);

  hp1s = NULL;
  hp2s = NULL;
  hp2os = NULL;

  // Because PMs are generated and stored for the purpose of blocking,
  //   remove those PMs that don't have additional PMs on the same nodes
  for (int p1 = 0; p1 < partitions; p1++) {
    for (int p2 = p1 + 1; p2 < partitions; p2++) {
      const int p1_size = g->partition_sizes[p1];
      for (int n1 = 0; n1 < p1_size; n1++) {
        if (graph_get_num_matchings(g, p1, n1, p2) > 0) {
          matching_t *m = graph_get_first_matching(g, p1, n1, p2);
          matching_t *prev;
          while (m != NULL) {
            prev = m;
            m = graph_get_next_set(m);
            if (graph_get_num_similar_matchings(prev) == 1) {
              graph_remove_matching(g, p1, n1, p2, prev);
            }
          }
        }
      }
    }
  }
}


/** @brief Gets the first perfect matching for the partitions and node.
 *
 *  @param g   A pointer to a graph.
 *  @param p1  The index of the first partition.
 *  @param n1  The node number of the partition the node comes from.
 *  @param p2  The index of the second partition.
 *  @return    The first perfect matching for the partitions and node.
 *             NULL is returned if there is no perfect matching at n1.
 */
matching_t *graph_get_first_matching(graph_t *g, int p1, int n1, int p2) {
  //check_graph_args(g, p1, n1, p2, 0);
  matching_t *m = g->matchings[p1][p2][n1].head;
  m->iterator = m->head;
  return m;
}


/** @brief Gets the previous perfect matching for that matching.
 *  
 *  @param m  A perfect matching.
 *  @return   A pointer to the previous prefect matching, NULL if no more.
 */
matching_t *graph_get_prev_matching(matching_t *m) {
  assert(m != NULL && m->iterator != NULL);
  if (m->iterator->prev == NULL) {
    if (m->prev != NULL) {
      m->prev->iterator = m->prev->tail;
    }

    return m->prev;
  } else {
    m->iterator = m->iterator->prev;
    return m;
  }
}


/** @brief Gets the next perfect matching for that node.
 *  
 *  The perfect matchings are ordered by size, and within a size class,
 *  lexicographically.
 *
 *  @param m  A perfect matching.
 *  @return   A pointer to the next perfect matching. NULL if no more.
 */
matching_t *graph_get_next_matching(matching_t *m) {
  assert(m != NULL && m->iterator != NULL);
  if (m->iterator->next == NULL) {
    if (m->next != NULL) {
      m->next->iterator = m->next->head;
    }

    return m->next;
  } else {
    m->iterator = m->iterator->next;
    return m;
  }
}

matching_t *graph_get_next_set(matching_t *m) {
  return m->next;
}

matching_t *graph_get_prev_set(matching_t *m) {
  return m->prev;
}

/** @brief Gets the number of matchings for that node.
 *
 *  Includes perfect matchings of all sizes.
 *
 *  @param g  A pointer to a graph.
 *  @param 
 */
int graph_get_num_matchings(graph_t *g, int p1, int n1, int p2) {
  return g->matchings[p1][p2][n1].matchings;
}


/** @brief Gets the size of the perfect matching.
 *
 *  @param m  A pointer to a matching.
 *  @return   The number of nodes on "one side" of the matching.
 */
int graph_get_matching_size(matching_t *m) {
  assert(m != NULL);
  return m->size;
}


/** @brief Returns the number of perfect matchings with p1_nodes and
 *         p2_nodes as the node subsets. For this size class only.
 *
 *  @param m  A pointer to a matching.
 *  @return   The number of perfect matchings on this subset of nodes.
 */
int graph_get_num_similar_matchings(matching_t *m) {
  return m->num_orderings;
}


/** @brief Gets a pointer to the nodes on the left of the perfect matching.
 *
 *  The nodes involved in the first partition.
 *
 *  @param m  A pointer to a perfect matching.
 *  @return   A pointer to the nodes involved on the left of the matching.
 */
const int *graph_get_matching_left_nodes(matching_t *m) {
  return m->p1_nodes;
}


/** @brief Gets a pointer to the nodes on the right of the perfect matching.
 *
 *  The nodes involved in the second partition.
 *
 *  @param m  A pointer to a perfect matching.
 *  @return   A pointer to the nodes involved on the right of the matching.
 */
const int *graph_get_matching_right_nodes(matching_t *m) {
  return m->p2_nodes;
}


const int *graph_get_matching_ordered_right_nodes(matching_t *m) {
  assert(m != NULL && m->iterator != NULL);
  return m->iterator->ordering;
}


/** @brief Removes the matching from the graph structure. Updates internal
 *         data and bookkeeping.
 *
 *  @param g   A pointer to a graph.
 *  @param p1  The first partition index.
 *  @param n1  The index of the first node.
 *  @param p2  The second partition index.
 *  @param m   A pointer to a perfect matching.
 */
void graph_remove_matching(graph_t *g, int p1, int n1, int p2, matching_t *m) {
  // Remove the matching from the linked list
  matching_header_t *h = &g->matchings[p1][p2][n1];

  // If the perfect matching has more than one ordering, remove the ordering
  if (m->num_orderings > 1) {
    assert(m->iterator != NULL);
    node_ordering_t *iter = m->iterator;

    // Remove from the queue by re-routing pointers
    if (m->iterator->prev != NULL) {
      m->iterator->prev->next = m->iterator->next;
    } else {
      m->head = m->iterator->next;
    }

    if (m->iterator->next != NULL) {
      m->iterator->next->prev = m->iterator->prev;
    } else {
      m->tail = m->iterator->prev;
    }

    m->num_orderings--;

    // Free the ordering
    xfree(iter->ordering);
    xfree(iter);
  } else {
    // Remove the entire perfect matching from the larger linked list
    // First, free the iterator
    xfree(m->head->ordering);
    xfree(m->head);

    // Re-route pointers
    if (m->prev != NULL) {
      m->prev->next = m->next;
    } else {
      h->head = m->next;
    }

    if (m->next != NULL) {
      m->next->prev = m->prev;
    } else {
      h->tail = m->prev;
    }  

    xfree(m->p2_nodes);
    xfree(m->p1_nodes);
    xfree(m);
  }

  h->matchings--;
}


// TODO Remove later
void graph_print_perfect_matching(matching_t *m) {
  if (m->iterator == NULL) {
    m->iterator = m->head;
  }

  node_ordering_t *o = m->iterator;

  printf("[");
  for (int i = 0; i < m->size; i++) {
    printf("%d ", m->p1_nodes[i]);
  }
  printf("] [");
  for (int i = 0; i < m->size; i++) {
    printf("%d ", m->p2_nodes[o->ordering[i]]);
  }
  printf("]\n");
}
