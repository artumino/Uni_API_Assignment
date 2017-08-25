#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "debug.h"
#include "local_random.h"
#include "fs_utils.h"

bool fs_create(node_t* root, char** path, int* key, int* len, bool isDir)
{
  if(root->depth + 1 > _FS_MAX_DEPTH_)
  {
    debug_print("[DEBUG] Max depth reached!\n");
    return false; //_FS_MAX_DEPTH_REACHED_
  }
  if(*path == NULL)
    return false; //_FS_FILE_ALREADY_EXISTS_ Esiste già il file/dir che volevo creare

  if(*key < 0)
    return false;
  int hash = fs_hash(*key, _FS_HASH_BUCKETS_);

  debug_print("[DEBUG] Calcolato key per %s = %d\n", *path, *key);

  //Sono all'ultimo nodo, devo inserire l'elemento se non già esistente
  if(*(path + 1) == NULL ) //Investigare sul secondo check
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
    node->first_child = NULL;
    node->hash_table = NULL;
    node->content = NULL;
    //node->rb_color = BLACK;
    node->depth = root->depth + 1;
    node->key = *key;

    int lenName = *len + 1;
    node->name_len = *len;
    int lenPath = root->path_len + lenName + 1;
    node->path_len = lenPath - 1;
    debug_print("[DEBUG] Parametri di base impostati, scrivo i contenuti delle stringhe di dimensione: %d - %d...\n", (int)(lenName * sizeof(char)), (int)(lenPath * sizeof(char)));

    //Alloco lo spazio per le stringhe
    node->name = (char*)malloc(lenName * sizeof(char));
    //node->path = (char*)malloc(lenPath * sizeof(char));
    //memset(node->name, 0, lenName * sizeof(char));
    //memset(node->path, 0, lenPath * sizeof(char));

    if(!isDir)
    {
      debug_print("[DEBUG] Creo contenuto per file vuoti\n");
      node->content = (char*)malloc(2*sizeof(char));
      *(node->content) = '\0';
      node->content++;
      *(node->content) = '\0';
    }

    //node->name = strcpy(node->name, *path);
    memcpy(node->name, *path, lenName * sizeof(char));
    /*debug_print("[DEBUG] Nome nodo impostato a %s\n", node->name);
    if(root->path_len > 0)
    {
      //node->path = strcpy(node->path, root->path);
      memcpy(node->path, root->path, root->path_len * sizeof(char));
      node->path[root->path_len] = '/';
      //strcat(node->path, "/");
    }
    else
      node->path[0] = '/';
      //strcpy(node->path, "/");

    memcpy(node->path + root->path_len + 1, *path, lenName);
    //strcat(node->path, *path);
    debug_print("[DEBUG] Path nodo impostato a %s\n", node->path);*/

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
    while(hash_spot != NULL && (hash_spot->key != *key || strcmp(hash_spot->name, *path))) //Scorro le collisioni (Effettuo la comparazione carattere per carattere solo se ho le stesse chiavi)
      hash_spot = hash_spot->hash_next;

    if(hash_spot != NULL)
      return false; //Elemento già esistente

    debug_print("[DEBUG] Inserisco l'elemento nell'hashtable\n");
    //Aggiorno l'hash table con il nuovo nodo
    node->hash_next = root->hash_table[hash];
    if(node->hash_next != NULL)
      node->hash_next->hash_prev = node;
    node->hash_prev = NULL;
    root->hash_table[hash] = node;


    debug_print("[DEBUG] Aggiorno il padre e la struttura dei figli\n");
    //Imposto i bambini
    root->childs++;
    node->list_next = root->first_child;
    if(node->list_next != NULL)
      node->list_next->list_prev = node;
    node->list_prev = NULL;
    root->first_child = node;

    return true;
  }

  node_t* next = fs_hash_next_node(root, *path, key);

  if(next == NULL)
    return false;

  debug_print("[DEBUG] Procedo al prossimo nodo\n");
  return fs_create(next, path + 1, key + 1, len + 1, isDir);
}

//Metodo per scrivere nei file
int fs_write(node_t* root, char** path, int* key, char* content, int contentLen)
{
  if(*path == NULL && !root->isDir)
  {
    debug_print("[DEBUG] Ultimo nodo, controllo se devo liberare il vecchio contenuto\n");
    //L'elemento è stato trovato, si trova in root
    if(root->content != NULL)
      free(root->content - 1);

    debug_print("[DEBUG] Scrivo %d byte di nuovo contenuto\n", (int)((contentLen + 1) * sizeof(char)));
    root->content = contentLen > 0 ? content : (char*)malloc(2*sizeof(char));

    if(contentLen == 0)
    {
      root->content[0] = '\0';
      root->content[1] = '\0';
      root->content++;
    }

    return contentLen; //Effettuo la scrittura
  }
  else if(path == NULL)
    return -1;

  //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
  node_t* next = fs_hash_next_node(root, *path, key);

  if(next == NULL)
    return -1;

  debug_print("[DEBUG] Procedo al prossimo nodo\n");
  return fs_write(next, path + 1, key + 1, content, contentLen);
}

char* fs_read(node_t* root, char** path, int* key)
{
  if(*path == NULL)
    return root->content;

  //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
  node_t* next = fs_hash_next_node(root, *path, key);

  if(next == NULL)
    return NULL;

  debug_print("[DEBUG] Procedo al prossimo nodo\n");
  return fs_read(next, path + 1, key + 1);
}

bool fs_delete(node_t* root, char** path, int* key, bool recursive)
{
  if(*path == NULL)
  {
    //Mi trovo nel nodo da cancellare
    if(recursive)
    {
      while(root->first_child != NULL)
        fs_delete(root->first_child, path, key, true);
    }

    //Se ho figli, errore (se è ricorsivo sono sicuro che li ho cancellati tutti)
    if(root->childs > 0)
      return false;

    //Sistemo la tabella hash
    if(root->hash_prev == NULL) //Sono la prima entry della tabella hash
      root->fs_parent->hash_table[fs_hash(root->key, _FS_HASH_BUCKETS_)] = root->hash_next;

    if(root->hash_prev != NULL)
      root->hash_prev->hash_next = root->hash_next;

    if(root->hash_next != NULL)
      root->hash_next->hash_prev = root->hash_prev;

    //Sistemo la lista dei figli ordinata
    if(root->list_prev == NULL)
      root->fs_parent->first_child = root->list_next;

    if(root->list_prev != NULL)
      root->list_prev->list_next = root->list_next;

    if(root->list_next != NULL)
      root->list_next->list_prev = root->list_prev;

    root->fs_parent->childs--;
    if(root->content-- != NULL)
      free(root->content);
    //if(root->path != NULL)
    //  free(root->path);
    if(root->name != NULL)
      free(root->name);
    if(root->hash_table != NULL)
      free(root->hash_table);
    free(root);

    return true;
  }

  //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
  node_t* next = fs_hash_next_node(root, *path, key);

  if(next == NULL)
    return false;

  debug_print("[DEBUG] Procedo al prossimo nodo\n");
  return fs_delete(next, path + 1, key + 1, recursive);
}

//Metodo per la ricerca di un elemento
node_t** fs_find(node_t* root, char* name, int key, node_t** items, int* count)
{
  //Si porta dietro la lista elementi, sennò la crea
  if(items == NULL || count == NULL)
  {
    debug_print("[DEBUG] Lista elementi non ancora definita, inizializzo la lista\n");
    if(items == NULL)
      items  = (node_t**)malloc(sizeof(node_t*));

    if(count == NULL)
      count = (int*)malloc(sizeof(int));

    //Mi assicuro che la memoria sia azzerata
    items[0] = NULL;
    *count = 0;
  }

  //Se ho concluso con un nodo
  debug_print("[DEBUG] Controllo se ho concluso con il nodo\n");
  if(root == NULL)
    return items;

  if(key == root->key && !strcmp(root->name, name))
  {
    debug_print("[DEBUG] Match trovato, aggiungo elemento alla lista dei risultati per un totale di %d elementi\n", (*count) + 1);
    items[(*count)++] = root;
    items = (node_t**)realloc(items, ((*count) + 1) * sizeof(node_t*));
    items[*count] = NULL;
  }

  debug_print("[DEBUG] Espando la ricerca ai figli\n");
  items = fs_find(root->first_child, name, key, items, count);

  debug_print("[DEBUG] Procedo al prossimo sottofiglio, coetaneo di %s\n", root->name);
  return fs_find(root->list_next, name, key, items, count);
}

//Non ritorno la lunghezza perchè posso facilmente ricavarla facendo ceil((double)(key+1)/_MAX_CHAR_COMBINATIONS_);
int fs_key(char* name)
{
  int key = 0;
  int len = 0;
  int sChar = 0;
  while(*name != '\0' && *name != '/')
  {
    sChar = *name;
    name++;
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
    key += sChar + (len++ * _MAX_CHAR_COMBINATIONS_);
  }

  //Se il ciclo non è mai stato eseguito
  if(len == 0)
    key = _FS_KEY_EMPTY_;

  return key;
}

//Metodo usato per calcolare la chiave carattere per carattere
int fs_partial_key(int currentKey, int currentLen, char c)
{
  if(currentKey < _FS_KEY_EMPTY_)
    return currentKey;

  if(c > 47 && c < 58) //0-9 -> 0-9
    c -= 48;
  else if(c > 64 && c < 91) // A-Z -> 10-35
    c -= 55;
  else if(c > 96 && c < 123)
    c -= 61;
  else
    return _FS_KEY_WRONG_CHARS_; //Caratteri inaspettati nel name
  return (currentKey < 0 ? 0 : currentKey) + ((int)c + (currentLen * _MAX_CHAR_COMBINATIONS_));
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

char* fs_calculate_path(node_t* node)
{
  node_t* currentNode = node;
  debug_print("Creo path di dimensione %d per elemento %s\n", node->path_len + 1, node->name);
  char* path = (char*)malloc((node->path_len + 1) * sizeof(char));
  int currentIndex = node->path_len;
  path[currentIndex--] = '\0';
  path[currentIndex] = '!';
  while(currentNode->fs_parent != NULL)
  {
    currentIndex -= (currentNode->name_len - 1);
    memcpy(path + (currentIndex--), currentNode->name, currentNode->name_len * sizeof(char));
    path[currentIndex--] = '/';
    currentNode = currentNode->fs_parent;
  }

  return path;
}

//Metodo per ricercare un elemento per chiave in una tabella hash
node_t* fs_hash_next_node(node_t* root, char* name, int* key)
{
  debug_print("[DEBUG][FIND-NEXT] Controllo che ci siano figli\n");
  //Sono in un nodo intermedio, devo ricercare
  if(root->childs == 0) // Questo livello non ha più sottofigli, il percorso deve essere errato
    return NULL;

  if(*key < 0)
    return NULL;

  debug_print("[DEBUG] Trovo il nodo con il nome che mi serve\n");
  node_t* next = root->hash_table[fs_hash(*key, _FS_HASH_BUCKETS_)]; // = fs_bst_find_node(root->rb_root, key);
  while(next != NULL && (next->key != *key|| strcmp(next->name, name))) //Trovo il nodo che mi serve
    next = next->hash_next;

  return next;
}

void fs_mergesort(char** items, int left, int right)
{
  if(left < right)
  {
    int center = (left + right) / 2;
    fs_mergesort(items, left, center);
    fs_mergesort(items, center + 1, right);
    fs_merge(items, left, center, right);
  }
}

void fs_merge(char** items, int left, int center, int right)
{
  int i = left;
  int j = center + 1;
  int k = 0;
  char* queue[right - left - 1];

  //Esegue il confronto
  while(i <= center && j <= right)
  {
    if(strcmp(items[i], items[j]) <= 0)
    {
      queue[k] = items[i];
      i++;
    }
    else
    {
      queue[k] = items[j];
      j++;
    }
    k++;
  }

  while(i <= center)
    queue[k++] = items[i++];

  while(j <= right)
    queue[k++] = items[j++];

  for(k = left; k <= right; k++)
    items[k] = queue[k-left];
}
