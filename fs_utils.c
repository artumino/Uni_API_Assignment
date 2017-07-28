#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "fs_utils.h"

int fs_write(node_t* root, char* percorso, char* contenuto)
{
  node_t* child;
  if(*percorso == '\0')
  {
    //Trovato elemento
  }

  //Calcolo hash del file per la ricerca binaria
  int hash = fs_hash(percorso);
  
  return hash;
}

//Non ritorno la lunghezza perchè posso facilmente ricavarla facendo ceil((double)(hash+1)/_MAX_CHAR_COMBINATIONS_);
int fs_hash(char* percorso)
{
  int hash = 0;
  int len = 0;
  int sChar = 0;
  while(*percorso != '\0' && *percorso != '/')
  {
    sChar = *percorso++;
    if(sChar > 47 && sChar < 58) //0-9 -> 0-9
    {
      sChar -= 48;
    }
    else if(sChar > 64 && sChar < 91) // A-Z -> 10-34
    {
      sChar -= 55;
    }
    else if(sChar > 96 && sChar < 123)
    {
      sChar -= 61;
    }
    else
      return _FS_HASH_WRONG_CHARS_; //Caratteri inaspettati nel path
    hash = sChar + (len++ * _MAX_CHAR_COMBINATIONS_);
  }

  //Se il ciclo non è mai stato eseguito
  if(len == 0)
    hash = _FS_HASH_EMPTY_;

  return hash;
}

//Metodo comodo per eseguire il calcolo della lunghezza rispetto all'hash
int fs_hash_length(int hash)
{
  return ceil((double)(hash+1)/_MAX_CHAR_COMBINATIONS_);
}
