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

/* action_open abre un archivo */
char* action_open(package_open* package) {
    log_info(logger,"Se recibió una accion open");
    return "je";
}

/* action_read leer un archivo abierto */
char* action_read(package_read* package) {
    log_info(logger,"Se recibio una accion read");
    return "ja";
}

/* action_getattr obtiene los atributos de un archivo */
char* action_getattr(package_getattr* package) {
    log_info(logger,"Se recibio una accion getattr");
    return "ja";
}

/* action_mknod crea el nodo de un archivo */
char* action_mknod(package_mknod* package) {
    log_info(logger,"Se recibio una accion mknod");
    return "jo";
}

/* action_mkdir crea un directorio */
char* action_mkdir(package_mkdir* package) {
    log_info(logger,"Se recibio una accion mkdir");
    return "jo";
}

/* action_write escribe en un archivo abierto */
char* action_write(package_write* package) {
    log_info(logger,"Se recibio una accion write");
    return "jo";
}

/* action_opendir abre un directorio */
char* action_opendir(package_opendir* package) {
    log_info(logger,"Se recibio una accion opendir");
    return "jo";
}

/* action_readdir abre un directorio */
char* action_readdir(package_readdir* package) {
    log_info(logger,"Se recibio una accion readdir");
    return "jo";
}