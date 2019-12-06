#ifndef MUSE_H_
#define MUSE_H_

/*        INCLUDES        */

#include <stdio.h>
#include <commons/config.h>
#include <commons/log.h>
#include <linuse/sockets/sockets.h>

typedef struct{
	char* puerto_escucha;
	char* ip_escucha;
	int tamanio_mem;
	int tamanio_pagina;
	int tamanio_swap;
}_muse_ini;


_muse_ini muse_config;
t_config* muse_ini;
t_log* logger;
int server_socket;

int main(void);
void init_server();
void init_logger();
void cargar_datos_muse();

#endif /* MUSE_H_ */
