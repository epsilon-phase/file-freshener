#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "freshen.h"
#include <limits.h>
#include <errno.h>
#include <utime.h>
#include "cp.h"
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
  listNode* root=calloc(1,sizeof(listNode));
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
      sqlite3_reset(prepared_insert);//Reset prepared statement

    }else if(strcmp(argv[i],"-freshen")==0){
      sqlite3_exec(db,"select * from FILES;",freshen,(void*)root,&zErrMsg);
      listNode* r=root;
      struct stat srcbuf,dstbuf;
      short destination_exists=1,skip_danger=0,can_replace=1;
      struct utimbuf replacement_time;
      if(argc>i+1&&strcmp(argv[i+1],"-safe")){
        skip_danger=1;
        i++;
      }
      while(r){
        rc=stat(r->destination,&dstbuf);
        if(rc==-1){
          if(errno!=ENOENT){
            fprintf(stderr,"It seems that the destination is inaccessible for some reason\n");
            exit(1);
          }else
            destination_exists=0;
        }else
          destination_exists=1;
        rc=stat(r->source,&srcbuf);
        if(rc==-1){
            fprintf(stderr,"It seems that the source file is inaccessible for some reason\n");
            exit(1);
        }
        if(r->dangerous)
          {
            if(skip_danger)
              {
                r=r->next;
                continue;
              }
            printf("%s is marked as being dangerous, is %s ready to be used?\n",r->destination,r->source);
            scanf("%c",&can_replace);
          }else
          can_replace=1;
        if(can_replace!=0&&can_replace!='n'){
          printf(srcbuf.st_mtime>dstbuf.st_mtime?
                 "Replacing %s with %s\n":"%s up to date with %s\n",r->destination,r->source);
          cp(r->destination,r->source);
          replacement_time.modtime=srcbuf.st_mtim.tv_sec;
          replacement_time.actime=srcbuf.st_atim.tv_sec;
          utime(r->destination,&replacement_time);//replace with the source file's time
        }
        r=r->next;
      }
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
  while(l->source||l->destination){
    if(!l->next)
    l->next=calloc(1,sizeof(listNode));
    l=l->next;
  }
  l->destination=calloc(strlen(argv[0]),sizeof(char));
  strcpy(l->destination,argv[0]);
  l->source=calloc(strlen(argv[1]),sizeof(char));
  strcpy(l->source,argv[1]);
  l->dangerous=atoi(argv[2])>0;
  return 0;
}
