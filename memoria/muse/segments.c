#include "segments.h"

#include <stdlib.h> //ES LIBMUSE
#include <commons/log.h>
#include <pthread.h>
#include <string.h>
#include <commons/config.h>
#include <unistd.h>
#include <math.h>

//Utilizado en MUSE_INIT para inicializar la tabla de segmentos VACIA de nuestro nuevo programa com ID Cliente
void createTableSegment(int idCli) {

	char *strIdCli = string_itoa(idCli);

	if (!dictionary_has_key(SEGMENT_TABLE, strIdCli)) {
		t_list *listaDeSegmentos = list_create();

		dictionary_put(SEGMENT_TABLE, strIdCli, listaDeSegmentos);
	}

//	free(strIdCli);
}

void createBitmapFrames(){
	bitmapFrames = list_create();
	for (int i = 0; i < cant_frames; i++) {

		frame_t* newFrame = malloc(sizeof(frame_t));

		newFrame->modif = 0;
		newFrame->used = 0;
		newFrame->isFree = true;

		list_add(bitmapFrames, newFrame);
	}
}

bool isFrameFree(int nFrame){
	frame_t* frame = list_get(bitmapFrames, nFrame);

	if(frame->isFree==true){
		return true;
	}
	else{
		return false;
	}
//return frame->isFree; De una?
}

void fillFrame(int frame){
	frame_t* provFrame = list_get(bitmapFrames, frame);

	provFrame->used = 1;
	provFrame->isFree = false;
}

void freeFrame(int frame){
	frame_t* provFrame = list_get(bitmapFrames, frame);

	provFrame->used = 0; //OK?
	provFrame->isFree = true;
}

int searchFreeFrame(){
	frame_t* frame;

	for(int i=0; i < cant_frames; i++){
		frame = list_get(bitmapFrames, i);

		if(frame->isFree == true){ //al pedo la comparacion al true?
			return i;
		} 
	}
	return -1;
}

bool anyFreeFrame(){
	int freeFrame = searchFreeFrame();

	if(freeFrame == -1){
		return false;
	}
	else{
		return true;
	}
}

void* framePos(int frame){
	int offset = tamanio_pagina * frame;

	void* pos = (MAIN_MEMORY + offset);

	return pos;
}


//Funcion encargada de crear un nuevo segmento
segment_t* createSegment(uint32_t tamanio, int idCli) {

	char* strIdCli = string_itoa(idCli);

//Inicializo nuevo segmento en la tabla del IDCli, sin contenito
	t_list* pSegmentList = dictionary_get(SEGMENT_TABLE, strIdCli);
	segment_t* newSegment = malloc(sizeof(segment_t));
	newSegment->pagsLib = 0;
	newSegment->metadatas = list_create();
//Lo relleno con respectivo numero de segmento
	if (list_is_empty(pSegmentList)) {
		newSegment->nSegment = 0;
	} 
	else {
		newSegment->nSegment = list_size(pSegmentList);
	}
//Lo relleno con respectiva base logica.
//0 si es el primero  Y  (base logica del lastSegment + 1) si no es el primero

	if (list_is_empty(pSegmentList)) {
		newSegment->bLogica = 0 + tamanio_mem; // o MAIN_MEMORY?
	} 
	else {
		int nLastSegment = list_size(pSegmentList) - 1;
		newSegment->bLogica = segmentSize(nLastSegment, idCli) + 1 + tamanio_mem;
	}
//Lo relleno con respectivas paginas/frames. Tengo que analizar segun el TAMANIO ALOCADO cuantas paginas necesita.
//"Incluye requerir el espacio solicitado mas el tamanio de 2 metadatas".
//Una para indicar X bytes en uso y otra para indicar Y bytes posteriores libres.
int pagesNeeded, allocatedSize;
allocatedSize = (tamanio + 2 * sizeof(heapMetadata));
pagesNeeded = ceil(allocatedSize / tamanio_pagina); //Necesito redondear siempre para arriba, por eso uso la lib math.h y ceil
//Lo relleno con Tabla de Paginas, VACIA hasta el momento
//TODO LIST: DONE. Tengo que mostrar de alguna manera que hay un heap dividido. Algo en la estructura de mi segment?

newSegment->tablaPaginas = list_create();
bool* metadataSplitted = malloc(sizeof(bool));
*metadataSplitted = false; //falta un (*) ?
//Relleno Tabla de Paginas con Primera Pagina
newSegment = setFirstSegmentPage(newSegment, tamanio, metadataSplitted); //metadataSplitted puede ser directamente false?

//ALGO RARO HAY ACA///////////////////////////////////////////////////////////////////////////////////

heapList* lastHeap = list_get(newSegment->metadatas, list_size(newSegment->metadatas) -1 );

if(lastHeap->isFree == true){ //al pedo la comparativa contra el true?
	allocatedSize -= lastHeap->size;
}

lastHeap->size = tamanio;
lastHeap->isFree = false;

while(pagesNeeded > 0){

	if(allocatedSize > tamanio_pagina){
		newSegment = setNewPage(newSegment);
		allocatedSize -= tamanio_pagina;
	}
	else{
		newSegment = setLastSegmentPage(newSegment, allocatedSize);
		allocatedSize = 0;
	}

	pagesNeeded --;
}

list_add(pSegmentList, newSegment);

//free(strIdCli);

return newSegment;
}


segment_t* setNewPage(segment_t* segment){
	page_t* page = malloc(sizeof(page_t));
	page->nFrame = setAFrame();
	page->bitPresencia = 1;
	page->nSwap = -1;

	frame_t* frame = malloc(sizeof(frame_t));
	frame = list_get(bitmapFrames, page->nFrame);
	frame->modif = 1;
	frame->used = 1;
	frame->isFree = false;

	list_replace(bitmapFrames, page->nFrame, frame);//list_replace o replaceAndDestroy?

	list_add(segment->tablaPaginas, page);

	segment->tam += tamanio_pagina;

	return segment;
}

int setAFrame(){
	int frame;
	bool swapNeeded;

	if(anyFreeFrame()){
		frame = searchFreeFrame();
		swapNeeded = false;
	}
	else{
		frame = clockModif(); //TODO LIST:FALTA HACER. DONE
		swapNeeded = true;
	}

	frame_t* frameR = malloc(sizeof(frame_t));
	frameR = list_get(bitmapFrames, frame);
	frameR->modif = 1;
	frameR->used = 1;
	frameR->isFree = false;

	if(swapNeeded){//esta bien comprarar directo contra variable?
		int indexSwap;
		page_t* pageToSwap = searchPageFromFrame(frame);

		if(pageToSwap != NULL){
			indexSwap = swapPage(pageToSwap);
		}
	
	return frame;

	}
	return -1; //Mmm ??
}

//TODO  ////////////////////////////////////////////////////////////////////////////////////////////////
//Lleva una pagina a SWAP, almecenandolo en el archivo y liberando el frame donde se encontraba. No se que retorna
int swapPage(page_t* pageToSwap){
	return 1;
}



//ANALIZO CADA PAGINA DE CADA SEGMENTO DE CADA PROCESO
page_t* searchPageFromFrame(int nFrame){
	t_list* segments;
	t_list* pages;
	segment_t* segment;
	page_t* page;

	for (int i = 0; i < dictionary_size(SEGMENT_TABLE); i++) {
		segments = dictionary_get(SEGMENT_TABLE, string_itoa(/*ALGO QUE CHEQUEE TODOS LOS IDCli*/));

		for (int j = 0; j < list_size(segments); j++) { 
			segment = list_get(segments, j);
			pages = segment->tablaPaginas;

			for (int k = 0; k < list_size(pages); k++) {
				page = list_get(pages, k);

				if (page->nFrame == nFrame) {
					return page;
				}
			}

		}
	}

	return NULL;
}

uint32_t segmentSize(int nSegment, int idCli) {
	char* strIdCli = string_itoa(idCli);
	t_list* PSegmentList = dictionary_get(SEGMENT_TABLE, strIdCli);
	segment_t* provSegment = malloc(sizeof(segment_t)); //Me suena raro  (struct HeapMetadata)

	provSegment = list_get(PSegmentList, nSegment);

//	free(stringIdSocketCliente);
	return provSegment->tam;
}


// Me falta onsiderar si se partio la metadata. Por ahi algo asi? -> bool *metadataSplited)
segment_t* setFirstSegmentPage(segment_t* segment, int metadataSize, bool* metadataSplitted) { 

	page_t* firstPage = malloc(sizeof(page_t));
	firstPage->nFrame = setAFrame();
	firstPage->bitPresencia = 1;
	firstPage->nSwap = -1;
	frame_t* frame = malloc(sizeof(frame_t));
//TODOLIST BITMAP DE LOS FRAMES.
	frame = list_get(bitmapFrames, firstPage->nFrame);
	frame->modif = 1;
	frame->used = 1;
	frame->isFree = false;
	list_replace(bitmapFrames, firstPage->nFrame, frame);

	list_add(segment->tablaPaginas, firstPage);

	int index = getPageIndex(segment->tablaPaginas, firstPage) * tamanio_pagina;

	heapMetadata* metadata = malloc(sizeof(heapMetadata));
	metadata->isFree = false;
	metadata->size = metadataSize;

	locateMetadataAndHeaplist(segment, index, metadata->isFree, metadata->size, metadataSplitted, 1);

	if(*metadataSplitted){
		segment->tam += 2 * tamanio_pagina;
	}
	else{
		segment->tam += tamanio_pagina;
	}

return segment;

}




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

segment_t* setLastSegmentPage(segment_t* segment, int sizeLastMetadata) {

	bool* metadataSplitted = malloc(sizeof(bool));
	*metadataSplitted = false;

	heapMetadata* lastMetadata = malloc(sizeof(heapMetadata));
	lastMetadata->isFree = true;
	if (tamanio_pagina - sizeLastMetadata == 0) {
		lastMetadata->isFree = false;
	}
	lastMetadata->size = tamanio_pagina - sizeLastMetadata;
	page_t* page = (list_get(segment->tablaPaginas, list_size(segment->tablaPaginas)- 1));
	int dirHeap = (page->nFrame * tamanio_pagina)+ (tamanio_pagina - sizeof(heapMetadata)+ sizeLastMetadata);
	locateMetadataAndHeaplist(segment, dirHeap, lastMetadata->isFree, lastMetadata->size, metadataSplitted, list_size(segment->tablaPaginas));

	segment->tam += tamanio_pagina;

	return segment;
}


	heapList* locateMetadataAndHeaplist(segment_t* segment, int heaplocat, bool isFree, uint32_t size, bool* isSplitted, uint32_t page) {
	heapList* heap = malloc(sizeof(heapList));

	heap->dir = heaplocat;
	heap->isFree = isFree;
	heap->size = size;

	int indexFirstPage = page - 1;
	page_t* provPage = list_get(segment->tablaPaginas, indexFirstPage);


	frame_t* frameFirstPage = list_get(bitmapFrames, provPage->nFrame);
	frameFirstPage->modif = 1;
	frameFirstPage->used = 1;

	int despFirstPage = heaplocat % tamanio_pagina;

	heapMetadata* metadata = malloc(sizeof(metadata));
	metadata->size = size;
	metadata->isFree = isFree;
//U sure?
return heap; //ALGO MUY RARO
}

void* getPosMemoryPage(page_t* page){
	if(page->bitPresencia == 0){
		if(page->nSwap == -1){
			void* pageR = malloc(tamanio_pagina);
			memcpy(pageR,(swap + page->nSwap * tamanio_pagina), tamanio_pagina); //TENGO QUE CREAR ARCHIVO SWAP

			bitarray_clean_bit(bitmapSwap, page->nSwap); //libero el indice en swap
			page->nSwap = -1;

			int frame = searchFreeFrame();
			page->nFrame = frame;
			page->bitPresencia = 1;

			//Paso la pagina a memoria
			memcpy(framePos(frame), pageR, tamanio_pagina);
		}
	}

	frame_t* provFrame = list_get(bitmapFrames, page->nFrame);

	list_replace(bitmapFrames, page->nFrame, provFrame);

	return MAIN_MEMORY + (page->nFrame * tamanio_pagina);

}

segment_t* searchSegmentWithSizeAvailable(t_list* segments, int tam) {
	segment_t* segment;

	for (int i = 0; i < list_size(segments); i++) {
		segment = list_get(segments, i);
		heapList* heapAux;

		for (int i = 0; i < list_size(segment->metadatas); i++) {
			heapAux = list_get(segment->metadatas, i);

			if (heapAux->isFree == true && heapAux->size >= tam) {
				return segment;
			}
		}

	}

	return NULL;
}

/*

CLOCK MODIFICADO - Tomado y modificado de una repo indu de github

Pasos:
	1) Busca U = 0 y M = 0;
	2) Busca U = 0 y M = 1;   
		a. Sino reemplazo U = 1 por U = 0;
	3) Repito paso 1.
	4) Repito paso 2 sin reemplazar.

*/

int clockModif() {
	frame_t* frame = malloc(sizeof(frame_t));
	int framesSearched = 0;

	//Paso 1
	while (framesSearched < cant_frames) {
		frame = list_get(bitmapFrames, clockPointer);

		if (frame->used == 0 && frame->modif == 0) {

			int result = clockPointer;
			increaseClockPointer();
			return result;

		}

		framesSearched++;
		increaseClockPointer();

	}

	framesSearched = 0;

	//Paso 2
	while (framesSearched < cant_frames) {
		frame = list_get(bitmapFrames, clockPointer);

		if (frame->used == 0 && frame->modif == 1) {

			int resultado = clockPointer;
			increaseClockPointer();
			return resultado;

		}

		frame->used = 0;

		framesSearched++;
		increaseClockPointer();

	}

	framesSearched = 0;

	//Paso 3
	while (framesSearched < cant_frames) {
		frame = list_get(bitmapFrames, clockPointer);

		if (frame->used == 0 && frame->modif == 0) {

			int result = clockPointer;
			increaseClockPointer();
			return result;

		}

		framesSearched++;
		increaseClockPointer();

	}

	framesSearched = 0;

	//Paso 1 - repite
	while (framesSearched < cant_frames) {
		frame = list_get(bitmapFrames, clockPointer);

		if (frame->used == 0 && frame->modif == 1) {

			int resultado = clockPointer;
			increaseClockPointer();
			return resultado;

		}

		framesSearched++;
		increaseClockPointer();

	}

	return -1;
}

void increaseClockPointer() {

	if (clockPointer < (cant_frames - 1)) {

		clockPointer++;

	} else {

		clockPointer = 0;

	}

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