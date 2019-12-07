#ifndef SAC_SERVIDOR_H_
#define SAC_SERVIDOR_H_

/*        INCLUDES        */

#include <stdio.h>
#include "server.h"
#include "parser.h"

#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>

#include <string.h>
#include <commons/string.h>

/*        CONSTANTS        */
/*        GLOBALS        */
/*        DEFINITIONS        */

#define MAX_FILENAME_LENGTH 71
#define MAGIC_NUMBER_NAME "SAC"
#define BLOCK_SIZE 4096

// HEADER:
typedef struct sac_header_t {
    unsigned char id[3];
    uint32_t version;
    uint32_t bitmap_start;
    uint32_t bitmap_size; //tamaño en bloques
    unsigned char padding[4081];
} GHeader;

// NODO:
typedef struct sac_file_t {
    uint8_t state;
    unsigned char fname[MAX_FILENAME_LENGTH];
    uint32_t parent_block;//bloque padre
    uint32_t filesize;
    uint64_t created;//fecha de creacion
    uint64_t modified;//fecha de modificacion
    uint32_t blocks[1000];
} GFile;


/*        PROTOTYPES        */

int main(void);

char* action_open(package_open* package);
char* action_read(package_read* package);
char* action_getattr(package_getattr* package);
char* action_mknod(package_mknod* package);
char* action_mkdir(package_mkdir* package);
char* action_write(package_write* package);
char* action_opendir(package_opendir* package);
char* action_readdir(package_readdir* package);

int search_for_file(char* path);
char** parse_path(char* path);
int find_node(char** path_array);

#endif /* SAC_SERVIDOR_H_ */
