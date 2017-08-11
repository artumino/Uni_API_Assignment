#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "fs_utils.h"

//File per verificare che l'key rimanga nei limiti di memoria

#define CHAR_COMBINATIONS 62
#define MAX_CHARS 255

int main(void)
{
  int total = 0;
  int i = 0;
  int j;
  int extraction;
  int key;
  for(i = 0; i < MAX_CHARS; i++)
  {
    total += (CHAR_COMBINATIONS - 1) + (CHAR_COMBINATIONS * i);
  }
  printf("%d\n", total);
  char percorso[] = "/testasd/effucco/effanculen";
  char contenuto[] = "ehehehe";
  //char** arr_percorso = fs_parse_path(percorso);


  /*if(arr_percorso == NULL)
  {
    printf("Percorso malformato, manca l'indicatore della root...\n");
    return 0;
  }

  printf("\tKey\t|\tHash\t|\tLength\t|\tPercorso\n");
  i = 0;
  while(arr_percorso[i] != NULL)
  {
    key = fs_key(arr_percorso[i]);
    printf("\t%d\t|\t%d\t|\t%d\t|\t", key, fs_hash(key, _FS_MAX_CHILDS_), fs_key_length(key));
    for(j = 0; j <= i; j++)
      printf("%s%s", arr_percorso[j], i == j ? "\n" : "/");
    i++;
  }*/

  int buckets = 1259;
  int hash_table[buckets];
  int collisions = 0;
  char extracted = '\0';
  char* str;
  int len = 0;
  srand(time(NULL));
  memset(hash_table, 0, sizeof hash_table);
  for(i = 0; i < 1024; i++)
  {
    len = (rand() % 255) + 1;
    str = (char*)malloc(sizeof(char)*(len + 1));
    memset( str, 0, len+1 );
    for(j = 0; j < len; j++)
    {
      extraction = (unsigned int)(rand())% 62;
      extracted = (char)(extraction < 10 ? extraction + 48 : (extraction >= 10 && extraction < 36) ? extraction + 55 : extraction + 61);
      str[j] = extracted;
    }

    int key = fs_key(str);
    free(str);
    int hash = fs_hash(key, buckets);
    if(hash_table[hash] > 0)
    {
      printf("%d collisions for hash %d\n", hash_table[hash], hash);
      collisions++;
    }
    hash_table[hash]++;
  }
  printf("In totale ci sono state %d collisioni su %d buckets\n", collisions, buckets);

  return 0;
}
