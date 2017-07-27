#include <stdio.h>

//Per ora pieno di merda, prima o poi separerò il tutto
int main()
{
  char* comando;
  char* data;
  int scanResult = 0;

  do
  {
      scanResult = scanf("%s %s", comando, data);

  } while(scanResult > 0 && !strcmp(comando, "exit"));
  return -1;
}
