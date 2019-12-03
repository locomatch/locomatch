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
} t_burst;

typedef struct {
	int socket;
	pid_t tid;
	t_state state;
	t_burst *time;
	t_list *joined;
} t_tcb;

typedef struct {
	pthread_t short_term_scheduler;
	bool runing;
} t_short_scheduler;

typedef struct {
	int id;
	int socket;
	t_list *ready;
	t_tcb *exec;
	pthread_t short_scheduler;
	pthread_mutex_t ready_mutex;
	pthread_mutex_t exec_mutex;
} t_program;

typedef struct {
	char* id;
	int valor;
	t_list *blocked;
	pthread_mutex_t block_mutex;
} t_semaforo;

/*        GLOBALS        */

int program_id_counter;
int multiprog_degree;

t_list *clientes;
t_list *semaforos;

t_list *newList;
t_list *blockedList;
t_queue *exitQueue;

pthread_mutex_t state_list_mutex;
pthread_mutex_t create_program_mutex;
pthread_mutex_t metrics_mutex;

pthread_t long_term_scheduler;

/*        PROTOTYPES        */

void init_scheduler();
void init_semaforos();
void *schedule_long_term(void *arg);
void *schedule_short_term(void *arg);
void schedule_next_for(t_program * program);
void suse_wait(int tid, char* sem_name, t_program *program);
void suse_signal(int tid, char* sem_name, t_program *program);
void block(t_semaforo *semaforo, t_tcb *exec);
void wakeup(t_semaforo *semaforo);
void suse_join(int tid, t_program *program);
void suse_close(int tid, t_program *program);
void unjoin_exec(t_program *program);
void change_exec_tcb(t_program *program, t_tcb *shortest_tcb, int shortest_estimate);
void notify_program(t_program *program, op_code codigo);
int get_estimate(t_burst *burst);
void set_new_timings(t_burst *timings);
t_tcb *get_new_tcb();
t_semaforo *get_semaforo(char* sem_name);
void move_tcb_to(t_tcb *tcb, int state);
int find_tcb_pos(t_list *list, pid_t tid, int socket);
int find_program_pos(int socket);
bool is_program_loaded(int socket);
t_program *get_program(int socket);
int get_program_id(int socket);
void program_remove_tcb_from_state(int socket, pid_t tid, int state);
void program_add_tcb_to_state(t_tcb *tcb, int state);
void log_metrics();
float get_exec_percentage(t_program *program);
int get_new_list_size_for(t_program *program);
int get_blocked_list_size_for(t_program *program);

t_semaforo *create_semaforo(t_config_semaforo *config_semaforo);
t_burst *create_burst();
t_tcb *create_tcb(int socket, pid_t thread_id);
t_program *create_program(int socket);

void destroy_semaforo(t_semaforo *semaforo);
void destroy_burst(t_burst *burst);
void destroy_tcb(t_tcb *tcb);
void destroy_program(t_program *program);
void destroy_scheduler(pthread_t *scheduler);
void destroy_lists();
void destroy_mutexes();

#endif /* SUSE_SCHEDULER_H_ */
