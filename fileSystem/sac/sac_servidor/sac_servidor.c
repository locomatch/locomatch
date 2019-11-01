#include "sac_servidor.h"

t_log* logger;

int main(void) {
    printf("Corriendo el servidor\n");

    /*CONFIG*/
    t_config* config = config_create("config");
    int PORT = config_get_int_value(config, "LISTEN_PORT");

    /*LOG*/
    char* LOGPATH = strdup(config_get_string_value(config, "LOG_PATH"));
    logger = log_create(LOGPATH, "Sac_servidor", 1, LOG_LEVEL_INFO);

    /*SERVER*/
    log_info(logger, "iniciando thread server");
    server_info* serverInfo = malloc(sizeof(server_info));
    serverInfo->logger = logger;
    serverInfo->portNumber = PORT;
    pthread_t tid;
    pthread_create(&tid, NULL, run_server, (void*) serverInfo);

    //run_server();
    //JOIN THREADS
    pthread_join(tid,NULL);
   
    //FREE MEMORY
    free(LOGPATH);
    free(logger);
    free(serverInfo);
    config_destroy(config);
   
 
    return 0;
}

char* action_open(package_open* package) {
    log_info(logger,"Se recibió una accion open");
    return "je";
}