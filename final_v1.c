#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_PARAMS 3

#define _FS_MAX_DEPTH_ 255
#define _FS_MAX_CHILDS_ 1024
#define _FS_HASH_BUCKETS_ 1259
#define _MAX_CHAR_COMBINATIONS_ 62

//Error Codes
#define _FS_ITEM_ALREADY_EXISTS_ -1
#define _FS_MAX_DEPTH_REACHED_ -1
#define _FS_MAX_CHILDS_REACHED_ -2
#define _FS_FILE_ALREADY_EXISTS_ -3
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
  char* path; //Utile per riordinare
  char* content;
} node_t;

int local_rand(unsigned int seed, unsigned int max)  // RAND_MAX assumed to be 32767
{
    return (unsigned int)((seed * 1103515245 + 12345)/65536) % max;
}

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
    //path_arr[i] = (char*)malloc((strlen(token) + 1) * sizeof(char));
    //strcpy(path_arr[i], token);
    token = strtok(NULL, "/");
    i++;
    path_arr = (char**)realloc(path_arr, (i+1) * sizeof(char*));
  }

  if(i == 0)
  {
    free(path_arr);
    return NULL;
  }

  path_arr[i] = NULL; //Imposto l'ultimo elemento a null
  return path_arr;
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
node_t* fs_hash_next_node(node_t* root, char* name)
{
  //Sono in un nodo intermedio, devo ricercare
  if(root->childs == 0) // Questo livello non ha più sottofigli, il percorso deve essere errato
    return NULL;

  int key = fs_key(name);

  if(key < 0)
    return NULL;

  node_t* next = root->hash_table[fs_hash(key, _FS_HASH_BUCKETS_)]; // = fs_bst_find_node(root->rb_root, key);
  while(next != NULL && (next->key != key|| strcmp(next->name, name))) //Trovo il nodo che mi serve
    next = next->hash_next;

  return next;
}

bool fs_create(node_t* root, char** path, bool isDir)
{
  if(root->depth + 1 > _FS_MAX_DEPTH_)
    return false; //_FS_MAX_DEPTH_REACHED_

  if(*path == NULL)
    return false; //_FS_FILE_ALREADY_EXISTS_ Esiste già il file/dir che volevo creare

  int key = fs_key(*path);
  if(key < 0)
    return false;
  int hash = fs_hash(key, _FS_HASH_BUCKETS_);

  //Sono all'ultimo nodo, devo inserire l'elemento se non già esistente
  if(*(path + 1) == NULL ) //Investigare sul secondo check
  {
    //Non sono in una directory
    if(!root->isDir)
      return false;

    //Sono all'elemento finale, devo aggiungere qua il file
    if(root->childs +1  > _FS_MAX_CHILDS_)
      return false; //_FS_MAX_CHILDS_REACHED_

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
    node->key = key;

    int lenName = strlen(*path) + 1;
    int lenPath = (lenName + (root->path != NULL ? strlen(root->path) : 0) + 1);

    //Alloco lo spazio per le stringhe
    node->name = (char*)malloc(lenName * sizeof(char));
    node->path = (char*)malloc(lenPath * sizeof(char));
    memset(node->name, 0, lenName * sizeof(char));
    memset(node->path, 0, lenPath * sizeof(char));

    if(!isDir)
    {
      node->content = (char*)malloc(sizeof(char));
      *(node->content) = '\0';
    }

    node->name = strcpy(node->name, *path);
    if(root->path != NULL)
    {
      node->path = strcpy(node->path, root->path);
      strcat(node->path, "/");
    }
    else
      strcpy(node->path, "/");
    strcat(node->path, *path);

    if(root->childs == 0)
    {
      //Creo la tabella
      root->hash_table = (node_t**)malloc(_FS_HASH_BUCKETS_ * sizeof(node_t*));
      memset(root->hash_table, 0, _FS_HASH_BUCKETS_ * sizeof(node_t*));
    }

    //Cerco se esiste un elemento con lo stesso nome
    node_t* hash_spot = root->hash_table[hash];
    while(hash_spot != NULL && (hash_spot->key != key || strcmp(hash_spot->name, *path))) //Scorro le collisioni (Effettuo la comparazione carattere per carattere solo se ho le stesse chiavi)
      hash_spot = hash_spot->hash_next;

    if(hash_spot != NULL)
      return false; //Elemento già esistente

    //Aggiorno l'hash table con il nuovo nodo
    node->hash_next = root->hash_table[hash];
    if(node->hash_next != NULL)
      node->hash_next->hash_prev = node;
    node->hash_prev = NULL;
    root->hash_table[hash] = node;


    //Imposto i bambini
    root->childs++;
    node->list_next = root->first_child;
    if(node->list_next != NULL)
      node->list_next->list_prev = node;
    node->list_prev = NULL;
    root->first_child = node;

    return true;
  }

  node_t* next = fs_hash_next_node(root, *path);

  if(next == NULL)
    return false;

  return fs_create(next, path + 1, isDir);
}

//Metodo per scrivere nei file
int fs_write(node_t* root, char** path, char* content)
{
  if(*path == NULL && !root->isDir)
  {
    //L'elemento è stato trovato, si trova in root
    if(root->content != NULL)
      free(root->content);

    int contentLen = strlen(content);
    root->content = (char*)malloc((contentLen + 1) * sizeof(char));
    strcpy(root->content, content);
    return contentLen; //Effettuo la scrittura
  }
  else if(path == NULL)
    return -1;

  //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
  node_t* next = fs_hash_next_node(root, *path);

  if(next == NULL)
    return -1;

  return fs_write(next, path + 1, content);
}

char* fs_read(node_t* root, char** path)
{
  if(*path == NULL)
    return root->content;

  //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
  node_t* next = fs_hash_next_node(root, *path);

  if(next == NULL)
    return NULL;

  return fs_read(next, path + 1);
}

bool fs_delete(node_t* root, char** path, bool recursive)
{
  if(*path == NULL)
  {
    //Mi trovo nel nodo da cancellare
    if(recursive)
    {
      while(root->first_child != NULL)
        fs_delete(root->first_child, path, true);
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
    if(root->content != NULL)
      free(root->content);
    if(root->path != NULL)
      free(root->path);
    if(root->name != NULL)
      free(root->name);
    if(root->hash_table != NULL)
      free(root->hash_table);
    free(root);

    return true;
  }

  //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
  node_t* next = fs_hash_next_node(root, *path);

  if(next == NULL)
    return false;

  return fs_delete(next, path + 1, recursive);
}

//Metodo per la ricerca di un elemento
node_t** fs_find(node_t* root, char* name, int key, node_t** items, int* count)
{
  //Si porta dietro la lista elementi, sennò la crea
  if(items == NULL || count == NULL)
  {
    if(items == NULL)
      items  = (node_t**)malloc(sizeof(node_t*));

    if(count == NULL)
      count = (int*)malloc(sizeof(int));

    //Mi assicuro che la memoria sia azzerata
    items[0] = NULL;
    *count = 0;
  }

  //Se ho concluso con un nodo
  if(root == NULL)
    return items;

  if(key == root->key && !strcmp(root->name, name))
  {
    items[(*count)++] = root;
    items = (node_t**)realloc(items, ((*count) + 1) * sizeof(node_t*));
    items[*count] = NULL;
  }

  items = fs_find(root->first_child, name, key, items, count);

  return fs_find(root->list_next, name, key, items, count);
}


void fs_merge(node_t** items, int left, int center, int right)
{
  int i = left;
  int j = center + 1;
  int k = 0;
  node_t* queue[right - left - 1];

  //Esegue il confronto
  while(i <= center && j <= right)
  {
    if(strcmp(items[i]->path, items[j]->path) <= 0)
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

void fs_mergesort(node_t** items, int left, int right)
{
  if(left < right)
  {
    int center = (left + right) / 2;
    fs_mergesort(items, left, center);
    fs_mergesort(items, center + 1, right);
    fs_merge(items, left, center, right);
  }
}


static node_t root;
int readCommand(char** command)
{
  char* str = (char*)malloc(sizeof(char));
  int c;
  int len = 0;
  int clen = 0;
  while((c = getchar()) != '\n' && c != EOF)
  {
    //Se non ho uno spazio tra caratteri leggo la stringa
    if(c != ' ' || clen >= MAX_PARAMS - 1)
    {
      str[len++] = c;
      str = (char*)realloc(str, (len + 1) * sizeof(char));
    }
    else if(len > 0)
    {
      str[len] = '\0';
      command[clen++] = str;
      str = (char*)malloc(sizeof(char)); //Reinstazio una nuova stringa, pronta per il prossimo parametro
      len = 0;
    }
  }

  //Prendo l'ultimo parametro prima di EOF
  if(clen < MAX_PARAMS)
  {
    str[len] = '\0';
    command[clen++] = str;
  }

  return clen;
}

void parseCommand(char** command, int count, node_t* root)
{
  char** path = NULL;
  if(count < 1)
    return;

  //create <percorso_file>
  if(!strcmp(command[0], "create"))
  {
    if(count != 2)
    {
      printf("no\n");
      return;
    }

    path = fs_parse_path(command[1]);

    if(path != NULL)
      printf("%s\n", fs_create(root, path, false) ? "ok" : "no");
    else printf("no\n");
  }

  //create_dir <percorso_dir>
  if(!strcmp(command[0], "create_dir"))
  {
    if(count != 2)
    {
      printf("no\n");
      return;
    }

    path = fs_parse_path(command[1]);
    if(path != NULL)
      printf("%s\n", fs_create(root, path, true) ? "ok" : "no");
    else printf("no\n");
  }

  //write <percorso_file> <contenuto>
  if(!strcmp(command[0], "write"))
  {
    if(count != 3)
    {
      printf("no\n");
      return;
    }

    path = fs_parse_path(command[1]);
    if(path != NULL)
    {
      int fullLen = strlen(command[2]);
      if(fullLen < 2 || command[2][0] != '"' || command[2][fullLen - 1] != '"')
        printf("no\n");
      else
      {
        command[2] += 1;
        command[2][fullLen - 2] = 0;
        int result = fs_write(root, path, command[2]);
        if(result > 0)
          printf("ok %d\n", result);
        else
          printf("no\n");
        command[2] -= 1; //Ritorno al puntatore giusto per poter pulire
      }
    }
    else printf("no\n");
  }

  //read <percorso_file>
  if(!strcmp(command[0], "read"))
  {
    if(count != 2)
    {
      printf("no\n");
      return;
    }

    path = fs_parse_path(command[1]);
    if(path != NULL)
    {
      char* result = fs_read(root, path);
      if(result != NULL)
        printf("contenuto %s\n", result);
      else
        printf("no\n");
    }
    else printf("no\n");
  }

  //delete <percorso_file>
  if(!strcmp(command[0], "delete"))
  {
    if(count != 2)
    {
      printf("no\n");
      return;
    }


    path = fs_parse_path(command[1]);

    if(path != NULL)
      printf("%s\n", fs_delete(root, path, false) ? "ok" : "no");
    else printf("no\n");
  }

  //delete <percorso_file>
  if(!strcmp(command[0], "delete_r"))
  {
    if(count != 2)
    {
      printf("no\n");
      return;
    }


    path = fs_parse_path(command[1]);

    if(path != NULL)
      printf("%s\n", fs_delete(root, path, true) ? "ok" : "no");
    else printf("no\n");
  }

  //find <name>
  if(!strcmp(command[0], "find"))
  {
    if(count != 2)
    {
      printf("no\n");
      return;
    }

    if(command[1] != NULL)
    {
      int count = 0;
      node_t** findResults = fs_find(root, command[1], fs_key(command[1]), NULL, &count);
      fs_mergesort(findResults, 0, count-1);

      if(count > 0)
      {
        int i = 0;
        while(findResults[i] != NULL)
        {
          printf("ok %s\n", findResults[i]->path);
          i++;
        }
      }
      else
        printf("no\n");
      free(findResults);
    }
    else printf("no\n");
  }

  if(path != NULL)
    free(path);
}

int main(void)
{
  root.fs_parent = NULL;
  root.name = NULL;
  root.path = NULL;
  root.content = NULL;
  root.childs = 0;
  root.first_child = NULL;
  root.depth = 0;
  root.isDir = true;

  char* command[MAX_PARAMS] = {NULL};
  int count = 0;
  do
  {
    //Reistanzio comando
    if(command[0] != NULL)
    {
      for(int j = 0; j < count; j++)
        free(command[j]);
    }
    command[0] = NULL;
    command[1] = NULL;
    command[2] = NULL;

    //Leggo il prossimo comando
    count = readCommand(command);
    parseCommand(command, count, &root);
  } while(count > 0 && strcmp(command[0], "exit"));
  return 0;
}
