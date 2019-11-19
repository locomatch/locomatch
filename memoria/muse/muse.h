#ifndef MUSE_H_
#define MUSE_H_

/*        INCLUDES        */

#include <stdio.h>
#include <commons/config.h>

typedef struct{
	int puerto_escucha;
	char* ip_escucha;
	int tamanio_mem;
	int tamanio_pagina;
	int tamanio_swap;
}_muse_ini;

typedef struct {
	t_link_element *head;
	int elements_count;
} t_list;

_muse_ini muse_config;
t_config* muse_ini;
t_log* logger;
int server_socket;

int main(void);

#endif /* MUSE_H_ */
