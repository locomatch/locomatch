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


int main() { //Tomado de otra repo, sirve para inicializar diferentes configs //char const *argv[]

	init_config();

	init_logger();

	log_info(logger, "[MUSE] Iniciando.. \n");

	init_memoria();
	
	init_server();
	

	exit(EXIT_SUCCESS);
}

void init_config(){
	config_data = malloc(sizeof(mi_config));

	generate_config(config_data, "config0");
	printf("Config Inicializado. \n");
}

void init_logger(){

	logger = log_create(LOGPATH, "Muse", 1, LOG_LEVEL_INFO); //Que tipo de level le pongo?
	log_info(logger, "Logger inicializado \n");
}
void init_memoria(){
//INICIALIZO MEMORIA PRINCIPAL Y RELLENO CON 0
  tamanio_mem = config_data -> tamanio_mem;
  tamanio_pagina = config_data -> tamanio_pagina;
  tamanio_swap = config_data -> tamanio_swap;
  cant_pags = tamanio_mem/tamanio_pagina;

  MAIN_MEMORY = malloc(tamanio_mem);
  if(MAIN_MEMORY == NULL) {
    log_error(logger, "No se pudo alocar espacio para la memoria principal.");
  }
  memset(MAIN_MEMORY, 0, tamanio_mem); 
}

void init_server(){
	server_socket = iniciar_servidor(config_data->ip, config_data->puerto);
	if(server_socket == -1) exit(-1);

	clientes = list_create();

	if (pthread_create (&socket_thread, NULL, &wait_for_client, NULL) != 0){
		print_pthread_create_error("socket_thread");
		exit(-1);
	}

void* wait_for_client(void *arg){

	log_info(logger, "Esperando Clientes.");
	int cliente;

	while(1){
		cliente = esperar_cliente(server_socket);
		if (cliente > 0) {
			list_add(clientes, cliente);
		}

		cliente = NULL_CLIENTE;
	}

	log_debug(logger, "Finalizando Servidor Escucha");

	pthread_exit(EXIT_SUCCESS);
}
//Deberia hacerse con Threads??
	while(1){

	int cliente_s = esperar_cliente(server_socket);

while(1){
	
t_list *parametros = list_create();
int cod_op = recibir_operacion(cliente_s);

switch (cod_op) {
	case MUSE_ALLOC:
		parametros = recibir_paquete(server_socket);
		uint32_t tam = atoi(list_get(parametros, 0));
		//ejecutar proceso e enviar respuesta
		break;

	case MUSE_FREE:
		parametros = recibir_paquete(server_socket);
		uint32_t dir = atoi(list_get(parametros, 0));
		//ejecutar proceso e enviar respuesta
		break;

	case MUSE_GET:
		parametros = recibir_paquete(server_socket);
		int dst = atoi(list_get(parametros, 0));
		uint32_t src = atoi(list_get(parametros, 1));
		int n = atoi(list_get(parametros, 2));
		//ejecutar proceso e enviar respuesta
		break;

	case MUSE_CPY:
		parametros = recibir_paquete(server_socket);
		int dst = atoi(list_get(parametros, 0));
		uint32_t src = atoi(list_get(parametros, 1));
		int n = atoi(list_get(parametros, 2));
		//ejecutar proceso e enviar respuesta		
		break;

	case MUSE_MAP:
		break;

	case MUSE_SYNC:
		break;

	case MUSE_UNMAP:
		break;
		
	default:
		log_warning(logger, "Operacion desconocida.");
		break;
	}
}
//return EXIT_SUCCESS;

}

void generate_config(mi_config* config_d, char* config_n){
	t_config* config = config_create(config_n);
	
	char* LOGPATH = config_get_string_value(config, "LOG_PATH");

	config_d->ip = malloc(strlen(config_get_string_value(config, "IP")) + 1);
	strcpy(config_d->ip, config_get_string_value(config, "IP"));

	config_d->puerto = config_get_int_value(config, "LISTEN_PORT");
	config_d->tamanio_mem = config_get_int_value(config,"MEMORY_SIZE");
	config_d->tamanio_pagina = config_get_int_value(config, "PAGE_SIZE");
	config_d->tamanio_swap = config_get_int_value(config, "SWAP_SIZE");

	config_destroy(config);
}
