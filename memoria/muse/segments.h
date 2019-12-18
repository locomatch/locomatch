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

typedef struct{
  char* value;
  bool bitPresencia; //Agregado recien, antes estaba en page_info_t.
  int nFrame;
}page_t;

typedef struct page_info{
  page_t* page_ptr; //page pointer
  struct page_info* next;
  struct page_info* prev;
}page_info_t;

typedef struct{
  heapMetadata* seg_heap;
  page_info_t* pages;
  struct segment_t* next;
  struct segment_t* prev;
}segment_t;

typedef struct heapMetadata{
  u_int32_t size;
  bool isFree;
}heapMetadata;


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



// ---- GLOBAL VARIABLES ----

segment_t* SEGMENT_TABLE;
int NUMBER_OF_PAGES;
int PAGE_SIZE;
t_log* logger;
int VALUE_SIZE;
t_config* config;

int SEGMENT_NUM = 0;
//COMUNICACION CON SOCKET? Muse?

// --------------------------


page_t* create_page();
page_info_t* create_page_info();
segment_t* create_segment();
page_info_t* find_page_info(char* table_name, int key);
page_info_t* save_page(char* table_name, page_t* page);
page_info_t* insert_page(char* table_name, page_t* page);
void remove_from_segment(segment_t* segment, page_info_t* page_info);
page_info_t* save_page_to_memory(char* table_name, page_t* page, int dirtybit);
segment_t* find_segment(char* table_name);
segment_t* find_or_create_segment(char* table_name);
int find_free_page();
void remove_page(page_info_t* page_info);
void remove_and_save_page(page_info_t* page_info);
void remove_all_pages_from_segment(segment_t* segment, int save_to_fs_bit);
void add_segment_to_table(segment_t* segment);
void add_page_to_segment(segment_t* segment, page_info_t* page_info);
segment_t* get_last_segment();
page_info_t* get_last_page(page_info_t* page_info);
void print_page(page_info_t* page_info);
void print_segment_table();
void print_segment_pages(segment_t* segment);
int find_page_in_LRU(page_info_t* page);

int is_modified(page_info_t* page_info);
char* exec_in_memory(int memory_fd, char* payload);
void remove_segment(char* table_name, int save_to_fs_bit);
int* get_used_pages();
int* update_used_pages();
int find_unmodified_page();

void print_everything();
int memory_full();
int is_memory_full();


#endif