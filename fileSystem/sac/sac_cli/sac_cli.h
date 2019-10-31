#ifndef SAC_CLI_H_
#define SAC_CLI_H_

/*        INCLUDES        */

#include <stdio.h>
//#include "../server.c"

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

/*        CONSTANTS        */
/*        GLOBALS        */

/*struct fuse_operations sac_oper = {
  .open = sac_open,
  .read = sac_read
  //faltan varias operaciones
};*/

/*        DEFINITIONS        */
/*        PROTOTYPES        */

int main(void);
void sac_open(char* msg);/*
void sac_read(void);*/
void sac_send(char* msg,int serverSocket);

#endif /* SAC_CLI_H_ */