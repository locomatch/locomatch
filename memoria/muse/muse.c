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

	server_socket = iniciar_servidor("127.0.0.1", PUERTO);
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