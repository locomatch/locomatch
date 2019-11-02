#include "sockets.h"


/*	 SOCKETS CLIENTE	*/

int crear_conexion_con_servidor(char *ip, char* puerto){

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	if(socket_cliente == -1) printf("Error al crear el socket - Error: [%s]\n", strerror(errno));

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1){
		printf("Error al conectar con el servidor - Error: [%s]\n",strerror(errno));
		close(socket_cliente);
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

void* serializar_paquete(t_paquete* paquete, int bytes){

	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_mensaje(int op_code, char* mensaje, int socket_cliente){

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = op_code;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	if(send(socket_cliente, a_enviar, bytes, 0) == -1){
		printf("Error al enviar el mensaje - Mensaje: [%s]\n", mensaje);
		exit(-1);
	}

	free(a_enviar);
	eliminar_paquete(paquete);
}

int enviar_operacion(int socket_cliente, int op_code){

	if(send(socket_cliente, &op_code, sizeof(op_code), 0) == -1){
		printf("Error al enviar la operacion\n");
		return -1;
	}

	return 0;
}

void crear_buffer(t_paquete* paquete){

	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(int op_code){

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = op_code;
	crear_buffer(paquete);

	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio){

	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente){

	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	if(send(socket_cliente, a_enviar, bytes, 0) == -1){
		printf("Error al enviar paquete\n");
		exit(-1);
	}

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete){

	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente){

	close(socket_cliente);
}


/*	 SOCKETS SERVIDOR	 */

int iniciar_servidor(char* ip, char* puerto){

	int socket_servidor, error;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
//Mismo con pedir IP si usas LOCAL HOST?
	error = getaddrinfo(ip, puerto, &hints, &servinfo);
	if(error != 0){
		printf("Error al crear el socket: %s\n", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

    for (p = servinfo; p != NULL; p = p->ai_next){
		
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
        	printf("Error al crear el socket: %s\n", strerror(errno));
            continue;
		}
        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
        	printf("Error de Bind: %s\n", strerror(errno));
            close(socket_servidor);
            continue;		
        }
        break;
    }	

	if(listen(socket_servidor, SOMAXCONN) == -1) printf("Error de Socket: %s\n", strerror(errno));
	
    freeaddrinfo(servinfo);			

    printf("Listo para escuchar a mi cliente!\n");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor){

	struct sockaddr_in dir_cliente;
	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
	if(socket_cliente == -1){
		printf("Error al aceptar la conexion: %s\n", strerror(errno));
		return socket_cliente;
	}

	printf("Se conecto un cliente!\n");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente){

	int cod_op;

	int aux = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);

	if(aux > 0) return cod_op;
	else if (aux == -1) printf("Error al recibir la operacion: %s\n", strerror(errno));

	close(socket_cliente);
	return -1;
}

void* recibir_buffer(int* size, int socket_cliente){

	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente){

	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	printf("Me llego el mensaje: %s\n", buffer);

	free(buffer);
}

t_list* recibir_paquete(int socket_cliente){

	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size){

		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}

	free(buffer);
	return valores;
}
