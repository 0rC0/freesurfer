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

double randomNumber(double low, double hi) ;
double normAngle(double angle) ;
double calcDeltaPhi(double target_phi, double robot_phi) ;
#if 1
double latan2(double y, double x) ;
#else
#define latan2(y,x)  atan2(y,x)
#endif
int    QuadEqual(double a1, double a2) ;
void   fComplementCode(double *pdIn, double *pdOut, int iLen) ;
#ifndef _HOME_
char *fgetl(char *s, int n, FILE *fp) ;
#endif

int IntSqrt(int n) ;

char *StrUpper(char *str) ;
char *StrLower(char *str) ;
char *FileName(char *str) ;
int  FileExists(char *fname) ;


#endif
