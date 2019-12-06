#ifndef SEGMENTS_H
#define SEGMENTS_H

#include <stdlib.h> //ES LIBMUSE
#include <commons/log.h>
#include <pthread.h>
#include <string.h>
#include <commons/config.h>
#include <unistd.h>

typedef struct{
  char* value; //MMMM mepa cualqueira esto
}page_t;

typedef struct heapMetadata{
  u_int32_t size;
  bool isFree;
}heapMetadata;

typedef struct page_info{
  int num_pag;
  page_t* page_ptr;
  bool isFree; //Es mejor usar 0/1?
  struct page_info* next;
  struct page_info* prev;
}page_info_t;

typedef struct segment{
  int seg_num;
  page_info_t* pages;
  heapMetadata* seg_heap;
  struct segment* next;
  struct segment* prev;
}segment_t;

// ---- GLOBAL VARIABLES ----

segment_t* SEGMENT_TABLE;
page_t* MAIN_MEMORY; 
int NUMBER_OF_PAGES;
int PAGE_SIZE;
t_log* logger;
int VALUE_SIZE;
t_config* config;

int SEGMENT_NUM = 0;
//COMUNICACION CON SOCKET? Muse?

pthread_mutex_t main_memory_mutex;

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