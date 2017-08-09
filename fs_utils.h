#include <stdbool.h>

#ifndef _FS_UTILS_
#define _FS_UTILS_

#define _FS_MAX_DEPTH_ 255
#define _FS_MAX_CHILDS_ 1024
#define _MAX_CHAR_COMBINATIONS_ 62

//Error Codes
#define _FS_ITEM_ALREADY_EXISTS_ -1
#define _FS_HASH_WRONG_CHARS_ -2
#define _FS_HASH_EMPTY_ -1

typedef enum rb_color { RED, BLACK } rb_color_t;

typedef struct node_tag
{
  //Accesso come lista (Struttura ad Albero)
  struct node_tag* fs_parent;

  //Organizzazione interna alla Directory ad Albero RB
  struct node_tag* rb_parent;
  struct node_tag* rb_left;
  struct node_tag* rb_right;
  rb_color_t rb_color; //1 Red, 0 Black
  int rb_hash;

  //Parametri comuni
  bool isDir;
  int childs;
  int depth;
  char* name;
  char* percorso; //Utile per riordinare
  char* content;
} node_t;

//Interfaccia per i comandi da console
bool fs_create(node_t* root, char* percorso, bool is_dir);
char* fs_read(node_t* root, char* percorso);
int fs_write(node_t* root, char* percorso, char* contenuto);
bool fs_delete(node_t* root, char* percorso, bool recursive);
node_t* fs_find(node_t* root, char* name); //NULL come ultimo elemento

//Metodi utility per BST-RB
int fs_hash(char* percorso);
int fs_hash_length(int hash);
void fs_bst_rotate(node_t** root, node_t* node, bool left);
int fs_rb_insert(node_t** root, node_t* node);
void fs_rb_insert_fixup(node_t** root, node_t* node);
#endif
