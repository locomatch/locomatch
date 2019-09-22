#ifndef SUSE_H_
#define SUSE_H_

/*        INCLUDES        */

#ifndef SUSE_SHARED_H_
	#include "suse_shared.h"
#endif

/*        DEFINITIONS        */

#define CONFIG_PATH "./suse.ini"
const int MIN_CONFIG_KEY = 7;

/*        CONSTANTS        */
/*        GLOBALS        */

bool endsuse;
t_config* config;
int server_socket;

pthread_t socket_thread;

/*        PROTOTYPES        */

int main(void);
void init_logger();
void init_suse();
t_config* generar_config();
void cargar_config();
void init_server();
void* wait_for_client(void *arg);
void end_suse();
bool has_property(char* property);
t_list* _get_semaforos_from_config();

#endif /* SUSE_H_ */
