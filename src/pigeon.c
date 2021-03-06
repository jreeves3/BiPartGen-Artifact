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


/** @file pigeon.c
 *  @brief Instance generator for pigeonhole problems and its variants.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#include "pigeon.h"
#include "xmalloc.h"

/** @brief Defines a pigeonhole problem.
 *
 *  A pigeonhole problem is defined by an integer n, where n is the number
 *  of holes and (n + 1) is the number of pigeons.
 *
 *  TODO variants? Bookkeeping information?
 */
struct pigeonhole_problem {
  int n;
}; // pigeon_t


/** @brief Creates a pigeonhole problem.
 *
 *  @param n  The number of holes. The number of pigeons is (n + 1).
 *  @return   A pointer to a pigeonhole problem struct.
 */
pigeon_t *pigeon_create(int n) {
  pigeon_t *p = xmalloc(sizeof(pigeon_t));
  p->n = n;
  return p;
}


/** @brief Frees memory allocated by a pigeonhole problem.
 *
 *  @param p  A pointer to a pigeonhole problem.
 */
void pigeon_free(pigeon_t *p) {
  xfree(p);
}


/** @brief Gets the number of holes in the pigeonhole problem.
 *
 *  @param   A pointer to a pigeonhole problem.
 *  @return  The number of holes in the pigeonhole problem.
 */
int pigeon_get_n(pigeon_t *p) {
  return p->n;
}


/** @brief Generates a graph structure from a pigeonhole problem.
 *
 *  @param   A pointer to a pigeonhole problem.
 *  @return  A pointer to a graph structure corresponding to the pigeonhole
 *           problem.
 */
graph_t *pigeon_generate_graph(pigeon_t *p) {
  int sizes[2] = { p->n + 1, p->n };
  graph_t *g = graph_create_with_sizes(2, sizes);

  // Connect every pigeon to every hole
  graph_fully_connect_partition(g, 0, 1);

  return g;
}
