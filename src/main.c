#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "freshen.h"
#include <limits.h>
#define CREATE_TABLE "CREATE TABLE IF NOT EXISTS FILES("\
  "DESTINATION TEXT NOT NULL UNIQUE,"\
  "SOURCE TEXT NOT NULL,"\
  "DANGEROUS INT NOT NULL);"
static int callback(void *NotUsed,int argc, char** argv, char ** azColName);
static int freshen(void *List,int argc, char **argv, char **azColName);
time_t get_mtime(const char *path);
int main(int argc,char* argv[]){
  sqlite3 *db;
  char *zErrMsg=0;
  char file[100];
  char *sql;
  int i;
  sqlite3_stmt *prepared_insert;
  struct listNode* root=calloc(1,sizeof(listNode));
  strcat(file,getenv("HOME"));
  strncat(file,"/freshen.db",13);
  int rc;
  rc=sqlite3_open(file,&db);
  if(rc){
    fprintf(stderr,"Can't open database:%s\n",sqlite3_errmsg(db));
    exit(0);
  }else{
    fprintf(stderr,"database opened successfully\n");
  }
  sql=CREATE_TABLE;
  rc=sqlite3_exec(db,sql,callback,0,&zErrMsg);
  if(rc!=SQLITE_OK){
    fprintf(stderr,"SQL Error:%s\n",zErrMsg);
    sqlite3_free(zErrMsg);
  }else{
    fprintf(stderr,"Table created sucessfully\n");
  }
  rc=sqlite3_prepare_v2(db,"insert into FILES (DESTINATION,SOURCE,DANGEROUS) values (?,?,?);",-1,&prepared_insert,NULL);
  if(rc!=SQLITE_OK){
    //write this sometime
  }
  for(i=1;i<argc;i++){
    if(strcmp(argv[i],"-insert")==0){
      char *dest=argv[i+1];
      char *src=argv[i+2];
      char destpath[PATH_MAX+1],
        srcpath[PATH_MAX+1];
      realpath(dest,destpath);
      realpath(src,srcpath);
      i+=2;
      short dangerous=0;
      if(argc>=i+4){
        if(strcmp(argv[i+3],"-dangerous")==0)
          {
            dangerous=1;
            i++;
          }
      }
      if(sqlite3_bind_text(prepared_insert,1,destpath,-1,SQLITE_STATIC)!=SQLITE_OK)
        fprintf(stderr,"Failed to bind destination\n");

      if(sqlite3_bind_text(prepared_insert,2,srcpath,-1,SQLITE_STATIC)!=SQLITE_OK)
        fprintf(stderr,"Failed to bind source\n");
      if(sqlite3_bind_int(prepared_insert,3,dangerous)!=SQLITE_OK)
        fprintf(stderr,"Failed to bind dangerous\n");
      rc=sqlite3_step(prepared_insert);
      if(rc!=SQLITE_DONE){
        fprintf(stderr,"Didn't run right: %s\n",sqlite3_errstr(rc));
      }
      sqlite3_reset(prepared_insert);
    }else if(strcmp(argv[i],"freshen")==0){
      sqlite3_exec(db,"select * from FILES;",freshen,(void*)root,&zErrMsg);
    }
  }
  sqlite3_close(db);
}

static int callback(void *NotUsed,int argc, char** argv, char ** azColName){
  int i;
  for(i=0;i<argc;i++){
    printf("%s = %s\n",azColName[i], argv[i]?argv[i]:"NULL");
  }
  printf("\n");
  return 0;
}

time_t get_mtime(const char *path){
  struct stat statbuf;
  if(stat(path,&statbuf)==-1){
    perror(path);
    exit(1);
  }
  return statbuf.st_mtime;
}

static int freshen(void *List,int argc, char **argv, char **azColName){
  listNode *l=List;
  if(l->source||l->destination){
    l->next=malloc(sizeof(listNode));
    l=l->next;
  }
  l->destination=calloc(strlen(argv[1]),sizeof(char));
  strcpy(l->destination,argv[1]);
  l->source=calloc(strlen(argv[2]),sizeof(char));
  strcpy(l->source,argv[2]);
  l->dangerous=atoi(argv[3])>0;
  return 0;
}
