#ifndef COMPARE_H
#define COMPARE_H

#include <stdint.h>

/*
 * The type of comparison functions.
 * comparison functions return:
 * * -1  if a > b
 * *  1  if a < b
 * *  0  if a == b
 */
typedef int8_t (*cmpfunc_t)(void *, void *);

/* 
 * string comparision, case sensitive
*/
int8_t compare_strings(void *a, void *b);

/* 
 * string comparison, NOT case sensitive
*/
int8_t compare_words(void *a, void *b);

/*
 * int comparison
*/
int8_t compare_ints(void *a, void *b);


#endif
