#ifndef SRC_SOCKET_H_
#define SRC_SOCKET_H_

/*		INCLUDES		 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "codigos.h"

/*		CONSTANTS		*/
/*		GLOBALS		*/
/*		DEFINITIONS		*/

typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

/*		PROTOTYPES		*/

//	SOCKETS CLIENTE
int crear_conexion_con_servidor(char* ip, char* puerto);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(int op_code, char* mensaje, int socket_cliente);
int enviar_operacion(int socket_cliente, int op_code);
t_paquete* crear_paquete(int op_code);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

//	SOCKETS SERVIDOR
void* recibir_buffer(int*, int);
int iniciar_servidor(char* ip, char* puerto);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

#endif /* SRC_SOCKET_H_ */
