/**
 * @authors
 * Odin Bjerke <odin.bjerke@uit.no>
 */

#ifndef COMMON_H
#define COMMON_H

#include "defs.h"
#include <stdint.h>

/**
 * @brief Compare two integers.
 * @param a,b: pointer to integer
 * @returns see `cmp_fn`
 */
int compare_integers(const int *a, const int *b);

/**
 * @brief Compare two characters. If strings are passed, only the first character is compared
 * @param a,b: pointer to character
 * @returns see `cmp_fn`
 */
int compare_characters(const char *a, const char *b);

/**
 * @brief Compare two pointers (by memory address)
 * @param a,b: pointer
 * @returns see `cmp_fn`
 */
int compare_pointers(const void *a, const void *b);

/**
 * @brief Fowler-Noll-Vo (FNV-1a) hash algorithm for strings, 64-bit variant
 * @param str: null-terminated string
 * @returns The 64 bit hash of `str`
 * @note see [this](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash)
 * page for further information on the algorithm.
 */
uint64_t hash_string_fnv1a64(const void *str);


#endif /* COMMON_H */
