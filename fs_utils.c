#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "fs_utils.h"

bool fs_create(node_t* root, char* percorso, bool is_dir)
{

  return true;
}

int fs_write(node_t* root, char* percorso, char* contenuto)
{
  node_t* child;
  if(*percorso == '/')
    percorso += sizeof(char);

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

//Metodi per effettuare le rotazioni negli alberi binari
void fs_bst_rotate(node_t** root, node_t* node, bool left)
{
  node_t* y = left ? node->rb_right : node->rb_left;

  //Effettua la rotazione
  if(left)
  {
    //Left Rotate
    node->rb_right = y->rb_left;

    if(y->rb_left != NULL)
      y->rb_left->rb_parent = node;
    y->rb_parent = node->rb_parent;
  }
  else
  {
    //Right Rotate
    node->rb_left = y->rb_right;

    if(y->rb_right != NULL)
      y->rb_right->rb_parent = node;
    y->rb_parent = node->rb_parent;
  }

  //Sistema Root e Nodi sovrastanti
  if(node->rb_parent == NULL)
    *root = y;
  else if(node == node->rb_parent->rb_left)
    node->rb_parent->rb_left = y;
  else
    node->rb_parent->rb_right = y;

  //Imposta Node come figlio opportuno del pivot
  if(left)
    y->rb_left = node;
  else
    y->rb_right = node;

  node->rb_parent = y;
}
