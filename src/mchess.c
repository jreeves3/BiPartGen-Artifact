/** @file mchess.c
 *  @brief Instance generator for mutilated chessboard and its variants.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mchess.h"
#include "xmalloc.h"
#include "graph.h"

#ifndef BITS_IN_BYTE
#define BITS_IN_BYTE 8
#endif

#ifndef BYTE_MASK
#define BYTE_MASK    (BITS_IN_BYTE - 1)
#endif

/** @brief Rounds x to the next multiple of y.
 *
 *  @param x The number to be rounded.
 *  @param y The mutiple.
 *  @return  Ceiling(x / y) * y
 */
#ifndef ROUND_UP
#define ROUND_UP(x, y)    ((((x) + (y) - 1) / (y)) * (y))
#endif


/** @brief Gets the char bitvector "slice" corresponding to a board position.
 *
 *  The board squares are stored in an (n x (n / 8)) char bitvector. To index
 *  into the bitvector, the row is indexed, followed by the column divided
 *  by 8. Bitshifting is required to access within a bitvector "slice."
 *
 *  @param mc  A pointer to a mutilated chessboard.
 *  @param p   A position on the chessboard.
 *  @return    The char containing the bit for that row and column.
 */
#define BOARD(mc, p)      ((mc->squares)[(p)->row][(p)->col / BITS_IN_BYTE])


/** @brief Gets the "present" bit corresponding to a board position.
 *
 *  The macro gets the bitvector "slice" for the specified board position,
 *  and then shifts the requisite number of times to isolate the bit.
 *
 *  @param mc  A pointer to a mutilated chessboard.
 *  @param p   A position on the chessboard.
 *  @return    1 if the square at p is present, 0 otherwise.
 */
#define GET_BIT(mc, p)    ((BOARD(mc, p) >> ((p)->col & BYTE_MASK)) & 0x1)


/** @brief Sets the "present" bit at the specified board position to 1.
 *
 *  The macro gets the bitvector "slice" for the specified board position,
 *  and then bitwise-ORs in a 1 at the appropriate bit.
 *
 *  @param mc  A pointer to a mutilated chessboard.
 *  @param p   A position on the chessboard.
 */
#define SET_BIT(mc, p)    (BOARD(mc, p) |= (0x1 << ((p)->col & BYTE_MASK)))


/** @brief Sets the "present" bit at the specified board position to 0.
 *
 *  The macro gets the bitvector "slice" for the specified board position, and
 *  then bitwise-ANDs a bitmask with all 1s, except for the bit to set to 0.
 *
 *  @param mc  A pointer to a mutilated chessboard.
 *  @param p   A position on the chessboard.
 */
#define CLEAR_BIT(mc, p)  (BOARD(mc, p) &= ~(0x1 << ((p)->col & BYTE_MASK)))


/** @brief Returns if the position is white or black.
 *  
 *  By convention, white squares start at (0, 0), and have even parity.
 *
 *  @param p  A position on the chessboard.
 *  @return   1 if the position is a white square, 0 otherwise.
 */
#define IS_WHITE(p)   (((p)->row + (p)->col) % 2 == 0)


/** @brief Used to exit the program if an enum is not a recognized format. */
#ifndef UNRECOGNIZED_ENUM
#define UNRECOGNIZED_ENUM  fprintf(stderr, "Unrecognized format\n"); exit(-1)
#endif


/** @brief If either row or col of a pos_t is BAD_POS, then it does not
 *         exist on the board.
 */
#define BAD_POS     (-1)


/** @brief Defines orthogonal directions, for neighbor checking. */
typedef enum neighbor {
  LEFT, RIGHT, UP, DOWN
} neigh_t;


/** @brief Defines a mutilated chessboard.
 *
 *  The typical mutilated chessboard is an NxN board with two opposite
 *  corners removed. In a graph representation, squares on the outside
 *  border connect with only two or three neighbors, while internal squares
 *  connect to all four of their orthogonal neighbors.
 *
 *  The classic mutilated chessboard can be varied in several ways:
 *  by changing the geometry, by adding more missing squares (but always
 *  ensuring that WLOG there are more white squares than black), and by
 *  moving the missing squares to different locations on the board (e.g.
 *  not always in the corners). More variations in number and placement of
 *  missing squares present themselves when the board geometry is changed.
 *  See enum mutilated_chessboard_variant for the types of geometry.
 *
 *  The fields of the struct are as follows:
 *
 *  n:       The size of the board. The number of squares per side.
 *  white:   The number of white squares on the board. Roughly half n^2.
 *             By convention, the first white square is at (0, 0), and so
 *             white will have fewer squares than black.
 *  black:   The number of black squares on the board. Roughly half n^2.
 *  variant: The geometry of the board.
 *  squares: A 2D bitvector representing whether the squares are present.
 *           A typical mutilated chessboard only has two squares missing,
 *           so in most cases, a majority of the bits are set to 1.
 */
struct mutilated_chessboard {
  unsigned int n;
  unsigned int white;
  unsigned int black;
  mchess_variant_t variant;
  char **squares;
}; // mchess_t;


/** Helper functions */

/** @brief Performs common invariant checks on a mutilated chessboard.
 *
 *  Ensures that the row and column are in bounds for the chessboard.
 *
 *  @param mc   A pointer to the mutilated chessboard.
 *  @param pos  A position on the chessboard.
 */
static void check_mchess_args(mchess_t *mc, mchess_pos_t *pos) {
  assert(mc != NULL && mc->n > 0);
  assert(0 <= pos->row && pos->row < mc->n);
  assert(0 <= pos->col && pos->col < mc->n);
}


/** @brief Checks whether the neighbor of a position exists, and if it does,
 *         writes the position to the passed pos_t struct.
 *
 *  @param mc     A pointer to a mutilated chessboard.
 *  @param pos    A position on the chessboard.
 *  @param neigh  A neighboring direction to check.
 *  @param n_pos[out]  The position struct to write the result to. In the case
 *                     that the neighbor does not exist on the board (e.g.
 *                     LEFT for (0, 0)), then BAD_POS is written to both
 *                     row and col fields.
 */
static void get_neighbor(mchess_t *mc, 
    mchess_pos_t *pos, neigh_t neigh, mchess_pos_t *n_pos) {
  // Neighbor position will only adjust by 1 from here, in most cases
  n_pos->row = pos->row;
  n_pos->col = pos->col;

  // Leave special cases to the end
  if (neigh == LEFT && pos->col == 0) {
    switch (mc->variant) {
      case NORMAL:
        n_pos->row = BAD_POS;
        n_pos->col = BAD_POS;
        break;
      case CYLINDER:
      case TORUS:
        n_pos->col = mc->n - 1;
        break;
      default:
        UNRECOGNIZED_ENUM;
    }
  } else if (neigh == RIGHT && pos->col == mc->n - 1) {
    switch (mc->variant) {
      case NORMAL:
        n_pos->row = BAD_POS;
        n_pos->col = BAD_POS;
        break;
      case CYLINDER:
      case TORUS:
        n_pos->col = 0;
        break;
      default:
        UNRECOGNIZED_ENUM;
    }
  } else if (neigh == UP && pos->row == 0) {
    switch (mc->variant) {
      case NORMAL:
      case CYLINDER:
        n_pos->row = BAD_POS;
        n_pos->col = BAD_POS;
        break;
      case TORUS:
        n_pos->row = mc->n - 1;
        break;
      default:
        UNRECOGNIZED_ENUM;
    }
  } else if (neigh == DOWN && pos->row == mc->n - 1) {
    switch (mc->variant) {
      case NORMAL:
      case CYLINDER:
        n_pos->row = BAD_POS;
        n_pos->col = BAD_POS;
        break;
      case TORUS:
        n_pos->row = 0;
        break;
      default:
        UNRECOGNIZED_ENUM;
    }
  } else {
    // Slightly change the position's coordinates depending on neighbor
    switch (neigh) {
      case LEFT:
        n_pos->col--;
        break;
      case RIGHT:
        n_pos->col++;
        break;
      case UP:
        n_pos->row--;
        break;
      case DOWN:
        n_pos->row++;
        break;
      default:
        UNRECOGNIZED_ENUM;
    }
  }
}


/** @brief Gets the index for the tile.
 *
 *  Is a translator to graph_t struct, so different positions, one black and
 *  one white, can return the same value.
 *
 *  TODO currently inefficient, can make this O(n) instead of O(n^2)
 *  Or even O(1)
 *
 *  @param mc   A pointer to a chessboard.
 *  @param pos  A pointer to a position.
 *  @return     A unique id for that color tile, starting from 0 in the upper
 *              left corner and proceeding in row-major order.
 *              Returns -1 in the case that the position is not present.
 */
int mchess_get_tile_id(mchess_t *mc, mchess_pos_t *pos) {
  if (GET_BIT(mc, pos) == 0) {
    return -1;
  }

  const int n = mc->n;
  const int prow = pos->row;
  const int pcol = pos->col;
  const int iswhite = IS_WHITE(pos);

  // Scan from top-left to bottom right, tallying up similar present colors
  int id = 0;
  for (int row = 0; row < n; row++) {
    for (int col = 0; col < n; col++) {
      mchess_pos_t scan_pos;
      scan_pos.row = row;
      scan_pos.col = col;

      // If same color, either return id or add one if present
      if (IS_WHITE(&scan_pos) == iswhite) {
        if (prow == row && pcol == col) {
          return id;
        } else if (GET_BIT(mc, &scan_pos)) {
          id++;
        }
      }
    }
  }

  return id;
}


/** @brief Checks if the specified neighbor is present on the board.
 *
 *  If the position is on an edge and does not have a neighboring square,
 *  e.g. (0, 0) on a NORMAL variant with LEFT, then 0 is always returned.
 *
 *  If the position does have a valid neighbor, then the bitvector is checked
 *  for if that position's bit is set to 1.
 *
 *  @param mc   A pointer to a mutilated chessboard.
 *  @param pos  A position on the chessboard.
 *  @param n    The neighboring direction to check.
 *  @return     1 if the neighbor is present, 0 otherwise.
 */
static int is_neighbor_present(mchess_t *mc, mchess_pos_t *pos, neigh_t n) {
  mchess_pos_t n_pos;
  get_neighbor(mc, pos, n, &n_pos);
  if (n_pos.row == BAD_POS || n_pos.col == BAD_POS) {
    return 0;
  } else {
    return GET_BIT(mc, &n_pos);
  }
}


/** Chessboard API */


/** @brief Adds a square to the mutilated chessboard.
 *
 *  As the squares are stored as booleans in a 2D bitvector, the bitvector
 *  is indexed into depending on row and col, and the bit is set to 1.
 *
 *  @param mc   A pointer to the mutilated chessboard.
 *  @param pos  A position on the chessboard. 
 */
void mchess_add_square(mchess_t *mc, mchess_pos_t *pos) {
  check_mchess_args(mc, pos);

  if (GET_BIT(mc, pos) == 1) {
    return;
  }

  // Add the color of square depending on the parity of row and col
  if (IS_WHITE(pos)) {
    mc->white++;
  } else {
    mc->black++;
  }

  SET_BIT(mc, pos);
}


/** @brief Removes a square from the mutilated chessboard.
 *
 *  As the squares are stored as booleans in a 2D bitvector, the bitvector
 *  is indexed into depending on row and col, and the bit is set to 0.
 *
 *  @param mc   A pointer to the mutilated chessboard.
 *  @param pos  A position on the chessboard.
 */
void mchess_remove_square(mchess_t *mc, mchess_pos_t *pos) {
  check_mchess_args(mc, pos);

  if (GET_BIT(mc, pos) == 0) {
    return;
  }

  // Remove the color of square depending on parity of row and col
  if (IS_WHITE(pos)) {
    mc->white--;
  } else {
    mc->black--;
  }

  CLEAR_BIT(mc, pos);
}


/** @brief Creates a new mutilated chessboard with the specified size and
 *         geometry variant.
 *
 *  Regardless of variant, an (n x n) chessboard is created. Typically, the
 *  removed squares are as far away from each other as possible, so depending
 *  on the variant, the removed squares will be in different positions.
 *
 *  WLOG, the two removed squares just need to be as far away from each other
 *  as possible, so the top-left corner (0, 0) is always removed.
 *
 *  For a NORMAL variant, the second square removed is the bottom-right corner, 
 *  with position (n - 1, n - 1).
 *
 *  For a CYLINDER variant, the left and right sides are joined, meaning the 
 *  left-to-right diameter is now n / 2. So, the second square removed is in 
 *  the bottom row and the middle column, at (n - 1, n / 2).
 *
 *  For a TORUS variant, the left and right, and top and bottom sides are
 *  joined, meaning the left-to-right and top-to-bottom diameters are both
 *  n / 2. So, the second square removed is in the middle row and column,
 *  at position (n / 2, n / 2).
 *
 *  Note that, on memory allocation failure, exit(-1) is called, so if
 *  mchessboard_create returns, it will return a pointer to a valid chessboard.
 *
 *  @param n        The size of the chessboard, the number of squares to a side.
 *  @param variant  The geometry variant of the chessboard.
 *  @return         A pointer to a chessboard with two squares removed.
 */
mchess_t *mchess_create(int n, mchess_variant_t variant) {
  mchess_t *mc = xmalloc(sizeof(mchess_t));
  mc->n = n;
  mc->white = (n * n + 1) / 2;
  mc->black = (n * n) / 2;
  mc->variant = variant;
  mc->squares = xmalloc(n * sizeof(char *));

  // Bitvector size is basically n / BITS_IN_BYTE, modulo some rounding
  const int bv_size = ROUND_UP(n, BITS_IN_BYTE) / BITS_IN_BYTE;
  for (int i = 0; i < n; i++) {
    mc->squares[i] = xmalloc(bv_size * sizeof(char));
    memset(mc->squares[i], 0xff, bv_size * sizeof(char)); // Set squares to 1
  }

  // Drop first square, always top-left corner
  mchess_pos_t first_square;
  first_square.row = 0;
  first_square.col = 0;
  mchess_remove_square(mc, &first_square);

  // Drop second square, location depends on variant
  mchess_pos_t second_square;
  switch (variant) {
    case NORMAL:
      // Remove bottom right corner
      second_square.row = n - 1;
      second_square.col = n - 1;
      break;
    case CYLINDER:
      // Remove bottom-middle edge square
      second_square.row = n - 1;
      second_square.col = n / 2;
      break;
    case TORUS:
      // Remove middle square
      second_square.row = n / 2;
      second_square.col = n / 2;
      break;
    default:
      UNRECOGNIZED_ENUM;
  }

  mchess_remove_square(mc, &second_square);
  return mc;
}


/** @brief TODO
 *
 *  Something about diameter on a scale
 *  Calculation?
 *
 *  @param n        The size of the chessboard, the number of squares to a side.
 *  @param variant  The geometry variant of the chessboard.
 *  @param diameter A scale from 0.0 to 1.0.
 *  @return         A pointer to a chessboard with two squares removed.
 */
mchess_t *mchess_create_with_diameter(
    int n, mchess_variant_t variant, double diameter) {
  return NULL;
}


/** @brief Frees the memory allocated by a mutilated chessboard.
 *
 *  @param mc  A pointer to the mutilated chessboard struct to free.
 */
void mchess_free(mchess_t *mc) {
  const unsigned int n = mc->n;
  for (int i = 0; i < n; i++) {
    xfree(mc->squares[i]);
  }

  xfree(mc->squares);
  xfree(mc);
}


/** @brief Returns the size of the mutilated chessboard.
 *
 *  @param mc  A pointer to a mutilated chessboard.
 *  @return    The size of the chessboard, n.
 */
int mchess_get_n(mchess_t *mc) {
  return mc->n;
}


/** @brief Gets the number of present squares neighboring the given square.
 *
 *  @param mc   A pointer to a mutilated chessboard.
 *  @param pos  A position on the chessboard.
 *  @return     The number of neighboring squares present on the board.
 */
int mchess_get_num_neighbors(mchess_t *mc, mchess_pos_t *pos) {
  check_mchess_args(mc, pos);

  int count = 0;
  count += is_neighbor_present(mc, pos, LEFT);
  count += is_neighbor_present(mc, pos, RIGHT);
  count += is_neighbor_present(mc, pos, UP);
  count += is_neighbor_present(mc, pos, DOWN);
  return count;
}

/** @brief Returns an array of present neighbors around a position.
 *
 *  An array of board positions is returned. The length of the array is
 *  written to the int* passed in. On memory allocation error, the function
 *  does not return, so a return value of NULL indicates that the position
 *  has zero present neighbors.
 *
 *  @param mc   A pointer to a mutilated chessboard.
 *  @param pos  A position on the chessboard.
 *  @param len[out]  The length of the returned array.
 *  @return     An array of board positions corresponding to present neighbors.
 */
mchess_pos_t *mchess_get_neighbors(mchess_t *mc, mchess_pos_t *pos, int *len) {
  const int num_neighbors = mchess_get_num_neighbors(mc, pos);
  *len = num_neighbors;

  if (num_neighbors == 0) {
    return NULL;
  }

  mchess_pos_t *neighbors = xmalloc(num_neighbors * sizeof(mchess_pos_t));

  // Manually check each neighbor
  int idx = 0;
  if (is_neighbor_present(mc, pos, LEFT)) {
    get_neighbor(mc, pos, LEFT, &neighbors[idx++]);
  }

  if (is_neighbor_present(mc, pos, RIGHT)) {
    get_neighbor(mc, pos, RIGHT, &neighbors[idx++]);
  }

  if (is_neighbor_present(mc, pos, UP)) {
    get_neighbor(mc, pos, UP, &neighbors[idx++]);
  }

  if (is_neighbor_present(mc, pos, DOWN)) {
    get_neighbor(mc, pos, DOWN, &neighbors[idx++]);
  }

  return neighbors;
}


/** @brief Generates a graph struct from a mutilated chessboard.
 *
 *  Board variants map to edges in the natural way.
 *
 *  Note that if a non-NORMAL variant is selected, and e.g. two white squares
 *  are touching, then the edge is not added to the graph. As a result, an
 *  even number for n is advised.
 *
 *  On memory allocation failure, exit(-1) is called.
 *
 *  @param mc  A pointer to a mutilated chessboard.
 *  @return    A pointer to a graph structure corresponding to the squares
 *             present in the mc chessboard and the geometry variant.
 */
graph_t *mchess_generate_graph(mchess_t *mc) {
  // By convention, white is the 0th partition, black the 1st
  int colors[2] = { mc->white, mc->black };
  graph_t *g = graph_create_with_sizes(2, colors);

  // Add in edges for neighbors
  const int n = mc->n;
  int len;
  mchess_pos_t pos;
  for (int row = 0; row < n; row++) {
    for (int col = 0; col < n; col++) {
      pos.row = row;
      pos.col = col;

      if (GET_BIT(mc, &pos) == 0) {
        continue;
      }

      // Half are white, half are black
      int id = mchess_get_tile_id(mc, &pos);

      // For each neighbor, add an edge
      mchess_pos_t *neighbors = mchess_get_neighbors(mc, &pos, &len);
      for (int i = 0; i < len; i++) {
        int neigh_id = mchess_get_tile_id(mc, &neighbors[i]);

        // Skip squares of the same color
        if (IS_WHITE(&pos) == IS_WHITE(&neighbors[i])) {
          continue;
        }

        // White is 0th partition
        if (IS_WHITE(&pos)) {
          graph_add_edge(g, 0, id, 1, neigh_id);
        } else {
          graph_add_edge(g, 0, neigh_id, 1, id);
        }
      }

      xfree(neighbors);
    }
  }

  return g;
}
