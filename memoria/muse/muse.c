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
//deberia ir directo al main?
	server_socket = iniciar_servidor(config_data->ip, config_data->puerto);
	if(server_socket == -1) exit(-1);
//Deberia hacerse con Threads??
t_list* clients;
	while(1){
	int cliente_s = esperar_cliente(server_socket);


t_list* lista;
while(1)
{
t_list *parametros = list_create();
int cod_op = recibir_operacion(cliente_s);
uint32_t tam; //int?
uint32_t dir; //int?
uint32_t src; //PODRIAN SER EL MISMO, NO?
size_t n;
void* dst;

//charX = (char*)list_get(parametros, x);

switch (cod_op) {
	case MUSE_ALLOC:
		parametros = recibir_paquete(server_socket);
		tam = atoi(list_get(parametros, 0));
		//ejecutar proceso e enviar respuesta
		break;

	case MUSE_FREE:
		parametros = recibir_paquete(server_socket);
		dir = atoi(list_get(parametros, 0));
		//ejecutar proceso e enviar respuesta
		break;

	case MUSE_GET:
		parametros = recibir_paquete(server_socket);
		dst = //COMO HAGO ACA? (char*)list_get(parametros, 0);
		src = atoi(list_get(parametros, 1));
		n = atoi(list_get(parametros, 2));
		//ejecutar proceso e enviar respuesta
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
