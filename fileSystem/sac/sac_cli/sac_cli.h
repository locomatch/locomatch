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
#include <commons/config.h>

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

static int sac_open(const char *path, struct fuse_file_info *fi);
static int sac_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int sac_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
static int sac_mknod(char* path, mode_t mode, dev_t rdev);
static int sac_mkdir(char* path, mode_t mode);
static int sac_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int sac_opendir(const char *path, struct fuse_file_info *fi);
static int sac_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

void sac_send(char* msg,int serverSocket);

#endif /* SAC_CLI_H_ */