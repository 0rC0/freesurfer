/**
 * @file  utils.h
 * @brief well....utils!
 *
 */
/*
 * Original Author: Bruce Fischl
 * CVS Revision Info:
 *    $Author: mreuter $
 *    $Date: 2010/01/13 20:26:24 $
 *    $Revision: 1.36 $
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


#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define MATLAB_FILE   0
#define HIPS_FILE     1
#define LIST_FILE     2
#define UNKNOWN_FILE  3

double randomNumber(double low, double hi) ;
int    setRandomSeed(long seed) ;
double normAngle(double angle) ;
float  deltaAngle(float angle1, float angle2) ;
double calcDeltaPhi(double phi1, double phi2) ;
#if 1
double latan2(double y, double x) ;
#else
#define latan2(y,x)  atan2(y,x)
#endif
float  angleDistance(float theta1, float theta2) ;
int    QuadEqual(double a1, double a2) ;
void   fComplementCode(double *pdIn, double *pdOut, int iLen) ;
#ifndef _HOME_
char *fgetl(char *s, int n, FILE *fp) ;
#endif

int  IntSqrt(int n) ;

char *StrRemoveSpaces(char *str) ;
char *StrUpper(char *str) ;
char *StrLower(char *str) ;
char *StrSkipNumber(char *str) ;
char *StrReplace(const char *src, char *dst, char csrc, int cdst) ;

char *FileNameOnly(const char *str, char *fname) ;
char *FileNameFromWildcard(const char *inStr, char *outStr) ;
int  FileExists(const char *fname) ;
int  FileType(const char *fname) ;
int  FileNumber(const char *fname) ;
int  FileNumberOfEntries(const char *fname) ;
char *FileName(char *full_name) ;
char *FileFullName(char *full_name) ;
char *FileTmpName(char *base) ;
char *FileTmpName(char *basename) ;
void FileRename(const char *inName,const char *outName) ;
char *FileNameAbsolute(const char *fname, char *absFname) ;
char *FileNamePath(const char *fname, char *pathName) ;
char *FileNameRemoveExtension(const char *in_fname, char *out_fname) ;
char *FileNameExtension(const char *fname, char *ext) ;
char *AppendString(char *src, char *app);

int devIsinf(float value);
int devIsnan(float value);
int devFinite(float value);

int getMemoryUsed(void); // return total virtual memory used by Progname 
                     // in Kbytes. works only under Linux /proc system
void printMemoryUsed(void); // print function of the above.
char *strcpyalloc(const char *str);
int  ItemsInString(const char *str);
char *deblank(const char *str);
char *str_toupper(char *str);
double sum2stddev(double xsum, double xsum2, int nx);
int compare_ints(const void *v1,const void *v2);
int nunqiue_int_list(int *idlist, int nlist);
int *unqiue_int_list(int *idlist, int nlist, int *nunique);
int most_frequent_int_list(int *idlist, int nlist, int *nmax);

/* Necessary when Intel C/C++ compiler is used... */
void __ltoq(void);
void __qtol(void);

char *GetNthItemFromString(const char *str, int nth) ;
int CountItemsInString(const char *str) ;

/* Routines for Robust Gaussians (Median and MAD) */
float kth_smallest(float a[], int n, int k);
float quick_select(float a[], int n, int k);
float median(float a[],int n);
float mad(float a[], int n);

#endif
