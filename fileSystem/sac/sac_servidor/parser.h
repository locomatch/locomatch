#ifndef PARSER_H_
#define PARSER_H_

/*        INCLUDES        */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <commons/string.h>
#include <commons/error.h>
#include "sac_servidor.h"

/*        CONSTANTS        */
/*        GLOBALS        */
/*        DEFINITIONS        */


// OPEN [path] [mode]
typedef struct {
    char* instruction;
    char* path;
    char* mode;
} package_open;

/*        PROTOTYPES        */

char* parse_input(char* instr_buff);

#endif /* PARSER_H_ */