#include <hilolay_alumnos.h>
#include <linuse/sockets/sockets.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

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

	return 0;
}

int suse_schedule_next(void){

	enviar_operacion(suse_socket, SUSE_SCHEDULE_NEXT);

	return 0;
}

int suse_join(int tid){

	return 0;
}

int suse_close(int tid){

	return 0;
}

int suse_wait(int tid, char *sem_name){

	return 0;
}

int suse_signal(int tid, char *sem_name){

	return 0;
}

void hilolay_init(void){

	printf("Conectandose a SUSE..\n");

	suse_socket = crear_conexion_con_servidor("127.0.0.1",	"5003");

	printf("Conectado con SUSE\n");

	init_internal(&hiloops);
}
