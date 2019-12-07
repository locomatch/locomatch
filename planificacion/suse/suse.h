#ifndef SUSE_H_
#define SUSE_H_

/*        INCLUDES        */

#include <readline/readline.h>
#include <readline/history.h>

#ifndef SUSE_SCHEDULER_H_
	#include "suse_scheduler.h"
#endif

/*        CONSTANTS        */

const int MIN_CONFIG_KEY = 7;

/*        DEFINITIONS        */

#define CONFIG_PATH "./suse.ini"
#define NULL_CLIENTE -1;

/*        GLOBALS        */

t_config* config;
int server_socket;

pthread_t socket_thread;
pthread_t metrics_thread;

/*        PROTOTYPES        */

int main(void);
void init_logger();
void init_suse();
void init_metrics();
void *timed_metrics(void *arg);
t_config* generar_config();
void cargar_config();
void init_server();
void* wait_for_client(void *arg);
void join_threads();
void end_suse();
bool has_property(char* property);
t_list* _get_semaforos_from_config();
void init_console();
void destroy_configData();
void destroy_config_semaforo(t_config_semaforo *config_semaforo);

#endif /* SUSE_H_ */
