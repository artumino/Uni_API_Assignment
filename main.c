#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "debug.h"
#include "fs_utils.h"

#define _INITIAL_PATH_BUFFER_SIZE_ 256

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
          debug_print("/");
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
            debug_print("Resize buffer path da %d a %d\n", command->pathBufferSize, command->pathBufferSize * 2);
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
          debug_print("c");
          str[len++] = c;
          currentKey = fs_partial_key(currentKey, len - 1, c);
          str = (char*)realloc(str, (len + 1) * sizeof(char));
        }
      }
      else
      {
        debug_print("c");
        str[len++] = c;
        str = (char*)realloc(str, (len + 1) * sizeof(char));
      }
    }
    else if(len > 0)
    {
      debug_print(" ");
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

        debug_print("Got path parameter with name %s\n", command->path[pathLen - 1]);
        if(pathLen >= command->pathBufferSize)
        {
          debug_print("Resize buffer path da %d a %d\n", command->pathBufferSize, command->pathBufferSize * 2);
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

  debug_print("\n");
  //Prendo l'ultimo parametro prima di EOF
  if(len > 0 && clen < 3)
  {
    str[len] = '\0';
    if(clen == 0)
    {
      debug_print("Salvato comando %s\n", str);
      command->command = str;
    }
    else if(clen == 1)
    {
      debug_print("Salvata path con parametri: nome %s e key %d\n", str, currentKey);
      command->name_key[pathLen] = currentKey;
      command->name_len[pathLen] = len;
      command->path[pathLen++] = str;

      if(currentKey < 0)
        command->isPathValid = false;

      if(pathLen >= 255) //Limite di profondità
        command->isPathValid = false;

      if(pathLen >= command->pathBufferSize)
      {
        debug_print("Resize buffer path da %d a %d\n", command->pathBufferSize, command->pathBufferSize + 1);
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
      debug_print("Salvato contenuto %s\n", str);
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

    debug_print("[DEBUG] Creazione file iniziata...\n");


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

    debug_print("[DEBUG] Creazione directory iniziata...\n");

    if(command->isPathValid)
    {
      printf("%s\n", (keepLastPath = fs_create(root, command->path, command->name_key, command->name_len, true)) ? "ok" : "no");
    }
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

    debug_print("[DEBUG] Scrittura file iniziata...\n");

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

    debug_print("[DEBUG] Lettura file iniziata...\n");

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

    debug_print("[DEBUG] Rimozione percorso iniziata...\n");


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

    debug_print("[DEBUG] Rimozione ricorsiva percorso iniziata...\n");


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

    debug_print("[DEBUG] Ricerca ricorsiva nome iniziata...\n");

    if(command->path[0] != NULL && command->name_key[0] > 0)
    {
      int count = 0;
      node_t** findResults = fs_find(root, command->path[0], command->name_key[0], NULL, &count);

      if(count > 0)
      {
        int i = 0;
        debug_print("Trovati %d elementi\n", count);
        char** paths = (char**)malloc(count * sizeof(char*));
        while(findResults[i] != NULL)
        {
          paths[i] = fs_calculate_path(findResults[i]);
          debug_print("Item[%d] : %s\n", i, paths[i]);
          i++;
        }

        debug_print("[DEBUG] Ricerca ricorsiva conclusa stampo risultati (%d paths)...\n", i);
        fs_mergesort(paths, 0, i - 1);
        debug_print("[DEBUG] Sort completato\n");

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
      debug_print("[DEBUG] Puliti sottoargomenti\n");
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
    debug_print("Scelta--> ");

    //Leggo il prossimo comando
    command.count = readCommand(&command) + 1;
    parseCommand(&command, &root);
  } while(command.command != NULL && strcmp(command.command, "exit"));
  return 0;
}
