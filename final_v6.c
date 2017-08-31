#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define _INITIAL_PATH_BUFFER_SIZE_ 256

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

typedef struct command_tag
{
  char* command;
  char** path;
  int* name_key;
  int* name_len;
  char* content;
  bool isPathValid;
  int content_len;
  int pathBufferSize;
  int count;
} command_t;

static node_t root;
bool badExecution = false;
bool keepLastPath = false;

int local_rand(unsigned int seed, unsigned int max)  // RAND_MAX assumed to be 32767
{
    return (unsigned int)((seed * 1103515245 + 12345)/65536) % max;
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

char* fs_calculate_path(node_t* node)
{
  node_t* currentNode = node;
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
  //Sono in un nodo intermedio, devo ricercare
  if(root->childs == 0) // Questo livello non ha più sottofigli, il percorso deve essere errato
    return NULL;

  if(*key < 0)
    return NULL;

  node_t* next = root->hash_table[*key % _FS_HASH_BUCKETS_]; // = fs_bst_find_node(root->rb_root, key);
  while(next != NULL && (next->key != *key|| strcmp(next->name, name))) //Trovo il nodo che mi serve
    next = next->hash_next;

  return next;
}

void fs_merge(char** items, int left, int center, int right)
{
  int i = left;
  int j = center + 1;
  int k = 0;
  char* queue[right - left];

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

bool fs_create(node_t* root, char** path, int* key, int* len, bool isDir)
{
  while(root != NULL)
  {
    if(root->depth + 1 > _FS_MAX_DEPTH_)
    {
      return false; //_FS_MAX_DEPTH_REACHED_
    }
    if(*path == NULL)
      return false; //_FS_FILE_ALREADY_EXISTS_ Esiste già il file/dir che volevo creare

    if(*key < 0)
      return false;
    int hash = *key % _FS_HASH_BUCKETS_;

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
      node->depth = root->depth + 1;
      node->key = *key;

      int lenName = *len + 1;
      node->name_len = *len;
      int lenPath = root->path_len + lenName + 1;
      node->path_len = lenPath - 1;
      if(!isDir)
      {
        node->content = (char*)malloc(2*sizeof(char));
        *(node->content) = '\0';
        node->content++;
        *(node->content) = '\0';
      }

      node->name = *path;

      if(root->childs == 0)
      {
        //Creo la tabella
        root->hash_table = (node_t**)malloc(_FS_HASH_BUCKETS_ * sizeof(node_t*));
        memset(root->hash_table, 0, _FS_HASH_BUCKETS_ * sizeof(node_t*));
      }

      //Cerco se esiste un elemento con lo stesso nome
      node_t* hash_spot = root->hash_table[hash];
      while(hash_spot != NULL && (hash_spot->key != *key || strcmp(hash_spot->name, *path))) //Scorro le collisioni (Effettuo la comparazione carattere per carattere solo se ho le stesse chiavi)
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

    node_t* next = fs_hash_next_node(root, *path, key);

    if(next == NULL)
      return false;

    path++;
    key++;
    len++;
    root = next;
  }
  return false;
}

//Metodo per scrivere nei file
int fs_write(node_t* root, char** path, int* key, char* content, int contentLen)
{
  while(root != NULL)
  {
    if(*path == NULL && !root->isDir)
    {
      //L'elemento è stato trovato, si trova in root
      if(root->content != NULL)
        free(root->content - 1);

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

    root = next;
    path++;
    key++;
  }
  return -1;
}

char* fs_read(node_t* root, char** path, int* key)
{
  while(root != NULL)
  {
    if(*path == NULL)
      return root->content;

    //Mi trovo in un nodo intermedio, cerco il prossimo nodo in lista
    node_t* next = fs_hash_next_node(root, *path, key);

    if(next == NULL)
      return NULL;

    root = next;
    path++;
    key++;
  }
  return NULL;
}

bool fs_delete(node_t* root, char** path, int* key, bool recursive)
{
  while(root != NULL)
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
        root->fs_parent->hash_table[root->key % _FS_HASH_BUCKETS_] = root->hash_next;

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

    root = next;
    path++;
    key++;
  }
  return false;
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

int readCommand(command_t* command)
{
  char* str = (char*)malloc(sizeof(char));
  int c;
  int len = 0;
  int pathLen = 0;
  int clen = 0;
  int currentKey = _FS_KEY_EMPTY_;

  while((c = getchar()) != '\n' && c != EOF)
  {
    //Se non ho uno spazio tra caratteri leggo la stringa
    if(c != ' ' || clen >= 2)
    {
      if(clen == 1)
      {
        //Se sto facendo il parse del percorso
        if(c == '/' && len != 0)
        {
          str[len] = '\0';
          command->name_key[pathLen] = currentKey;
          command->name_len[pathLen] = len;
          command->path[pathLen++] = str;

          if(currentKey < 0)
            command->isPathValid = false;

          if(pathLen >= 255) //Limite di profondità
            command->isPathValid = false;

          if(pathLen >= command->pathBufferSize)
          {
            command->pathBufferSize *= 2;
            command->path = (char**)realloc(command->path, command->pathBufferSize * sizeof(char*));
            command->name_key = (int*)realloc(command->name_key, command->pathBufferSize * sizeof(int));
            command->name_len = (int*)realloc(command->name_len, command->pathBufferSize * sizeof(int));
          }
          str = (char*)malloc(sizeof(char));
          len = 0;
          currentKey = _FS_KEY_EMPTY_;
        }
        else if(c != '/')
        {
          str[len++] = c;
          currentKey = fs_partial_key(currentKey, len - 1, c);
          str = (char*)realloc(str, (len + 1) * sizeof(char));
        }
      }
      else
      {
        str[len++] = c;
        str = (char*)realloc(str, (len + 1) * sizeof(char));
      }
    }
    else if(len > 0)
    {
      str[len] = '\0';
      if(clen == 0)
      {
        command->command = str;
        command->path = (char**)malloc(command->pathBufferSize * sizeof(char*));
        command->name_key = (int*)malloc(command->pathBufferSize * sizeof(int));
        command->name_len = (int*)malloc(command->pathBufferSize * sizeof(int));
      }
      else if(clen == 1)
      {
        command->name_key[pathLen] = currentKey;
        command->name_len[pathLen] = len;
        command->path[pathLen++] = str;

        if(currentKey < 0)
          command->isPathValid = false;

        if(pathLen >= 255) //Limite di profondità
          command->isPathValid = false;

        if(pathLen >= command->pathBufferSize)
        {
          command->pathBufferSize += 1;
          command->path = (char**)realloc(command->path, command->pathBufferSize * sizeof(char*));
          command->name_key = (int*)realloc(command->name_key, command->pathBufferSize * sizeof(int));
          command->name_len = (int*)realloc(command->name_len, command->pathBufferSize * sizeof(int));
        }
        command->path[pathLen] = NULL;
        command->name_key[pathLen] = _FS_KEY_END_;
        command->name_len[pathLen] = 0;
      }
      else
      {
        command->content = str;
        command->content_len = len;
      }
      str = (char*)malloc(sizeof(char)); //Reinstazio una nuova stringa, pronta per il prossimo parametro
      len = 0;
      clen++;
    }
  }

  //Prendo l'ultimo parametro prima di EOF
  if(len > 0 && clen < 3)
  {
    str[len] = '\0';
    if(clen == 0)
    {
      command->command = str;
    }
    else if(clen == 1)
    {
      command->name_key[pathLen] = currentKey;
      command->name_len[pathLen] = len;
      command->path[pathLen++] = str;

      if(currentKey < 0)
        command->isPathValid = false;

      if(pathLen >= 255) //Limite di profondità
        command->isPathValid = false;

      if(pathLen >= command->pathBufferSize)
      {
        command->pathBufferSize += 1;
        command->path = (char**)realloc(command->path, command->pathBufferSize * sizeof(char*));
        command->name_key = (int*)realloc(command->name_key, command->pathBufferSize * sizeof(int));
        command->name_len = (int*)realloc(command->name_len, command->pathBufferSize * sizeof(int));
      }
      command->path[pathLen] = NULL;
      command->name_key[pathLen] = _FS_KEY_END_;
      command->name_len[pathLen] = 0;
    }
    else
    {
      command->content = str;
      command->content_len = len;
    }
  }
  return clen;
}

void parseCommand(command_t* command, node_t* root)
{
  if(command->command == NULL)
    return;

  //create <percorso_file>
  if(!strcmp(command->command, "create"))
  {
    if(command->count != 2)
    {
      badExecution = true;
      printf("no\n");
      return;
    }


    if(command->isPathValid)
      printf("%s\n", (keepLastPath = fs_create(root, command->path, command->name_key, command->name_len, false)) ? "ok" : "no");
    else
      printf("no\n");
  }

  //create_dir <percorso_dir>
  else if(!strcmp(command->command, "create_dir"))
  {
    if(command->count != 2)
    {
      badExecution = true;
      printf("no\n");
      return;
    }


    if(command->isPathValid)
      printf("%s\n", (keepLastPath = fs_create(root, command->path, command->name_key, command->name_len, true)) ? "ok" : "no");
    else
      printf("no\n");
  }

  //write <percorso_file> <contenuto>
  else if(!strcmp(command->command, "write"))
  {
    if(command->count != 3)
    {
      badExecution = true;
      printf("no\n");
      return;
    }


    if(command->isPathValid)
    {
      if(command->content_len < 2 || command->content[0] != '"' || command->content[command->content_len - 1] != '"')
      {
        badExecution = true;
        printf("no\n");
      }
      else
      {
        command->content += 1;
        command->content[command->content_len - 2] = 0;
        command->content_len -= 2;
        int result = fs_write(root, command->path, command->name_key, command->content, command->content_len);
        if(result > 0)
          printf("ok %d\n", result);
        else
        {
          command->content -= 1; //Ritorno al puntatore giusto per poter pulire
          badExecution = true;
          printf("no\n");
        }
      }
    }
    else
    {
      badExecution = true;
      printf("no\n");
    }
  }

  //read <percorso_file>
  else if(!strcmp(command->command, "read"))
  {
    if(command->count != 2)
    {
      badExecution = true;
      printf("no\n");
      return;
    }

    if(command->isPathValid)
    {
      char* result = fs_read(root, command->path, command->name_key);
      if(result != NULL)
        printf("contenuto %s\n", result);
      else
        printf("no\n");
    }
    else printf("no\n");
  }

  //delete <percorso_file>
  else if(!strcmp(command->command, "delete"))
  {
    if(command->count != 2)
    {
      badExecution = true;
      printf("no\n");
      return;
    }



    if(command->isPathValid)
      printf("%s\n", fs_delete(root, command->path, command->name_key, false) ? "ok" : "no");
    else printf("no\n");
  }

  //delete <percorso_file>
  else if(!strcmp(command->command, "delete_r"))
  {
    if(command->count != 2)
    {
      badExecution = true;
      printf("no\n");
      return;
    }


    if(command->isPathValid)
      printf("%s\n", fs_delete(root, command->path, command->name_key, true) ? "ok" : "no");
    else printf("no\n");
  }

  //find <name>
  else if(!strcmp(command->command, "find"))
  {
    if(command->count != 2)
    {
      badExecution = true;
      printf("no\n");
      return;
    }

    if(command->path[0] != NULL && command->name_key[0] > 0)
    {
      int count = 0;
      node_t** findResults = fs_find(root, command->path[0], command->name_key[0], NULL, &count);

      if(count > 0)
      {
        int i = 0;
        char** paths = (char**)malloc(count * sizeof(char*));
        while(findResults[i] != NULL)
        {
          paths[i] = fs_calculate_path(findResults[i]);
          i++;
        }

        fs_mergesort(paths, 0, i - 1);

        count = i;
        for(i = 0; i < count; i++)
        {
          printf("ok %s\n", paths[i]);
          free(paths[i]);
        }
        free(paths);

      }
      else
        printf("no\n");
      free(findResults);
    }
    else printf("no\n");
  }

  else if(!strcmp(command->command, "exit"))
  {
    //DO NOTHING
  }
  else
  {
    badExecution = true;
    printf("no\n");
  }
}

void cleanupCommand(command_t* command)
{
    //Reistanzio comando
    if(command->command != NULL)
    {
      free(command->command);
      free(command->name_key);
      free(command->name_len);
      //Libero il contentuto del percorso
      if(command->path != NULL)
      {
        int j = 0;
        while(command->path[j] != NULL)
        {
          if(!keepLastPath)
            free(command->path[j]);
          else if(command->path[j+1] != NULL)
            free(command->path[j]);
          j++;
        }
        free(command->path);
      }

      if(badExecution)
      {
        //Libero il 3o parametro
        if(command->content != NULL)
          free(command->content);
        badExecution = false;
      }
      keepLastPath = false;
    }

    command->command = NULL;
    command->path = NULL;
    command->content = NULL;
    command->count = 0;
    command->name_key = NULL;
    command->name_len = NULL;
    command->content_len = -1;
    command->isPathValid = true;
    command->pathBufferSize = _INITIAL_PATH_BUFFER_SIZE_;
}

int main(void)
{
  root.fs_parent = NULL;
  root.name = NULL;
  root.content = NULL;
  root.childs = 0;
  root.first_child = NULL;
  root.depth = 0;
  root.isDir = true;
  root.path_len = 0;

  command_t command;
  command.command = NULL;
  command.path = NULL;
  command.content = NULL;
  command.count = 0;
  command.name_key = NULL;
  command.name_len = NULL;
  command.content_len = -1;
  command.isPathValid = true;
  command.pathBufferSize = _INITIAL_PATH_BUFFER_SIZE_;

  do
  {
    cleanupCommand(&command);

    //Leggo il prossimo comando
    command.count = readCommand(&command) + 1;
    parseCommand(&command, &root);
  } while(command.command != NULL && strcmp(command.command, "exit"));
  return 0;
}
