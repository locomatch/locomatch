#include "suse.h"

int main(void) {

	init_logger();

	log_info(logger, "[SUSE] Iniciando..");
	init_suse();

	init_console();

	log_info(logger, "[SUSE] Finalizando..");
	end_suse();

	printf("[SUSE] GoodBye!\n");
	exit(EXIT_SUCCESS);
}

void init_logger(){

	logger = log_create("suse.log", "suse", 1, LOG_LEVEL_DEBUG);
}

void init_suse(){

	program_id_counter = 0;

	config = generar_config();
	cargar_config(config);

	init_scheduler();
	init_semaforos();
	init_server();

	log_info(logger, "[SUSE] Inicializacion Completa");
}

t_config* generar_config(){

	t_config *configuracion;
	if((configuracion = config_create(CONFIG_PATH)) == NULL){
		log_error(logger, "Error al crear el t_config.");
		exit(-1);
	}

	return configuracion;
}

void cargar_config(){

	log_debug(logger, "Cargando archivo de configuracion..");

	configData = malloc(sizeof(t_configData));
	if(configData == NULL){
		print_malloc_error("Archivo de Configuracion");
		log_destroy(logger);
		config_destroy(config);
		exit(-1);
	}

	if(config_keys_amount(config) < MIN_CONFIG_KEY){
		log_error(logger, "Faltan parametros en el archivo de configuracion.");
		log_destroy(logger);
		config_destroy(config);
		exit(-1);
	}

	if(has_property("LISTEN_PORT")){
		configData->listen_port = string_new();
		string_append(&configData->listen_port, config_get_string_value(config, "LISTEN_PORT"));
	} else exit(-1);

	if(has_property("METRICS_TIMER")){
		configData->metrics_timer = config_get_int_value(config, "METRICS_TIMER");
	}else exit(-1);

	if(has_property("MAX_MULTIPROG")){
		configData->max_multiprog = config_get_int_value(config, "MAX_MULTIPROG");
	}else exit(-1);

	if(has_property("SEM_IDS") && has_property("SEM_INIT") && has_property("SEM_MAX")){
		configData->semaforos = _get_semaforos_from_config();
		if(configData->semaforos == NULL){
			print_malloc_error("Archivo de Configuracion");
			log_destroy(logger);
			config_destroy(config);
			exit(-1);
		}
	}else exit(-1);

	if(has_property("ALPHA_SJF")){
		configData->alpha_sjf = atof(config_get_string_value(config, "ALPHA_SJF"));
	}else exit(-1);

	config_destroy(config);

	log_debug(logger,"Se ha cargado correctamente el archivo de configuracion.");
}

void init_server(){

	server_socket = iniciar_servidor("127.0.0.1", configData->listen_port);
	if(server_socket == -1) exit(-1);

	clientes = list_create();

	if (pthread_create (&socket_thread, NULL, &wait_for_client, NULL) != 0){
		print_pthread_create_error("socket_thread");
		exit(-1);
	}
}

void *wait_for_client(void *arg){

	log_info(logger, "Esperando Clientes.");
	int cliente;

	while(!endsuse){
		cliente = esperar_cliente(server_socket);
		if (cliente > 0) {
			if(!is_program_loaded(cliente)) create_program(cliente);
		}

		cliente = NULL_CLIENTE;
	}

	log_debug(logger, "Finalizando Servidor Escucha");

	pthread_exit(EXIT_SUCCESS);
}

void join_threads(){

	shutdown(server_socket,0);

	pthread_join(socket_thread, NULL);
	pthread_join(long_term_scheduler, NULL);

	t_program *cliente;
	for(int i = 0; i < list_size(clientes); i++){
		cliente = list_get(clientes, i);
		pthread_join(cliente->short_scheduler, NULL);
	}
}

void end_suse(){

	join_threads();

	destroy_lists();
	destroy_mutexes();
	destroy_configData();
	log_destroy(logger);
}

bool has_property(char* property){

	if(!config_has_property(config, property)){
		log_error(logger, "No se encuentra %s en el archivo de configuracion.", property);
		log_destroy(logger);
		config_destroy(config);
		return false;
	}

	return true;
}

t_list* _get_semaforos_from_config(){

	t_list* semaforos = list_create();

	char** array_ids = config_get_array_value(config, "SEM_IDS");
	char** array_init = config_get_array_value(config, "SEM_INIT");
	char** array_max = config_get_array_value(config, "SEM_MAX");

	int i = 0;
	while (array_ids[i] != NULL) {

		t_config_semaforo* semaforo = malloc(sizeof(t_config_semaforo));
		if(semaforo == NULL) return NULL;

		semaforo->id = array_ids[i];
		semaforo->init = atoi(array_init[i]);
		semaforo->max = atoi(array_max[i]);

		list_add(semaforos, semaforo);

		i++;
	}

	return semaforos;
}

void init_console(){

	char* linea;
	while (!endsuse){

		linea = readline("");
		if(!linea) continue;

		if(!strncasecmp(linea, "clear", 5)){
			add_history(linea);
			free(linea);
			system("clear");
			continue;
		}

		if(!strncasecmp(linea, "salir", 5)){
			free(linea);
			endsuse = true;
			continue;
		}

		add_history(linea);

		free(linea);
	}
}

void destroy_configData(){

	free(configData->listen_port);
	list_destroy_and_destroy_elements(configData->semaforos, (void*) destroy_config_semaforo);
	free(configData);
}

void destroy_config_semaforo(t_config_semaforo *config_semaforo){

	free(config_semaforo->id);
	free(config_semaforo);
}
