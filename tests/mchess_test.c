/** @file mchess_test.c
 *  @brief Tests the mchess.c file.
 *
 *  TODO Full neighbor checking
 *
 *  @author Cayden Codel (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "mchess.h" // TODO think about inc/ and src/ directories
#include "graph.h"

#define N 8

int main() {
  mchess_t *mc = mchess_create(N, NORMAL);

  assert(mchess_get_n(mc) == N);

  // Verify number of neighbors in the NORMAL variant
  mchess_pos_t pos;
  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      pos.row = row;
      pos.col = col;

      // Special cases of corners
      if ((row == 0 && col == 0) ||
          (row == N - 1 && col == 0) ||
          (row == 0 && col == N - 1) ||
          (row == N - 1 && col == N - 1)) {
        assert(mchess_get_num_neighbors(mc, &pos) == 2);
      } 
      // Special cases of squares adjacent to missing ones
      else if ((row == 0 && col == 1) ||
               (row == 1 && col == 0) ||
               (row == N - 1 && col == N - 2) ||
               (row == N - 2 && col == N - 1)) {
        assert(mchess_get_num_neighbors(mc, &pos) == 2);
      }
      // Any other edge
      else if (row == 0 || col == 0 || row == N - 1 || col == N - 1) {
        assert(mchess_get_num_neighbors(mc, &pos) == 3);
      } else {
        assert(mchess_get_num_neighbors(mc, &pos) == 4);
      }
    }
  }

  // Generate graph from chessboard
  graph_t *g = mchess_generate_graph(mc);

  // Verify the graph
  assert(graph_get_num_partitions(g) == 2);
 
  // TODO better checks here, but just check 4 neighbors per internal node
  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      mchess_pos_t pos;
      pos.row = row;
      pos.col = col;

      int id = mchess_get_tile_id(mc, &pos);
      if (id == -1) {
        assert((row == 0 && col == 0) || (row == N - 1 && col == N - 1));
        continue;
      }

      // Any remaining corners
      if ((row == 0 && col == 0) ||
          (row == N - 1 && col == 0) ||
          (row == 0 && col == N - 1) ||
          (row == N - 1 && col == N - 1)) {
        if ((row + col) % 2 == 0) {
          assert(graph_get_num_neighbors(g, 0, id, 1) == 2);
        } else {
          assert(graph_get_num_neighbors(g, 1, id, 0) == 2);
        }
      } 
      // Special cases of squares adjacent to missing corners
      else if ((row == 0 && col == 1) ||
          (row == 1 && col == 0) ||
          (row == N - 1 && col == N - 2) ||
          (row == N - 2 && col == N - 1)) {
        if ((row + col) % 2 == 0) {
          assert(graph_get_num_neighbors(g, 0, id, 1) == 2);
        } else {
          assert(graph_get_num_neighbors(g, 1, id, 0) == 2);
        }
      }
      // Any other edge
      else if (row == 0 || col == 0 || row == N - 1 || col == N - 1) {
        if ((row + col) % 2 == 0) {
          assert(graph_get_num_neighbors(g, 0, id, 1) == 3);
        } else {
          assert(graph_get_num_neighbors(g, 1, id, 0) == 3);
        }
      } else {
        if ((row + col) % 2 == 0) {
          assert(graph_get_num_neighbors(g, 0, id, 1) == 4);
        } else {
          assert(graph_get_num_neighbors(g, 1, id, 0) == 4);
        }
      }
    }
  }

  return 0;
}
