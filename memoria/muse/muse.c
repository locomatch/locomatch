#include "muse.h"
#include "segments.h"

#include <stdlib.h>
#include <commons/log.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <signal.h>


int main(void) {

	init_logger();

	log_info(logger, "[MUSE] Iniciando..");

	init_server();
	
	init_memoria();

	exit(EXIT_SUCCESS);
}

void init_logger(){

	logger = log_create("Muse.log", "muse", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "Logger inicializado");
}
void init_memoria(){
//INICIALIZO MEMORIA PRINCIPAL Y RELLENO CON 0
  int main_memory_size = muse_config.tamanio_mem;
  MAIN_MEMORY = malloc(main_memory_size);
  if(MAIN_MEMORY == NULL) {
    log_error(logger, "No se pudo alocar espacio para la memoria principal.");
    return 0;
  }
  memset(MAIN_MEMORY, 0, main_memory_size);
}

void init_server(){
//deberia ir directo al main?
	cargar_datos_muse();
	server_socket = iniciar_servidor(muse_config.ip_escucha, muse_config.puerto_escucha);
	if(server_socket == -1) exit(-1);
//Deberia hacerse con Threads??
	int cliente_s = esperar_cliente(server_socket);

t_list* lista;
while(1)
{
int cod_op = recibir_operacion(cliente_s);
switch (cod_op) {
	case MUSE_ALLOC:
	recibir_mensaje(cliente_s);
		break;
	case MUSE_FREE:
	recibir_mensaje(cliente_s);
		break;
	//TODAS LAS PROXIMAS RECIBEN PAQUETES Y NO MENSAJES
	case MUSE_GET:
		break;
	case MUSE_CPY:
		break;
	case MUSE_MAP:
		break;
	case MUSE_SYNC:
		break;
	case MUSE_UNMAP:
		break;   
	case -1:
		log_error(logger, "el cliente se desconecto. Terminando servidor");
		return EXIT_FAILURE;
	default:
		log_warning(logger, "Operacion desconocida. No quieras meter la pata");
		break;
	}
}
return EXIT_SUCCESS;

}

void cargar_datos_muse(){
	muse_ini = config_create("muse.ini");
	muse_config.puerto_escucha = config_get_string_value(muse_ini,"LISTEN_PORT");
	muse_config.ip_escucha = config_get_string_value(muse_ini,"LISTEN_IP");
	muse_config.tamanio_mem = config_get_int_value(muse_ini,"MEMORY_SIZE");
	muse_config.tamanio_pagina = config_get_int_value(muse_ini,"PAGE_SIZE");
	muse_config.tamanio_swap = config_get_int_value(muse_ini,"SWAP_SIZE");
}
