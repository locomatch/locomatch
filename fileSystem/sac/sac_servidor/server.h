#ifndef SERVER_H_
#define SERVER_H_

/*        INCLUDES        */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h> //loggger

/*        CONSTANTS        */
/*        GLOBALS        */
/*        DEFINITIONS        */
// Ver de mandar esto para arriba y sacarlo de un config o algo asi:
//Client:
#define IP "127.0.0.1"
#define PUERTO "6667"
#define PACKAGESIZE 1024 // Define cual va a ser el size maximo del package a enviar
//Server:
//#define PUERTO "6667" Queda comentado porque ya esta definido para el client
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
//#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

typedef struct {
    int portNumber;
    t_log* logger;
} server_info;

/*        PROTOTYPES        */

void* run_server(void* args);

#endif /* SERVER_H_ */