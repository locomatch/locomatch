#ifndef SAC_SERVIDOR_H_
#define SAC_SERVIDOR_H_

/*        INCLUDES        */

#include <stdio.h>
#include "server.h"
#include "parser.h"

#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>

/*        CONSTANTS        */
/*        GLOBALS        */
/*        DEFINITIONS        */
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

#endif /* SAC_SERVIDOR_H_ */
