#include <stdbool.h>

#ifndef _FS_UTILS_
#define _FS_UTILS_

#define _FS_MAX_DEPTH_ 255
#define _FS_MAX_CHILDS_ 1024
#define _FS_HASH_BUCKETS_ 1259
#define _MAX_CHAR_COMBINATIONS_ 62

//Error Codes
#define _FS_ITEM_ALREADY_EXISTS_ -1
#define _FS_MAX_DEPTH_REACHED_ -1
#define _FS_MAX_CHILDS_REACHED_ -2
#define _FS_FILE_ALREADY_EXISTS_ -3
#define _FS_KEY_END_ -3
#define _FS_KEY_WRONG_CHARS_ -2
#define _FS_KEY_EMPTY_ -1

typedef struct node_tag
{
  //Accesso come lista (Struttura ad Albero)
  struct node_tag* fs_parent;

  //Organizzazione per HashTable
  struct node_tag* first_child; //Contiene il primo figlio per scorrere tutti i figli come lista
  struct node_tag* list_next;        //Contiene il prossimo elemento su questo livello
  struct node_tag* list_prev;        //Contiene il prossimo elemento su questo livello
  struct node_tag* hash_next;   //Contiene in prossimo elemento con lo stesso hash (Chaining)
  struct node_tag* hash_prev;   //Contiene in prossimo elemento con lo stesso hash (Chaining)
  struct node_tag** hash_table; //Istanziata a dimensione _FS_HASH_BUCKETS_ al primo inserimento di un figlio

  //Parametri comuni
  int key;
  bool isDir;
  int childs;
  int depth;
  char* name;
  //char* path; //Utile per riordinare
  int path_len;
  int name_len;
  char* content;
} node_t;

//Interfaccia per i comandi da console
bool fs_create(node_t* root, char** path, int* key, int* len, bool isDir);
char* fs_read(node_t* root, char** path, int* key);
int fs_write(node_t* root, char** path, int* key, char* content, int contentLen);
bool fs_delete(node_t* root, char** path, int* key, bool recursive);
node_t** fs_find(node_t* root, char* name, int key, node_t** items, int* count); //NULL come ultimo elemento

//Metodi utility
int fs_key(char* name);
int fs_partial_key(int currentKey, int currentLen, char c);
int fs_key_length(int key);
//int fs_hash(int key, int buckets);
char* fs_calculate_path(node_t* node);

//Metodi hash
node_t* fs_hash_next_node(node_t* root, char* name, int* key);

//Sort
void fs_mergesort(char** items, int left, int right);
void fs_merge(char** items, int left, int center, int right);
#endif
