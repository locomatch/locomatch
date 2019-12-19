#ifndef MUSE_H_
#define MUSE_H_

/*        INCLUDES        */

#include <stdio.h>
#include <commons/config.h>
#include <commons/log.h>
#include <linuse/sockets/sockets.h>



t_log* logger;
int server_socket;

void init_server();
void init_logger();
void cargar_datos_muse();


typedef struct{
	char* ip;
	int puerto;
	int tamanio_mem;
	int tamanio_pagina;
	int tamanio_swap;
}mi_config;

mi_config* config_data;
char* LOGPATH;

void init_config();

void init_logger();

void init_server();
	
void init_memoria();


#endif /* MUSE_H_ */
