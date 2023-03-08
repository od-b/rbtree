// Author: Morten Grønnesby <morten.gronnesby@uit.no>
#ifndef PRINTING_H
#define PRINTING_H

#include <stdlib.h>
#include <stdio.h>


/* This file defines different macros for printing debug and error information using different colors.
 * To control the amount of printing use the LOG_LEVEL constant in the Makefile.
 * The lower the log level, the higher the verbosity of the printing.
 * Defining -DERROR_FATAL in the Makefile will casue error prints to also exit the program.
 */

//Regular text
#define BLK "\033[0;30m"
#define RED "\033[0;31m"
#define GRN "\033[0;32m"
#define YEL "\033[0;33m"
#define BLU "\033[0;34m"
#define MAG "\033[0;35m"
#define CYN "\033[0;36m"
#define WHT "\033[0;37m"

//Regular bold text
#define BBLK "\033[1;30m"
#define BRED "\033[1;31m"
#define BGRN "\033[1;32m"
#define BYEL "\033[1;33m"
#define BBLU "\033[1;34m"
#define BMAG "\033[1;35m"
#define BCYN "\033[1;36m"
#define BWHT "\033[1;37m"

//Regular underline text
#define UBLK "\033[4;30m"
#define URED "\033[4;31m"
#define UGRN "\033[4;32m"
#define UYEL "\033[4;33m"
#define UBLU "\033[4;34m"
#define UMAG "\033[4;35m"
#define UCYN "\033[4;36m"
#define UWHT "\033[4;37m"

//Regular background
#define BLKB "\033[40m"
#define REDB "\033[41m"
#define GRNB "\033[42m"
#define YELB "\033[43m"
#define BLUB "\033[44m"
#define MAGB "\033[45m"
#define CYNB "\033[46m"
#define WHTB "\033[47m"

//High intensty background 
#define BLKHB "\033[0;100m"
#define REDHB "\033[0;101m"
#define GRNHB "\033[0;102m"
#define YELHB "\033[0;103m"
#define BLUHB "\033[0;104m"
#define MAGHB "\033[0;105m"
#define CYNHB "\033[0;106m"
#define WHTHB "\033[0;107m"

//High intensty text
#define HBLK "\033[0;90m"
#define HRED "\033[0;91m"
#define HGRN "\033[0;92m"
#define HYEL "\033[0;93m"
#define HBLU "\033[0;94m"
#define HMAG "\033[0;95m"
#define HCYN "\033[0;96m"
#define HWHT "\033[0;97m"

//Bold high intensity text
#define BHBLK "\033[1;90m"
#define BHRED "\033[1;91m"
#define BHGRN "\033[1;92m"
#define BHYEL "\033[1;93m"
#define BHBLU "\033[1;94m"
#define BHMAG "\033[1;95m"
#define BHCYN "\033[1;96m"
#define BHWHT "\033[1;97m"

//Reset
#define reset "\033[0m"


/*
 * Prints an info message to stdout.
 * Use this to print information that should be redirected.
 */
#if LOG_LEVEL <= 0
#define INFO_PRINT(...) do { fprintf(stderr, "%s", BGRN); fprintf(stderr, "[INFO][%s:%d]: ", __FILE__, __LINE__); fprintf(stderr, "%s", reset); fprintf(stdout, __VA_ARGS__); } while(0)
#else
#define INFO_PRINT(...) do { } while(0)
#endif

/*
 * Prints a debug message to stderr.
 * Use this to print debug information.
 */
#if LOG_LEVEL <= 1
#define DEBUG_PRINT(...) do { fprintf(stderr, "%s", BYEL); fprintf(stderr, "[DEBUG][%s:%d]: ", __FILE__, __LINE__); fprintf(stderr, "%s", reset); fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG_PRINT(...) do { } while(0)
#endif

/*
 * Prints an error message and terminates the program.
 * Use this to report fatal errors that prevent your program from proceeding.
 * If ERROR_FATAL is defined, the program will exit after printing.
 */
#if LOG_LEVEL <= 2
#ifdef ERROR_FATAL
#define ERROR_PRINT(...) do { fprintf(stderr, "%s", BRED); fprintf(stderr, "[ERROR][%s:%d]: ", __FILE__, __LINE__); fprintf(stderr, "%s", reset); fprintf(stderr, __VA_ARGS__); exit(1); } while(0)
#else
#define ERROR_PRINT(...) do { fprintf(stderr, "%s", BRED); fprintf(stderr, "[ERROR][%s:%d]: ", __FILE__, __LINE__); fprintf(stderr, "%s", reset); fprintf(stderr, __VA_ARGS__); } while(0)
#endif
#else
#define ERROR_PRINT(...) do { } while(0)
#endif


#define TEST_PRINT(...) do { fprintf(stderr, "%s", BCYN); fprintf(stderr, "[TEST]: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "%s", reset);} while(0)

#endif // __PRINTING_H__
