#include "segments.h"

#include <stdlib.h> //ES LIBMUSE
#include <commons/log.h>
#include <pthread.h>
#include <string.h>
#include <commons/config.h>
#include <unistd.h>


//Utilizado en MUSE_INIT para inicializar la tabla de segmentos VACIA de nuestro nuevo programa com ID Cliente
void createTableSegment(int idCli) {

	char *strIdCli = string_itoa(idCli);

	if (!dictionary_has_key(SEGMENT_TABLE, strIdCli)) {
		t_list *listaDeSegmentos = list_create();

		dictionary_put(SEGMENT_TABLE, strIdCli,
				listaDeSegmentos);
	}

//	free(strIdCli);

}

//Funcion encargada de crear un nuevo segmento
struct segment_t* createSegment(uint32_t tamanio, int idCli) {

	char* strIdCli = string_itoa(idCli);

//Inicializo nuevo segmento en la tabla del IDCli, sin contenito
	t_list* PSegmentList = dictionary_get(SEGMENT_TABLE,
			idCli);
	segment_t* newSegment = malloc(sizeof(segment_t));
	newSegment->pagsLib = 0;
	newSegment->metadatas = list_create();
//Lo relleno con respectivo numero de segmento
	if (list_is_empty(PSegmentList)) {
		newSegment->nSegment = 0;
	} 
	else {
		newSegment->nSegment = list_size(PSegmentList);
	}
//Lo relleno con respectiva base logica.
//0 si es el primero  Y  (base logica del lastSegment + 1) si no es el primero

	if (list_is_empty(PSegmentList)) {
		newSegment->bLogica = 0 + MAIN_MEMORY;
	} 
	else {
		int nLastSegment = list_size(PSegmentList) - 1;
		newSegment->bLogica = segmentSize(nLastSegment, idCli) + 1 + MAIN_MEMORY;
	}
//Lo relleno con respectivas paginas/frames. Tengo que analizar segun el TAMANIO ALOCADO cuantas paginas necesita.
//"Incluye requerir el espacio solicitado mas el tamanio de 2 metadatas".
//Una para indicar X bytes en uso y otra para indicar Y bytes posteriores libres.
int pagesNeeded, allocatedSize;
allocatedSize = (tamanio + 2 * sizeof(heapMetadata));
pagesNeeded = ceil(allocatedSize / tamanio_pagina) //Necesito redondear siempre para arriba, por eso uso la lib math.h y ceil
//Lo relleno con Tabla de Paginas, VACIA hasta el momento
//TODO LIST: Tengo que mostrar de alguna manera que hay un heap dividido. Algo en la estructura de mi segment?
//Lo dejo aca hasta pensar como sigue esto.





}




uint32_t segmentSize(int nSegment, int idCli) {
	char* strIdCli = string_itoa(idCli);
	t_list* PSegmentList = dictionary_get(SEGMENT_TABLE,
			idCli);
	segment_t* provSegment = malloc(sizeof(segment_t)); //Me suena raro  (struct HeapMetadata)

	provSegment = list_get(PSegmentList, nSegment);

//	free(stringIdSocketCliente);
	return provSegment->tam;
}


// Me falta onsiderar si se partio la metadata. Por ahi algo asi? -> bool *metadataSplited)
struct segment_t* setFirstSegmentPage(segment_t* segment, int metadataSize) { 

	page_t* firstPage = malloc(sizeof(page_t));
	firstPage->nFrame = asignarUnFrame();
	firstPage->bitPresencia = 1;
	firstPage->nSwap = -1;
	frame_t* frame = malloc(sizeof(frame_t));
//TODOLIST BITMAP DE LOS FRAMES.
	frame = list_get(/*Bitmap de frames?*/, firstPage->nFrame);
	frame->modif = 1;
	frame->used = 1;
	frame->isFree = false;
	list_replace(/*Bitmap de frames?*/, firstPage->nFrame, frame);

	list_add(segment->tablaPaginas, firstPage);

		int pos = getPageIndex(segment->tablaPaginas, firstPage)* tamanio_pagina;

	heapMetadata* metadata = malloc(sizeof(heapMetadata));
	metadata->isFree = false;
	metadata->size = metadataSize;


int getPageIndex(t_list* pagesList, page_t* page) {

	page_t* provPage;

	for (int i = 0; i < list_size(pagesList); i++) {
		provPage = list_get(pagesList, i);

		if (!memcmp(page, pagesList, sizeof(page_t))) {
			return i;
		}

	}

	return -1;
}


























// APRENDI LIST Y DICTIONARY

/*
page_t* create_page(){
	page_t* page = (page_t*)malloc(sizeof(page_t));
	page->value = (char*)malloc(VALUE_SIZE); //VALUE SIZE SERIA EL TAMANIO DEL MALLOC?
	// strcpy(page->value, value);
	return page;
}

//	page->value = (char*)malloc(VALUE_SIZE);
//	memcpy(page->value, value, VALUE_SIZE);

page_info_t* create_page_info(){
  	page_info_t* page_info = (page_info_t*)malloc(sizeof(page_info_t));
// Numero de pagina? Index?
	page_info->isFree = true;
	page_info->next = NULL;
	page_info->prev = NULL;
  	return page_info;
}

segment_t* create_segment(){
  	segment_t* segment = (segment_t*)malloc(sizeof(segment_t));
	segment->seg_num = SEGMENT_NUM;
	SEGMENT_NUM++; //Cada segmento tiene un numero. Problema: Cada malloc genera un nuevo segmento
	segment->seg_heap = NULL;
  	segment->pages = NULL;
	segment->next = NULL;
	segment->prev = NULL;
	add_segment_to_table(segment);
  	return segment;
}

void add_segment_to_table(segment_t* segment){
	if(SEGMENT_TABLE == NULL){
		SEGMENT_TABLE = segment;
		segment->prev = NULL;
	}
	else{
		segment_t* temp = get_last_segment();
		temp->next = segment;
		segment->prev = temp;
	}
	print_segment_table();
}

segment_t* get_last_segment(){
  segment_t* temp = SEGMENT_TABLE;
  while(temp->next != NULL){
    temp = temp->next;
  }
  return temp;
}

void add_page_to_segment(segment_t* segment, page_info_t* page_info){
	if(segment->pages == NULL){
		segment->pages = page_info;
		page_info->prev = NULL;
	}
	else{
		page_info_t* temp = get_last_page(segment->pages);
		temp->next = page_info;
		page_info->prev = temp;
	}
}

page_info_t* get_last_page(page_info_t* page_info){
  page_info_t* temp = page_info;
  while(temp->next != NULL){
    temp = temp->next;
  }
  return temp;
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

int find_free_page(segment_t* segment){
	page_info_t* temp_page = segment->pages;
		for(int i = 0; i < NUMBER_OF_PAGES; i++){ 
			if(page_is_free(temp_page){ //No entiendo el error
				return i; //RETORNA LA PRIMERA PAGINA VACIA
			}
		temp_page = temp_page->next;
		}
		
	}

void page_is_free(page_info_t* page){
	bool rta = page->isFree; ///MMM 
	return rta;
}

segment_t* find_segment(int num){
	segment_t* temp = SEGMENT_TABLE;

	while(temp != NULL){
		if(strcmp(temp->seg_num, num) == 0){
			return temp;
		}
		temp = temp->next
	}
	return temp; //SI NO EXISTE SERA NULL
}
*/