#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "fs_utils.h"

#define MAX_PARAMS 3

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

int main(void)
{
  char** comando = NULL;
  int count = 0;
  do
  {
    //Reistanzio comando
    if(comando != NULL)
      free(comando);
    comando =  (char**)malloc(MAX_PARAMS * sizeof(char*));

    //Leggo il prossimo comando
    count = readCommand(comando);
  } while(count > 0 && strcmp(comando[0], "exit"));
  return 1;
}
