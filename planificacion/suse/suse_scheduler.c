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
			if(current->tid == 0) main_exec_signal(current);
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
	int tid = -2;
	char* sem = string_new();

	log_debug(logger, "[ProgramID: %d] Iniciando Planificador de Corto Plazo", self->id);

	while(!close){

		int codigo = recibir_operacion(self->socket);

		switch (codigo) {
			case SUSE_CREATE:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_CREATE", self->id, tid);
				if(tid == 0){
					t_tcb *tcb = create_tcb(self->socket, tid);
					pthread_mutex_lock(&self->main_loaded_mutex);
					move_tcb_to(tcb, EXEC);
					pthread_mutex_unlock(&self->main_loaded_mutex);
					pthread_mutex_destroy(&self->main_loaded_mutex);
				}else create_tcb(self->socket, tid);
				break;
			case SUSE_SCHEDULE_NEXT:
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_SCHEDULE_NEXT", self->id);
				schedule_next_for(self);
				break;
			case SUSE_WAIT:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				sem = (char*)list_get(parametros, 1);
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_WAIT tid: %d , sem: %s", self->id, tid, sem);
				suse_wait(atoi(list_get(parametros, 0)), sem, self);
				break;
			case SUSE_SIGNAL:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				sem = (char*)list_get(parametros, 1);
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_SIGNAL tid: %d , sem: %s", self->id, tid, sem);
				suse_signal(tid, sem, self);
				break;
			case SUSE_JOIN:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_JOIN tid: %d", self->id, tid);
				suse_join(tid, self);
				break;
			case SUSE_CLOSE:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_CLOSE tid: %d", self->id, tid);
				suse_join(tid, self);
				break;
			case -1:
				log_debug(logger, "[ProgramID: %d] El cliente se desconecto.", self->id);
				close = true;
				break;
			default:
				log_debug(logger, "[ProgramID: %d] Operacion desconocida - op_code: %d", self->id, codigo);
				break;
		}
		pthread_mutex_lock(&state_list_mutex);
		if((list_size(self->ready) == 0) && endsuse) close = true;
		pthread_mutex_unlock(&state_list_mutex);
	}

	log_debug(logger, "[FINALIZADO] Planificador de Corto Plazo: program_id = %d", self->id);

	pthread_exit(EXIT_SUCCESS);
}

// Planifica el siguiente hilo con SJF
void schedule_next_for(t_program *program){

	start:
	pthread_mutex_lock(&state_list_mutex);

	if(list_size(program->ready) == 0){
		if(program->exec == NULL){
			wait_for_wakeup(program);
			goto start;
		}
		else{
			set_new_timings(program->exec->time);
			pthread_mutex_unlock(&state_list_mutex);
			goto notify;
		}
	}

	float best_estimate;

	if(program->exec == NULL) best_estimate = FLT_MAX;
	else best_estimate = get_estimate(program->exec->time);

	float this_estimate = -1;
	int best_pos = -1;
	t_tcb *tcb = NULL;

	for(int i = 0; i < list_size(program->ready); i++){

		tcb = list_get(program->ready, i);
		this_estimate = get_estimate(tcb->time);

		if(this_estimate < best_estimate){
			best_estimate = this_estimate;
			best_pos = i;
		}
	}

	if(best_pos != -1) tcb = list_get(program->ready, best_pos);

	pthread_mutex_unlock(&state_list_mutex);

	if(best_pos != -1) change_exec_tcb(program, tcb, best_estimate);
	else set_new_timings(program->exec->time);

	notify:
	notify_program(program, SUSE_SCHEDULE_NEXT);
}

void wait_for_wakeup(t_program *program){

	pthread_mutex_unlock(&state_list_mutex);
	log_debug(logger, "[ProgramID: %d] No hay hilos para ejecutar", program->id);
	sem_wait(&program->tcb_counter);
	sem_post(&program->tcb_counter);
	log_debug(logger, "[ProgramID: %d] Hilos para ejecutar encontrados", program->id);
}

void suse_wait(int tid, char* sem_name, t_program *program){

	if(program->exec->tid != tid){
		print_suse_not_exec(program->id, tid);
		return;
	}

	t_semaforo *semaforo = get_semaforo(sem_name);

	pthread_mutex_lock(&semaforo->valor_mutex);
	semaforo->valor--;
	if(semaforo->valor < 0){
		pthread_mutex_unlock(&semaforo->valor_mutex);
		block(semaforo, program->exec);
		return;
	}
	pthread_mutex_unlock(&semaforo->valor_mutex);
}

void suse_signal(int tid, char* sem_name, t_program *program){

	if(program->exec->tid != tid){
		print_suse_not_exec(program->id, tid);
		return;
	}

	t_semaforo *semaforo = get_semaforo(sem_name);

	pthread_mutex_lock(&semaforo->valor_mutex);
	semaforo->valor++;
	if(semaforo->valor <= 0){
		pthread_mutex_unlock(&semaforo->valor_mutex);
		wakeup(semaforo);
		return;
	}
	pthread_mutex_unlock(&semaforo->valor_mutex);
}

void block(t_semaforo *semaforo, t_tcb *exec){

	pthread_mutex_lock(&semaforo->block_mutex);
	list_add(semaforo->blocked, exec);
	pthread_mutex_unlock(&semaforo->block_mutex);

	move_tcb_to(exec, BLOCKED);
}

void wakeup(t_semaforo *semaforo){

	pthread_mutex_lock(&semaforo->block_mutex);
	t_tcb *tcb = list_get(semaforo->blocked, 0);
	list_remove(semaforo->blocked, 0);
	pthread_mutex_unlock(&semaforo->block_mutex);

	move_tcb_to(tcb, READY);
}

void suse_join(int tid, t_program *program){

	t_tcb *tcb;

	pthread_mutex_lock(&state_list_mutex);

	int pos = find_tcb_pos(program->ready, tid, program->socket);
	tcb = list_get(program->ready, pos);

	list_add(tcb->joined, program->exec);

	pthread_mutex_unlock(&state_list_mutex);

	move_tcb_to(program->exec, BLOCKED);
}

void suse_close(int tid, t_program *program){

	if(program->exec->tid != tid){
		print_suse_not_exec(program->id, tid);
		return;
	}

	set_new_timings(program->exec->time);

	unjoin_exec(program);

	move_tcb_to(program->exec, EXIT);

	log_metrics();
}

void unjoin_exec(t_program *program){

	t_tcb *tcb;
	log_debug(logger, "unjoin_exec -> exec tid: %d", program->exec->tid);
	log_debug(logger, "unjoin_exec -> list_size: %d", list_size(program->exec->joined));
	for(int i = 0; i < list_size(program->exec->joined); i++){
		tcb = list_get(program->exec->joined, i);
		move_tcb_to(tcb, READY);
	}
}

void change_exec_tcb(t_program *program, t_tcb *shortest_tcb, int shortest_estimate){

	if(program->exec != NULL){

		set_new_timings(program->exec->time);
		move_tcb_to(program->exec, READY);
	}

	pthread_mutex_lock(&state_list_mutex);

	struct timeval temp;
	gettimeofday(&temp, NULL);

	shortest_tcb->time->exec_start = temp;
	shortest_tcb->time->last_estimate = shortest_estimate;

	pthread_mutex_unlock(&state_list_mutex);

	move_tcb_to(shortest_tcb, EXEC);

}

void notify_program(t_program *program, op_code codigo){

	switch(codigo){
		case SUSE_SCHEDULE_NEXT:{
			char* threadid = string_itoa(program->exec->tid);
			t_paquete *reply = crear_paquete(SUSE_SCHEDULE_NEXT_RETURN);
			agregar_a_paquete(reply, threadid, sizeof(int));
			enviar_paquete(reply, program->socket);
			log_debug(logger, "[ProgramID: %d] SUSE_SCHEDULE_NEXT - Next tid: %s", program->id, threadid);
			free(threadid);	}
			break;
		default:
			break;
	}
}

void main_exec_signal(t_tcb *tcb){

	t_program *program = get_program(tcb->socket);

	pthread_mutex_unlock(&program->main_loaded_mutex);
}

int get_estimate(t_burst *burst){

	int alpha = configData->alpha_sjf;
	return (alpha * burst->last_burst) + ((1-alpha) * burst->last_estimate);
}

void set_new_timings(t_burst *timings){
	log_debug(logger, "set_new_timings -> total_exec: %d", timings->total_exec);
	// Obtener el tiempo actual
	gettimeofday(&timings->exec_end, 0);

	// Calcular la diferencia entre el exec_start y exec_end en milisegundos
	timings->last_burst = (timings->exec_end.tv_sec - timings->exec_start.tv_sec) * 1000.0f + (timings->exec_end.tv_usec - timings->exec_start.tv_usec) / 1000.0f;

	// Sumar el last_burst al total
	timings->total_exec += timings->last_burst;
}

t_tcb *get_new_tcb(){

	pthread_mutex_lock(&state_list_mutex);
	t_tcb *tcb = list_get(newList, 0);
	pthread_mutex_unlock(&state_list_mutex);

	return tcb;
}

t_semaforo *get_semaforo(char* sem_name){

	t_semaforo *semaforo = NULL;
	for(int i = 0; i < list_size(semaforos); i++){
		semaforo = list_get(semaforos, i);
		if(strcmp(semaforo->id, sem_name) == 0) return semaforo;
	}

	return NULL;
}

void move_tcb_to(t_tcb *tcb, int state){

	pthread_mutex_lock(&state_list_mutex);

	if(tcb->state == state){
		pthread_mutex_unlock(&state_list_mutex);
		return;
	}

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

int find_tcb_pos(t_list *list, int tid, int socket){

	t_tcb *tcb = NULL;
	for (int i = 0; i < list_size(list); i++) {
		 tcb = list_get(list, i);
		if(tcb->tid == tid && tcb->socket == socket) return i;
	}

	return ELEMENT_NOT_FOUND;
}

int find_program_pos(int socket){

	t_program *program = NULL;

	pthread_mutex_lock(&clientes_mutex);
	for(int i = 0; i < list_size(clientes); i++){
		program = list_get(clientes, i);
		if(program->socket == socket) {
			pthread_mutex_unlock(&clientes_mutex);
			return i;
		}
	}
	pthread_mutex_unlock(&clientes_mutex);

	return ELEMENT_NOT_FOUND;
}

bool is_program_loaded(int socket){

	return find_program_pos(socket) != ELEMENT_NOT_FOUND;
}

t_program *get_program(int socket){

	int pos = find_program_pos(socket);

	pthread_mutex_lock(&clientes_mutex);
	t_program *program = list_get(clientes, pos);
	pthread_mutex_unlock(&clientes_mutex);

	return pos == ELEMENT_NOT_FOUND ? NULL : program;
}

int get_program_id(int socket){

	t_program *program = NULL;

	pthread_mutex_lock(&clientes_mutex);
	for(int i = 0; i < list_size(clientes); i++){
		program = list_get(clientes, i);
		if(program->socket == socket) {
			pthread_mutex_unlock(&clientes_mutex);
			return program->id;
		}
	}
	pthread_mutex_unlock(&clientes_mutex);

	return ELEMENT_NOT_FOUND;
}

// 	Solo READY y EXEC
void program_remove_tcb_from_state(int socket, int tid, int state){

	t_program *program = get_program(socket);

	if(state == READY){
		int tcb_pos = find_tcb_pos(program->ready, tid, socket);
		list_remove(program->ready, tcb_pos);
		sem_wait(&program->tcb_counter);
	}
	else if(state == EXEC){
		program->exec = NULL;
		sem_wait(&program->tcb_counter);
	}
	else return;
}

// 	Solo READY y EXEC
void program_add_tcb_to_state(t_tcb *tcb, int state){

	t_program *program = get_program(tcb->socket);

	if(state == READY){
		list_add(program->ready, tcb);
		sem_post(&program->tcb_counter);
	}
	else if(state == EXEC){
		program->exec = tcb;
		sem_post(&program->tcb_counter);
	}
	else return;
}

void log_metrics(){

	pthread_mutex_lock(&metrics_mutex);

	t_program *program;

	pthread_mutex_lock(&state_list_mutex);
	log_info(metricsLog, "Metricas por Hilo");
	pthread_mutex_lock(&clientes_mutex);
	for(int i = 0; i < list_size(clientes); i++){
		program = list_get(clientes, i);
		if(program->exec != NULL){
			log_info(metricsLog, "- Programa: %d - Hilo: %d", program->id, program->exec->tid);
			log_info(metricsLog, "- - Tiempo en Espera: %d ms", program->exec->time->total_ready);
			log_info(metricsLog, "- - Tiempo de uso de CPU: %d ms", program->exec->time->total_ready);
			log_info(metricsLog, "- - Porcentaje del Tiempo de Ejecucion: %d", get_exec_percentage(program));
		}
	}

	log_info(metricsLog, "Metricas por Programa");
	for(int i = 0; i < list_size(clientes); i++){
		program = list_get(clientes, i);
		log_info(metricsLog, "- Programa: %d", program->id);
		log_info(metricsLog, "- - Hilos en NEW: %d", get_new_list_size_for(program));
		log_info(metricsLog, "- - Hilos en READY: %d", list_size(program->ready));
		log_info(metricsLog, "- - Hilos en RUN: %d", program->exec != NULL ? 1 : 0);
		log_info(metricsLog, "- - Hilos en BLOCKED: %d", get_blocked_list_size_for(program));
	}
	pthread_mutex_unlock(&clientes_mutex);
	pthread_mutex_unlock(&state_list_mutex);;

	t_semaforo *semaforo;
	log_info(metricsLog, "Metricas del Sistema");
	for(int i = 0; i < list_size(semaforos); i++){
		semaforo = list_get(semaforos, i);
		pthread_mutex_lock(&semaforo->valor_mutex);
		log_info(metricsLog, "- Semaforo - id: %s - valor: %d", semaforo->id, semaforo->valor);
		pthread_mutex_unlock(&semaforo->valor_mutex);
	}
	log_info(metricsLog, "- Grado actual de multiprogramacion: %d", configData->max_multiprog);

	log_info(metricsLog, "------------------------------------------------------------");

	pthread_mutex_unlock(&metrics_mutex);
}

float get_exec_percentage(t_program *program){

	float suma = 0;
	t_tcb *tcb;
	for(int i = 0; i < list_size(program->ready); i++){
		tcb = list_get(program->ready, i);
		suma += tcb->time->total_exec;
	}

	return (program->exec->time->total_exec * 100.0f)/suma;
}

int get_new_list_size_for(t_program *program){

	int size = 0;
	t_tcb *tcb;
	for(int i = 0; i < list_size(newList); i++){
		tcb = list_get(newList, i);
		if(tcb->socket == program->socket) size+=1;
	}

	return size;
}

int get_blocked_list_size_for(t_program *program){

	int size = 0;
	t_tcb *tcb;
	for(int i = 0; i < list_size(blockedList); i++){
		tcb = list_get(blockedList, i);
		if(tcb->socket == program->socket) size+=1;
	}

	return size;
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
	pthread_mutex_init(&semaforo->block_mutex, NULL);
	pthread_mutex_init(&semaforo->valor_mutex, NULL);

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
	burst->total_exec = 0;
	burst->total_ready = 0;

	return burst;
}

t_tcb *create_tcb(int socket, int thread_id){

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
	tcb->joined = list_create();

	pthread_mutex_lock(&state_list_mutex);

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

	pthread_mutex_init(&programa->main_loaded_mutex, NULL);
	pthread_mutex_lock(&programa->main_loaded_mutex);
	sem_init(&programa->tcb_counter, 0, 0);

	pthread_mutex_lock(&clientes_mutex);
	list_add(clientes, programa);
	pthread_mutex_unlock(&clientes_mutex);

	pthread_create(&programa->short_scheduler, NULL, schedule_short_term, (void*)(&programa->socket));
	if(&programa->short_scheduler == NULL){
		print_pthread_create_error("schedule_short_term");
		goto destroy;
	}

	log_debug(logger,"Nuevo Programa - id = %d - socket: %d", programa->id, programa->socket);

	return programa;

	destroy:
	destroy_program(programa);
	return NULL;
}

/*               DESTROY               */

void destroy_semaforo(t_semaforo *semaforo){

	list_destroy_and_destroy_elements(semaforo->blocked,(void*) destroy_tcb);

	pthread_mutex_destroy(&semaforo->block_mutex);
	pthread_mutex_destroy(&semaforo->valor_mutex);

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

	int id = tcb->tid;

	if(tcb->time != NULL) destroy_burst(tcb->time);
	list_destroy(tcb->joined);

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
	pthread_mutex_destroy(&metrics_mutex);
	pthread_mutex_destroy(&clientes_mutex);
}
