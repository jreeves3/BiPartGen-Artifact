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


/** @file pigeon.h
 *  @brief Instance generator for pigeonhole problems and its variants.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#ifndef _PIGEON_H_
#define _PIGEON_H_

#include "graph.h"

/** @brief Defines a pigeonhole problem.
 *
 *  See pigeon.c for struct fields and motivation.
 */
typedef struct pigeonhole_problem pigeon_t;


/** Pigeonhole API */

/** Creation and free functions */
pigeon_t *pigeon_create(int n);
void pigeon_free(pigeon_t *p);

/** Getters */
int pigeon_get_n(pigeon_t *p);

/** Generators */
graph_t *pigeon_generate_graph(pigeon_t *p);

#endif /* _PIGEON_H_ */
