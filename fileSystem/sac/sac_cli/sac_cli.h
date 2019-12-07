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
#include <commons/log.h>

#include <stddef.h>
#include <fuse.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

/*        CONSTANTS        */
/*        GLOBALS        */


/*        DEFINITIONS        */
/*        PROTOTYPES        */

int main(int argc,char *argv[]);
int sac_open(char* msg);
int sac_read(char* msg);
static int sac_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
int sac_mknod(char* msg);
int sac_mkdir(char* msg);
int sac_write(char* msg);
int sac_opendir(char* msg);
int sac_readdir(char* msg);
void sac_send(char* msg,int serverSocket);

#endif /* SAC_CLI_H_ */