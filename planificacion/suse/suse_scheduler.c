#include "suse_scheduler.h"

void init_scheduler(){

	log_debug(logger, "[SUSE] Iniciando Planificador de Largo Plazo..");

	// Variables Globales
	multiprog_degree = 0;

	// Listas de Estado
	newList = list_create();
	blockedList = list_create();
	exitQueue = queue_create();

	// Semaforos
	pthread_mutex_init(&state_list_mutex, NULL);

	// Scheduler
	if (pthread_create (&long_term_scheduler, NULL, schedule_long_term, NULL) != 0){
		print_pthread_create_error("schedule_long_term");
		destroy_scheduler(&long_term_scheduler);
		exit(-1);
	}

	log_debug(logger, "[SUSE] Planificador de Largo Plazo Iniciado.");
}

void init_semaforos(){

	log_debug(logger,"[SUSE] Iniciando Semaforos..");

	semaforos = list_create();

	for(int i = 0; i < list_size(configData->semaforos); i++ ){

		list_add(semaforos, create_semaforo(list_get(configData->semaforos, i)));
	}

	log_debug(logger,"[SUSE] Semaforos Iniciados.");
}

void *schedule_long_term(void *arg){

	t_tcb *current = NULL;
	while(true){
		if(current == NULL) current = get_new_tcb();
		if(current != NULL && multiprog_degree < configData->max_multiprog){
			move_tcb_to(current, READY);
			current = NULL;
		}else if((current == NULL) && endsuse) break;
	}

	log_debug(logger, "[SUSE] Finalizando Planificador de Largo Plazo");

	pthread_exit(EXIT_SUCCESS);
}

void *schedule_short_term(void *arg){

	bool close = false;
	t_program *self = get_program(*((int *)arg));
	t_list *parametros = list_create();

	log_debug(logger, "[ProgramID: %d] Iniciando Planificador de Largo Plazo", self->id);

	while(!close){

		int codigo = recibir_operacion(self->socket);

		switch (codigo) {
			case SUSE_CREATE:
				log_debug(logger, "[ProgramID: %d] Operacion Recibida: SUSE_CREATE", self->id);
				parametros = recibir_paquete(self->socket);
				create_tcb(self->socket, (int)list_get(parametros, 0));
				break;
			case SUSE_SCHEDULE_NEXT:
				log_debug(logger, "[ProgramID: %d] Operacion Recibida: SUSE_SCHEDULE_NEXT", self->id);
				schedule_next_for(self);
				break;
			case SUSE_WAIT:
				log_debug(logger, "[ProgramID: %d] Operacion Recibida: SUSE_WAIT", self->id);
				parametros = recibir_paquete(self->socket);
				break;
			case SUSE_SIGNAL:
				log_debug(logger, "[ProgramID: %d] Operacion Recibida: SUSE_SIGNAL", self->id);
				parametros = recibir_paquete(self->socket);
				break;
			case SUSE_JOIN:
				log_debug(logger, "[ProgramID: %d] Operacion Recibida: SUSE_JOIN", self->id);
				parametros = recibir_paquete(self->socket);
				break;
			case SUSE_CLOSE:
				log_debug(logger, "[ProgramID: %d] Operacion Recibida: SUSE_CLOSE", self->id);
				parametros = recibir_paquete(self->socket);
				break;
			case -1:
				log_debug(logger, "[ProgramID: %d] El cliente se desconecto.", self->id);
				close = true;
				break;
			default:
				log_debug(logger, "[ProgramID: %d] Operacion desconocida - op_code: %d", self->id, codigo);
				break;
		}

		if((list_size(self->ready) == 0) && endsuse) close = true;
	}

	log_debug(logger, "[FINALIZADO] Planificador de Largo Plazo: program_id = %d", self->id);

	pthread_exit(EXIT_SUCCESS);
}

void schedule_next_for(t_program *program){

	float this_estimate = -1;
	float best_estimate = 9999;
	int best_pos = -1;
	t_tcb *tcb = NULL;

	pthread_mutex_lock(&program->ready_mutex);

	for(int i = 0; i < list_size(program->ready); i++){

		tcb = list_get(program->ready, i);
		this_estimate = get_estimate(tcb->time);

		if(this_estimate < best_estimate){
			best_estimate = this_estimate;
			best_pos = i;
		}
	}

	tcb = list_get(program->ready, best_pos);

	pthread_mutex_unlock(&program->ready_mutex);

	change_exec_tcb(program, tcb, best_estimate);

	notify_program(program, SUSE_SCHEDULE_NEXT);

}

void change_exec_tcb(t_program *program, t_tcb *shortest_tcb, int shortest_estimate){

	if(program->exec != NULL){

		set_new_timings(program->exec->time);
		move_tcb_to(program->exec, READY);
	}

	gettimeofday(&shortest_tcb->time->exec_start, 0);
	shortest_tcb->time->last_estimate = shortest_estimate;
	move_tcb_to(shortest_tcb, EXEC);

}

void notify_program(t_program *program, op_code codigo){

	switch(codigo){
		case SUSE_SCHEDULE_NEXT:{
			t_paquete *reply = crear_paquete(SUSE_SCHEDULE_NEXT_RETURN);
			agregar_a_paquete(reply, &(program->exec->tid), sizeof(int));
			enviar_paquete(reply, program->socket);}
			break;
		default:
			break;
	}
}

int get_estimate(t_burst *burst){

	int alpha = configData->alpha_sjf;
	return (alpha * burst->last_burst) + ((1-alpha) * burst->last_estimate);
}

void set_new_timings(t_burst *timings){

	// Obtener el tiempo actual
	gettimeofday(&timings->exec_end, 0);

	// Calcular la diferencia entre el exec_start y exec_end en milisegundos
	timings->last_burst = (timings->exec_end.tv_sec - timings->exec_start.tv_sec) * 1000.0f + (timings->exec_end.tv_usec - timings->exec_start.tv_usec) / 1000.0f;

	// Sumar el last_burst al total
	timings->total += timings->last_burst;
}

t_tcb *get_new_tcb(){

	return list_get(newList, 0);
}

void move_tcb_to(t_tcb *tcb, int state){

	if(tcb->state == state){
		return;
	}

	pthread_mutex_lock(&state_list_mutex);

	log_debug(logger, "Moviendo TCB: [tid: %d - ProgramID: %d] del Estado: [%d] al Estado: [%d] - [NEW = %d, READY = %d, EXEC = %d, BLOCKED = %d, EXIT = %d]", tcb->tid, get_program_id(tcb->socket), tcb->state, state, NEW, READY, EXEC, BLOCKED, EXIT);

	int pos;
	switch (tcb->state) {
		case NEW:
			pos = find_tcb_pos(newList, tcb->tid, tcb->socket);
			list_remove(newList, pos);
			break;
		case READY:
			program_remove_tcb_from_state(tcb->socket, tcb->tid, tcb->state);
			multiprog_degree--;
			break;
		case EXEC:
			program_remove_tcb_from_state(tcb->socket, tcb->tid, tcb->state);
			multiprog_degree--;
			break;
		case BLOCKED:
			pos = find_tcb_pos(blockedList, tcb->tid, tcb->socket);
			list_remove(blockedList, pos);
			multiprog_degree--;
			break;
		case EXIT:
			pos = find_tcb_pos(exitQueue->elements, tcb->tid, tcb->socket);
			list_remove(exitQueue->elements, pos);
			break;
	}

	switch (state) {
		case NEW:
			list_add(newList, tcb);
			break;
		case READY:
			program_add_tcb_to_state(tcb, READY);
			multiprog_degree++;
			break;
		case EXEC:
			program_add_tcb_to_state(tcb, EXEC);
			multiprog_degree++;
			break;
		case BLOCKED:
			list_add(blockedList, tcb);
			multiprog_degree++;
			break;
		case EXIT:
			queue_push(exitQueue, tcb);
			break;
	}

	tcb->state = state;

	pthread_mutex_unlock(&state_list_mutex);
}

int find_tcb_pos(t_list *list, pid_t tid, int socket){

	t_tcb *tcb = NULL;
	for (int i = 0; i < list_size(list); i++) {
		 tcb = list_get(list, i);
		if(tcb->tid == tid && tcb->socket == socket) return i;
	}

	return ELEMENT_NOT_FOUND;
}

int find_program_pos(int socket){

	t_program *program = NULL;
	for(int i = 0; i < list_size(clientes); i++){
		program = list_get(clientes, i);
		if(program->socket == socket) return i;
	}

	return ELEMENT_NOT_FOUND;
}

bool is_program_loaded(int socket){

	return find_program_pos(socket) != ELEMENT_NOT_FOUND;
}

t_program *get_program(int socket){

	int pos = find_program_pos(socket);

	return pos == ELEMENT_NOT_FOUND ? NULL : list_get(clientes, pos);
}

int get_program_id(int socket){

	t_program *program = NULL;
	for(int i = 0; i < list_size(clientes); i++){
		 program = list_get(clientes, i);
		if(program->socket == socket) return program->id;
	}

	return ELEMENT_NOT_FOUND;
}

// 	Solo READY y EXEC
void program_remove_tcb_from_state(int socket, pid_t tid, int state){

	t_program *program = get_program(socket);

	if(state == READY){
		pthread_mutex_lock(&program->ready_mutex);
		int tcb_pos = find_tcb_pos(program->ready, tid, socket);
		list_remove(program->ready, tcb_pos);
		pthread_mutex_unlock(&program->ready_mutex);
	}
	else if(state == EXEC) program->exec = NULL;
	else return;
}

// 	Solo READY y EXEC
void program_add_tcb_to_state(t_tcb *tcb, int state){

	t_program *program = get_program(tcb->socket);

	if(state == READY){
		pthread_mutex_lock(&program->ready_mutex);
		list_add(program->ready, tcb);
		pthread_mutex_unlock(&program->ready_mutex);
	}
	else if(state == EXEC) program->exec = tcb;
	else return;
}

/*               CREATE               */

t_semaforo *create_semaforo(t_config_semaforo *config_semaforo){

	t_semaforo *semaforo = malloc(sizeof(t_semaforo));
	if(semaforo == NULL){
		print_malloc_error("Semaforo");
		return NULL;
	}

	semaforo->id = strdup(config_semaforo->id);
	semaforo->valor = config_semaforo->init;
	semaforo->blocked = list_create();

	log_debug(logger,"Nuevo Semaforo - id = %s", semaforo->id);
	return semaforo;
}

t_burst *create_burst(){

	t_burst *burst = malloc(sizeof(t_burst));
	if(burst == NULL){
		print_malloc_error("t_burst");
		return NULL;
	}
	burst->last_burst = 0;
	burst->last_estimate = 0;
	burst->total = 0;

	return burst;
}

t_tcb *create_tcb(int socket, pid_t thread_id){

	t_tcb *tcb = malloc(sizeof(t_tcb));
	if(tcb == NULL){
		print_malloc_error("TCB");
		return NULL;
	}

	tcb->socket = socket;
	tcb->tid = thread_id;
	tcb->state = NEW;

	tcb->time = create_burst();
	if(tcb->time == NULL) goto destroy;

	pthread_mutex_unlock(&state_list_mutex);

	list_add(newList, tcb);
	log_debug(logger,"Nuevo TCB - tid: %d - ProgramID: %d", tcb->tid, get_program_id(socket));

	pthread_mutex_unlock(&state_list_mutex);

	return tcb;

	destroy:
	destroy_tcb(tcb);
	return NULL;
}

t_program *create_program(int client_socket){

	t_program *programa = malloc(sizeof(t_program));
	if(programa == NULL){
		print_malloc_error("Programa");
		return NULL;
	}

	programa->id = ++program_id_counter;
	programa->socket = client_socket;
	programa->ready = list_create();
	programa->exec = NULL;
	pthread_mutex_init(&programa->ready_mutex, NULL);

	list_add(clientes, programa);

	pthread_create(&programa->short_scheduler, NULL, schedule_short_term, socket);
	if(&programa->short_scheduler == NULL){
		print_pthread_create_error("schedule_short_term");
		goto destroy;
	}

	log_debug(logger,"Nuevo Programa - id = %d", programa->id);

	return programa;

	destroy:
	destroy_program(programa);
	return NULL;
}

/*               DESTROY               */

void destroy_semaforo(t_semaforo *semaforo){

	list_destroy_and_destroy_elements(semaforo->blocked,(void*) destroy_tcb);

	char* id = strdup(semaforo->id);
	free(semaforo->id);

	free(semaforo);

	log_debug(logger,"Semaforo Eliminado - id = %s", id);
	free(id);
}

void destroy_burst(t_burst *burst){

	free(burst);
}

void destroy_tcb(t_tcb *tcb){

	pid_t id = tcb->tid;

	if(tcb->time != NULL) destroy_burst(tcb->time);

	free(tcb);

	log_debug(logger,"TCB Eliminado - tid = %d", id);
}

void destroy_program(t_program *program){

	if(!list_is_empty(program->ready)) return;
	if(program->exec != NULL) return;

	int id = program->id;

	list_remove(clientes, find_program_pos(program->socket));

	list_destroy_and_destroy_elements(program->ready, (void*) destroy_tcb);
	destroy_scheduler(&program->short_scheduler);

	close(program->socket);
	free(program);

	log_debug(logger,"Programa Eliminado - id = %d", id);
}

void destroy_scheduler(pthread_t *scheduler){

	if(scheduler != NULL) free(scheduler);
}

void destroy_lists(){

	pthread_mutex_lock(&state_list_mutex);

	list_destroy_and_destroy_elements(newList,(void*) destroy_tcb);
	list_destroy_and_destroy_elements(blockedList,(void*) destroy_tcb);
	queue_destroy_and_destroy_elements(exitQueue,(void*) destroy_tcb);

	list_destroy_and_destroy_elements(clientes, (void*) destroy_program);
	list_destroy_and_destroy_elements(semaforos, (void*) destroy_semaforo);

	pthread_mutex_unlock(&state_list_mutex);
}

void destroy_mutexes(){

	pthread_mutex_destroy(&state_list_mutex);
}
