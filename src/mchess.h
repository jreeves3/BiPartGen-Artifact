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


/** @file mchess.h
 *  @brief Instance generator for mutilated chessboard and its variants.
 *
 *  See mchess.c for documentation and implementation details.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#ifndef _MCHESS_H_ 
#define _MCHESS_H_ 

#include "graph.h"


/** @brief Defines the type of board geometry of the mutilated chessboard.
 *
 *  Thel enum defines the different types of board geometries supported
 *  by mchess.h/.c. See struct mutilated_chessboard in mchess.c for discussion 
 *  of how the board geometries interact with the struct.
 *
 *  The typical mutilated chessboard is an orthogonal NxN board in the
 *  plane. Variants on the geometry allow the edges to "wrap around,"
 *  essentially adding more connections, i.e. more edges to the graph.
 *
 *  The variants defined in this enum are:
 *
 *  NORMAL:   A typical NxN chessboard.
 *
 *  CYLINDER: The left and right edges are connected, in that the first
 *            and last columns are connected as if orthogonal neighbors.
 *
 *  TORUS:    The left and right edges, as well as the top and bottom edges,
 *            are connected.
 */
typedef enum mutlilated_chessboard_variant {
  NORMAL, CYLINDER, TORUS //, MOBIUS?
} mchess_variant_t;


/** @brief Defines a mutilated chessboard.
 *
 *  See generator.c for struct fields and motivation.
 */
typedef struct mutilated_chessboard mchess_t;


/** @brief Defines a position on a mutilated chessboard.
 *
 *  If the top-left corner of the chessboard is seen as having row 0
 *  and column 0, then increasing the row can be seen as moving down,
 *  while increasing the column can be seen as moving right.
 */
typedef struct mutilated_chessboard_position {
  unsigned int row;
  unsigned int col;
} mchess_pos_t;


/** Chessboard API */

/** Creation and free functions */
mchess_t *mchess_create(int n, mchess_variant_t variant);
mchess_t *mchess_create_with_diameter(
    int n, mchess_variant_t variant, double diameter);
void mchess_free(mchess_t *mc);

/** Getters */
int mchess_get_n(mchess_t *mc);
int mchess_get_tile_id(mchess_t *mc, mchess_pos_t *pos);
int mchess_get_num_neighbors(mchess_t *mc, mchess_pos_t *pos);
mchess_pos_t *mchess_get_neighbors(mchess_t *mc, mchess_pos_t *pos, int *len);

/** Modification functions */
void mchess_add_square(mchess_t *mc, mchess_pos_t *pos);
void mchess_remove_square(mchess_t *mc, mchess_pos_t *pos);

/** Generators */
graph_t *mchess_generate_graph(mchess_t *mc);


#endif /* _MCHESS_H_ */
