#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "debug.h"
#include "local_random.h"
#include "fs_utils.h"

char** fs_parse_path(char* path)
{
  //Gestione errori, impongo che ogni percorso parta dalla root
  int pathLen = strlen(path);
  if(pathLen == 0 || path[0] != '/')
    return NULL;
  //Ignoro il primo carattere (Avevo un free inutile)
  //memmove(path, path+1, pathLen);
  //path += 1;

  char** path_arr = (char**)malloc(sizeof(char*));
  path_arr[0] = NULL; //Imposto il primo elemento a null

  char *token;
  int i = 0;

  token = strtok(path, "/");
  while( token != NULL )
  {
    path_arr[i] = token;
    token = strtok(NULL, "/");
    i++;
    realloc(path_arr, (i+1) * sizeof(char*));
  }

  if(i == 0)
  {
    free(path_arr);
    return NULL;
  }

  debug_print("[DEBUG] Parsing della path effettuato, profondita rilevata: %d\n", i);

  path_arr[i] = NULL; //Imposto l'ultimo elemento a null
  return path_arr;
}

bool fs_create(node_t* root, char** path, bool isDir)
{
  if(root->depth + 1 > _FS_MAX_DEPTH_)
  {
    debug_print("[DEBUG] Max depth reached!\n");
    return false; //_FS_MAX_DEPTH_REACHED_
  }
  if(*path == NULL)
  {
    //Esiste già il file/dir che volevo creare
    return false; //_FS_FILE_ALREADY_EXISTS_
  }

  int key = fs_key(*path);
  if(key < 0)
    return false;
  int hash = fs_hash(key, _FS_HASH_BUCKETS_);

  debug_print("[DEBUG] Calcolato key per %s = %d\n", *path, key);

  //Sono all'ultimo nodo, devo inserire l'elemento se non già esistente
  if(*(path + 1) == NULL )//|| *(path + 1)[0] == 0) //Investigare sul secondo check
  {
    //Non sono in una directory
    if(!root->isDir)
      return false;

    //Sono all'elemento finale, devo aggiungere qua il file
    if(root->childs +1  > _FS_MAX_CHILDS_)
      return false; //_FS_MAX_CHILDS_REACHED_


    debug_print("[DEBUG] Directory di appartenenza trovata, creazione del nodo in corso...\n");

    //Creo il nodo e sistemo i suoi parametri (sistemerò il parente dopo la tentata creazione nel BST della directory)
    node_t* node = (node_t*)malloc(sizeof(node_t));
    node->fs_parent = root;
    node->isDir = isDir;
    node->childs = 0;
    //node->rb_color = BLACK;
    node->depth = root->depth + 1;
    node->key = key;

    int lenName = strlen(*path) + 1;
    int lenPath = (lenName + (root->path != NULL ? strlen(root->path) : 0) + 1);
    debug_print("[DEBUG] Parametri di base impostati, scrivo i contenuti delle stringhe di dimansione: %d - %d...\n", (int)(lenName * sizeof(char)), (int)(lenPath * sizeof(char)));

    //Alloco lo spazio per le stringhe
    node->name = (char*)malloc(lenName * sizeof(char));
    node->path = (char*)malloc(lenPath * sizeof(char));
    node->content = NULL;
    memset(node->name, 0, lenName + 1);
    memset(node->path, 0, lenPath + 1);

    node->name = strcpy(node->name, *path);
    debug_print("[DEBUG] Nome nodo impostato a %s\n", node->name);
    if(root->path != NULL)
    {
      node->path = strcpy(node->path, root->path);
      strcat(node->path, "/");
    }
    else
      strcpy(node->path, "/");
    strcat(node->path, *path);
    debug_print("[DEBUG] Path nodo impostato a %s\n", node->path);

    /*debug_print("[DEBUG] Parametri nodo scritti, inserimento nell'albero RB della directory...\n");

    int result = fs_rb_insert(&root->rb_root, node);
    if(result < 0) //Errore di inserimento
    {
      debug_print("[DEBUG] Impossibile inserire il file nel BST-RB, file già esistente!\n");

      free(node);
      return false;
    }
    */
    debug_print("[DEBUG] Inserisco il nodo nella hash table del livello...\n");
    if(root->childs == 0)
    {
      //Creo la tabella
      debug_print("[DEBUG] Hash table del livello non ancora inizializzata\n");
      root->hash_table = (node_t**)malloc(_FS_HASH_BUCKETS_ * sizeof(node_t*));
      memset(root->hash_table, 0, _FS_HASH_BUCKETS_ * sizeof(node_t*));
    }

    //Cerco se esiste un elemento con lo stesso nome
    debug_print("[DEBUG] Cerco se l'elemento è un doppione\n");
    node_t* hash_spot = root->hash_table[hash];
    while(hash_spot != NULL && hash_spot->key != key) //Scorro le collisioni
      hash_spot = hash_spot->hash_next;

    if(hash_spot != NULL)
      return false; //Elemento già esistente

    debug_print("[DEBUG] Inserisco l'elemto nell'hashtable ed aggiorno il padre\n");
    //Aggiorno l'hash table con il nuovo nodo
    node->hash_next = root->hash_table[hash];
    root->hash_table[hash] = node;


    //Imposto i bambini
    root->childs++;
    node->next = root->first_child;
    root->first_child = node;

    return true;
  }

  node_t* next = fs_hash_next_node(root, key);

  if(next == NULL)
    return false;

  debug_print("[DEBUG] Procedo al prossimo nodo\n");
  return fs_create(next, path + 1, isDir);
}

//Metodo per scrivere nei file
int fs_write(node_t* root, char** path, char* content)
{
  if(*path == NULL && !root->isDir)
  {
    debug_print("[DEBUG] Ultimo nodo, controllo se devo liberare il vecchio contenuto\n");
    //L'elemento è stato trovato, si trova in root
    if(root->content != NULL)
      free(root->content);

    int contentLen = strlen(content);
    debug_print("[DEBUG] Scrivo %d byte di nuovo contenuto\n", (int)((contentLen + 1) * sizeof(char)));
    root->content = (char*)malloc((contentLen + 1) * sizeof(char));
    strcpy(root->content, content);
    return contentLen; //Effettuo la scrittura
  }
  else if(path == NULL)
    return -1;

  //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
  node_t* next = fs_hash_next_node(root, fs_key(*path));

  if(next == NULL)
    return -1;

  debug_print("[DEBUG] Procedo al prossimo nodo\n");
  return fs_write(next, path + 1, content);
}

char* fs_read(node_t* root, char** path)
{
  if(*path == NULL)
    return root->content;

  //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
  node_t* next = fs_hash_next_node(root, fs_key(*path));

  if(next == NULL)
    return NULL;

  debug_print("[DEBUG] Procedo al prossimo nodo\n");
  return fs_read(next, path + 1);
}

//Non ritorno la lunghezza perchè posso facilmente ricavarla facendo ceil((double)(key+1)/_MAX_CHAR_COMBINATIONS_);
int fs_key(char* name)
{
  int key = 0;
  int len = 0;
  int sChar = 0;
  while(*name != '\0' && *name != '/')
  {
    sChar = *name++;
    if(sChar > 47 && sChar < 58) //0-9 -> 0-9
    {
      sChar -= 48;
    }
    else if(sChar > 64 && sChar < 91) // A-Z -> 10-35
    {
      sChar -= 55;
    }
    else if(sChar > 96 && sChar < 123)
    {
      sChar -= 61;
    }
    else
      return _FS_KEY_WRONG_CHARS_; //Caratteri inaspettati nel name
    key = sChar + (len++ * _MAX_CHAR_COMBINATIONS_);
  }

  //Se il ciclo non è mai stato eseguito
  if(len == 0)
    key = _FS_KEY_EMPTY_;

  return key;
}

//Metodo comodo per eseguire il calcolo della lunghezza rispetto alla key
int fs_key_length(int key)
{
  return ceil((double)(key+1)/_MAX_CHAR_COMBINATIONS_);
}

//Metodo per calcolare un'hash
int fs_hash(int key, int buckets)
{
  return local_rand(key, buckets);
}

//Metodo per ricercare un elemento per chiave in una tabella hash
node_t* fs_hash_next_node(node_t* root, int key)
{
  debug_print("[DEBUG][FIND-NEXT] Controllo che ci siano figli\n");
  //Sono in un nodo intermedio, devo ricercare
  if(root->childs == 0) // Questo livello non ha più sottofigli, il percorso deve essere errato
    return NULL;

  if(key < 0)
    return NULL;

  debug_print("[DEBUG] Trovo il nodo con il nome che mi serve\n");
  node_t* next = root->hash_table[fs_hash(key, _FS_HASH_BUCKETS_)]; // = fs_bst_find_node(root->rb_root, key);
  while(next != NULL && next->key != key) //Trovo il nodo che mi serve
    next = next->hash_next;

  return next;
}

//Metodi per effettuare le rotazioni negli alberi binari
void fs_bst_rotate(node_t** root, node_t* node, bool left)
{
  node_t* y = left ? node->rb_right : node->rb_left;

  //Effettua la rotazione
  if(left)
  {
    debug_print("[DEBUG][ROTATE] %s left\n", node->name);
    //Left Rotate
    node->rb_right = y->rb_left;

    if(y->rb_left != NULL)
      y->rb_left->rb_parent = node;
    y->rb_parent = node->rb_parent;
  }
  else
  {
    debug_print("[DEBUG][ROTATE] %s right\n", node->name);
    //Right Rotate
    node->rb_left = y->rb_right;

    if(y->rb_right != NULL)
      y->rb_right->rb_parent = node;
    y->rb_parent = node->rb_parent;
  }

  //Sistema Root e Nodi sovrastanti
  if(node->rb_parent == NULL)
    *root = y;
  else if(node == node->rb_parent->rb_left)
    node->rb_parent->rb_left = y;
  else
    node->rb_parent->rb_right = y;

  //Imposta Node come figlio opportuno del pivot
  if(left)
    y->rb_left = node;
  else
    y->rb_right = node;

  node->rb_parent = y;
}

//Metodo per effettuare la ricerca nei BST
node_t* fs_bst_find_node(node_t* root, int key)
{
  if(root == NULL)
    return NULL;

  if(root->key == key)
    return root;
  else
    return root->key > key ? fs_bst_find_node(root->rb_left, key) : fs_bst_find_node(root->rb_right, key);
}

int fs_rb_insert(node_t** root, node_t* node)
{
  node_t* y = NULL;
  node_t* x = *root;

  debug_print("[DEBUG] Inserimento BST-RB iniziato...\n");

  //Cerca la posizione dove inserire il nodo
  while(x != NULL)
  {
    y = x;
    if(node->key < x->key)
      x = x->rb_left;
    else if(node->key > x->key)
      x = x->rb_right;
    else
      return _FS_ITEM_ALREADY_EXISTS_;
  }


  debug_print("[DEBUG] Trovato parente, so che il nodo e': %s\n", y == NULL ? "root" : node->key < y->key ? "figlio sinistro" : "figlio destro/uguale");

  node->rb_parent = y;
  if(y == NULL)
    *root = node;
  else if(node->key < y->key)
    y->rb_left = node;
  else if(node->key > y->key)
    y->rb_right = node;
  else
    return _FS_ITEM_ALREADY_EXISTS_;

  node->rb_left = NULL;
  node->rb_right = NULL;
  node->rb_color = RED;
  fs_rb_insert_fixup(root, node);
  return 0;
}

void fs_rb_insert_fixup(node_t** root, node_t* node)
{
  debug_print("[DEBUG][FIX-UP] Inizio fix-up per %s...\n", node->name);
  if(node == *root)
    (*root)->rb_color = BLACK;
  else
  {
    debug_print("[DEBUG][FIX-UP] Nodo non root, con root %s...\n", (*root)->name);
    node_t* x = node->rb_parent;
    if(x->rb_color == RED)
    {
      if(x == x->rb_parent->rb_left) //Se x è figlio sinistro
      {
        debug_print("[DEBUG][FIX-UP] x è sinistro\n");
        node_t* y = x->rb_parent->rb_right;
        if(y->rb_color == RED)
        {
          debug_print("[DEBUG][FIX-UP] caso 1\n");
          x->rb_color = BLACK;                        //Caso 1
          y->rb_color = BLACK;                        //Caso 1
          x->rb_parent->rb_color = RED;               //Caso 1
          fs_rb_insert_fixup(root, x->rb_parent);     //Caso 1
        }
        else
        {
          if(node == x->rb_right)
          {
            debug_print("[DEBUG][FIX-UP] caso 2\n");
            node = x;                                       //Caso 2
            fs_bst_rotate(root, node, true); //Rotate Left  //Caso 2
            x = node->rb_parent;                            //Caso 2
          }
          debug_print("[DEBUG][FIX-UP] caso 3\n");
          x->rb_color = BLACK;                                      //Caso 3
          x->rb_parent->rb_color = RED;                             //Caso 3
          fs_bst_rotate(root, x->rb_parent, false); //Rotate Right  //Caso 3
        }
      }
      else
      {
        debug_print("[DEBUG][FIX-UP] x è destro\n");
        node_t* y = x->rb_parent->rb_left;
        if(y->rb_color == RED)
        {
          debug_print("[DEBUG][FIX-UP] caso 1\n");
          x->rb_color = BLACK;                        //Caso 1
          y->rb_color = BLACK;                        //Caso 1
          x->rb_parent->rb_color = RED;               //Caso 1
          fs_rb_insert_fixup(root, x->rb_parent);     //Caso 1
        }
        else
        {
          if(node == x->rb_left)
          {
            debug_print("[DEBUG][FIX-UP] caso 2\n");
            node = x;                                         //Caso 2
            fs_bst_rotate(root, node, false); //Rotate Right  //Caso 2
            x = node->rb_parent;                              //Caso 2
          }
          debug_print("[DEBUG][FIX-UP] caso 3\n");
          x->rb_color = BLACK;                                    //Caso 3
          x->rb_parent->rb_color = RED;                           //Caso 3
          fs_bst_rotate(root, x->rb_parent, true); //Rotate Left  //Caso 3
        }
      }
    }
  }
}
