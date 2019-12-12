#include <hilolay/alumnos.h>
#include <linuse/sockets/sockets.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

#define CONFIG_PATH "/home/utnso/workspace/tp-2019-2c-Los-Borbotones/lib/hilolay/hilolay.ini"

int suse_socket;
char* suse_ip;
char* suse_port;

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

	return -1;
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

	t_config *config;

	if((config = config_create(CONFIG_PATH)) == NULL){
		printf("Error al crear el config.\n");
		exit(-1);
	}

	printf("Cargando archivo de configuracion..\n");

	if(config_has_property(config, "SUSE_IP")){
		suse_ip = string_new();
		string_append(&suse_ip, config_get_string_value(config, "SUSE_IP"));
	}else{
		printf("No se encuentra SUSE_IP en el archivo de configuracion.\n");
		config_destroy(config);
		exit(-1);
	}

	if(config_has_property(config, "SUSE_PORT")){
		suse_port = string_new();
		string_append(&suse_port, config_get_string_value(config, "SUSE_PORT"));
	}else{
		printf("No se encuentra SUSE_PORT en el archivo de configuracion.\n");
		config_destroy(config);
		exit(-1);
	}

	config_destroy(config);

	printf("Se ha cargado correctamente el archivo de configuracion.\n");

	printf("Conectandose a SUSE..\n");

	suse_socket = crear_conexion_con_servidor(suse_ip, suse_port);

	printf("Conectado con SUSE\n");

	init_internal(&hiloops);
}
