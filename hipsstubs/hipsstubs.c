#if 1
int h_copy(void) ;
int h_copy(void) {return(-1);}
int perr(void) ;
int perr(void) {return(-1);}
int h_canny(void) ;
int h_canny(void) {return(-1);}
int h_enlarge(void) ;
int h_enlarge(void) {return(-1);}
int h_reduce(void) ;
int h_reduce(void) {return(-1);}
int h_invert(void) ;
int h_invert(void) {return(-1);}
#include <stdlib.h>
int h_tob(void) ;
int h_tob(void) {return(-1);}
int h_minmax(void) ;
int h_minmax(void) {return(-1);}
int h_linscale(void) ;
int h_linscale(void) {return(-1);}
int update_header(void) ;
int update_header(void) {return(-1);}
int free_hdrcon(void) ;
int free_hdrcon(void) {return(-1);}
int *memalloc(size_t, size_t)  ;
int *memalloc(size_t a, size_t b){ return(calloc(a,b));}
int init_header(void) ;
int init_header(void) {return(-1);}
int *halloc(size_t a, size_t b) ;
int *halloc(size_t a, size_t b) { return(calloc(a,b)) ; }
int free_header(void) ;
int free_header(void) {return(-1);}
int fwrite_header(void) ;
int fwrite_header(void) {return(-1);}
int fwrite_image(void) ;
int fwrite_image(void) {return(-1);}
int fread_header(void) ;
int fread_header(void) {return(-1);}
int fread_image(void) ;
int fread_image(void) {return(-1);}
int h_tof(void) ;
int h_tof(void) {return(-1);}
int h_toc(void) ;
int h_toc(void) {return(-1);}
int h_fourtr(void) ;
int h_fourtr(void) {return(-1);}
int h_invfourtr(void) ;
int h_invfourtr(void) {return(-1);}
int h_divscale(void) ;
int h_divscale(void) {return(-1);}
int h_mul(void) ;
int h_mul(void) {return(-1);}
int h_flipquad(void) ;
int h_flipquad(void) {return(-1);}
int h_softthresh(void) ;
int h_softthresh(void) {return(-1);}
int h_tod(void) ;
int h_tod(void) {return(-1);}
int h_todc(void) ;
int h_todc(void) {return(-1);}
int h_toi(void) ;
int h_toi(void) {return(-1);}
int h_mulscale(void) ;
int h_mulscale(void) {return(-1);}
int h_div(void) ;
int h_div(void) {return(-1);}
int h_add(void) ;
int h_add(void) {return(-1);}
int h_diff(void) ;
int h_diff(void) {return(-1);}
int alloc_histo(void) ;
int alloc_histo(void) {return(-1);}
int h_clearhisto(void) ;
int h_clearhisto(void) {return(-1);}
int h_histo(void) ;
int h_histo(void) {return(-1);}
int h_histoeq(void) ;
int h_histoeq(void) {return(-1);}
int h_pixmap(void) ;
int h_pixmap(void) {return(-1);}
int h_entropycnt(void) ;
int h_entropycnt(void) {return(-1);}
int h_entropy(void) ;
int h_entropy(void) {return(-1);}
#if 0
int WinShowImage(void) ;
int WinShowImage(void) {return(-1);}
int WinSetName(void) ;
int WinSetName(void) {return(-1);}
int WinCreate(void) ;
int WinCreate(void) {return(-1);}
#endif
int h_median(void) ;
int h_median(void) {return(-1);}
int hformatname(void) ;
int hformatname(void) {return(-1);}

#if 1
int hipserrno = 0 ;

int hips_hclip = 0 ;
int hips_lclip = 0 ;
int hips_oldhdr = 0 ;
int hips_cplxtor = 0 ;
int hips_rtocplx = 0 ;
#endif

void MachThreadInit(void) ;
void MachThreadInit(void) {}
void MachThreadGetTid(void) ;
void MachThreadGetTid(void) {}
void MachThreadKill(void) ;
void MachThreadKill(void) {}
void MachThreadStart(void) ;
void MachThreadStart(void) {}
void MachThreadSuspend(void) ;
void MachThreadSuspend(void) {}
void MachThreadResume(void) ;
void MachThreadResume(void) {}
void MachThreadSleep(void) ;
void MachThreadSleep(void) {}
void MachThreadYield(void) ;
void MachThreadYield(void) {}
void MachThreadExit(void) ;
void MachThreadExit(void) {}


#endif
/* getparam(hd,name,fmt,count,valuepointer) */

int getparam(void *hd, ...) { return(0) ; }
int setparam(void *hd, ...) { return(0) ; }
void *findparam(void *hd, char *name) { return(NULL) ; }
int clearparam(void *hd, char *name) { return(0) ; }

