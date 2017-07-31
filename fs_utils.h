#ifndef _FS_UTILS_
#define _FS_UTILS_

#define _MAX_CHAR_COMBINATIONS_ 62

#define _FS_HASH_WRONG_CHARS_ -2
#define _FS_HASH_EMPTY_ -1

typedef struct node_tag
{
  //Accesso come lista (Struttura ad Albero)
  struct node_tag* fs_parent;
  struct node_tag** fs_childs; //Access as List

  //Organizzazione interna alla Directory ad Albero RB
  struct node_tag* rb_parent;
  struct node_tag* rb_left;
  struct node_tag* rb_right;
  char color; //1 Red, 0 Black

  //Parametri comuni
  char* name;
  char* percorso; //Utile per riordinare
  int rb_hash;
  char* content;
} node_t;

int fs_write(node_t* root, char* percorso, char* contenuto);

int fs_hash(char* percorso);
int fs_hash_length(int hash);
#endif
