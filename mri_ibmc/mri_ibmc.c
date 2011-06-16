/**
 * @file  mri_ibmc.c
 * @brief Intersection-based motion correction
 *
 * Intersection-based motion correction based on Kim, et al, TMI,
 * 2010. Registers three volumes.
 */
/*
 * Original Author: Douglas N. Greve
 * CVS Revision Info:
 *    $Author: greve $
 *    $Date: 2011/06/16 18:58:42 $
 *    $Revision: 1.2 $
 *
 * Copyright © 2011 The General Hospital Corporation (Boston, MA) "MGH"
 *
 * Terms and conditions for use, reproduction, distribution and contribution
 * are found in the 'FreeSurfer Software License Agreement' contained
 * in the file 'LICENSE' found in the FreeSurfer distribution, and here:
 *
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferSoftwareLicense
 *
 * Reporting: freesurfer@nmr.mgh.harvard.edu
 *
 */

/*
  BEGINHELP

  ENDHELP
*/

/*
  BEGINUSAGE

  ENDUSAGE
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "macros.h"
#include "utils.h"
#include "fio.h"
#include "version.h"
#include "cmdargs.h"
#include "error.h"
#include "diag.h"
#include "fsenv.h"
#include "registerio.h"
#include "matrix.h"
#include "mri2.h"
#include "transform.h"


double round(double);

#define IBMC_NL_MAX 500

//------------------------------------------------------------------------
typedef struct {
  MRI *mriA, *mriB; // just a pointer, do not dealloc
  MATRIX *PA, *PB, *Pref; // tkr reg.dat
  int SnoA, SnoB; // slice number
  double ColFoVA, RowFoVA;
  double ColFoVB, RowFoVB;
  MATRIX *FA0, *FB0;
  MATRIX *FA, *FB;
  MATRIX *RA, *RB;
  double anglesA[3],anglesB[3];
  MATRIX *A, *B;
  MATRIX *vA, *vB;
  double tAz, tBz;
  MATRIX *cAB; // cross(vA,vB)
  double cmag, cmag2; // mag and mag2 of cross
  MATRIX *vAp, *vBp, *vABp; 
  double vAdotB;
  double g,h;
  MATRIX *ve;
  double Lmin,Lmax,Ldelta;
  int nL;
  double lambda[IBMC_NL_MAX],xRef[3][IBMC_NL_MAX];
  double XA[2][IBMC_NL_MAX], XB[2][IBMC_NL_MAX];
  double colA[IBMC_NL_MAX], rowA[IBMC_NL_MAX];
  double colB[IBMC_NL_MAX], rowB[IBMC_NL_MAX];
  double IA[IBMC_NL_MAX], IB[IBMC_NL_MAX];
  double D;
  MATRIX *tmpv3; 
} IBMC_PAIR;
//-------------------------------------------
typedef struct 
{
  int nvols;
  MRI *mri[10];
  MATRIX *P[10]; 
  IBMC_PAIR **p;
  int mriRef, SnoRef;
} IBMC;
//------------------------------------------------------------------------
IBMC_PAIR *IBMCallocPair(void)
{
  IBMC_PAIR *p;
  p = (IBMC_PAIR*)calloc(1,sizeof(IBMC_PAIR));
  p->A = NULL;
  p->B = NULL;
  p->FA = NULL;
  p->FB = NULL;
  p->RA = NULL;
  p->RB = NULL;
  p->vA = NULL;
  p->vB = NULL;
  p->cAB = NULL;
  p->vABp = NULL;
  p->vAp = NULL;
  p->vBp = NULL;
  p->ve = MatrixAlloc(3,1,MATRIX_REAL);
  p->tmpv3 = MatrixAlloc(3,1,MATRIX_REAL);
  return(p);
}
//------------------------------------------------------------------------
IBMC_PAIR *IBMCinitPair(MRI *mriA,   int SnoA,   MATRIX *PA, 
			MRI *mriB,   int SnoB,   MATRIX *PB,
			MRI *mriRef, int SnoRef, MATRIX *PRef)
{
  MATRIX *Tref, *Dref, *invDref, *invPRef, *invWref;
  MATRIX *TA, *invTA, *DA, *WA, *QA;
  MATRIX *TB, *invTB, *DB, *WB, *QB;

  IBMC_PAIR *p;
  p = IBMCallocPair();
  p->mriA = mriA;
  p->mriB = mriB;
  p->SnoA = SnoA;
  p->SnoB = SnoB;
  p->ColFoVA = mriA->xsize * mriA->width;
  p->RowFoVA = mriA->ysize * mriA->height;
  p->ColFoVB = mriB->xsize * mriB->width;
  p->RowFoVB = mriB->ysize * mriB->height;

  if(PA != NULL)  p->PA = MatrixCopy(PA,NULL);
  else            p->PA = MatrixIdentity(4,NULL);
  if(PB != NULL)  p->PB = MatrixCopy(PB,NULL);
  else            p->PB = MatrixIdentity(4,NULL);


  Tref = MRIxfmCRS2XYZtkreg(mriRef);
  Dref = MatrixIdentity(4,NULL);
  Dref->rptr[3][4] = -SnoRef;
  invDref = MatrixInverse(Dref,NULL);
  invWref = MatrixIdentity(4,NULL);
  invWref->rptr[1][1] = 1/mriRef->xsize;
  invWref->rptr[2][2] = 1/mriRef->ysize;
  invWref->rptr[3][3] = 1/mriRef->zsize;
  if(PRef != NULL) invPRef = MatrixInverse(PRef,NULL);
  else             invPRef = MatrixIdentity(4,NULL);

  TA = MRIxfmCRS2XYZtkreg(mriA);
  invTA = MatrixInverse(TA,NULL);
  DA = MatrixIdentity(4,NULL);
  DA->rptr[3][4] = -p->SnoA;
  WA = MatrixIdentity(4,NULL);
  WA->rptr[1][1] = mriA->xsize;
  WA->rptr[2][2] = mriA->ysize;
  WA->rptr[3][3] = mriA->zsize;
  //QA = inv(TA)*PA*inv(PRef)*Tref
  QA = MatrixMultiply(invTA,p->PA,NULL);
  QA = MatrixMultiply(QA,invPRef,QA);
  QA = MatrixMultiply(QA,Tref,QA);
  //FA0 = WA*DA*QA*inv(Dref)*inv(Wref);
  p->FA0 = MatrixMultiply(WA,DA,NULL);
  p->FA0 = MatrixMultiply(p->FA0,QA,p->FA0);
  p->FA0 = MatrixMultiply(p->FA0,invDref,p->FA0);
  p->FA0 = MatrixMultiply(p->FA0,invWref,p->FA0);

  TB = MRIxfmCRS2XYZtkreg(mriB);
  invTB = MatrixInverse(TB,NULL);
  DB = MatrixIdentity(4,NULL);
  DB->rptr[3][4] = -p->SnoB;
  WB = MatrixIdentity(4,NULL);
  WB->rptr[1][1] = mriB->xsize;
  WB->rptr[2][2] = mriB->ysize;
  WB->rptr[3][3] = mriB->zsize;
  //QB = inv(TB)*PB*inv(PRef)*Tref
  QB = MatrixMultiply(invTB,p->PB,NULL);
  QB = MatrixMultiply(QB,invPRef,QB);
  QB = MatrixMultiply(QB,Tref,QB);
  //FB0 = WB*DB*QB*inv(Dref)*inv(Wref);
  p->FB0 = MatrixMultiply(WB,DB,NULL);
  p->FB0 = MatrixMultiply(p->FB0,QB,p->FB0);
  p->FB0 = MatrixMultiply(p->FB0,invDref,p->FB0);
  p->FB0 = MatrixMultiply(p->FB0,invWref,p->FB0);

  MatrixFree(&Tref); MatrixFree(&Dref); MatrixFree(&invDref); MatrixFree(&invPRef);MatrixFree(&invWref);
  MatrixFree(&TA);MatrixFree(&invTA);MatrixFree(&DA);MatrixFree(&WA);MatrixFree(&QA);
  MatrixFree(&TB);MatrixFree(&invTB);MatrixFree(&DB);MatrixFree(&WB);MatrixFree(&QB);

  return(p);
}

//------------------------------------------------------------------------
int IBMClambdaLimitPair(IBMC_PAIR *p)
{
  static MATRIX *fr1=NULL,*fr2=NULL;
  double d1,d2,LA1,LA2,LB1,LB2,tmp1,tmp2;
  int k;

  if(fr1==NULL){
    fr1 = MatrixAlloc(3,1,MATRIX_REAL);
    fr2 = MatrixAlloc(3,1,MATRIX_REAL);
  }

  //A --------------------------------------------
  for(k=1;k<=3;k++){
    fr1->rptr[k][1] = p->FA->rptr[1][k];
    fr2->rptr[k][1] = p->FA->rptr[2][k];
  }
  d1 = VectorDot(fr1,p->vABp);
  d2 = VectorDot(fr2,p->vABp);
  if(fabs(d1) > fabs(d2)){
    tmp1 = -(p->FA->rptr[1][4]+VectorDot(fr1,p->ve))/d1;
    tmp2 = (p->ColFoVA-(p->FA->rptr[1][4]+VectorDot(fr1,p->ve)))/d1;
  }
  else{
    tmp1 = -(p->FA->rptr[2][4]+VectorDot(fr2,p->ve))/d2;
    tmp2 = (p->RowFoVA-(p->FA->rptr[2][4]+VectorDot(fr2,p->ve)))/d2;
  }
  if(tmp1 < tmp2) {LA1=tmp1;LA2=tmp2;}
  else            {LA1=tmp2;LA2=tmp1;}

  //B --------------------------------------------
  for(k=1;k<=3;k++){
    fr1->rptr[k][1] = p->FB->rptr[1][k];
    fr2->rptr[k][1] = p->FB->rptr[2][k];
  }
  d1 = VectorDot(fr1,p->vABp);
  d2 = VectorDot(fr2,p->vABp);
  if(fabs(d1) > fabs(d2)){
    tmp1 = -(p->FB->rptr[1][4]+VectorDot(fr1,p->ve))/d1;
    tmp2 = (p->ColFoVB-(p->FB->rptr[1][4]+VectorDot(fr1,p->ve)))/d1;
  }
  else{
    tmp1 = -(p->FB->rptr[2][4]+VectorDot(fr2,p->ve))/d2;
    tmp2 = (p->RowFoVB-(p->FB->rptr[2][4]+VectorDot(fr2,p->ve)))/d2;
  }
  if(tmp1 < tmp2) {LB1=tmp1;LB2=tmp2;}
  else            {LB1=tmp2;LB2=tmp1;}

  //A and B --------------------------------------------
  p->Lmin = MAX(LA1,LB1);
  p->Lmax = MIN(LA2,LB2);

  return(0);
}
//------------------------------------------------------------------------
IBMC_PAIR *IBMCsetupPair(IBMC_PAIR *p, double betaA[6], double betaB[6])
{
  int k;

  for(k=0;k<3;k++){
    p->anglesA[k] = betaA[k];
    p->anglesB[k] = betaB[k];
  }
  p->A = MRIangles2RotMat(p->anglesA);
  p->B = MRIangles2RotMat(p->anglesB);
  for(k=0;k<3;k++){  
    p->A->rptr[k+1][4] = betaA[k];
    p->B->rptr[k+1][4] = betaB[k];
  }
  p->FA = MatrixMultiply(p->A,p->FA0,p->FA);
  p->FB = MatrixMultiply(p->B,p->FB0,p->FB);
  p->RA = MatrixCopyRegion(p->FA,p->RA, 1,1, 3,3, 1,1);
  p->RB = MatrixCopyRegion(p->FB,p->RB, 1,1, 3,3, 1,1);

  p->tmpv3 = MatrixCopyRegion(p->RA,p->vA, 3,1, 1,3, 1,1);
  p->vA = MatrixTranspose(p->tmpv3,p->vA);

  p->tmpv3 = MatrixCopyRegion(p->RB,p->vB, 3,1, 1,3, 1,1);
  p->vB = MatrixTranspose(p->tmpv3,p->vB);

  p->tAz = p->FA->rptr[3][4];
  p->tBz = p->FB->rptr[3][4];
  p->cAB = VectorCrossProduct(p->vA,p->vB,p->cAB);
  p->cmag2 = 0;
  for(k=0;k<3;k++) p->cmag2 += (p->cAB->rptr[k+1][1]*p->cAB->rptr[k+1][1]);
  p->cmag = sqrt(p->cmag2);
  p->vABp = MatrixScalarMul(p->cAB,1/p->cmag,p->vABp);
  p->vAdotB = VectorDot(p->vA,p->vB);

  p->vAp = MatrixScalarMul(p->vA,p->vAdotB,p->vAp);
  p->vAp = MatrixSubtract(p->vAp,p->vB,p->vAp);
  p->vAp = MatrixScalarMul(p->vAp,1/p->cmag,p->vAp);

  p->vBp = MatrixScalarMul(p->vB,p->vAdotB,p->vBp);
  p->vBp = MatrixSubtract(p->vBp,p->vA,p->vBp);
  p->vBp = MatrixScalarMul(p->vBp,1/p->cmag,p->vBp);

  p->g = (p->tBz*p->vAdotB-p->tAz)/p->cmag2;
  p->h = (p->tAz*p->vAdotB-p->tBz)/p->cmag2;

  for(k=0; k< 3; k++) p->ve->rptr[k+1][1] = p->g*p->vA->rptr[k+1][1] + p->h*p->vB->rptr[k+1][1];
  IBMClambdaLimitPair(p);

  return(p);
}
//-----------------------------------------------------------------
int IBMCprintPairCoords(IBMC_PAIR *p,FILE *fp)
{
  int n;
  for(n=0; n < p->nL; n++){
    fprintf(fp,"%8.3f %5.1f %5.1f %8.3f %8.3f %8.3f  %8.3f %8.3f  %8.3f %8.3f \n",
	    p->lambda[n],p->IA[n],p->IB[n],p->xRef[0][n],p->xRef[1][n],p->xRef[2][n],
	    p->colA[n],p->rowA[n],p->colB[n],p->rowB[n]);

  }
  return(0);
}
//-----------------------------------------------------------------
int IBMCprintPair(IBMC_PAIR *p,FILE *fp)
{
  int k;
  fprintf(fp,"nColsA nRowsA %d %d\n",p->mriA->width,p->mriA->height);
  fprintf(fp,"ColResA RowResA %f %f\n",p->mriA->xsize,p->mriA->ysize);
  fprintf(fp,"ColFoVA RowFoVA %f %f\n",p->ColFoVA,p->RowFoVA);

  fprintf(fp,"nColsB nRowsB %d %d\n",p->mriB->width,p->mriB->height);
  fprintf(fp,"ColResB RowResB %f %f\n",p->mriB->xsize,p->mriB->ysize);
  fprintf(fp,"ColFoVB RowFoVB %f %f\n",p->ColFoVB,p->RowFoVB);

  fprintf(fp,"SnoA %d\n",p->SnoA);
  fprintf(fp,"SnoB %d\n",p->SnoB);
  fprintf(fp,"FA0--------\n");
  MatrixPrint(fp,p->FA0);
  fprintf(fp,"FB0--------\n");
  MatrixPrint(fp,p->FB0);

  fprintf(fp,"AnglesA ");
  for(k=0;k<3;k++) fprintf(fp,"%7.4f ",p->anglesA[k]);
  fprintf(fp,"\n");

  fprintf(fp,"AnglesB ");
  for(k=0;k<3;k++) fprintf(fp,"%7.4f ",p->anglesB[k]);
  fprintf(fp,"\n");

  fprintf(fp,"A--------\n");
  MatrixPrint(fp,p->A);
  fprintf(fp,"B--------\n");
  MatrixPrint(fp,p->B);

  fprintf(fp,"FA--------\n");
  MatrixPrint(fp,p->FA);
  fprintf(fp,"FB--------\n");
  MatrixPrint(fp,p->FB);

  fprintf(fp,"RA--------\n");
  MatrixPrint(fp,p->RA);
  fprintf(fp,"RB--------\n");
  MatrixPrint(fp,p->RB);

  fprintf(fp,"tAz %g\n",p->tAz);
  fprintf(fp,"tBz %g\n",p->tBz);

  fprintf(fp,"vA--------\n");
  MatrixPrint(fp,p->vA);
  fprintf(fp,"vB--------\n");
  MatrixPrint(fp,p->vB);

  fprintf(fp,"cAB cmag=%6.4f cmag2=%6.4f--------\n",p->cmag,p->cmag2);
  MatrixPrint(fp,p->cAB);

  fprintf(fp,"vABp --------\n");
  MatrixPrint(fp,p->vABp);
  fprintf(fp,"vAp --------\n");
  MatrixPrint(fp,p->vAp);
  fprintf(fp,"vBp --------\n");
  MatrixPrint(fp,p->vBp);

  fprintf(fp,"g %6.4f\n",p->g);
  fprintf(fp,"h %6.4f\n",p->h);
  fprintf(fp,"vAdotB %6.4f\n",p->vAdotB);
  fprintf(fp,"ve --------\n");
  MatrixPrint(fp,p->ve);

  fprintf(fp,"Lmin %g  Lmax %g  nL %d\n",p->Lmin,p->Lmax,p->nL);
  fprintf(fp,"D %g\n",p->D);

  return(0);
}

int IBMCfloatCoords(IBMC_PAIR *p, double Ldelta)
{
  int n,k;
  static VECTOR *xRef=NULL, *XA=NULL, *XB=NULL;

  if(xRef == NULL){
    xRef = MatrixAlloc(4,1,MATRIX_REAL);
    xRef->rptr[4][1] = 1;
    XA = MatrixAlloc(4,1,MATRIX_REAL);
    XB = MatrixAlloc(4,1,MATRIX_REAL);
  }

  p->nL = (int)round((p->Lmax-p->Lmin)/Ldelta);
  if(p->nL > IBMC_NL_MAX){
    printf("ERROR: nL = %d > %d, Ldelta = %g\n",p->nL,IBMC_NL_MAX,Ldelta);
    IBMCprintPair(p,stdout);
    return(1);
  }

  for(n=0; n < p->nL; n++){
    p->lambda[n] = p->Lmin + n*Ldelta;
    for(k=1; k<=3; k++) {
      p->xRef[k-1][n] = p->lambda[n] * p->vABp->rptr[k][1] + p->ve->rptr[k][1];
      xRef->rptr[k][1] = p->xRef[k-1][n];
    }
    XA = MatrixMultiply(p->FA,xRef, XA);
    XB = MatrixMultiply(p->FB,xRef, XB);
    fflush(stdout);
    for(k=1; k<=2; k++) {
      p->XA[k-1][n] = (double)XA->rptr[k][1];
      p->XB[k-1][n] = XB->rptr[k][1];
    }
    p->colA[n] = p->XA[0][n]/p->mriA->xsize;
    p->rowA[n] = p->XA[1][n]/p->mriA->ysize;
    p->colB[n] = p->XB[0][n]/p->mriB->xsize;
    p->rowB[n] = p->XB[1][n]/p->mriB->ysize;

  }

  return(0);
}
//-----------------------------------------------------
int IBMCsamplePair(IBMC_PAIR *p)
{
  int n,icsA,irsA,icsB,irsB;
  float valvect;

  p->D = 0;
  for(n=0; n < p->nL; n++){
    icsA = nint(p->colA[n]);
    irsA = nint(p->rowA[n]);
    if(icsA < 0 || icsA >= p->mriA->width)  continue;
    if(irsA < 0 || irsA >= p->mriA->height) continue;
    
    icsB = nint(p->colB[n]);
    irsB = nint(p->rowB[n]);
    if(icsB < 0 || icsB >= p->mriB->width)  continue;
    if(irsB < 0 || irsB >= p->mriB->height) continue;
    
    MRIsampleSeqVolume(p->mriA, p->colA[n], p->rowA[n], p->SnoA, &valvect, 0, 0);
    p->IA[n] = valvect;
    //p->IA[n] = MRIgetVoxVal(p->mriA,icsA,irsA,p->SnoA,0);

    MRIsampleSeqVolume(p->mriB, p->colB[n], p->rowB[n], p->SnoB, &valvect, 0, 0);
    p->IB[n] = valvect;
    //p->IB[n] = MRIgetVoxVal(p->mriB,icsB,irsB,p->SnoB,0);

    p->D += (p->IA[n]-p->IB[n])*(p->IA[n]-p->IB[n]);
  }
  p->D /= p->nL;

  return(0);
}


static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void dump_options(FILE *fp);
int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_ibmc.c,v 1.2 2011/06/16 18:58:42 greve Exp $";
char *Progname = NULL;
char *cmdline, cwd[2000];
int debug=0;
int checkoptsonly=0;
struct utsname uts;

char *V1File=NULL,*V2File=NULL,*V3File=NULL;
char *V1PFile=NULL,*V2PFile=NULL,*V3PFile=NULL;
char tmpstr[2000];
FILE *fp;

/*---------------------------------------------------------------*/
int main(int argc, char *argv[]) 
{
  int nargs,k;
  MRI *vol1, *vol2, *vol3;
  MATRIX *P1=NULL, *P2=NULL, *P3=NULL;
  IBMC_PAIR *p=NULL;
  double betaA[6] =  {0,0,0,0,0,0};
  double betaB[6] =  {0,0,0,0,0,0};

  nargs = handle_version_option (argc, argv, vcid, "$Name:  $");
  if (nargs && argc - nargs == 1) exit (0);
  argc -= nargs;
  cmdline = argv2cmdline(argc,argv);
  uname(&uts);
  getcwd(cwd,2000);

  Progname = argv[0] ;
  argc --;
  argv++;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;
  if (argc == 0) usage_exit();
  parse_commandline(argc, argv);
  check_options();
  if (checkoptsonly) return(0);
  dump_options(stdout);

  vol1 = MRIread(V1File);
  if(vol1 == NULL) exit(1);
  vol2 = MRIread(V2File);
  if(vol2 == NULL) exit(1);
  vol3 = MRIread(V3File);
  if(vol3 == NULL) exit(1);

  if(V1PFile) P1 = regio_read_registermat(V1PFile);
  if(V2PFile) P2 = regio_read_registermat(V2PFile);
  if(V3PFile) P3 = regio_read_registermat(V3PFile);

  fp = fopen("cost.dat","w");
  for(k=0;k<1000;k++){
    p = IBMCinitPair(vol1,10,P1,vol2,11,P2,vol3,10,P3);
    betaA[2] = (10*(k-500)/500)*M_PI/180;
    IBMCsetupPair(p, betaA, betaB);
    IBMCfloatCoords(p, 1);
    IBMCsamplePair(p);
    printf("%2d %8.7f %7.4f\n",k,betaA[2],p->D);
    fprintf(fp,"%2d %8.7f %7.4f\n",k,betaA[2]*180/M_PI,p->D);
    //sprintf(tmpstr,"coords.%02d.dat",k);
    //fp = fopen(tmpstr,"w");
    //IBMCprintPairCoords(p,fp);
    //fclose(fp);
  }
  fclose(fp);



  return 0;
}

/*------------------------------------------------------------------*/
static int parse_commandline(int argc, char **argv) {
  int  nargc , nargsused;
  char **pargv, *option ;

  if (argc < 1) usage_exit();

  nargc   = argc;
  pargv = argv;
  while (nargc > 0) {

    option = pargv[0];
    if (debug) printf("%d %s\n",nargc,option);
    nargc -= 1;
    pargv += 1;

    nargsused = 0;

    if (!strcasecmp(option, "--help"))  print_help() ;
    else if (!strcasecmp(option, "--version")) print_version() ;
    else if (!strcasecmp(option, "--debug"))   debug = 1;
    else if (!strcasecmp(option, "--checkopts"))   checkoptsonly = 1;
    else if (!strcasecmp(option, "--nocheckopts")) checkoptsonly = 0;

    else if (!strcasecmp(option, "--v1")) {
      if (nargc < 1) CMDargNErr(option,1);
      V1File = pargv[0];
      nargsused = 1;
      if(CMDnthIsArg(nargc, pargv, 1)) {
        V1PFile = pargv[1];
        nargsused ++;
      }
    } 
    else if (!strcasecmp(option, "--v2")) {
      if (nargc < 1) CMDargNErr(option,1);
      V2File = pargv[0];
      nargsused = 1;
      if(CMDnthIsArg(nargc, pargv, 1)) {
        V2PFile = pargv[1];
        nargsused ++;
      }
    } 
    else if (!strcasecmp(option, "--v3")) {
      if (nargc < 1) CMDargNErr(option,1);
      V3File = pargv[0];
      nargsused = 1;
      if(CMDnthIsArg(nargc, pargv, 1)) {
        V3PFile = pargv[1];
        nargsused ++;
      }
    } 
    else {
      fprintf(stderr,"ERROR: Option %s unknown\n",option);
      if (CMDsingleDash(option))
        fprintf(stderr,"       Did you really mean -%s ?\n",option);
      exit(-1);
    }
    nargc -= nargsused;
    pargv += nargsused;
  }
  return(0);
}
/*------------------------------------------------------------------*/
static void usage_exit(void) {
  print_usage() ;
  exit(1) ;
}
/*------------------------------------------------------------------*/
static void print_usage(void) {
  printf("USAGE: %s \n",Progname) ;
  printf("\n");
  printf("   --v1 vol1 <regfile>: template volume \n");
  printf("   --v2 vol2 <regfile>: template volume \n");
  printf("   --v3 vol3 <regfile>: template volume \n");
  printf("\n");
  printf("   --debug     turn on debugging\n");
  printf("   --checkopts don't run anything, just check options and exit\n");
  printf("   --help      print out information on how to use this program\n");
  printf("   --version   print out version and exit\n");
  printf("\n");
  printf("%s\n", vcid) ;
  printf("\n");
}
/*------------------------------------------------------------------*/
static void print_help(void) {
  print_usage() ;
  printf("WARNING: this program is not yet tested!\n");
  exit(1) ;
}
/*------------------------------------------------------------------*/
static void print_version(void) {
  printf("%s\n", vcid) ;
  exit(1) ;
}
/*------------------------------------------------------------------*/
static void check_options(void) {
  if(V1File == NULL){
    printf("ERROR: must spec v1\n");
    exit(1);
  }
  if(V2File == NULL){
    printf("ERROR: must spec v2\n");
    exit(1);
  }
  if(V3File == NULL){
    printf("ERROR: must spec v3\n");
    exit(1);
  }

  return;
}
/*------------------------------------------------------------------*/
static void dump_options(FILE *fp) {
  fprintf(fp,"\n");
  fprintf(fp,"%s\n",vcid);
  fprintf(fp,"cwd %s\n",cwd);
  fprintf(fp,"cmdline %s\n",cmdline);
  fprintf(fp,"sysname  %s\n",uts.sysname);
  fprintf(fp,"hostname %s\n",uts.nodename);
  fprintf(fp,"machine  %s\n",uts.machine);
  fprintf(fp,"user     %s\n",VERuser());
  fprintf(fp,"V1      %s %s\n",V1File,V1PFile);
  fprintf(fp,"V2      %s %s\n",V2File,V2PFile);
  fprintf(fp,"V3      %s %s\n",V3File,V3PFile);

  return;
}
