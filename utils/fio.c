#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "fio.h"
#include "machine.h"
#include "proto.h"
#include "error.h"

FILE *MGHopen_file(char *fname, char *rwmode)
{
  FILE *f1;

  if ((f1 = fopen(fname,rwmode)) == NULL)
  {
    printf("Can't open %s\n",fname);
    exit(1);
  }
  return f1;
}

int
putf(float f, FILE *fp)
{
#ifdef Linux
  f = swapFloat(f) ;
#endif
  return(fwrite(&f,4,1,fp));
}

float
getf(FILE *fp)
{
  float f;

  fread(&f,4,1,fp);
#ifdef Linux
  f = swapFloat(f) ;
#endif
  return f;
}

int
fread1(int *v, FILE *fp)
{
  unsigned char c;
  int  ret ;

  ret = fread(&c,1,1,fp);
  *v = c;
  return(ret) ;
}


int
fread2(int *v, FILE *fp)
{
  short s;
  int   ret ;

  ret = fread(&s,2,1,fp);
#ifdef Linux
  s = swapShort(s) ;
#endif
  *v = s;
  return(ret) ;
}

int
fread3(int *v, FILE *fp)
{
  unsigned int i = 0;
  int  ret ;

  ret = fread(&i,3,1,fp);
#ifdef Linux
  i = (unsigned int)swapInt(i) ;
#endif
  *v = ((i>>8) & 0xffffff);
  return(ret) ;
}

int
fread4(float *v, FILE *fp)
{
  float f;
  int   ret ;

  ret = fread(&f,4,1,fp);
#ifdef Linux
  f = swapFloat(f) ;
#endif
  *v = f;
  return(ret) ;
}

int
fwrite1(int v,FILE *fp)
{
  unsigned char c = (unsigned char)v;

  return(fwrite(&c,1,1,fp));
}

int
fwrite2(int v, FILE *fp)
{
  short s ;

  if (v > 0x7fff)    /* don't let it overflow */
    v = 0x7fff ;
  else if (v < -0x7fff)
    v = -0x7fff ;
  s = (short)v;
#ifdef Linux
  s = swapShort(s) ;
#endif
  return(fwrite(&s,2,1,fp));
}

int
fwrite3(int v, FILE *fp)
{
  unsigned int i = (unsigned int)(v<<8);

#ifdef Linux
  i = (unsigned int)swapInt(i) ;
#endif
  return(fwrite(&i,3,1,fp));
}

int
fwrite4(int v,FILE *fp)
{
#ifdef Linux
  v = swapInt(v) ;
#endif
  return(fwrite(&v,4,1,fp));
}

int
fwriteShort(short s, FILE *fp)
{
#ifdef Linux
  s = swapShort(s) ;
#endif
  return(fwrite(&s, sizeof(short), 1, fp)) ;
}
float
freadFloat(FILE *fp)
{
  float f;
  int   ret ;

  ret = fread(&f,4,1,fp);
#ifdef Linux
  f = swapFloat(f) ;
#endif
  if (ret != 1)
    ErrorPrintf(ERROR_BADFILE, "freadFloat: fread failed") ;
  return(f) ;
}
double
freadDouble(FILE *fp)
{
  double d;
  int   ret ;

  ret = fread(&d,sizeof(double),1,fp);
#ifdef Linux
  d = swapDouble(d) ;
#endif
  if (ret != 1)
    ErrorPrintf(ERROR_BADFILE, "freadDouble: fread failed") ;
  return(d) ;
}

int
freadInt(FILE *fp)
{
  int  i, nread ;

  nread = fread(&i,sizeof(int),1,fp);
#ifdef Linux
  i = swapInt(i) ;
#endif
  return(i) ;
}

short
freadShort(FILE *fp)
{
  int   nread ;
  short s ;

  nread = fread(&s,sizeof(short),1,fp);
#ifdef Linux
  s = swapShort(s) ;
#endif
  if (nread != 1)
    ErrorPrintf(ERROR_BADFILE, "freadShort: fread failed") ;
  return(s) ;
}

int
fwriteInt(int v, FILE *fp)
{
#ifdef Linux
  v = swapInt(v) ;
#endif
  return(fwrite(&v,sizeof(int),1,fp));
}

int
fwriteFloat(float f, FILE *fp)
{
#ifdef Linux
  f = swapFloat(f) ;
#endif
  return(fwrite(&f,sizeof(float),1,fp));
}

/*------------------------------------------------------
  fio_dirname() - function to replicate the functionality
  of the unix dirname.
  Author: Douglas Greve, 9/10/2001
  ------------------------------------------------------*/
char *fio_dirname(char *pathname)
{
  int l,n;
  char *dirname;

  if(pathname == NULL) return(NULL);

  l = strlen(pathname);

  /* strip off leading forward slashes */
  while(l > 0 && pathname[l-1] == '/'){
    pathname[l-1] = '\0';
    l = strlen(pathname);
  }

  if(l < 2){
    /* pathname is / or . or single character */
    dirname = (char *) calloc(2,sizeof(char));
    if(l==0 || pathname[0] == '/') dirname[0] = '/';
    else                           dirname[0] = '.';
    return(dirname);
  }

  /* Start at the end of the path name and step back
     until a forward slash is found */
  for(n=l; n >= 0; n--)if(pathname[n] == '/') break;

  if(n < 0){
    /* no forward slash found */
    dirname = (char *) calloc(2,sizeof(char));
    dirname[0] = '.';
    return(dirname);
  }

  if(n == 0){
    /* first forward slash is the first character */
    dirname = (char *) calloc(2,sizeof(char));
    dirname[0] = '/';
    return(dirname);
  }

  dirname = (char *) calloc(n+1,sizeof(char));
  memcpy(dirname,pathname,n);
  return(dirname);
}
/*------------------------------------------------------
  fio_basename() - function to replicate the functionality
  of the unix basename.
  Author: Douglas Greve, 9/10/2001
  ------------------------------------------------------*/
char *fio_basename(char *pathname, char *ext)
{
  int l,n,lext;
  char *basename;

  if(pathname == NULL) return(NULL);

  l = strlen(pathname);

  /* strip off the extension if it matches ext */
  if(ext != NULL){
    lext = strlen(ext);
    if(lext < (l + 2)){
      if( strcmp(ext,&(pathname[l-lext]) ) == 0){
  memset(&(pathname[l-lext]),'\0',lext+1);
  l = strlen(pathname);
      }
    }
  }

  /* strip off leading forward slashes */
  while(l > 0 && pathname[l-1] == '/'){
    pathname[l-1] = '\0';
    l = strlen(pathname);
  }

  if(l < 2){
    /* basename is / or . or single character */
    basename = (char *) calloc(2,sizeof(char));
    if(l==0) basename[0] = '/';
    else     basename[0] = pathname[0];
    return(basename);
  }

  /* Start at the end of the path name and step back
     until a forward slash is found */
  for(n=l; n >= 0; n--) if(pathname[n] == '/') break;

  basename = (char *) calloc(l-n,sizeof(char));
  memcpy(basename,&(pathname[n+1]),l-n);
  return(basename);
}
/*--------------------------------------------------------------
  fio_extension() - returns the extension of the given filename.
  Author: Douglas Greve, 1/30/2002
  -------------------------------------------------------------*/
char *fio_extension(char *pathname)
{
  int lpathname,n, lext;
  char *ext;

  if(pathname == NULL) return(NULL);

  lpathname = strlen(pathname);

  lext = 0;
  n = lpathname - 1;
  while(n >= 0 && pathname[n] != '.') {
    n--;
    lext++;
  }

  /* A dot was not found, return NULL */
  if(n < 0) return(NULL);

  /* A dot was not found at the end of the file name */
  if(lext == 0) return(NULL);

  ext = (char *) calloc(sizeof(char),lext+1);
  memcpy(ext,&(pathname[n+1]),lext);

  return(ext);
}
/* -----------------------------------------------------
  fio_DirIsWritable(char *dirname, int fname) -- tests
  whether the given directory is writable by creating
  and deleting a junk file there. If fname != 0, then
  dirname is treated as path to a filename. It will
  return 0 if the directory does not exist.
  ----------------------------------------------------- */
int fio_DirIsWritable(char *dirname, int fname)
{
  FILE *fp;
  char tmpstr[2000];

  if(fname != 0)
    sprintf(tmpstr,"%s.junk.54_-_sdfklj",dirname);
  else
    sprintf(tmpstr,"%s/.junk.54_-_sdfklj",dirname);
  
  fp = fopen(tmpstr,"w");
  if(fp == NULL) return(0);

  fclose(fp);
  unlink(tmpstr);

  return(1);
}
/*-----------------------------------------------------
  fio_FileExistsReadable() - file exists and is readable
  -----------------------------------------------------*/
int fio_FileExistsReadable(char *fname)
{
  FILE *fp;

  fp = fopen(fname,"r");
  if(fp != NULL){
    fclose(fp);
    return(1);
  }
  return(0);
}
