#ifndef _PROTO_H_
#define _PROTO_H_

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
/*#include <sys/time.h>*/
#include <time.h>
#include <sys/timeb.h>

#if defined(SunOS) | defined(IRIX)
#include <ieeefp.h>
#endif

/*----------------- ALL PLATFORMS *--------------------*/
int stricmp(char *str1, char *str2) ;


/*----------- SunOS -----------------------*/
#ifdef SunOSX

/* should be in stdlib.h */
#ifndef EXIT_FAILURE
#define EXIT_FAILURE  1
#endif

/* needed for select */
#include <sys/types.h>
#include <sys/time.h>

int _flsbuf(unsigned char c, FILE *p) ;
int _filbuf(FILE *p) ;
int select (int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, 
            struct timeval *timeout) ;

int puts(char *s) ;
int fputs(char *s, FILE *stream) ;
int getw(FILE *stream);
int putw(int w, FILE *stream);
int fputc(int c, FILE *stream);
int fgetc(FILE *stream);
int pclose(FILE *stream);
char *getwd(char *buf);
int ftime(struct timeb *tp) ;
/* void ftime(struct timeb *tm) ;*/
int fflush(FILE *fp) ;
int scanf(const char *fmt, ...) ;
int fscanf(FILE *fp, const char *fmt, ...) ;
int fprintf(FILE *fp, const char *fmt, ...) ;
int printf(const char *fmt, ...) ;
int vsprintf(char *str, const char *fmt, va_list args) ;
int vfprintf(FILE *fp, const char *fmt, va_list args) ;
int vprintf(const char *fmt, va_list args) ;
int sscanf(char *str, const char *fmt, ...) ;
void system(char *command_string) ;
void perror(char *s) ;
int fgetc(FILE *fp) ; 
time_t time(time_t *tloc) ;
void fclose(FILE *fp) ;
void rewind(FILE *fp) ;
char toupper(char c) ;
char tolower(char c) ;
int fseek(FILE *fp, long offset, int whence) ;
int fread(void *ptr, int size, int nitems, FILE *fp) ;
int fwrite(void *ptr, int size, int nitems, FILE *fp) ;
#endif   /* SunOS */

/*----------- Linux -----------------------*/
#ifdef Linux
#include "macros.h"
#define nint(f)   ((int)(rint((double)f)))

int getw(FILE *stream);
int putw(int w, FILE *stream);

void swab(const void *from, void *to, size_t n);

#endif   /* Linux */

#ifdef SunOS
int ftime(struct timeb *tp) ;
#define nint(f)   ((int)(rint((double)f)))
#include <ieeefp.h>
/*void bzero(void *s, int n);*/

#endif

/*----------- IRIX (SGI) -------------------*/
#ifdef IRIX
#define nint(f)   ((int)(rint((double)f)))
/*#define isnan(f)  0*/
double rint(double x) ;
#endif

/*----------- MSDOS -----------------------*/
#ifdef _MSDOS
#define nint(f)   ((int)((double)f+(f < 0 ? -0.5 : 0.5)))
#define isnan(f) 0
#define unlink _unlink
#define hypot  _hypot

#endif   /* MSDOS */

#endif   /* #ifndef PROTO_H */


