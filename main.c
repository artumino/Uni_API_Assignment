#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "debug.h"
#include "fs_utils.h"

#define MAX_PARAMS 3

static node_t root;
int readCommand(char** comando)
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
    else
    {
      str[len] = '\0';
      comando[clen++] = str;
      str = (char*)malloc(sizeof(char)); //Reinstazio una nuova stringa, pronta per il prossimo parametro
      len = 0;
    }
  }

  //Prendo l'ultimo parametro prima di EOF
  if(clen < MAX_PARAMS)
  {
    str[len] = '\0';
    comando[clen++] = str;
  }

  return clen;
}

void parseCommand(char** command, int count, node_t* root)
{
  char** path;
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

    debug_print("[DEBUG] Creazione file iniziata...\n");


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

    debug_print("[DEBUG] Creazione directory iniziata...\n");

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

    debug_print("[DEBUG] Scrittura file iniziata...\n");

    path = fs_parse_path(command[1]);
    if(path != NULL)
    {
      int result = fs_write(root, path, command[2]);
      if(result > 0)
        printf("ok %d\n", result);
      else
        printf("no\n");
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

    debug_print("[DEBUG] Lettura file iniziata...\n");

    path = fs_parse_path(command[1]);
    if(path != NULL)
    {
      char* result = fs_read(root, path);
      if(result != NULL)
        printf("ok %s\n", result);
      else
        printf("no\n");
    }
    else printf("no\n");
  }
}

int main(void)
{
  root.fs_parent = NULL;
  root.rb_root = NULL;
  root.name = NULL;
  root.path = NULL;
  root.content = NULL;
  root.childs = 0;
  root.depth = 0;
  root.isDir = true;

  char** comando = NULL;
  int count = 0;
  do
  {
    //Reistanzio comando
    if(comando != NULL)
      free(comando);
    comando =  (char**)malloc(MAX_PARAMS * sizeof(char*));

    debug_print("Scelta--> ");
    //Leggo il prossimo comando
    count = readCommand(comando);
    parseCommand(comando, count, &root);
  } while(count > 0 && strcmp(comando[0], "exit"));
  return 1;
}
