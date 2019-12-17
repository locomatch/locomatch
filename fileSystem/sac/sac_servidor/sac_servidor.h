#ifndef SAC_SERVIDOR_H_
#define SAC_SERVIDOR_H_

/*        INCLUDES        */

#include <stdio.h>
#include "server.h"
#include "parser.h"
#include "bitarray.h"

#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>

#include <string.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <time.h>

/*        CONSTANTS        */
/*        GLOBALS        */
/*        DEFINITIONS        */

#define MAX_FILENAME_LENGTH 71
#define MAGIC_NUMBER_NAME "SAC"
#define BLOCK_SIZE 4096

// BLOCK
typedef struct sac_block_t {
    unsigned char bytes[BLOCK_SIZE];
} GBlock;

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
    uint8_t state; // 0:borrado 1:ocupado 2:directorio
    unsigned char fname[MAX_FILENAME_LENGTH];
    uint32_t parent_block;//bloque padre
    uint32_t filesize;
    uint64_t created;//fecha de creacion
    uint64_t modified;//fecha de modificacion
    uint32_t blocks[1000];
} GFile;

// Entrada en la tabla de archivos abiertos:
struct sac_opened_file {
    uint16_t fd; // file descriptor
    uint32_t node_block; //bloque de nodos donde esta el archivo
    uint8_t mode; // modo en el que se abrio
    uint8_t state; // 0:borrado 1:ocupado 2:directorio
    unsigned char fname[MAX_FILENAME_LENGTH];
    uint32_t parent_block;//bloque padre
    struct sac_opened_file* next;
};


/*        PROTOTYPES        */

int main(void);
//void check_disc();
void dump_block(int b);

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
int check_node_state(int directory_node_number);
int find_free_block(int place); // place = 0 node table ; place = 1 data blocks
void set_block_as_occupied(int block_number);
char* read_block(int block, int offset);

int insert_to_opened_files_table(struct sac_opened_file* opened_file);

void format_bitmap_nodetable();

char* search_for_dir_childs(int dir_node_number);

#endif /* SAC_SERVIDOR_H_ */
