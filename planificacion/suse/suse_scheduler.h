#ifndef SUSE_SCHEDULER_H_
#define SUSE_SCHEDULER_H_

/*        INCLUDES        */

#include <pthread.h>

#ifndef SUSE_SHARED_H_
	#include "suse_shared.h"
#endif

/*        CONSTANTS        */

typedef enum {
	NEW,
	READY,
	EXEC,
	BLOCKED,
	EXIT
} t_state;

/*        DEFINITIONS        */

typedef struct {
	float total_ready;
	struct timeval ready_start;
	struct timeval ready_end;
	float total_exec;
	struct timeval exec_start;
	struct timeval exec_end;
	float last_burst;
	float last_estimate;
	struct timeval created;
	struct timeval finished;
} t_burst;

typedef struct {
	int socket;
	int tid;
	t_state state;
	t_burst *time;
	t_list *joined;
} t_tcb;

typedef struct {
	int id;
	int socket;
	t_list *ready;
	t_tcb *exec;
	pthread_t short_scheduler;
	pthread_mutex_t main_loaded_mutex;
	sem_t tcb_counter;
	pthread_mutex_t wakeup_mutex;
	t_list *wakeupList;
	bool close;
} t_program;

typedef struct {
	char* id;
	int valor;
	int max_value;
	t_list *blocked;
	pthread_mutex_t sem_mutex;
} t_semaforo;

/*        GLOBALS        */

int program_id_counter;
int multiprog_degree;

t_list *clientes;
t_list *semaforos;

t_list *newList;
t_list *blockedList;
t_list *exitList;

pthread_mutex_t state_list_mutex;
pthread_mutex_t metrics_mutex;
pthread_mutex_t clientes_mutex;
sem_t new_counter;
sem_t available_multiprog;

pthread_t long_term_scheduler;

/*        PROTOTYPES        */

void init_scheduler();
void init_semaforos();
void *schedule_long_term(void *arg);
void *schedule_short_term(void *arg);
void schedule_next_for(t_program * program);
void wait_for_threads(t_program *program);
void suse_wait(int tid, char* sem_name, t_program *program);
void suse_signal(int tid, char* sem_name, t_program *program);
void block(t_semaforo *semaforo, t_tcb *exec);
void wakeup(t_semaforo *semaforo);
void suse_join(int tid, t_program *program);
void suse_close(int tid, t_program *program);
void unjoin_exec(t_program *program);
void change_exec_tcb(t_program *program, t_tcb *shortest_tcb);
void notify_program(t_program *program, op_code codigo);
void main_exec_signal(t_tcb *tcb);
void notify_wakeup(t_tcb *tcb);
bool wakeup_threads(t_program *program);
float get_estimate(t_burst *burst);
float get_estimate_with_burst(t_burst *burst, float last_burst);
void set_state_start_timing(t_state state, t_burst *timings);
void set_new_timings_for(t_state state, t_burst *timings);
float get_last_burst_for(t_state state, t_burst *timings);
float get_total_for(t_state state, t_burst *timings);
float get_ms_difference_between(struct timeval *start, struct timeval *end);
struct timeval *get_time_now();
t_tcb *get_new_tcb();
t_semaforo *get_semaforo(char* sem_name);
void move_tcb_to(t_tcb *tcb, int state);
void increase_multiprog();
void decrease_multiprog();
void check_multiprog();
int find_tcb_pos(t_list *list, int tid, int socket);
int find_program_pos(int socket);
bool is_program_loaded(int socket);
t_program *get_program(int socket);
int get_program_id(int socket);
void program_remove_tcb_from_state(int socket, int tid, int state);
void program_add_tcb_to_state(t_tcb *tcb, int state);
void log_metrics(t_tcb *tcb);
void print_thread_metrics(t_tcb *tcb, t_program *program, t_list *sharedThreads);
float get_exec_percentage(t_program *program, int tid, t_list *sharedThreads);
int get_list_size_for(t_program *program, t_state state);
t_list *find_all_my_shared_threads(t_program *program);

t_semaforo *create_semaforo(t_config_semaforo *config_semaforo);
t_burst *create_burst();
t_tcb *create_tcb(int socket, int thread_id);
t_program *create_program(int socket);

void destroy_semaforo(t_semaforo *semaforo);
void destroy_burst(t_burst *burst);
void destroy_tcb(t_tcb *tcb);
void destroy_program(t_program *program);
void destroy_scheduler(pthread_t *scheduler);
void destroy_lists();
void destroy_mutexes();
void list_remove_and_destroy_all_tcb_from_program(t_list *lista, int socket);

#endif /* SUSE_SCHEDULER_H_ */
