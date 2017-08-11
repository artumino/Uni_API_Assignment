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
    if(c != ' ')
    {
      str[len++] = c;
      str = (char*)realloc(str, (len + 1) * sizeof(char));
    }
    else
    {
      //Se è uno spazio, allora sono nell'intermezzo tra più stringhe
      //cerco di inserire i dati nell'array dei parametri (se c'è spazio)
      if(clen < MAX_PARAMS)
      {
        str[len] = '\0';
        comando[clen++] = str;
      }
      else
      {
        //Non c'è più spazio nei parametri, ignoro la stringa e libero la sua memoria
        free(str);
      }
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
