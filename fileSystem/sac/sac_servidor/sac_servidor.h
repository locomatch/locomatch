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

#endif /* SAC_SERVIDOR_H_ */
