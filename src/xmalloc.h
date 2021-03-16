/** @file xmalloc.h
 *  @brief Exit() wrappers around malloc() family functions.
 *
 *  See xmalloc.c for implementation details.
 *
 *  @author Joseph Reeves (jereeves@andrew.cmu.edu)
 *  @author Cayden Codel  (ccodel@andrew.cmu.edu)
 *
 *  @bug No known bugs.
 */

#ifndef _XMALLOC_H_
#define _XMALLOC_H_

#include <unistd.h>

void *xcalloc(size_t count, size_t size);
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void *xreallocf(void *ptr, size_t size);

void xfree(void *ptr); // Technically just calls free()

#endif /* _XMALLOC_H_ */
