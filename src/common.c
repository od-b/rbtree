/**
 * @authors
 * Odin Bjerke <odin.bjerke@uit.no>
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "common.h"

int compare_integers(const int *a, const int *b) {
    return (*a) - (*b);
}

int compare_characters(const char *a, const char *b) {
    return (int) *a - (int) *b;
}

int compare_pointers(const void *a, const void *b) {
    uintptr_t pa = (uintptr_t) a;
    uintptr_t pb = (uintptr_t) b;

    if (pa > pb) {
        return 1;
    }
    if (pa < pb) {
        return -1;
    }
    return 0;
}

char *basename(const char *fpathlike) {
    char *s = strrchr(fpathlike, '/');

    if (s && ++s) {
        return s;
    }
    return (char *) fpathlike;
}

uint64_t hash_string_fnv1a64(const void *str) {
    /* note that these values are NOT chosen randomly. Modifying them will break the function. */
    static const uint64_t FNV_offset_basis = 0xcbf29ce484222325;
    static const uint64_t FNV_prime = 0x100000001b3;

    uint64_t hash = FNV_offset_basis;
    const uint8_t *p = (const uint8_t *) str;

    while (*p) {
        /* FNV-1a hash differs from the FNV-1 hash only by the
         * order in which the multiply and XOR is performed */
        hash ^= (uint64_t) *p;
        hash *= FNV_prime;
        p++;
    }

    return hash;
}
