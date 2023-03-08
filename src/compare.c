#include "compare.h"
#include <string.h>

int8_t compare_strings(void *a, void *b) {
    return strcmp(a, b);
}

int8_t compare_words(void *a, void *b) {
    return strcasecmp(a, b);
}

int8_t compare_ints(void *a, void *b) {
    return (*(int*)a) - (*(int*)b);
}
