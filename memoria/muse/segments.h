#ifndef SEGMENTS_H
#define SEGMENTS_H

#include <stdlib.h> //ES LIBMUSE
#include <commons/log.h>
#include <pthread.h>
#include <string.h>
#include <commons/config.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/string.h>

// ---- GLOBAL VARIABLES ----
int tamanio_mem;
int tamanio_pagina;
int tamanio_swap;
int cant_pags;
int cant_frames;

void* MAIN_MEMORY;
t_dictionary* SEGMENT_TABLE;

FILE* swap;


t_list* bitmapFrames;
t_bitarray* bitmapSwap;

int clockPointer;





//----------- MEMORY STRUCTS -------------

typedef struct{
  char* value;
  bool bitPresencia; //Podria ser un int?
  int nFrame;
  int nSwap; //Falta para esto
}page_t;

typedef struct page_info{
  bool isFree;
  int modif;
  int used;
}frame_t;

typedef struct{
  int nSegment;
  int pagsLib;
  uint32_t bLogica;
  uint32_t tam;
  t_list* tablaPaginas;
  t_list* metadatas;
}segment_t;

typedef struct{
  u_int32_t size;
  bool isFree;
}heapMetadata;

typedef struct{
  int dir;
  int size;
  int bytesUsed; //Bytes usados en el primer heap. SOlo si isSplitted = true
  bool isFree;
  bool isSplitted;
}heapList;

//-------------DESCLARE FUNCTIONS----------
void createBitmapFrames();
bool isFrameFree(int nFrame);
void fillFrame(int frame);
void freeFrame(int frame);
int searchFreeFrame();
bool anyFreeFrame();
void* framePos(int frame);

segment_t* createSegment(uint32_t tamanio, int idCli);
segment_t* setNewPage(segment_t* segment);
int setAFrame();
page_t* searchPageFromFrame(int nFrame);
uint32_t segmentSize(int nSegment, int idCli);
segment_t* setFirstSegmentPage(segment_t* segment, int metadataSize, bool* metadataSplitted);
int getPageIndex(t_list* pagesList, page_t* page);
segment_t* setLastSegmentPage(segment_t* segment, int sizeLastMetadata);
heapList* locateMetadataAndHeaplist(segment_t* segment, int heaplocat, bool isFree, uint32_t size, bool* isSplitted, uint32_t page);
void* getPosMemoryPage(page_t* page);
segment_t* searchSegmentWithSizeAvailable(t_list* segments, int tam);
int clockModif();
void increaseClockPointer();


//Faltaria una lista de heaps? DONE


//----------- PHARSER STRUCTS -------------
/*
UNMAP (X)
CHAR  (X)
VOID  (X)
*/

//MUSE ALLOC (uint32_t tam);
typedef struct{
  uint32_t tam_id;
  char* id;
  uint32_t tam;
}package_alloc;

//MUSE FREE (uint32_t dir);
typedef struct{
  uint32_t tam_id;
  char* id;
  uint32_t dir;
}package_free;

//MUSE GET (void* dst, uint32_t src, size_t n)
typedef struct{
  uint32_t tam_id;
  char* id;
  uint32_t dir;
  uint32_t tam;
}package_get;

//MUSE CPY (uint32_t dst, void* src, int n)
typedef struct{
  uint32_t tam_id;
  char* id;
  uint32_t dir;
  uint32_t tam_paq;
  void* paq;
}package_cpy;

//MUSE MAP (char *path, size_t length, int flags)
typedef struct{
  uint32_t tam_id;
  char* id;
  uint32_t tam_path;
  char* path;
  uint32_t tam;
  uint32_t flags;
}package_map;

//MUSE SYNC (uint32_t addr, size_t len)
typedef struct{
  uint32_t tam_id;
  char* id;
  uint32_t dir;
  size_t tam;
}package_sync;


// VIEJO
int NUMBER_OF_PAGES;
int PAGE_SIZE;
t_log* logger;
int VALUE_SIZE;
t_config* config;
//VIEJO



#endif