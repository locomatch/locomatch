#ifndef PARSER_H_
#define PARSER_H_

/*        INCLUDES        */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <commons/string.h>
#include <commons/error.h>

typedef struct {
    char* instruction;
    char* path;
    char* flags;
} package_open;

typedef struct {
    char* instruction;
    char* path;
    char* size;
    char* offset;
} package_read;

typedef struct {
    char* instruction;
    char* path;
    //char* mode;
} package_getattr;

typedef struct {
    char* instruction;
    char* path;
    //char* mode;
} package_mknod;

typedef struct {
    char* instruction;
    char* path;
    //char* mode;
} package_mkdir;

typedef struct {
    char* instruction;
    char* path;
    char* buf;
    char* size_s;
    char* offset_s;
} package_write;

typedef struct {
    char* instruction;
    char* path;
    char* flags;
} package_opendir;

typedef struct {
    char* instruction;
    char* path;
    char* mode;
} package_readdir;

#include "sac_servidor.h"

/*        CONSTANTS        */
/*        GLOBALS        */
/*        DEFINITIONS        */


// OPEN [path] [mode]
/*typedef struct {
    char* instruction;
    char* path;
    char* mode;
} package_open;*/

/*        PROTOTYPES        */

char* parse_input(char* instr_buff);

#endif /* PARSER_H_ */