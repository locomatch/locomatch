#include "segments.h"

#include <stdlib.h> //ES LIBMUSE
#include <commons/log.h>
#include <pthread.h>
#include <string.h>
#include <commons/config.h>
#include <unistd.h>

page_t* 
create_page(int key, char* value){
	//TODO: LEVANTAR EXCEPCION SI EL VALUE ES MUY GRANDE????
	page_t* page = (page_t*)malloc(sizeof(page_t));
	page->key = key;
	page->value = (char*)malloc(VALUE_SIZE);
	memcpy(page->value, value, VALUE_SIZE);
	// strcpy(page->value, value);
	return page;
}

page_info_t* create_page_info(){
  	page_info_t* page_info = (page_info_t*)malloc(sizeof(page_info_t));
	page_info->next = NULL;
	page_info->prev = NULL;
  	return page_info;
}

segment_t* create_segment(char* table_name){
  	segment_t* segment = (segment_t*)malloc(sizeof(segment_t));
	segment->name = table_name;
  	segment->pages = NULL;
	segment->next = NULL;
	segment->prev = NULL;
	add_segment_to_table(segment);
  	return segment;
}

int find_free_page(){
	log_info(logger, "Searching for  index");
	if(!is_memory_full()){ //NECESITO GENERAR ESTA FUNCION
		for(int i = 0; i < NUMBER_OF_PAGES; i++){ // me fijo que indexes de pagina estan siendo usados
			// printf("Index %d is // ? ", i);
			if(!page_is_on_use(i)){
				// printf("Yes\n");
				return i;
			}
			// printf("No\n");
		}
		
	}
	else{
		log_info(logger, "MEMORIA LLENA"); //ESTA BIEN?
	return -1;
}

page_info_t* insert_page(char* table_name, page_t* page){
	page_info_t* page_info = find_page_info(table_name, page->key);
	// si ya existe la pagina, reemplazo el value y toco el dirtybit
	if(page_info != NULL){
		if(page_info->page_ptr->timestamp < page->timestamp){ // si por alguna razon de la vida el timestamp del insert es menor al timestamp que ya tengo en la page, no la modifico
			log_info(logger, "Updating value %s->%s\n", page_info->page_ptr->value, page->value);
			
			memcpy(page_info->page_ptr->value, page->value, VALUE_SIZE);
			page_info->dirty_bit = 1;
		}
	}
	// si no existe, creo una nueva con dirtybit (si no tiene dirtybit no se la mando a fs en el journaling)
	else{
		page_info = save_page_to_memory(table_name, page, 1);
	}
	// free_page(page);
	return page_info;
}