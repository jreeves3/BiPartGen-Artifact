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


/** @file bipartgen.c
 *  @brief CNF formula generator for bipartite graph problems, including
 *         mutilated chessboard, pigeonhole principle, and their variants.
 *
 *  ///////////////////////////////////////////////////////////////////////////
 *  // POTENTIAL IMPROVEMENTS
 *  ///////////////////////////////////////////////////////////////////////////
 *
 *  Many of the graph add() and remove() functions take in (p1, n1, p2, n2)
 *  arguments, which could take up less space if encapsulated in a struct.
 *
 *  Randomize flipping edges?
 *
 *  ///////////////////////////////////////////////////////////////////////////
 *  // USAGE
 *  ///////////////////////////////////////////////////////////////////////////
 *
 *  @usage ./bipartgen (-options)
 *
 *  ///////////////////////////////////////////////////////////////////////////
 *  // ACKNOWLEDGEMENTS
 *  ///////////////////////////////////////////////////////////////////////////
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "xmalloc.h"
#include "graph.h"
#include "mchess.h"
#include "pigeon.h"
#include "additionalgraphs.h"

#define BLOCKED_CLAUSE_PROB_DENOM 1000

/** @brief Method for how perfect matchings are blocked.
 *
 *  ALL means that all perfect matchings that can be blocked, are blocked.
 *  They are blocked in lexicographical order.
 *
 *  PROB means that the perfect matchings are probabilistically added or not.
 *  The probability is specified with the -B flag.
 *
 *  COUNT means that the first X perfect matchings are blocked for any fixed
 *  first node. They are blocked in lexicographic order. COUNT INFTY reduces
 *  to ALL.
 */
typedef enum perfect_matching_blocking_method {
  ALL, PROB, COUNT
} blocking_method_t;

/** @brief Generates blocked clauses of perfect matchings up to this size. */
static int blocked_clause_size = -1;

/** @brief Either adds blocked clauses with this probability, or adds this many
 *         blocked clauses for each fixed first node.
 *
 *  If the -B flag is a float that is less than 1, then it is converted into
 *  an integer (out of 1000) to serve as the odds of blocking any perfect
 *  matching. Otherwise, the first X perfect matchings are blocked.
 */
static int blocked_clause_prob;
static blocking_method_t blocking_method = ALL;
static int avoid_blocking_overlap = 0;

static int rand_seed = 0;

FILE *pgbdd_bucket_f, *pgbdd_var_f;
int *aux_var_map1, *aux_var_map2;
static bool pgbdd_bucket = false;
static bool pgbdd_var_ord = false;
static bool randomGr = false;
static int verbosity_level = 0;

static void print_help(char *runtime_path) {
  printf("\n%s: BiPartGen Hard CNF Generator\n", runtime_path);
  printf("Developed by: Joseph Reeves and Cayden Codel\n\n");
  printf("  -a            Perfect matchings and witnesses do not overlap\n");
  printf("  -b <size>     Block perfect matchings up to this size.\n");
  printf("  -B <float>    If < 1.0, block prob. if >= 1, int, num per node.\n");
  printf("  -c <int>      Cardinality (difference in partition size)\n");
  printf("  -E <int>      Edge count for graph\n");
  printf("  -C <method>   Specify chess variant (NORMAL|TORUS|CYLINDER).\n");
  printf("  -D <float>    Density for random graphs.\n");
  printf("  -e <method>   Specify encoding variant (direct|linear|sinz|mixed).\n");
  printf("  -f <name>     Output file to write CNF to.\n");
  printf("  -g <graph>    Specify type of problem (chess|pigeon|random).\n");
  printf("  -h            Display this help message.\n");
  printf("  -L            Use an additional \"At least one\" encoding.\n");
  printf("  -M            Use an additional \"At most one\" encoding.\n");
  printf("  -n <size>     Size of problem (nxn chess, n holes, n nodes).\n");
  printf("  -s <int>      Randomization seed, if applicable.\n");
  printf("  -p            Bucket permutation (used for Sinz encoding).\n");
  printf("  -o            Variable ordering (used for linear and Sinz encoding).\n");
  printf("  -v            Verbosity level 1 (print graph density).\n");
}

/********** Graph CNF Encodings ************/

// Functions written assuming a bipartite graph structure for now.

/** @brief Write direct At Most 1 encoding.
 *
 *  @param f               A pointer to the output file.
 *  @param edges      An array of the connected nodes.
 *  @param size_edges Size of array edges.
 */
static void direct_atMost_encoding(FILE *f, int *edges, int size_edges) {
  int i,j;
  for(i=0; i<size_edges; i++) {
    for(j=i+1; j<size_edges; j++) {
      fprintf(f, "%d %d 0\n", -edges[i], -edges[j]);
    }
  }
}

/** @brief Write linear At Most 1 encoding.
 *
 *  @param f              A pointer to the output file.
 *  @param edges     An array of the connected nodes.
 *  @param size_edges Size of array edges.
 *  @param curr_i   Current index into edges.
 *  @param ex_var   Extension variable id.
 *
 *  @return Value of next availiable variable ID.
 */
static int linear_atMost_encoding(
                                  FILE *f, int *edges, int size_edges, int curr_i, int ex_var) {
  
  bool linear = (size_edges-curr_i>4)?true:false;
  int s = (linear)?4:(size_edges-curr_i);
  int linear_edges[s];
  
  for(int i=0; i<s; i++) {
    if (i == 3 && linear) {
      linear_edges[i] = ex_var;
      if (pgbdd_var_ord) {
        aux_var_map1[linear_edges[2]] = ex_var;
      }
    }
    else linear_edges[i] = edges[i+curr_i];
  }
  
  // Direct Encoding for the linear
  direct_atMost_encoding(f, linear_edges, s);
  
  if (linear) {
    edges[curr_i+2] = -ex_var;
    // Recursive Call for remaining variables
    return linear_atMost_encoding(f,edges,size_edges,curr_i+2,ex_var+1);
  }
  else return ex_var;
}

/** @brief Get sinz variable ID, Si,j (i starts at 0).
 *
 *  @param i          Xi, starting from 0 .. size_edges-1.
 *  @param j          Signal value from 1 .. size_edges.
 *  @param sinz_var   Offset of sinz variable.
 */
static int sinz_variableID(int i, int sinz_var) {
  return (sinz_var + i);
}

/** @brief Write Sinz At Most 1 encoding.
 *
 *  @param f                   A pointer to the output file.
 *  @param edges          An array of the connected nodes.
 *  @param size_edges Size of array edges.
 *  @param sinz_var   First ID of sinz variable, updated before return.
 *
 *  @return Value of next availiable variable ID.
 */
static int sinz_atMost_encoding(FILE *f, int *edges, int size_edges, int sinz_var) {
  
  if (size_edges == 2) {
    if (randomGr) {
      fprintf(f, "%d %d 0\n", -edges[0], sinz_variableID(0,sinz_var));
      fprintf(f, "%d %d 0\n", -edges[1], -sinz_variableID(0,sinz_var));
      if (pgbdd_bucket) {
        fprintf(pgbdd_var_f,"%d \n", edges[0]);
        fprintf(pgbdd_var_f,"%d \n", sinz_variableID(0,sinz_var));
        fprintf(pgbdd_var_f,"%d \n", edges[1]);
        aux_var_map1[edges[1]] = sinz_variableID(0,sinz_var);
        return sinz_variableID(size_edges-1,sinz_var);
      }
      else if (pgbdd_var_ord) {
        aux_var_map1[edges[0]] = sinz_variableID(0,sinz_var);
        return sinz_variableID(size_edges-1,sinz_var);
      }
      return sinz_var + 1;
    }
    else {
      fprintf(f, "%d %d 0\n", -edges[0], -edges[1]);
      return sinz_var;
    }
  }
  else {
    if (pgbdd_bucket) fprintf(pgbdd_var_f,"%d \n", edges[0]);
    for(int i = 0; i < size_edges; i++) {
      if (i < (size_edges-1)) {
        // signal variable (no signal for last variable Xn)
        fprintf(f, "%d %d 0\n", -edges[i], sinz_variableID(i,sinz_var));
        if (pgbdd_bucket) {
          fprintf(pgbdd_var_f,"%d \n", sinz_variableID(i,sinz_var));
          fprintf(pgbdd_var_f,"%d \n", edges[i+1]);
          aux_var_map1[edges[i+1]] = sinz_variableID(i,sinz_var);
        }
        else if (pgbdd_var_ord) aux_var_map1[edges[i]] = sinz_variableID(i,sinz_var);
      }
      if (i > 0) {
        // Not previous signal and current variable
        fprintf(f, "%d %d 0\n", -edges[i], -sinz_variableID(i-1,sinz_var));
        if (i < (size_edges - 1)) {
          // signal propogates forward
          fprintf(f, "%d %d 0\n", -sinz_variableID(i-1,sinz_var), sinz_variableID(i,sinz_var));
        }
      }
    }
    // Update for next call
    return sinz_variableID(size_edges-1,sinz_var);
  }
  return 0;
}

/** @brief Get edge variable ID.
 *
 *   Edge variable IDs given for evert possible edge. Count up from all
 *   possible edges of first node in first patition (partitions ordered), then
 *   second node, etc.
 *
 *  @param g  A pointer to the graph structure.
 *  @param p1 The index of the partition the node is in.
 *  @param n1 The node number of the node.
 *  @param p2 The index of the partition the node is "querying."
 *  @param n2 The node number of connected node from p2.
 */
static int get_variableID(graph_t *g, int p1, int n1, int p2, int n2) {
  int s = (p1<p2)?p2:p1;
  int n1N =(p1<p2)?n1:n2;
  int n2N =(p1<p2)?n2:n1;
  return (1 + n2N + (graph_get_partition_sizes(g)[s] * n1N));
}

/** @brief Extract CNF formulas from graph.
 *
 *  @param g  A pointer to the graph structure.
 *  @param f  A pointer to the output file.
 *  @param en The translation encoding type
 *  @param atMost1 Partitions to get at most 1 constraints.
 *  @param aLeast1 Partitions to get at least 1 constraints.
 *  @param atMSize Size of atMost1.
 *  @param atLSize Size of atLeast1.
 */
static void write_cnf_from_graph(
                                 graph_t *g, FILE *f, char* en, int* atMost1, int* atLeast1,
                                 int atMSize, int atLSize) {
  
  const int *partition_sizes = graph_get_partition_sizes(g);
  
  // Vaiable name for every possible edge (many will be unused)
  int nvars = partition_sizes[0] * partition_sizes[1];
  int ex_var = nvars+1;
  int p1,p2, nclauses = 0, nAtLeast = 0, nAtMost = 0,r;
  int *size_nodes, *connected_nodes, *edges;
  bool mixed = strcmp(en,"mixed")==0;
  char* mixed_encodings[partition_sizes[0]+partition_sizes[1]];
  
  size_nodes = xmalloc(sizeof(int));
  
  srand(rand_seed);
  
  // get number of clauses
  for(int p = 0; p < atLSize; p++) {
    // at Least 1 clauses
    p1 = atLeast1[p];
    p2 = (p1==0)?1:0;
    for(int i=0; i< partition_sizes[p1]; i++) {
      connected_nodes = graph_get_neighbors(g, p1, i, p2, size_nodes);
      if (*size_nodes > 0) nAtLeast++;
      free(connected_nodes);
    }
  }
  
  int encCnt = 0;
  for(int p = 0; p < atMSize; p++) {
    p1 = atMost1[p];
    p2 = (p1==0)?1:0;
    for(int i=0; i < partition_sizes[p1]; i++) {
      connected_nodes = graph_get_neighbors(g, p1, i, p2, size_nodes);
      if (*size_nodes > 1) {
        if (mixed) { // mixed encoding selects from three encoding options
          r = rand() % 3;
          if (r==0) {
            en = "direct";
          }
          else if (r==1) {
            en = "sinz";
          }
          else {
            en = "linear";
          }
          mixed_encodings[encCnt++] = en;
        }
        if (strcmp(en,"direct")==0) {
          // Direct encoding
          nAtMost += (*size_nodes*(*size_nodes-1))/2;
        } else if (strcmp(en,"sinz")==0) {
          // Sinz encoding
          if (*size_nodes > 2) {
            nvars += (*size_nodes-1); // No ex_var for last var
            nAtMost += (*size_nodes-2)*3; // constraints for all but first and last var
            nAtMost += 2; // First and last clauses
          }
          else {
            nAtMost += 1;
            if (randomGr) {nAtMost++;nvars++;}
          }
          // here for pgbdd options
        }
        else if (strcmp(en,"linear")==0) {
          // linear encoding
          if (*size_nodes == 2) nAtMost += 1;
          else {
            nAtMost += 3*(*size_nodes) - 6;
            nvars += (*size_nodes-3)/2;
          }
        }
        free(connected_nodes);
      }
    }
  }
  
  
  nclauses = nAtMost + nAtLeast;
  
  // Blocked clauses here - check options
  if (blocked_clause_size >= 2) {
    graph_generate_perfect_matchings(g, blocked_clause_size);
    
    // TODO remove later - sanity check
    /*
     for (int i = 0; i < partition_sizes[p1]; i++) {
     if (graph_get_num_matchings(g, 0, i, 1) > 0) {
     matching_t *m = graph_get_first_matching(g, 0, i, 1);
     while (m != NULL) {
     graph_print_perfect_matching(m);
     m = graph_get_next_matching(m);
     }
     }
     }
     printf("\n");
     */
    
    if (blocking_method == PROB) {
      srand(rand_seed);
    }
    
    /* We consider all perfect matchings on each set of left and right nodes.
     *
     * We must leave at least one perfect matching on those nodes, but are
     *   free to block all but one.
     *
     * However, we don't want to reduce the number of solutions, so optionally,
     *   we keep track of the non-PM edges that are "kept" on a PM block.
     *   Future blockings of PMs consult a dictionary of these non-PM edges
     *   and will avoid blocking PMs that have non-PM edges from earlier PM
     *   blockings.
     */
    // TODO hard-coded bipartite
    // TODO how does COUNT get implemented?
    const int p1_size = partition_sizes[0];
    const int p2_size = partition_sizes[1];
    
    // Keep track of edges involved in PMs, and edges involved as witnesses
    int blocked_edges[p1_size][p2_size];
    int witness_edges[p1_size][p2_size];
    for (int i = 0; i < p1_size; i++) {
      for (int j = 0; j < p2_size; j++) {
        blocked_edges[i][j] = 0;
        witness_edges[i][j] = 0;
      }
    }
    
    int matchings_blocked = 0;
    for (int i = 0; i < p1_size; i++) {
      // int num_blocked = 0;
      if (graph_get_num_matchings(g, 0, i, 1) > 0) {
        matching_t *m = graph_get_first_matching(g, 0, i, 1);
        while (m != NULL) {
          int num_similar = graph_get_num_similar_matchings(m);
          assert(num_similar >= 2);
          
          // Alias various data in the matching
          int size = graph_get_matching_size(m);
          const int *p1s = graph_get_matching_left_nodes(m);
          const int *p2s = graph_get_matching_right_nodes(m);
          
          // Deterministic, random, avoid
          if (avoid_blocking_overlap) {
            /* For each set of left and right nodes, we want to find one PM
             * that will act as a witness. The rest can be blocked, assuming
             * the edges don't intersect those used in other witnesses.
             */
            // Look for a witness first
            int found_witness = 0;
            int witness_idx = 0;
            int mi;
            for (mi = 0; mi < num_similar && !found_witness; mi++) {
              const int *p2o = graph_get_matching_ordered_right_nodes(m);
              
              // Check all edges in this ordering to see if any are blocked
              int witness_candidate = 1;
              for (int n = 0; n < size; n++) {
                if (blocked_edges[p1s[n]][p2s[p2o[n]]]) {
                  witness_candidate = 0;
                  break;
                }
              }
              
              // If a witness candidate made it through, mark the edges
              if (witness_candidate) {
                found_witness = 1;
                witness_idx = mi;
                for (int n = 0; n < size; n++) {
                  witness_edges[p1s[n]][p2s[p2o[n]]]++;
                }
                break;
              }
              
              m = graph_get_next_matching(m);
            }
            
            // Now if witness found, block remaining
            if (found_witness) {
              for (; mi > 0; mi--) {
                m = graph_get_prev_matching(m);
              }
              
              for (mi = 0; mi < num_similar; mi++) {
                if (mi == witness_idx) {
                  m = graph_get_next_matching(m);
                  continue;
                }
                
                // Block ordering only if doesn't intersect witness edges
                const int *p2o = graph_get_matching_ordered_right_nodes(m);
                int blocking_candidate = 1;
                for (int n = 0; n < size; n++) {
                  if (witness_edges[p1s[n]][p2s[p2o[n]]]) {
                    blocking_candidate = 0;
                    break;
                  }
                }
                
                if (blocking_candidate) {
                  matchings_blocked++;
                  for (int n = 0; n < size; n++) {
                    blocked_edges[p1s[n]][p2s[p2o[n]]]++;
                  }
                }
                
                m = graph_get_next_matching(m);
              }
            }
          } else if (blocking_method == PROB) {
            for (int n = 0; n < num_similar - 1; n++) {
              if (rand() % BLOCKED_CLAUSE_PROB_DENOM < blocked_clause_prob) {
                matchings_blocked++;
              }
            }
            m = graph_get_next_set(m);
          } else {
            matchings_blocked += num_similar - 1;
            m = graph_get_next_set(m);
          }
        }
      }
    }
    
    // Now add the number of blocked clauses
    nclauses += matchings_blocked;
    printf("%d matchings were blocked\n", matchings_blocked);
  }
  
  // Write Header
  fprintf(f, "p cnf %d %d\n", nvars, nclauses);
  
  // Write constraints
  for(int p = 0; p < atLSize; p++) {
    // Write atLeast constraints
    p1 = atLeast1[p];
    p2 = (p1==0)?1:0;
    for(int i=0; i< partition_sizes[p1]; i++) {
      connected_nodes = graph_get_neighbors(g, p1, i, p2, size_nodes);
      if (*size_nodes > 0) {
        //At least one node
        for(int n = 0; n < *size_nodes; n++) {
          fprintf(f, "%d ", get_variableID(g,p1,i,p2,connected_nodes[n]));
        }
        fprintf(f, "0\n");
      }
      free(connected_nodes);
    }
  }
  
  encCnt = 0;
  for(int p = 0; p < atMSize; p++) {
    // Write atMost constraints
    p1 = atMost1[p];
    p2 = (p1==0)?1:0;
    for(int i=0; i< partition_sizes[p1]; i++) {
      connected_nodes = graph_get_neighbors(g, p1, i, p2, size_nodes);
      if (*size_nodes > 1) {
        // At least two nodes
        edges = xmalloc(*size_nodes * sizeof(int));
        // Get edge variable names
        for(int n = 0; n < *size_nodes; n++) {
          edges[n] = get_variableID(g,p1,i,p2,connected_nodes[n]);
        }
        if (mixed) en = mixed_encodings[encCnt++];
        if (strcmp(en,"direct")==0) {
          // Direct encoding
          direct_atMost_encoding(f, edges, *size_nodes);
          
        } else if (strcmp(en,"sinz")==0) {
          // Sinz encoding
          ex_var = sinz_atMost_encoding(f, edges, *size_nodes, ex_var);
        }
        else if (strcmp(en,"linear")==0) {
          ex_var = linear_atMost_encoding(f,edges,*size_nodes,0,ex_var);
        }
        free(edges);
      }
      free(connected_nodes);
    }
  }
  
  // TODO hard-coded 0 and 1 bipartite
  // Write blocked clauses - same identification protocol as before
  fprintf(f, "c Below are the blocked clauses from perfect matchings\n");
  if (blocked_clause_size >= 2) {
    if (blocking_method == PROB) {
      srand(rand_seed);
    }
    
    const int p1_size = partition_sizes[0];
    const int p2_size = partition_sizes[1];
    
    // Keep track of edges involved in PMs, and edges involved as witnesses
    int blocked_edges[p1_size][p2_size];
    int witness_edges[p1_size][p2_size];
    for (int i = 0; i < p1_size; i++) {
      for (int j = 0; j < p2_size; j++) {
        blocked_edges[i][j] = 0;
        witness_edges[i][j] = 0;
      }
    }
    
    for (int i = 0; i < p1_size; i++) {
      // int num_blocked = 0;
      if (graph_get_num_matchings(g, 0, i, 1) > 0) {
        matching_t *m = graph_get_first_matching(g, 0, i, 1);
        while (m != NULL) {
          int num_similar = graph_get_num_similar_matchings(m);
          assert(num_similar >= 2);
          
          // Alias various data in the matching
          int size = graph_get_matching_size(m);
          const int *p1s = graph_get_matching_left_nodes(m);
          const int *p2s = graph_get_matching_right_nodes(m);
          
          // Deterministic, random, avoid
          if (avoid_blocking_overlap) {
            /* For each set of left and right nodes, we want to find one PM
             * that will act as a witness. The rest can be blocked, assuming
             * the edges don't intersect those used in other witnesses.
             */
            // Look for a witness first
            int found_witness = 0;
            int witness_idx = 0;
            int mi;
            for (mi = 0; mi < num_similar && !found_witness; mi++) {
              const int *p2o = graph_get_matching_ordered_right_nodes(m);
              
              // Check all edges in this ordering to see if any are blocked
              int witness_candidate = 1;
              for (int n = 0; n < size; n++) {
                if (blocked_edges[p1s[n]][p2s[p2o[n]]]) {
                  witness_candidate = 0;
                  break;
                }
              }
              
              // If a witness candidate made it through, mark the edges
              if (witness_candidate) {
                found_witness = 1;
                witness_idx = mi;
                for (int n = 0; n < size; n++) {
                  witness_edges[p1s[n]][p2s[p2o[n]]]++;
                }
                break;
              }
              
              m = graph_get_next_matching(m);
            }
            
            // Now if witness found, block remaining
            if (found_witness) {
              for (; mi > 0; mi--) {
                m = graph_get_prev_matching(m);
              }
              
              for (mi = 0; mi < num_similar; mi++) {
                if (mi == witness_idx) {
                  m = graph_get_next_matching(m);
                  continue;
                }
                
                // Block ordering only if doesn't intersect witness edges
                const int *p2o = graph_get_matching_ordered_right_nodes(m);
                int blocking_candidate = 1;
                for (int n = 0; n < size; n++) {
                  if (witness_edges[p1s[n]][p2s[p2o[n]]]) {
                    blocking_candidate = 0;
                    break;
                  }
                }
                
                // Add the clause to the CNF
                if (blocking_candidate) {
                  for (int n = 0; n < size; n++) {
                    blocked_edges[p1s[n]][p2s[p2o[n]]]++;
                  }
                  
                  for (int n = 0; n < size; n++) {
                    fprintf(f, "-%d ",
                            get_variableID(g, 0, p1s[n], 1, p2s[p2o[n]]));
                  }
                  fprintf(f, "0\n");
                  
                  printf("Blocking ");
                  graph_print_perfect_matching(m);
                  
                }
                
                m = graph_get_next_matching(m);
              }
            }
          } else if (blocking_method == PROB) {
            for (int n = 0; n < num_similar - 1; n++) {
              m = graph_get_next_matching(m);
              const int *p2o = graph_get_matching_ordered_right_nodes(m);
              
              if (rand() % BLOCKED_CLAUSE_PROB_DENOM < blocked_clause_prob) {
                for (int n = 0; n < size; n++) {
                  fprintf(f, "-%d ",
                          get_variableID(g, 0, p1s[n], 1, p2s[p2o[n]]));
                }
                fprintf(f, "0\n");
              }
            }
            
            m = graph_get_next_matching(m);
          } else {
            // Block all but one in this set
            for (int m_idx = 0; m_idx < num_similar - 1; m_idx++) {
              m = graph_get_next_matching(m);
              const int *p2o = graph_get_matching_ordered_right_nodes(m);
              for (int n = 0; n < size; n++) {
                fprintf(f, "-%d ",
                        get_variableID(g, 0, p1s[n], 1, p2s[p2o[n]]));
              }
              fprintf(f, "0\n");
              
              // printf("Blocking ");
              // graph_print_perfect_matching(m);
            }
            
            m = graph_get_next_matching(m);
          }
          
          /*
           // If COUNT exceeded for this node, advance to next size for node
           if (blocking_method == COUNT && num_blocked == blocked_clause_prob) {
           // printf("Reached count on %d, skipping to size %d\n",
           //    i, size + 1);
           while (m != NULL && size == graph_get_matching_size(m)) {
           m = graph_get_next_matching(m);
           }
           
           num_blocked = 0;
           }
           */
        }
      }
    }
  }
}

void write_pgbdd_var_ord(graph_t *g) {
  const int *partition_sizes;
  partition_sizes = graph_get_partition_sizes(g);
  int atM = partition_sizes[0]>partition_sizes[1]?1:0;
  int atL = partition_sizes[0]>=partition_sizes[1]?0:1;
  int *neighbors = NULL, *neigh_size = xmalloc(sizeof(int));
  for(int i = 0; i < partition_sizes[atL]; i++) {
    neighbors = graph_get_neighbors(g, atL, i, atM, neigh_size);
    for(int j = 0; j < *neigh_size; j++) {
      fprintf(pgbdd_var_f, "%d \n", get_variableID(g, atL,i,atM,neighbors[j]));
      if (aux_var_map1[get_variableID(g, atL,i,atM,neighbors[j])] > 0) fprintf(pgbdd_var_f, "%d \n", aux_var_map1[get_variableID(g, atL,i,atM,neighbors[j])]);
      //      if (sinz_var_map2[get_variableID(g, atL,i,atM,neighbors[j])] > 0) fprintf(pgbdd_var_f, "%d \n", sinz_var_map2[get_variableID(g, atL,i,atM,neighbors[j])]);
    }
    free(neighbors);
  }
  
  // Fill in remaining edges
  for(int i = 0;i < partition_sizes[atL]; i++) {
    for(int j = 0;j < partition_sizes[atM]; j++) {
      if (!graph_is_edge_between(g, atL, i, atM, j)) {
        fprintf(pgbdd_var_f, "%d \n", get_variableID(g,atL,i,atM,j));
      }
    }
  }
  fclose(pgbdd_var_f);
}

void write_pgbdd_bucket(graph_t *g) {
  const int *partition_sizes;
  partition_sizes = graph_get_partition_sizes(g);
  int atM = partition_sizes[0]>partition_sizes[1]?1:0;
  int atL = partition_sizes[0]>=partition_sizes[1]?0:1;
  int *neighbors = NULL, *neigh_size = xmalloc(sizeof(int));
  for(int i = 0; i < partition_sizes[atL]; i++) {
    neighbors = graph_get_neighbors(g, atL, i, atM, neigh_size);
    for(int j = 0; j < *neigh_size; j++) {
      fprintf(pgbdd_bucket_f, "%d \n", get_variableID(g, atL,i,atM,neighbors[j]));
    }
    if (i > 0) {
      for(int j = 0; j < *neigh_size; j++) {
        if (aux_var_map1[get_variableID(g, atL,i,atM,neighbors[j])] > 0) fprintf(pgbdd_bucket_f, "%d \n", aux_var_map1[get_variableID(g, atL,i,atM,neighbors[j])]);
      }
    }
    free(neighbors);
  }
  
  // Fill in remaining edges/*
  for(int i = 0;i < partition_sizes[atL]; i++) {
    for(int j = 0;j < partition_sizes[atM]; j++) {
      if (!graph_is_edge_between(g, atL, i, atM, j)) {
        fprintf(pgbdd_var_f, "%d \n", get_variableID(g,atL,i,atM,j));
        fprintf(pgbdd_bucket_f, "%d \n", get_variableID(g,atL,i,atM,j));
      }
    }
  }
  fclose(pgbdd_var_f);
  fclose(pgbdd_bucket_f);
}


/** @brief Handles main execution. Parses CLI. */
int main(int argc, char *argv[]) {
  
  FILE *f = NULL;
  mchess_t *mc = NULL;
  pigeon_t *pigeon = NULL;
  graph_var_t *gt = NULL;
  graph_t *g = NULL;
  char *gvalue = NULL, *fvalue = NULL, *evalue ="direct", *mchessOpt = "NORMAL";
  const int *partition_sizes;
  int nvalue=4; // Default evalue to direct encoding, nvalue to 4
  int *atMost, *atLeast, atM, atL, atLSize = 1, atMSize = 1;
  bool atMFlag = false, atLFlag = false;
  int cardinality = 1;
  float density = 1.0;
  int nedges = 0;
  
  
  
  // Parse command line arguments
  extern char *optarg;
  char opt;
  while ((opt = getopt(argc, argv, "vahLMopb:B:c:C:D:e:f:g:n:s:E:")) != -1) {
    switch (opt) {
      case 'a':
        avoid_blocking_overlap = 1;
        break;
      case 'b':
        blocked_clause_size = atoi(optarg);
        break;
      case 'B':;
        double b_arg = atof(optarg);
        if (b_arg <= 0.0) {
          printf("Invalid -B flag argument, must be positive\n");
          exit(-1);
        } else if (b_arg < 1.0) {
          blocking_method = PROB;
          blocked_clause_prob = (int) (BLOCKED_CLAUSE_PROB_DENOM * b_arg);
        } else {
          blocking_method = COUNT;
          blocked_clause_prob = (int) b_arg;
        }
        break;
      case 'c':
        cardinality = (int) strtol(optarg, (char **)NULL, 10);
        break;
      case 'C':
        mchessOpt = optarg;
        break;
      case 'D':
        density = atof(optarg);
        break;
      case 'e':
        evalue = optarg;
        break;
      case 'f':
        fvalue = optarg;
        break;
      case 'g':
        gvalue = optarg;
        break;
      case 'h':
        print_help(argv[0]);
        exit(0);
      case 'L':
        atLFlag = true;
        break;
      case 'M':
        atMFlag = true;
        break;
      case 'n':
        nvalue = (int) strtol(optarg, (char **)NULL, 10);
        break;
      case 's':
        rand_seed = (int) strtol(optarg, (char **)NULL, 10);
        break;
      case 'E':
        nedges = (int) strtol(optarg, (char **)NULL, 10);
        break;
      case 'p':
        pgbdd_bucket = true;
        break;
      case 'o':
        pgbdd_var_ord = true;
        break;
      case 'v':
        verbosity_level = 1;;
        break;
      default:
        fprintf(stderr, "Unrecognized option, exiting\n");
        exit(-1);
    }
  }
  
  if (fvalue == NULL || gvalue == NULL) {
    printf("Program requires filename -f and graph generator -g options\n");
    exit(-1);
  }
  if (pgbdd_bucket && pgbdd_var_ord) {
    printf("Cannot run bucket permutation and variable ordering simultaneously\n");
    exit(-1);
  }
  if (nedges > 0 && density < 1.0) {
    printf("Must choose between edge count or density to bound size of random graph\n");
    exit(-1);
  }
  
  // Generate graph
  if (strcmp(gvalue,"chess")==0) {
    // mutilated chess
    if (strcmp(mchessOpt,"TORUS")==0)
      mc = mchess_create(nvalue, TORUS);
    else if (strcmp(mchessOpt,"CYLINDER")==0)
      mc = mchess_create(nvalue, CYLINDER);
    else if (strcmp(mchessOpt,"NORMAL")==0)
      mc = mchess_create(nvalue, NORMAL);
    g = mchess_generate_graph(mc);
    //g = get_chess_graph(nvalue);
  } else if (strcmp(gvalue,"pigeon")==0) {
    // pigeon hole
    pigeon = pigeon_create(nvalue);
    g = pigeon_generate_graph(pigeon);
  } else if (strcmp(gvalue,"random")==0) {
    // random graph with user defined parameters
    gt = graph_var_create(nvalue,cardinality,density,nedges);
    g = generate_random_graph(gt,rand_seed);
    randomGr = true;
  } else {
    fprintf(stderr, "Unrecognized problem variant, try again\n");
    return 0;
  }
  
  partition_sizes = graph_get_partition_sizes(g);
  
  // default partitions for at most at least constraints
  atM = partition_sizes[0]>partition_sizes[1]?1:0;
  atL = partition_sizes[0]>=partition_sizes[1]?0:1;
  
  if (atMFlag) { // atmost constraint for other partition
    atMost = xmalloc(sizeof(int)*2);
    atMost[1] =atL;
    atMSize = 2;
  }
  else atMost = xmalloc(sizeof(int));
  if (atLFlag) { // atleast constraint for other partition
    atLeast = xmalloc(sizeof(int)*2);
    atLeast[1] = atM;
    atLSize = 2;
  }
  else atLeast = xmalloc(sizeof(int));
  
  atMost[0] = atM;
  atLeast[0] = atL;
  
  char cnf_name[100];
  strcpy(cnf_name,fvalue);
  strcat(cnf_name,".cnf");
  f = fopen(cnf_name, "w+");
  // initialize PGBDD variable and bucket ordering files and data structures
  if (pgbdd_bucket) {
    char buck_name[100];
    strcpy(buck_name,fvalue);
    strcat(buck_name,"_bucket.order");
    pgbdd_bucket_f = fopen(buck_name, "w+");
  }
  if (pgbdd_var_ord || pgbdd_bucket) {
    char ord_name[100];
    strcpy(ord_name,fvalue);
    strcat(ord_name,"_variable.order");
    pgbdd_var_f  = fopen(ord_name, "w+");
    aux_var_map1 = xmalloc(sizeof(int) * (partition_sizes[0]*partition_sizes[1])+1);
    aux_var_map2 = xmalloc(sizeof(int) * (partition_sizes[0]*partition_sizes[1])+1);
    for (int i = 0; i < (partition_sizes[0]*partition_sizes[1]+1); i++){
      aux_var_map1[i] = 0;
      aux_var_map2[i] = 0;
    }
  }
  
  // Write CNF formula of graph g to file f with encoding opt evalue
  write_cnf_from_graph(g, f, evalue, atMost, atLeast, atMSize, atLSize);
  
  fclose(f);
  if (pgbdd_bucket) write_pgbdd_bucket(g);
  if (pgbdd_var_ord) write_pgbdd_var_ord(g);
  
  int nEdges = 0;
  // Print Graph Density
  if (verbosity_level > 0) {
    for (int i = 0; i < partition_sizes[0]; i++) nEdges += graph_get_num_neighbors(g,0,i,1);
    printf("%f\n",nEdges/(1.0*partition_sizes[0]* partition_sizes[1]));
  }
  
  return 0;
}
