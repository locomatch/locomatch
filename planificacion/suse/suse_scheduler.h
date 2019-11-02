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
	int total;
	int last_burst;
} t_burst;

typedef struct {
	pid_t pid;
	char* name;
} t_pcb;

typedef struct {
	pid_t tid;
	t_state state;
	t_pcb *pcb;
	t_burst *time;
	int pid;
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
	t_short_scheduler *scheduler;
} t_program;

typedef struct {
	char* id;
	int valor;
	t_list *blocked;
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

pthread_t *long_term_scheduler;

/*        PROTOTYPES        */

void init_scheduler();
void init_semaforos();
void *schedule_long_term(void *arg);
void schedule_next_for(int socket);
t_tcb *get_new_tcb();
void move_tcb_to(t_tcb *tcb, int state);
int find_tcb_pos(t_list *list, pid_t tid, int pid);
int find_program_pos(int pid);
int find_program_pos_by_socket(int socket);
bool is_program_loaded(int socket);
t_program *get_program(int(*find_by)(int), int unint);
void program_remove_tcb_from_state(int pid, pid_t tid, int state);
void program_add_tcb_to_state(t_tcb *tcb, int state);

t_semaforo *create_semaforo(t_config_semaforo *config_semaforo);
t_burst *create_burst();
t_pcb *create_pcb(pid_t process_id, char* process_name);
t_tcb *create_tcb(int program_id, pid_t thread_id, pid_t process_id, char* process_name);
t_program *create_program(int socket);
t_short_scheduler *create_short_scheduler();

void destroy_semaforo(t_semaforo *semaforo);
void destroy_burst(t_burst *burst);
void destroy_pcb(t_pcb *pcb);
void destroy_tcb(t_tcb *tcb);
void destroy_program(t_program *program);
void destroy_scheduler(pthread_t *scheduler);
void destroy_lists();
void destroy_mutexes();

#endif /* SUSE_SCHEDULER_H_ */
