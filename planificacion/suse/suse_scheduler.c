#include "suse_scheduler.h"

void init_scheduler(){

	log_debug(logger, "Iniciando Planificador..");

	// Variables Globales
	multiprog_degree = 0;

	// Listas de Estado
	newList = list_create();
	blockedList = list_create();
	exitQueue = queue_create();

	// Semaforos
	pthread_mutex_init(&state_list_mutex, NULL);

	// Scheduler
	long_term_scheduler = malloc(sizeof(pthread_t));
	if(long_term_scheduler == NULL) exit(-1);

	if (pthread_create (long_term_scheduler, NULL, schedule_long_term, NULL) != 0){
		print_pthread_create_error("schedule_long_term");
		destroy_scheduler(long_term_scheduler);
		exit(-1);
	}

	log_debug(logger, "Planificador Iniciado.");
}

void init_semaforos(){

	log_debug(logger,"Iniciando Semaforos..");

	semaforos = list_create();

	for(int i = 0; i < list_size(configData->semaforos); i++ ){

		list_add(semaforos, create_semaforo(list_get(configData->semaforos, i)));
	}

	log_debug(logger,"Semaforos Iniciados.");
}

void *schedule_long_term(void *arg){

	log_debug(logger, "[OK] Planificador de Largo Plazo");

	t_tcb *current = NULL;
	while(true){
		if(current == NULL) current = get_new_tcb();
		if(current != NULL && multiprog_degree < configData->max_multiprog){
			move_tcb_to(current, READY);
			current = NULL;
		}else if((current == NULL) && endsuse) break;
	}

	log_debug(logger, "Finalizando Planificador de Largo Plazo");

	pthread_exit(EXIT_SUCCESS);
}

void schedule_next_for(int socket){


}

t_tcb *get_new_tcb(){

	return list_get(newList, 0);
}

void move_tcb_to(t_tcb *tcb, int state){

	if(tcb->state == state){
		return;
	}

	pthread_mutex_lock(&state_list_mutex);

	log_debug(logger, "Mover TCB: tid [%d] de [%d] a [%d] - [NEW = %d, READY = %d, EXEC = %d, BLOCKED = %d, EXIT = %d]", tcb->tid, tcb->state, state, NEW, READY, EXEC, BLOCKED, EXIT);

	int pos;
	switch (tcb->state) {
		case NEW:
			pos = find_tcb_pos(newList, tcb->tid, tcb->pid);
			list_remove(newList, pos);
			break;
		case READY:
			program_remove_tcb_from_state(tcb->pid, tcb->tid, tcb->state);
			multiprog_degree--;
			break;
		case EXEC:
			program_remove_tcb_from_state(tcb->pid, tcb->tid, tcb->state);
			multiprog_degree--;
			break;
		case BLOCKED:
			pos = find_tcb_pos(blockedList, tcb->tid, tcb->pid);
			list_remove(blockedList, pos);
			multiprog_degree--;
			break;
		case EXIT:
			pos = find_tcb_pos(exitQueue->elements, tcb->tid, tcb->pid);
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

int find_tcb_pos(t_list *list, pid_t tid, int pid){

	for (int i = 0; i < list_size(list); i++) {
		t_tcb *tcb = list_get(list, i);
		if(tcb->tid == tid && tcb->pid == pid) return i;
	}

	return ELEMENT_NOT_FOUND;
}

int find_program_pos(int pid){

	for(int i = 0; i < list_size(clientes); i++){
		t_program *program = list_get(clientes, i);
		if(program->id == pid) return i;
	}

	return ELEMENT_NOT_FOUND;
}

int find_program_pos_by_socket(int socket){

	for(int i = 0; i < list_size(clientes); i++){
		t_program *program = list_get(clientes, i);
		if(program->socket == socket) return i;
	}

	return ELEMENT_NOT_FOUND;
}

int find_program_id(int socket){

	for(int i = 0; i < list_size(clientes); i++){
		t_program *program = list_get(clientes, i);
		if(program->socket == socket) return program->id;
	}

	return ELEMENT_NOT_FOUND;
}

bool is_program_loaded(int socket){

	return find_program_pos_by_socket(socket) != ELEMENT_NOT_FOUND;
}

t_program *get_program(int(*find_by)(int), int unint){

	int pos = find_by(unint);

	return pos == ELEMENT_NOT_FOUND ? NULL : list_get(clientes, pos);
}

// 	Solo READY y EXEC
void program_remove_tcb_from_state(int pid, pid_t tid, int state){

	t_program *program = get_program(find_program_pos, pid);

	if(state == READY){
		int tcb_pos = find_tcb_pos(program->ready, tid, pid);
		list_remove(program->ready, tcb_pos);
	}
	else if(state == EXEC) program->exec = NULL;
	else return;
}

// 	Solo READY y EXEC
void program_add_tcb_to_state(t_tcb *tcb, int state){

	t_program *program = get_program(find_program_pos, tcb->pid);

	if(state == READY) list_add(program->ready, tcb);
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
	burst->total = 0;

	return burst;
}

t_pcb *create_pcb(pid_t process_id, char* process_name){

	t_pcb *pcb = malloc(sizeof(t_pcb));
	if(pcb == NULL){
		print_malloc_error("t_pcb");
		return NULL;
	}
	pcb->pid = process_id;
	pcb->name = strdup(process_name);

	return pcb;
}

t_tcb *create_tcb(int socket, pid_t thread_id, pid_t process_id, char* process_name){

	t_tcb *tcb = malloc(sizeof(t_tcb));
	if(tcb == NULL){
		print_malloc_error("TCB");
		return NULL;
	}

	tcb->tid = thread_id;
	tcb->state = NEW;

	tcb->pcb = create_pcb(process_id, process_name);
	if(tcb->pcb == NULL) goto destroy;

	tcb->time = create_burst();
	if(tcb->time == NULL) goto destroy;

	tcb->pid = find_program_id(socket);
	if(tcb->pid == ELEMENT_NOT_FOUND) goto destroy;

	move_tcb_to(tcb, NEW);

	log_debug(logger,"Nuevo TCB - tid: %d - pid: %d", tcb->tid, tcb->pid);
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
	programa->scheduler = create_short_scheduler();

	list_add(clientes, programa);

	log_debug(logger,"Nuevo Programa - id = %d", programa->id);
	return programa;
}

t_short_scheduler *create_short_scheduler(){

	t_short_scheduler *scheduler = malloc(sizeof(t_short_scheduler));
	if (scheduler == NULL){
	   print_malloc_error("Short_Scheduler");
	   return NULL;
	}

	return scheduler;
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

void destroy_pcb(t_pcb *pcb){

	free(pcb->name);
	free(pcb);
}

void destroy_tcb(t_tcb *tcb){

	pid_t id = tcb->tid;

	if(tcb->pcb != NULL) destroy_pcb(tcb->pcb);
	if(tcb->time != NULL) destroy_burst(tcb->time);

	free(tcb);

	log_debug(logger,"TCB Eliminado - tid = %d", id);
}

void destroy_program(t_program *program){

	if(!list_is_empty(program->ready)) return;
	if(program->exec != NULL) return;

	int id = program->id;

	close(program->socket);
	list_destroy_and_destroy_elements(program->ready, (void*) destroy_tcb);

	destroy_scheduler(&program->scheduler->short_term_scheduler);
	free(program);

	log_debug(logger,"Programa Eliminado - tid = %d", id);
}

void destroy_scheduler(pthread_t *scheduler){

	free(scheduler);
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
