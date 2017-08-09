#include <stdio.h>
#include <math.h>
#include <string.h>
#include "fs_utils.h"

//File per verificare che l'hash rimanga nei limiti di memoria

#define CHAR_COMBINATIONS 62
#define MAX_CHARS 255

int main(void)
{
  int total = 0;
  int i = 0;
  int j;
  int hash;
  for(i = 0; i < MAX_CHARS; i++)
  {
    total += (CHAR_COMBINATIONS - 1) + (CHAR_COMBINATIONS * i);
  }
  printf("%d\n", total);
  char percorso[] = "/testasd/effucco/effanculen";
  char contenuto[] = "ehehehe";
  char** arr_percorso = fs_parse_path(percorso);

  printf("\tHash\t|\tLength\t|\tPercorso\n");
  i = 0;
  while(arr_percorso[i] != NULL)
  {
    hash = fs_hash(arr_percorso[i]);
    printf("\t%d\t|\t%d\t|\t", hash, fs_hash_length(hash));
    for(j = 0; j <= i; j++)
      printf("%s%s", arr_percorso[j], i == j ? "\n" : "/");
    i++;
  }
  return 0;
}
