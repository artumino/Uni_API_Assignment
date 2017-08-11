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
  char** arr_percorso = fs_parse_path(percorso);


  if(arr_percorso == NULL)
  {
    printf("Percorso malformato, manca l'indicatore della root...\n");
    return 0;
  }

  printf("\tKey\t|\tHash\t|\tLength\t|\tPercorso\n");
  i = 0;
  while(arr_percorso[i] != NULL)
  {
    key = fs_key(arr_percorso[i]);
    printf("\t%d\t|\t%d\t|\t%d\t|\t", key, fs_hash(key), fs_key_length(key));
    for(j = 0; j <= i; j++)
      printf("%s%s", arr_percorso[j], i == j ? "\n" : "/");
    i++;
  }

  int hash_table[1024];
  char* str;
  int len = 0;
  srand(time(NULL));
  for(i = 0; i < 1024; i++)
  {
    len = (rand() % 255) + 1;
    str = (char*)malloc(sizeof(char)*len);
    for(j = 0; j < len; j++)
    {
      extraction = (int)(rand() % 62);
      str[j] = (char)(extraction < 10 ? extraction : extraction >= 10 && extraction < 35 ? extraction + 55 : extraction + 61);
    }

    int key = fs_key(str);
    int hash = fs_hash(key);
    if(hash_table[hash] > 0 && hash_table[hash] != key)
      printf("Collision for key %d and key %d\n", hash_table[hash], key);
    hash_table[hash] = key;
    sleep(0.01);
  }


  return 0;
}
