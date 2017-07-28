#include <stdio.h>
#include <math.h>
#include "fs_utils.h"

//File per verificare che l'hash rimanga nei limiti di memoria

#define CHAR_COMBINATIONS 62
#define MAX_CHARS 255

int main(void)
{
  int total = 0;
  int i = 0;
  for(i = 0; i < MAX_CHARS; i++)
  {
    total += (CHAR_COMBINATIONS - 1) + (CHAR_COMBINATIONS * i);
  }
  printf("%d\n", total);
  char* percorso = "test asd";
  char* contenuto = "ehehehe";
  printf("Percorso prima dell'hash: %s\n", percorso);
  int hash = fs_hash(percorso);
  printf("Hash: %d | Length: %d\n", hash, fs_hash_length(hash));
  printf("Percorso dopo l'hash: %s\n", percorso);
  printf("Percorso dopo evere applicato la lenght %s\n", percorso + fs_hash_length(hash));
  return -1;
}
