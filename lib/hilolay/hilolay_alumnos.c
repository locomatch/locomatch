#include <hilolay/alumnos.h>
#include <linuse/sockets/sockets.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

int suse_socket;

int suse_create(int tid){

	char* threadid = string_itoa(tid);

	t_paquete *create = crear_paquete(SUSE_CREATE);
	agregar_a_paquete(create, threadid, sizeof(int));
	enviar_paquete(create, suse_socket);

	free(threadid);
	return 0;
}

int suse_schedule_next(void){

	enviar_operacion(suse_socket, SUSE_SCHEDULE_NEXT);

	if(recibir_operacion(suse_socket) == SUSE_SCHEDULE_NEXT_RETURN){
		t_list *parametros = list_create();
		parametros = recibir_paquete(suse_socket);
		return atoi((char*)list_get(parametros, 0));
	}

	return 0;
}

int suse_join(int tid){

	char* threadid = string_itoa(tid);

	t_paquete *join = crear_paquete(SUSE_JOIN);
	agregar_a_paquete(join, threadid, sizeof(int));
	enviar_paquete(join, suse_socket);

	free(threadid);
	return 0;
}

int suse_close(int tid){
	
	char* threadid = string_itoa(tid);

	t_paquete *close = crear_paquete(SUSE_CLOSE);
	agregar_a_paquete(close, threadid, sizeof(int));
	enviar_paquete(close, suse_socket);

	free(threadid);
	return 0;
}

int suse_wait(int tid, char *sem_name){

	char* threadid = string_itoa(tid);

	t_paquete *wait = crear_paquete(SUSE_WAIT);
	agregar_a_paquete(wait, threadid, sizeof(int));
	agregar_a_paquete(wait, sem_name, strlen(sem_name)+1);
	enviar_paquete(wait, suse_socket);

	free(threadid);
	return 0;
}

int suse_signal(int tid, char *sem_name){

	char* threadid = string_itoa(tid);

	t_paquete *signal = crear_paquete(SUSE_SIGNAL);
	agregar_a_paquete(signal, threadid, sizeof(int));
	agregar_a_paquete(signal, sem_name, strlen(sem_name)+1);
	enviar_paquete(signal, suse_socket);

	free(threadid);
	return 0;
}

static struct hilolay_operations hiloops = {
		.suse_create = &suse_create,
		.suse_schedule_next = &suse_schedule_next,
		.suse_join = &suse_join,
		.suse_close = &suse_close,
		.suse_wait = &suse_wait,
		.suse_signal = &suse_signal
};

void hilolay_init(void){

	printf("Conectandose a SUSE..\n");

	suse_socket = crear_conexion_con_servidor("127.0.0.1",	"5003");

	printf("Conectado con SUSE\n");

	init_internal(&hiloops);
}
