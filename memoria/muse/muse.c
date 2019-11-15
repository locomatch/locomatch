#include "muse.h"
#define PUERTO "5555"


int main(void) {

	init_logger();

	log_info(logger, "[MUSE] Iniciando..");
	init_server();

	exit(EXIT_SUCCESS);
}

void init_logger(){

	logger = log_create("Muse.log", "muse", 1, LOG_LEVEL_DEBUG);
}

void init_server(){

	cargar_datos_muse();
	server_socket = iniciar_servidor(muse_config.ip_escucha, muse_config.puerto_escucha);
	if(server_socket == -1) exit(-1);

	clientes = list_create();
}

void *wait_for_client(void *arg){

	log_info(logger, "Esperando Clientes.");
	int cliente;

	while(1){
		cliente = esperar_cliente(server_socket);
		if (cliente > 0) {
			manage_operation(cliente);
		}

		cliente = NULL_CLIENTE;
	}

	log_debug(logger, "Finalizando Servidor Escucha");

	pthread_exit(EXIT_SUCCESS);
}

void manage_operation(int client_fd){

	log_debug(logger, "Recibiendo operacion..");

	int codigo = recibir_operacion(client_fd);
	switch (codigo) {
		case MUSE_ALLOC:
			break;
		case MUSE_FREE:
			break;
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
		default:
			log_debug(logger, "Operacion desconocida - op_code: %d", codigo);
			break;
	}

}

void cargar_datos_muse(){
	muse_ini = config_create("muse.ini");
	muse_config.puerto_escucha = config_get_int_value(muse_ini,"LISTEN_PORT");
	muse_config.ip_escucha = config_get_string_value(muse_ini,"LISTEN_IP");
	muse_config.tamanio_mem = config_get_int_value(muse_ini,"MEMORY_SIZE");
	muse_config.tamanio_pagina = config_get_int_value(muse_ini,"PAGE_SIZE");
	muse_config.tamanio_swap = config_get_int_value(muse_ini,"SWAP_SIZE");
}
