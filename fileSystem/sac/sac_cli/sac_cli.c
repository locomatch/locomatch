#include "sac_cli.h"

#define IP "127.0.0.1"
#define PUERTO "8003"
#define PACKAGESIZE 1024

int serverSocket;
t_log* logger;

int main (void) {
    printf("Corriendo el cliente\n");
    
    //acá tengo que implementar fuse para mandar todo a sac_servidor que se va a encargar de implementar el fs

    /*LOG*/
    char* LOGPATH = "sac_cli.log";
    logger = log_create(LOGPATH, "Sac_servidor", 1, LOG_LEVEL_INFO);

    //run client
    struct addrinfo hints;
    struct addrinfo *serverInfo;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP
    getaddrinfo(IP, PUERTO, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion
    //int serverSocket;
	serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);	// No lo necesitamos mas


    //Prueba de envio
    //sac_send(serverSocket);
    sac_open("open bla bla");
    sac_read("read bla bla");
}

void sac_send(char* msg, int serverSocket){

    /*int enviar = 1;
	char message[PACKAGESIZE];

	printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

	while(enviar){
		fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
		if (!strcmp(message,"exit\n")) enviar = 0;			// Chequeo que el usuario no quiera salir
		if (enviar) send(serverSocket, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.
	}*/

    send(serverSocket, msg, strlen(msg) + 1, 0);
}

/*void sac_open(char* msg) {
    sac_send(msg, serverSocket);
}*/

/* sac_open abre un archivo */
void sac_open(char* msg) {
    log_info(logger,"Se recibio una instruccion open");
    sac_send(msg, serverSocket);
    return;
}

/* sac_read leer un archivo abierto */
void sac_read(char* msg) {
    log_info(logger,"Se recibio una instruccion read");
    sac_send(msg, serverSocket);
    return;
}

/* sac_getattr obtiene los atributos de un archivo */
void sac_getattr(char* msg) {
    log_info(logger,"Se recibio una instruccion getattr");
    sac_send(msg, serverSocket);
    return;
}

/* sac_mknod crea el nodo de un archivo */
void sac_mknod(char* msg) {
    log_info(logger,"Se recibio una instruccion mknod");
    sac_send(msg, serverSocket);
    return;
}

/* sac_mkdir crea un directorio */
void sac_mkdir(char* msg) {
    log_info(logger,"Se recibio una instruccion mkdir");
    sac_send(msg, serverSocket);
    return;
}

/* sac_write escribe en un archivo abierto */
void sac_write(char* msg) {
    log_info(logger,"Se recibio una instruccion write");
    sac_send(msg, serverSocket);
    return;
}

/* sac_opendir abre un directorio */
void sac_opendir(char* msg) {
    log_info(logger,"Se recibio una instruccion opendir");
    sac_send(msg, serverSocket);
    return;
}

/* sac_readdir abre un directorio */
void sac_readdir(char* msg) {
    log_info(logger,"Se recibio una instruccion readdir");
    sac_send(msg, serverSocket);
    return;
}