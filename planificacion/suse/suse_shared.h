#ifndef SUSE_SHARED_H_
#define SUSE_SHARED_H_

/*        INCLUDES        */

#include <stdio.h>
#include <stdbool.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <linuse/sockets/sockets.h>
#include <pthread.h>

/*        DEFINITIONS        */

typedef struct {
	char * listen_port;
	int metrics_timer;
	int max_multiprog;
	t_list *semaforos;
	float alpha_sjf;
} t_configData;

typedef struct {
	char* id;
	int init;
	int max;
} t_semaforo;

/*        CONSTANTS        */
/*        GLOBALS        */

t_log *logger;
t_configData *configData;

/*        PROTOTYPES        */

#endif /* SUSE_SHARED_H_ */