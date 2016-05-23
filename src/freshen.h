#ifndef FRESHEN_H
#define FRESHEN_H
typedef struct{
  char *destination;
  char *source;
  short dangerous;
  void *next;
}listNode;
#endif
