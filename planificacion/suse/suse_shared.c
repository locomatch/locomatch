#include "suse_shared.h"

void print_malloc_error(char* element){
	log_error(logger, "No hay memoria para crear el %s", element);
}

void print_pthread_create_error(char* element){
	log_error(logger, "Error al crear el thread: %s", element);
}

void print_suse_not_exec(int program_id, int tid){
	log_error(logger, "[ProgramID: %d] SUSE_SIGNAL TID NOT EXEC - tid: %d", program_id, tid);
}
