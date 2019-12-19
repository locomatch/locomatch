#include "suse_scheduler.h"

void init_scheduler(){

	log_debug(logger, "[SUSE] Iniciando Planificador de Largo Plazo..");

	// Variables Globales
	multiprog_degree = 0;

	// Listas de Estado
	newList = list_create();
	blockedList = list_create();
	exitList = list_create();

	// Semaforos
	pthread_mutex_init(&state_list_mutex, NULL);
	sem_init(&new_counter, 0, 0);
	sem_init(&available_multiprog, 0, configData->max_multiprog);

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
	while(!endsuse){

		sem_wait(&new_counter);
		if(endsuse) break;
		current = get_new_tcb();
		check_multiprog();
		if(current != NULL){
			move_tcb_to(current, READY);
			if(current->tid == 0) main_exec_signal(current);
		}
		current = NULL;
	}

	log_debug(logger, "[SUSE] Finalizando Planificador de Largo Plazo");

	pthread_exit(EXIT_SUCCESS);
}

void *schedule_short_term(void *arg){

	t_program *self = get_program(*((int *)arg));
	t_list *parametros;
	int tid = -2;
	char* sem;

	log_debug(logger, "[ProgramID: %d] Iniciando Planificador de Corto Plazo", self->id);

	while(!self->close){

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
				list_destroy_and_destroy_elements(parametros, (void*)free);
				break;
			case SUSE_SCHEDULE_NEXT:
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_SCHEDULE_NEXT", self->id);
				schedule_next_for(self);
				break;
			case SUSE_WAIT:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				sem = strdup(list_get(parametros, 1));
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_WAIT tid: %d , sem: %s", self->id, tid, sem);
				suse_wait(atoi(list_get(parametros, 0)), sem, self);
				free(sem);
				list_destroy_and_destroy_elements(parametros, (void*)free);
				break;
			case SUSE_SIGNAL:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				sem = strdup(list_get(parametros, 1));
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_SIGNAL tid: %d , sem: %s", self->id, tid, sem);
				suse_signal(tid, sem, self);
				free(sem);
				list_destroy_and_destroy_elements(parametros, (void*)free);
				break;
			case SUSE_JOIN:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_JOIN tid: %d", self->id, tid);
				suse_join(tid, self);
				list_destroy_and_destroy_elements(parametros, (void*)free);
				break;
			case SUSE_CLOSE:
				parametros = recibir_paquete(self->socket);
				tid = atoi(list_get(parametros, 0));
				log_debug(logger, "[ProgramID: %d] Operacion Recibida - SUSE_CLOSE tid: %d", self->id, tid);
				suse_close(tid, self);
				list_destroy_and_destroy_elements(parametros, (void*)free);
				break;
			case -1:
				log_debug(logger, "[ProgramID: %d] El cliente se desconecto.", self->id);
				self->close = true;
				break;
			default:
				log_debug(logger, "[ProgramID: %d] Operacion desconocida - op_code: %d", self->id, codigo);
				break;
		}

		wakeup_threads(self);

		pthread_mutex_lock(&state_list_mutex);
		if((list_size(self->ready) == 0) && endsuse) self->close = true;
		pthread_mutex_unlock(&state_list_mutex);
	}

	log_debug(logger, "[FINALIZADO] Planificador de Corto Plazo: program_id = %d", self->id);

	//if(sem != NULL) free(sem);
	//if(parametros != NULL) list_destroy_and_destroy_elements(parametros, (void*)free);

	pthread_exit(EXIT_SUCCESS);
}

// Planifica el siguiente hilo con SJF utilizando estimacion
void schedule_next_for(t_program *program){

	start:
	pthread_mutex_lock(&state_list_mutex);

	if(list_size(program->ready) == 0){
		if(program->exec == NULL){
			pthread_mutex_unlock(&state_list_mutex);
			if(wakeup_threads(program)) goto start;
			wait_for_threads(program);
			goto start;
		}
	}

	float best_estimate = 0;

	if(program->exec == NULL) best_estimate = FLT_MAX;
	else best_estimate = get_estimate_with_burst(program->exec->time, get_last_burst_for(EXEC, program->exec->time));

	if(program->exec == NULL) log_debug(logger, "[ProgramID: %d] No hay Exec", program->id); // TODO BORRAR
	else log_debug(logger, "[ProgramID: %d] Exec tid: %d Estimate: %10.0f", program->id, program->exec->tid, best_estimate); //

	float this_estimate = -1;
	int best_pos = -1;
	t_tcb *tcb = NULL;

	for(int i = 0; i < list_size(program->ready); i++){

		tcb = list_get(program->ready, i);
		this_estimate = get_estimate(tcb->time);
		log_debug(logger, "[ProgramID: %d] Ready tid:%d Estimate: %10.0f", program->id, tcb->tid, this_estimate); //
		if(this_estimate < best_estimate){
			best_estimate = this_estimate;
			best_pos = i;
		}
	}

	if(best_pos != -1) tcb = list_get(program->ready, best_pos);
	else{
		tcb = program->exec;
		set_new_timings_for(EXEC, program->exec->time);
		set_state_start_timing(EXEC, program->exec->time);
	}

	tcb->time->last_estimate = best_estimate;

	pthread_mutex_unlock(&state_list_mutex);

	if(best_pos != -1) change_exec_tcb(program, tcb);
	log_debug(logger, "[ProgramID: %d] Choosen Exec tid: %d", program->id, program->exec->tid); //

	notify_program(program, SUSE_SCHEDULE_NEXT);
}

void wait_for_threads(t_program *program){

	log_debug(logger, "[ProgramID: %d] No hay hilos para ejecutar", program->id);
	sem_wait(&program->tcb_counter);
	sem_post(&program->tcb_counter);
	log_debug(logger, "[ProgramID: %d] Hilos para ejecutar encontrados", program->id);

	wakeup_threads(program);
}

void suse_wait(int tid, char* sem_name, t_program *program){

	if(program->exec->tid != tid){
		print_suse_not_exec(program->id, tid);
		return;
	}

	t_semaforo *semaforo = get_semaforo(sem_name);

	pthread_mutex_lock(&semaforo->sem_mutex);
	semaforo->valor--;
	if(semaforo->valor < 0){
		block(semaforo, program->exec);
		return;
	}
	pthread_mutex_unlock(&semaforo->sem_mutex);
}

void suse_signal(int tid, char* sem_name, t_program *program){

	if(program->exec->tid != tid){
		print_suse_not_exec(program->id, tid);
		return;
	}

	t_semaforo *semaforo = get_semaforo(sem_name);

	pthread_mutex_lock(&semaforo->sem_mutex);
	if(semaforo->valor < semaforo->max_value) semaforo->valor++;
	if(semaforo->valor <= 0){
		wakeup(semaforo);
		return;
	}
	pthread_mutex_unlock(&semaforo->sem_mutex);
}

void block(t_semaforo *semaforo, t_tcb *exec){

	list_add(semaforo->blocked, exec);

	move_tcb_to(exec, BLOCKED);
	pthread_mutex_unlock(&semaforo->sem_mutex);
}

void wakeup(t_semaforo *semaforo){

	t_tcb *tcb = list_get(semaforo->blocked, 0);
	list_remove(semaforo->blocked, 0);

	notify_wakeup(tcb);
	pthread_mutex_unlock(&semaforo->sem_mutex);
}

void suse_join(int tid, t_program *program){

	t_tcb *tcb;

	start:
	pthread_mutex_lock(&state_list_mutex);

	int pos = find_tcb_pos(program->ready, tid, program->socket);
	if(pos == ELEMENT_NOT_FOUND){

		pos = find_tcb_pos(exitList, tid, program->socket);
		if(pos != ELEMENT_NOT_FOUND){
			log_debug(logger, "[ProgramID: %d] El tid: %d ya finalizo", program->id, tid);
			pthread_mutex_unlock(&state_list_mutex);
			return;
		}

		pos = find_tcb_pos(blockedList, tid, program->socket);
		if(pos != ELEMENT_NOT_FOUND){
			tcb = list_get(blockedList, pos);
			goto tcbFound;
		}else{
			pthread_mutex_unlock(&state_list_mutex);
			wakeup_threads(program);
			goto start;
		}
	}

	tcb = list_get(program->ready, pos);

	tcbFound:
	list_add(tcb->joined, program->exec);

	pthread_mutex_unlock(&state_list_mutex);

	move_tcb_to(program->exec, BLOCKED);
}

void suse_close(int tid, t_program *program){

	if(program->exec->tid != tid){
		print_suse_not_exec(program->id, tid);
		return;
	}

	unjoin_exec(program);

	move_tcb_to(program->exec, EXIT);

	t_tcb *tcb;
	for(int i = list_size(exitList)-1; i >= 0; i--){
		tcb = list_get(exitList, i);
		if(tcb->socket == program->socket && tcb->tid == tid) log_metrics(tcb);
	}
}

void unjoin_exec(t_program *program){

	t_tcb *tcb;

	for(int i = 0; i < list_size(program->exec->joined); i++){
		tcb = list_get(program->exec->joined, i);
		move_tcb_to(tcb, READY);
	}
}

void change_exec_tcb(t_program *program, t_tcb *shortest_tcb){

	if(program->exec != NULL){

		move_tcb_to(program->exec, READY);
	}

	move_tcb_to(shortest_tcb, EXEC);

}

void notify_program(t_program *program, op_code codigo){

	switch(codigo){
		case SUSE_SCHEDULE_NEXT:{
			char* threadid = string_itoa(program->exec->tid);
			t_paquete *reply = crear_paquete(SUSE_SCHEDULE_NEXT_RETURN);
			agregar_a_paquete(reply, threadid, sizeof(int));
			enviar_paquete(reply, program->socket);
			log_debug(logger, "[ProgramID: %d] Siguiente thread notificado - tid: %s", program->id, threadid);
			eliminar_paquete(reply);
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

void notify_wakeup(t_tcb *tcb){

	t_program *program = get_program(tcb->socket);

	pthread_mutex_lock(&program->wakeup_mutex);
	list_add(program->wakeupList, tcb);
	sem_post(&program->tcb_counter);
	pthread_mutex_unlock(&program->wakeup_mutex);
}

bool wakeup_threads(t_program *program){

	pthread_mutex_lock(&program->wakeup_mutex);

	if(list_size(program->wakeupList) == 0){
		pthread_mutex_unlock(&program->wakeup_mutex);
		return false;
	}

	t_tcb *tcb;
	for(int i = 0; i < list_size(program->wakeupList); i++){
		tcb = list_remove(program->wakeupList, i);
		move_tcb_to(tcb, READY);
		sem_wait(&program->tcb_counter);
	}
	pthread_mutex_unlock(&program->wakeup_mutex);

	return true;
}

float get_estimate(t_burst *burst){

	return get_estimate_with_burst(burst, burst->last_burst);
}

float get_estimate_with_burst(t_burst *burst, float last_burst){

	float alpha = configData->alpha_sjf;
	float estimate = (alpha * last_burst) + ((1-alpha) * burst->last_estimate);

	return estimate;
}

void set_state_start_timing(t_state state, t_burst *timings){

	switch(state){
		case READY:
			gettimeofday(&timings->ready_start, NULL);
			break;
		case EXEC:
			gettimeofday(&timings->exec_start, NULL);
			break;
		default:
			break;
	}
}

void set_new_timings_for(t_state state, t_burst *timings){

	switch(state){
		case READY:{
			float difference = 0;
			gettimeofday(&timings->ready_end, NULL);
			difference = get_ms_difference_between(&timings->ready_start, &timings->ready_end);
			timings->total_ready += difference;}
			break;
		case EXEC:
			gettimeofday(&timings->exec_end, NULL);
			timings->last_burst = get_ms_difference_between(&timings->exec_start, &timings->exec_end);
			timings->total_exec += timings->last_burst;
			break;
		default:
			break;
	}
}

float get_last_burst_for(t_state state, t_burst *timings){

	struct timeval *now = get_time_now();

	float burst = 0;

	switch(state){
		case READY:
			burst = get_ms_difference_between(&timings->ready_start, now);
			break;
		case EXEC:
			burst = get_ms_difference_between(&timings->exec_start, now);
			break;
		default:
			break;
	}

	free(now);
	return burst;
}

float get_total_for(t_state state, t_burst *timings){

	float total;

	switch(state){
		case READY:
			total = get_last_burst_for(READY, timings) + timings->total_ready;
			break;
		case EXEC:
			total = get_last_burst_for(EXEC, timings) + timings->total_exec;
			break;
		default:
			break;
	}

	return total;
}

float get_ms_difference_between(struct timeval *start, struct timeval *end){

	return (end->tv_sec - start->tv_sec) * 1000.0f + (end->tv_usec - start->tv_usec) / 1000.0f;
}

struct timeval *get_time_now(){

	struct timeval *now = malloc(sizeof(struct timeval));
	gettimeofday(now, NULL);
	return now;
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
			set_new_timings_for(READY, tcb->time);
			program_remove_tcb_from_state(tcb->socket, tcb->tid, tcb->state);
			decrease_multiprog();
			break;
		case EXEC:
			set_new_timings_for(EXEC, tcb->time);
			program_remove_tcb_from_state(tcb->socket, tcb->tid, tcb->state);
			decrease_multiprog();
			break;
		case BLOCKED:
			pos = find_tcb_pos(blockedList, tcb->tid, tcb->socket);
			list_remove(blockedList, pos);
			decrease_multiprog();
			break;
		case EXIT:
			pos = find_tcb_pos(exitList, tcb->tid, tcb->socket);
			list_remove(exitList, pos);
			break;
	}

	switch (state) {
		case NEW:
			list_add(newList, tcb);
			break;
		case READY:
			set_state_start_timing(READY, tcb->time);
			program_add_tcb_to_state(tcb, READY);
			increase_multiprog();
			break;
		case EXEC:
			set_state_start_timing(EXEC, tcb->time);
			program_add_tcb_to_state(tcb, EXEC);
			increase_multiprog();
			break;
		case BLOCKED:
			list_add(blockedList, tcb);
			increase_multiprog();
			break;
		case EXIT:
			gettimeofday(&tcb->time->finished, NULL);
			list_add(exitList, tcb);
			break;
	}

	tcb->state = state;

	pthread_mutex_unlock(&state_list_mutex);
}

void increase_multiprog(){

	sem_wait(&available_multiprog);
	multiprog_degree++;
}

void decrease_multiprog(){

	sem_post(&available_multiprog);
	multiprog_degree--;
}

void check_multiprog(){

	sem_wait(&available_multiprog);
	sem_post(&available_multiprog);
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
	}
	else return;
}

void log_metrics(t_tcb *tcb){

	pthread_mutex_lock(&metrics_mutex);
	pthread_mutex_lock(&state_list_mutex);

	t_program *program;

	if(tcb != NULL){

		log_info(metricsLog, "Hilo Finalizado");
		program = get_program(tcb->socket);
		t_list *auxList = find_all_my_shared_threads(program);
		print_thread_metrics(tcb, program, auxList);
		log_info(metricsLog, "------------------------------------------------------------");
		pthread_mutex_unlock(&state_list_mutex);
		pthread_mutex_unlock(&metrics_mutex);
		list_destroy(auxList);
		return;
	}

	int multiprog_actual = multiprog_degree;
	log_info(metricsLog, "Metricas por Hilo");
	pthread_mutex_lock(&clientes_mutex);
	for(int i = 0; i < list_size(clientes); i++){
		t_tcb *thread;
		program = list_get(clientes, i);
		t_list *auxList = find_all_my_shared_threads(program);
		int j;
		if(program->exec != NULL){
			print_thread_metrics(program->exec, program, auxList);
		}
		for(j = 0; j < list_size(program->ready); j++){
			thread = list_get(program->ready, j);
			print_thread_metrics(thread, program, auxList);
		}
		for(j = 0; j < list_size(auxList); j++){
			thread = list_get(auxList, j);
			print_thread_metrics(thread, program, auxList);
		}
		list_destroy(auxList);
	}

	log_info(metricsLog, "Metricas por Programa");
	for(int i = 0; i < list_size(clientes); i++){
		program = list_get(clientes, i);
		log_info(metricsLog, "- Programa: %d", program->id);
		log_info(metricsLog, "- - Hilos en NEW: %d", get_list_size_for(program, NEW));
		log_info(metricsLog, "- - Hilos en READY: %d", list_size(program->ready));
		log_info(metricsLog, "- - Hilos en RUN: %d", program->exec != NULL ? 1 : 0);
		log_info(metricsLog, "- - Hilos en BLOCKED: %d", get_list_size_for(program, BLOCKED));
		log_info(metricsLog, "- - Hilos en EXIT: %d", get_list_size_for(program, EXIT));
	}
	pthread_mutex_unlock(&clientes_mutex);
	pthread_mutex_unlock(&state_list_mutex);

	t_semaforo *semaforo;
	log_info(metricsLog, "Metricas del Sistema");
	for(int i = 0; i < list_size(semaforos); i++){
		semaforo = list_get(semaforos, i);
		pthread_mutex_lock(&semaforo->sem_mutex);
		log_info(metricsLog, "- Semaforo - id: %s - valor: %d", semaforo->id, semaforo->valor);
		pthread_mutex_unlock(&semaforo->sem_mutex);
	}
	log_info(metricsLog, "- Grado actual de multiprogramacion: %d", multiprog_actual);
	log_info(metricsLog, "------------------------------------------------------------");

	pthread_mutex_unlock(&metrics_mutex);
}

void print_thread_metrics(t_tcb *tcb, t_program *program, t_list *sharedThreads){

	switch(tcb->state){
		case NEW:{
			struct timeval *now = get_time_now();
			log_info(metricsLog, "- Programa: %d - Hilo: %d - State: NEW", program->id, tcb->tid);
			log_info(metricsLog, "- - Tiempo en ejecucion: %10.0f ms", get_ms_difference_between(&tcb->time->created, now));
			log_info(metricsLog, "- - Tiempo en Espera: 0 ms");
			log_info(metricsLog, "- - Tiempo de uso de CPU: 0 ms");
			log_info(metricsLog, "- - Porcentaje del Tiempo de Ejecucion: 0 %%");
			free(now);}
			break;
		case READY:{
			struct timeval *now = get_time_now();
			log_info(metricsLog, "- Programa: %d - Hilo: %d - State: READY", program->id, tcb->tid);
			log_info(metricsLog, "- - Tiempo en ejecucion: %10.0f ms", get_ms_difference_between(&tcb->time->created, now));
			log_info(metricsLog, "- - Tiempo en Espera: %10.0f ms", get_total_for(READY, tcb->time));
			log_info(metricsLog, "- - Tiempo de uso de CPU: %10.0f ms", tcb->time->total_exec);
			log_info(metricsLog, "- - Porcentaje del Tiempo de Ejecucion: %10.2f %%", get_exec_percentage(program, tcb->tid, sharedThreads));
			free(now);}
			break;
		case EXEC:{
			struct timeval *now = get_time_now();
			log_info(metricsLog, "- Programa: %d - Hilo: %d - State: EXEC", program->id, tcb->tid);
			log_info(metricsLog, "- - Tiempo en ejecucion: %10.0f ms", get_ms_difference_between(&tcb->time->created, now));
			log_info(metricsLog, "- - Tiempo en Espera: %10.0f ms", program->exec->time->total_ready);
			log_info(metricsLog, "- - Tiempo de uso de CPU: %10.0f ms", get_total_for(EXEC, program->exec->time));
			log_info(metricsLog, "- - Porcentaje del Tiempo de Ejecucion: %10.2f %%", get_exec_percentage(program, tcb->tid, sharedThreads));
			free(now);}
			break;
		case BLOCKED:{
			struct timeval *now = get_time_now();
			log_info(metricsLog, "- Programa: %d - Hilo: %d - State: BLOCKED", program->id, tcb->tid);
			log_info(metricsLog, "- - Tiempo en Ejecucion: %10.0f ms", get_ms_difference_between(&tcb->time->created, now));
			log_info(metricsLog, "- - Tiempo en Espera: %10.0f ms", tcb->time->total_ready);
			log_info(metricsLog, "- - Tiempo de uso de CPU: %10.0f ms", tcb->time->total_exec);
			log_info(metricsLog, "- - Porcentaje del Tiempo de Ejecucion: %10.2f %%", get_exec_percentage(program, tcb->tid, sharedThreads));
			free(now);}
			break;
		case EXIT:
			log_info(metricsLog, "- Programa: %d - Hilo: %d - State: EXIT", program->id, tcb->tid);
			log_info(metricsLog, "- - Tiempo en Ejecucion: %10.0f ms", get_ms_difference_between(&tcb->time->created, &tcb->time->finished));
			log_info(metricsLog, "- - Tiempo en Espera: %10.0f ms", tcb->time->total_ready);
			log_info(metricsLog, "- - Tiempo de uso de CPU: %10.0f ms", tcb->time->total_exec);
			log_info(metricsLog, "- - Porcentaje del Tiempo de Ejecucion: %10.2f %%", get_exec_percentage(program, tcb->tid, sharedThreads));
			break;
		default:
			break;
	}
}

float get_exec_percentage(t_program *program, int tid, t_list *sharedThreads){

	if(program->exec == NULL && list_size(program->ready) == 0 && list_size(sharedThreads) == 0) return 0;

	float actual = 0;
	float suma = 0;

	if(program->exec != NULL){
		suma = get_total_for(EXEC, program->exec->time);
		if(program->exec->tid == tid) actual = suma;
	}

	t_tcb *thread;
	int i;
	for(i = 0; i < list_size(program->ready); i++){
		thread = list_get(program->ready, i);
		suma += thread->time->total_exec;
		if(thread->tid == tid) actual = thread->time->total_exec;
	}

	for(i = 0;i < list_size(sharedThreads); i++){
		thread = list_get(sharedThreads, i);
		suma += thread->time->total_exec;
		if(thread->tid == tid) actual = thread->time->total_exec;
	}

	return suma != 0 ? (actual * (float)100)/suma : 0;
}

int get_list_size_for(t_program *program, t_state state){

	t_list *lista;

	switch(state){
		case NEW:
			lista = newList;
			break;
		case BLOCKED:
			lista = blockedList;
			break;
		case EXIT:
			lista = exitList;
			break;
		default:
			return -1;
	}

	int size = 0;
	t_tcb *tcb;

	for(int i = 0; i < list_size(lista); i++){
		tcb = list_get(lista, i);
		if(tcb->socket == program->socket) size+=1;
	}

	return size;
}

t_list *find_all_my_shared_threads(t_program *program){

	t_list *myThreads = list_create();
	t_tcb *tcb = NULL;

	for(int i = 0; i < list_size(newList); i++){
		tcb = list_get(newList, i);
		if(tcb->socket == program->socket) list_add(myThreads, tcb);
	}
	for(int i = 0; i < list_size(blockedList); i++){
		tcb = list_get(blockedList, i);
		if(tcb->socket == program->socket) list_add(myThreads, tcb);
	}
	for(int i = 0; i < list_size(exitList); i++){
		tcb = list_get(exitList, i);
		if(tcb->socket == program->socket) list_add(myThreads, tcb);
	}

	return myThreads;
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
	semaforo->max_value = config_semaforo->max;
	semaforo->blocked = list_create();
	pthread_mutex_init(&semaforo->sem_mutex, NULL);

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
	gettimeofday(&burst->created, NULL);

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
	sem_post(&new_counter);
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
	pthread_mutex_init(&programa->wakeup_mutex, NULL);
	programa->wakeupList = list_create();

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

	pthread_mutex_destroy(&semaforo->sem_mutex);

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

	program->close = true;

	pthread_mutex_lock(&clientes_mutex);
	pthread_mutex_lock(&state_list_mutex);

	list_remove_and_destroy_all_tcb_from_program(newList, program->socket);
	list_destroy_and_destroy_elements(program->ready, (void*) destroy_tcb);
	if(program->exec != NULL) destroy_tcb(program->exec);
	list_remove_and_destroy_all_tcb_from_program(blockedList, program->socket);
	list_remove_and_destroy_all_tcb_from_program(exitList, program->socket);
	list_destroy_and_destroy_elements(program->wakeupList, (void*) destroy_tcb);

	t_semaforo *sem;
	int i;
	for(i = 0; i < list_size(semaforos); i++){
		sem = list_get(semaforos, i);
		pthread_mutex_lock(&sem->sem_mutex);
		list_remove_and_destroy_all_tcb_from_program(sem->blocked, program->socket);
		pthread_mutex_unlock(&sem->sem_mutex);
	}

	sem_destroy(&program->tcb_counter);
	pthread_mutex_destroy(&program->wakeup_mutex);

	t_program *aux;
	for(i = 0; i < list_size(clientes); i++){
		aux = list_get(clientes, i);
		if(aux->socket == program->socket) break;
	}
	list_remove(clientes, i);

	close(program->socket);

	pthread_mutex_unlock(&state_list_mutex);
	pthread_mutex_unlock(&clientes_mutex);

	log_debug(logger,"Programa Eliminado - id = %d", program->id);
}

void destroy_scheduler(pthread_t *scheduler){

	if(scheduler != NULL) free(scheduler);
}

void destroy_lists(){

	list_clean_and_destroy_elements(clientes, (void*) destroy_program);
	list_destroy_and_destroy_elements(clientes, (void*) free);
	list_destroy_and_destroy_elements(semaforos, (void*) destroy_semaforo);

	list_destroy_and_destroy_elements(newList,(void*) destroy_tcb);
	list_destroy_and_destroy_elements(blockedList,(void*) destroy_tcb);
	list_destroy_and_destroy_elements(exitList,(void*) destroy_tcb);

}

void destroy_mutexes(){

	pthread_mutex_destroy(&state_list_mutex);
	pthread_mutex_destroy(&metrics_mutex);
	pthread_mutex_destroy(&clientes_mutex);
}

void list_remove_and_destroy_all_tcb_from_program(t_list *lista, int socket){

	t_tcb *tcb;
	t_list *pos = list_create();
	int i;

	for(i = 0; i < list_size(lista); i++){
		tcb = list_get(lista, i);
		if(tcb->socket == socket) list_add(pos, string_itoa(i));
	}

	for(i = list_size(pos)-1; i >= 0; i--){
		list_remove_and_destroy_element(lista, atoi(list_get(pos, i)), (void*) destroy_tcb);
	}

	list_destroy_and_destroy_elements(pos, (void*) free);
}
