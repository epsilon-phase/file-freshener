#include "freshen.h"
#include <stdlib.h>
#include <sys/stat.h>
//This definitely justifies an additional file
int isArgFile(const char* arg){
  return arg[0]!='-';
}
void freeList(listNode *root){
  if(root->next)
    freeList(root->next);
  free(root->destination);
  free(root->source);
  free(root);
}
int fileExists(const char* file){
  struct stat b;
  return stat(file,&b)!=-1;
}
