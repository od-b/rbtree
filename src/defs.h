/**
 * @authors
 * Odin Bjerke <odin.bjerke@uit.no>
 */

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

/**
 * Signal to the compiler that a function may be intentionally unused
 */
#define ATTR_MAYBE_UNUSED __attribute__((unused))

/**
 * Signal to the compiler that a switch case may intetionally fall through
 */
#define ATTR_FALLTHROUGH __attribute__((fallthrough))

#define UNUSED(x) (void) (x)

/**
 * @brief Type of comparison function
 *
 * @returns
 * - 0 on equality
 *
 * - >0 if a > b
 *
 * - <0 if a < b
 *
 * @note When passing references to comparison functions as parameters,
 * typecast to cmp_fn if said functions' parameters are non-void pointers.
 */
typedef int (*cmp_fn)(const void *, const void *);

/**
 * @brief Type of free (resource deallocation) function
 * @note may be the actual `free` function, or another user-defined function
 */
typedef void (*free_fn)(void *);

/**
 * @brief Type of 64-bit hash function
 */
typedef uint64_t (*hash64_fn)(const void *);


#endif /* DEFS_H */
