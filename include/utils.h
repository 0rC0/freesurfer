/*
   @(#)utils.h  1.9
   10/16/95
*/
/*------------------------------------------------------------------------
      File Name:  utils.h

         Author:  Bruce Fischl

        Created:  Jan. 1994

    Description:  

------------------------------------------------------------------------*/
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define MATLAB_FILE   0
#define HIPS_FILE     1
#define LIST_FILE     2
#define UNKNOWN_FILE  3

double randomNumber(double low, double hi) ;
double normAngle(double angle) ;
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

int IntSqrt(int n) ;

char *StrRemoveSpaces(char *str) ;
char *StrUpper(char *str) ;
char *StrLower(char *str) ;
char *StrSkipNumber(char *str) ;

char *FileName(char *str) ;
int  FileExists(char *fname) ;
int  FileType(char *fname) ;
int  FileNumber(char *fname) ;
int  FileNumberOfEntries(char *fname) ;
char *FileFullName(char *full_name) ;
char *FileTmpName(char *base) ;
char *FileTmpName(char *basename) ;
void FileRename(char *inName, char *outName) ;
char *FileNameAbsolute(char *fname, char *absFname) ;
char *FileNamePath(char *fname, char *pathName) ;

#endif
