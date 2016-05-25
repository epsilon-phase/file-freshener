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
  sqlite3_stmt *prepared_insert,*prepared_delete,*prepared_replace,*prepared_source_redirect;
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
  rc=sqlite3_prepare_v2(db,"delete from FILES where DESTINATION = ?",-1,&prepared_delete,NULL);
  if(rc!=SQLITE_OK){
    //do something here sometime.
  }
  rc=sqlite3_prepare_v2(db,"update FILES set SOURCE = ? where DESTINATION = ?",-1,&prepared_replace,NULL);
  if(rc!=SQLITE_OK){
    //Needs to be written
  }
  rc=sqlite3_prepare_v2(db,"UPDATE FILES SET SOURCE = ? WHERE SOURCE = ?",-1,&prepared_source_redirect,NULL);
  if(rc!=SQLITE_OK){
    //This sort of thing might not need to actually be written, after all, they
    //should compile to the same thing each time.
    fprintf(stderr,"Something broke!\n");
  }
  for(i=1;i<argc;i++){
    if(strcmp(argv[i],"-insert")==0){
      char *dest=argv[i+1];
      char *src=argv[i+2];
      char destpath[PATH_MAX+1],
        srcpath[PATH_MAX+1];
      if(!isArgFile(dest)){
        fprintf(stderr,"-insert requires two arguments <destination file> <source file> [-dangerous]?\n%s is not visible to this as a file\n",dest);
        exit(1);
      }
      if(!isArgFile(src)){
        fprintf(stderr,"-insert requires two arguments <destination file> <source file> [-dangerous]?\n%s is not visible to this as a file\n",src);
        exit(1);
      }
      if(!fileExists(src))
        {
          fprintf(stderr,"%s does not exist, source files must exist\n",src);
          exit(1);
        }
      realpath(dest,destpath);
      realpath(src,srcpath);
      i+=2;
      short dangerous=0;
      if(argc>=i+2){
        if(strcmp(argv[i+1],"-dangerous")==0)
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
      short destination_exists=1,skip_danger=0;
      char can_replace=1;
      struct utimbuf replacement_time;
      if(argc>i+1&&strcmp(argv[i+1],"-safe-only")){
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
        if((can_replace!=0&&can_replace!='n')||!destination_exists){
          printf(srcbuf.st_mtime>dstbuf.st_mtime?
                 "Replacing %s with %s\n":"%s up to date with %s\n",r->destination,r->source);
          cp(r->destination,r->source);
          replacement_time.modtime=srcbuf.st_mtim.tv_sec;
          replacement_time.actime=srcbuf.st_atim.tv_sec;
          utime(r->destination,&replacement_time);//replace with the source file's time
        }
        r=r->next;
      }
      freeList(root);
      root=calloc(1,sizeof(listNode));
    }else if (strcmp(argv[i],"-remove")==0){//Remove destination file from database, doesn't really matter if it succeeds.
      char rpath[PATH_MAX+1];
      realpath(argv[i+1],rpath);
      i++;
      sqlite3_bind_text(prepared_delete,1,rpath,-1,SQLITE_STATIC);
      sqlite3_step(prepared_delete);
      sqlite3_reset(prepared_delete);
    }else if(strcmp(argv[i],"-replace")==0){
      char srcpath[PATH_MAX+1],dstpath[PATH_MAX+1];
      realpath(argv[i+1],dstpath);
      realpath(argv[i+2],srcpath);
      sqlite3_bind_text(prepared_replace,1,srcpath,-1,SQLITE_STATIC);
      sqlite3_bind_text(prepared_replace,2,dstpath,-1,SQLITE_STATIC);
      sqlite3_step(prepared_replace);
      sqlite3_reset(prepared_replace);
    }else if(strcmp(argv[i],"-redirect")==0){
      char srcpath[PATH_MAX+1],newpath[PATH_MAX+1];
      if(argc<=i+2){
        fprintf(stderr,"The proper invocation of -redirect requires two arguments <original source> <new source>\n");
        exit(1);
      }
      realpath(argv[i+1],srcpath);//Get the full path of the file.
      realpath(argv[i+2],newpath);
      printf("%s\n",srcpath);
      sqlite3_bind_text(prepared_source_redirect,1,newpath,-1,SQLITE_STATIC);
      sqlite3_bind_text(prepared_source_redirect,2,srcpath,-1,SQLITE_STATIC);
      rc=sqlite3_step(prepared_source_redirect);
      if(rc!=SQLITE_DONE){
        fprintf(stderr,"%s\n",sqlite3_errmsg(db));
      }
      sqlite3_reset(prepared_source_redirect);
      i+=2;
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
