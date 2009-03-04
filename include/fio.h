/**
 * @file  fio.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: mreuter $
 *    $Date: 2009/03/04 19:20:35 $
 *    $Revision: 1.19 $
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

//#define fwriteLong(l, fp)   fwrite4((int)l, fp)

#endif
