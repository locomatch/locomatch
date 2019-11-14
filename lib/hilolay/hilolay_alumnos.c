#include <hilolay_alumnos.h>
#include <linuse/sockets/sockets.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

int suse_socket;

static struct hilolay_operations hiloops = {
		.suse_create = &suse_create,
		.suse_schedule_next = &suse_schedule_next,
		.suse_join = &suse_join,
		.suse_close = &suse_close,
		.suse_wait = &suse_wait,
		.suse_signal = &suse_signal
};

int suse_create(int tid){

	t_paquete *create = crear_paquete(SUSE_CREATE);
	agregar_a_paquete(create, &tid, sizeof(int));
	enviar_paquete(create, suse_socket);

	return 0;
}

int suse_schedule_next(void){

	enviar_operacion(suse_socket, SUSE_SCHEDULE_NEXT);

	if(recibir_operacion(suse_socket) == SUSE_SCHEDULE_NEXT_RETURN){
		t_list *parametros = list_create();
		parametros = recibir_paquete(suse_socket);
		return (int)list_get(parametros, 0);
	}

	return 0;
}

int suse_join(int tid){

	t_paquete *create = crear_paquete(SUSE_JOIN);
	agregar_a_paquete(create, &tid, sizeof(int));
	enviar_paquete(create, suse_socket);

	return 0;
}

int suse_close(int tid){

	t_paquete *create = crear_paquete(SUSE_CLOSE);
	agregar_a_paquete(create, &tid, sizeof(int));
	enviar_paquete(create, suse_socket);

	return 0;
}

int suse_wait(int tid, char *sem_name){

	t_paquete *create = crear_paquete(SUSE_WAIT);
	agregar_a_paquete(create, &tid, sizeof(int));
	agregar_a_paquete(create, sem_name, strlen(sem_name)+1);
	enviar_paquete(create, suse_socket);

	return 0;
}

int suse_signal(int tid, char *sem_name){

	t_paquete *create = crear_paquete(SUSE_SIGNAL);
	agregar_a_paquete(create, &tid, sizeof(int));
	agregar_a_paquete(create, sem_name, strlen(sem_name)+1);
	enviar_paquete(create, suse_socket);

	return 0;
}

void hilolay_init(void){

	printf("Conectandose a SUSE..\n");

	suse_socket = crear_conexion_con_servidor("127.0.0.1",	"5003");

	printf("Conectado con SUSE\n");

	init_internal(&hiloops);
}
