#ifndef SUSE_SHARED_H_
#define SUSE_SHARED_H_

/*        INCLUDES        */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <linuse/sockets/sockets.h>
#include <pthread.h>
#include <sys/time.h>
#include "suse_errors.h"

/*        CONSTANTS        */
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
} t_config_semaforo;

/*        GLOBALS        */

bool endsuse;
t_log *logger;
t_configData *configData;

/*        PROTOTYPES        */

void print_malloc_error(char* element);
void print_pthread_create_error(char* element);

#endif /* SUSE_SHARED_H_ */
