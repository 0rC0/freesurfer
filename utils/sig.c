#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef Darwin
#include <float.h>
#else
#include <values.h>
#endif

#include "error.h"
#include "sig.h"
#include "diag.h"
#include "macros.h"
#include "proto.h"

#define ITMAX 100
#define EPS 3.0e-7

float betacf(float a, float b, float x)
{
  float qap,qam,qab,em,tem,d;
  float bz,bm=1.0,bp,bpp;
  float az=1.0,am=1.0,ap,app,aold;
  int m;
  float amax = 0.0, bmax = 0.0 ;

  if (!finite(a) || !finite(b) || !finite(x))
    DiagBreak() ;
  if (a > amax) amax = a;
  if (b > bmax) bmax = b;
  qab=a+b;
  qap=a+1.0;
  qam=a-1.0;
  bz=1.0-qab*x/qap;
  for (m=1;m<=ITMAX;m++) {
    em=(float) m;
    tem=em+em;
    d=em*(b-em)*x/((qam+tem)*(a+tem));
    ap=az+d*am;
    bp=bz+d*bm;
    d = -(a+em)*(qab+em)*x/((qap+tem)*(a+tem));
    app=ap+d*az;
    bpp=bp+d*bz;
    aold=az;
    am=ap/bpp;
    bm=bp/bpp;
    az=app/bpp;
    bz=1.0;
    if (fabs(az-aold) < (EPS*fabs(az))) return az;
  }
  ErrorExit(ERROR_BADPARM, "a or b too big, or ITMAX too small in BETACF\n"
            "a=%f b=%f x=%f m=%d",a,b,x,m);
  /*
    printf("az-aold=%f az=%f\n",(az-aold),az);
  */
  return(az) ;
}

#undef ITMAX
#undef EPS

float gammln(float xx)
{
        double x,tmp,ser,res;
        static double cof[6]={76.18009173,-86.50532033,24.01409822,
                -1.231739516,0.120858003e-2,-0.536382e-5};
        int j;

        x=xx-1.0;
        tmp=x+5.5;
        tmp -= (x+0.5)*log(tmp);
        ser=1.0;
        for (j=0;j<=5;j++) {
                x += 1.0;
                ser += cof[j]/x;
        }
        res = -tmp+log(2.50662827465*ser);
        return res;
}

float betai(float a, float b, float x)
{
        float bt;

        if (x < 0.0 || x > 1.0) 
          ErrorExit(ERROR_BADPARM, "Bad x in routine BETAI");
        if (x == 0.0 || x == 1.0) bt=0.0;
        else
                bt=exp(gammln(a+b)-gammln(a)-gammln(b)+a*log(x)+b*log(1.0-x));
        if (x < (a+1.0)/(a+b+2.0))
                return bt*betacf(a,b,x)/a;
        else
                return 1.0-bt*betacf(b,a,1.0-x)/b;
}

/* End from numrec_c */


#if 1
#define MAXT    30.0
#define MAXDOF 200
/* note, everything here is Doug's fault */
double sigt(double t,int df)
{
  double sig=0.0,sig1,sig2;

  if (df==0)    
    sig = 1;
  else if (df < MAXDOF && fabs(t) < MAXT)
    sig = betai(0.5*df,0.5,df/(df+t*t));

  if(df>MAXDOF || sig < DBL_MIN){
    sig1 = erfc(fabs(t)/sqrt(2.0));        
    sig2 = 1.0/sqrt(2.0*M_PI)*exp(-0.5*(t*t)); /* use Normal approximation */
    if(sig1 > sig2)  sig = sig1;
    else             sig = sig2;
  }

  if (!finite(sig))
    printf("### Numerical error: sigt(%e,%d) = %e\n",t,df,sig);
  if(sig > 1.0) sig = 1.0;
  
  return sig;
}
#else
double sigt(double t, int df)
{
  double sig;

  if (df==0)
    sig = 1;
  else if (df<50)
    sig = betai(0.5*df,0.5,df/(df+t*t));
  else
    sig = 1/sqrt(2*M_PI)*exp(-0.5*(t*t)); /* use Normal approximation */ 
  if (!finite(sig))
    printf("### Numerical error: sigt(%e,%d) = %e\n",t,df,sig);
  return sig;
}
#endif
/*
float sigt(float t, int df)
{
  float sig;

  sig = betai(0.5*df,0.5,df/(df+t*t));
  return sig;
}
*/

float gammp(float a,float x)
{
  float gamser,gammcf,gln;

  if (x < 0.0 || a <= 0.0) 
    ErrorReturn(0.0f,
                (ERROR_BADPARM, "Invalid arguments gammp(%2.6f, %2.6f)",a,x));


  if (x < (a+1.0)) 
  {
    gser(&gamser,a,x,&gln);
    return gamser;
  } 
  else 
  {
    gcf(&gammcf,a,x,&gln);
    return 1.0-gammcf;
  }
}

float
sigchisq(double chisq, int df)
{
  float p ;

  if (FZERO(chisq) || df == 0)
    return(1.0f) ;
  p = gammp((float)df/2, (float)chisq/2.0) ;
  return(1-p) ;
}


#ifdef ITMAX
#undef ITMAX
#endif

#ifdef EPS
#undef EPS
#endif

#define ITMAX 100
#define EPS 3.0e-7

void gser(float *gamser,float a,float x, float *gln)
{
  int n;
  float sum,del,ap;

  *gln=gammln(a);
  if (x <= 0.0) {
    if (x < 0.0) 
    {
      ErrorPrintf(ERROR_BADPARM, "gser(%2.1f) x less than 0",x);
      return ;
    }
    *gamser=0.0;
    return;
  } else {
    ap=a;
    del=sum=1.0/a;
    for (n=1;n<=ITMAX;n++) {
      ap += 1.0;
      del *= x/ap;
      sum += del;
      if (fabs(del) < fabs(sum)*EPS) {
        *gamser=sum*exp(-x+a*log(x)-(*gln));
        return;
      }
    }
    ErrorPrintf(ERROR_BADPARM, "gser(%2.1f) a too large, ITMAX too small", a);
    return;
  }
}

#ifdef ITMAX
#undef ITMAX
#endif

#ifdef EPS
#undef EPS
#endif

#define ITMAX 100
#define EPS 3.0e-7

void gcf(float *gammcf, float a, float x, float *gln)
{
  int n;
  float gold=0.0,g,fac=1.0,b1=1.0;
  float b0=0.0,anf,ana,an,a1,a0=1.0;

  *gln=gammln(a);
  a1=x;
  for (n=1;n<=ITMAX;n++) {
    an=(float) n;
    ana=an-a;
    a0=(a1+a0*ana)*fac;
    b0=(b1+b0*ana)*fac;
    anf=an*fac;
    a1=x*a0+anf*a1;
    b1=x*b0+anf*b1;
    if (a1) {
      fac=1.0/a1;
      g=b1*fac;
      if (fabs((g-gold)/g) < EPS) {
        *gammcf=exp(-x+a*log(x)-(*gln))*g;
        return;
      }
      gold=g;
    }
  }
  ErrorPrintf(ERROR_BADPARM,
            "gcf(%2.1f, %2.1f): a too large, ITMAX too small", a, x);
}

#undef ITMAX
#undef EPS
