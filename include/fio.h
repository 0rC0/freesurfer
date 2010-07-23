/**
 * @file  fio.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: ayendiki $
 *    $Date: 2010/07/23 21:07:44 $
 *    $Revision: 1.22 $
 *
 * Copyright (C) 2002-2007,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 * Bug reports: analysis-bugs@nmr.mgh.harvard.edu
 *
 */


#ifndef FIO_H
#define FIO_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "znzlib.h"

FILE  *MGHopen_file(const char *fname,const char *rwmode) ;
int   putf(float f, FILE *fp) ;
float getf(FILE *fp) ;

int   fread1(int *v, FILE *fp) ;
int   fread2(int *v, FILE *fp) ;
int   fread3(int *v, FILE *fp) ;
int   fread4(float *v, FILE *fp) ;
double freadDouble(FILE *fp) ;
float freadFloat(FILE *fp) ;
int   freadInt(FILE *fp) ;
long  long freadLong(FILE *fp) ;
short freadShort(FILE *fp) ;

/* return 1 if succeed, return 0 if fail */
int freadDoubleEx(double *pd, FILE *fp) ;
int freadFloatEx(float *pf, FILE *fp) ;
int freadIntEx(int *pi, FILE *fp) ;
int freadShortEx(short *ps, FILE *fp) ;

int   fwriteDouble(double d, FILE *fp) ;
int   fwriteFloat(float f, FILE *fp) ;
int   fwriteShort(short s, FILE *fp) ;
int   fwriteInt(int v, FILE *fp) ;
int   fwriteLong(long long v, FILE *fp) ;
int   fwrite1(int v,FILE *fp) ;
int   fwrite2(int v, FILE *fp) ;
int   fwrite3(int v, FILE *fp) ;
int   fwrite4(int v, FILE *fp) ;

/* znzlib support routines */
int   znzread1(int *v, znzFile fp) ;
int   znzread2(int *v, znzFile fp) ;
int   znzread3(int *v, znzFile fp) ;
int   znzread4(float *v, znzFile fp) ;
double 	znzreadDouble  (znzFile fp) ;
float   znzreadFloat	  (znzFile fp) ;
int     znzreadInt     (znzFile fp) ;
long long znzreadLong  (znzFile fp) ;
short   znzreadShort   (znzFile fp) ;

/* return 1 if succeed, return 0 if fail */
int znzreadDoubleEx  (double *pd,  znzFile fp) ;
int znzreadFloatEx   (float *pf,   znzFile fp) ;
int znzreadIntEx     (int *pi,     znzFile fp) ;
int znzreadShortEx   (short *ps,   znzFile fp) ;

int znzwriteDouble (double d,  znzFile fp) ;
int znzwriteFloat  (float f,   znzFile fp) ;
int znzwriteShort  (short s,   znzFile fp) ;
int znzwriteInt    (int v,     znzFile fp) ;
int znzwriteLong   (long long v, znzFile fp) ;
int znzwrite1      (int v,     znzFile fp) ;
int znzwrite2      (int v,     znzFile fp) ;
int znzwrite3      (int v,     znzFile fp) ;
int znzwrite4      (int v,     znzFile fp) ;

char *fio_basename(const char *pathname,const char *ext);
char *fio_dirname(const char *pathname);
char *fio_extension(const char *pathname);
int fio_DirIsWritable(const char *dirname, int fname);
int fio_FileExistsReadable(const char *fname);
int fio_IsDirectory(const char *fname);
int fio_NLines(const char *fname);

int fio_pushd(const char *dir);
int fio_popd(void);
char *fio_fullpath(const char *fname);
int fio_mkdirp(const char *path, mode_t mode);

//#define fwriteLong(l, fp)   fwrite4((int)l, fp)

#if defined(__cplusplus)
};
#endif

#endif
