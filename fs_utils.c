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

int fs_rb_insert(node_t** root, node_t* node)
{
  node_t* y = NULL;
  node_t* x = *root;

  //Cerca la posizione dove inserire il nodo
  while(x != NULL)
  {
    y = x;
    if(node->rb_hash < x->rb_hash)
      x = x->rb_left;
    else
      x = x->rb_right;
  }

  node->rb_parent = y;
  if(y == NULL)
    *root = node;
  else if(node->rb_hash < y->rb_hash)
    y->rb_left = node;
  else if(node->rb_hash > y->rb_hash)
    y->rb_right = node;
  else
    return _FS_ITEM_ALREADY_EXISTS_;

  node->rb_left = NULL;
  node->rb_right = NULL;
  node->rb_color = RED;
  fs_rb_insert_fixup(root, node);
  return 0;
}

void fs_rb_insert_fixup(node_t** root, node_t* node)
{
  if(node == *root)
    (*root)->rb_color = BLACK;
  else
  {
    node_t* x = node->rb_parent;
    if(x->rb_color == RED)
    {
      if(x == x->rb_parent->rb_left) //Se x è figlio sinistro
      {
        node_t* y = x->rb_parent->rb_right;
        if(y->rb_color == RED)
        {
          x->rb_color = BLACK;                        //Caso 1
          y->rb_color = BLACK;                        //Caso 1
          x->rb_parent->rb_color = RED;               //Caso 1
          fs_rb_insert_fixup(root, x->rb_parent);     //Caso 1
        }
        else
        {
          if(node == x->rb_right)
          {
            node = x;                                       //Caso 2
            fs_bst_rotate(root, node, true); //Rotate Left  //Caso 2
            x = node->rb_parent;                            //Caso 2
          }
          x->rb_color = BLACK;                                      //Caso 3
          x->rb_parent->rb_color = RED;                             //Caso 3
          fs_bst_rotate(root, x->rb_parent, false); //Rotate Right  //Caso 3
        }
      }
      else
      {
        node_t* y = x->rb_parent->rb_left;
        if(y->rb_color == RED)
        {
          x->rb_color = BLACK;                        //Caso 1
          y->rb_color = BLACK;                        //Caso 1
          x->rb_parent->rb_color = RED;               //Caso 1
          fs_rb_insert_fixup(root, x->rb_parent);     //Caso 1
        }
        else
        {
          if(node == x->rb_left)
          {
            node = x;                                         //Caso 2
            fs_bst_rotate(root, node, false); //Rotate Right  //Caso 2
            x = node->rb_parent;                              //Caso 2
          }
          x->rb_color = BLACK;                                    //Caso 3
          x->rb_parent->rb_color = RED;                           //Caso 3
          fs_bst_rotate(root, x->rb_parent, true); //Rotate Left  //Caso 3
        }
      }
    }
  }
}
