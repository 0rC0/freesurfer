/* 
   fmriutils.c 
   $Id: fmriutils.c,v 1.16 2005/09/26 18:34:47 greve Exp $

Things to do:
1. Add flag to turn use of weight on and off


*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
double round(double x);
#include "matrix.h"
#include "mri.h"
#include "MRIio_old.h"
#include "sig.h"
#include "fmriutils.h"
#include "fsglm.h"
#include "gsl/gsl_cdf.h"

#ifdef X
#undef X
#endif

/* --------------------------------------------- */
// Return the CVS version of this file.
const char *fMRISrcVersion(void) { 
  return("$Id: fmriutils.c,v 1.16 2005/09/26 18:34:47 greve Exp $");
}
/*--------------------------------------------------------*/
MRI *fMRImatrixMultiply(MRI *inmri, MATRIX *M, MRI *outmri)
{
  int c, r, s, fin, fout;
  int nframesout;
  float val;
  int nin, nout;
  int nin0, nout0;
  float *pin=NULL, *pout=NULL;

  if(inmri->nframes != M->cols){
    printf("ERROR: fMRImatrixMultiply: input dimension mismatch\n");
    return(NULL);
  }

  nframesout = M->rows;
  if(outmri==NULL){
    outmri = MRIallocSequence(inmri->width, inmri->height, inmri->depth, 
			      MRI_FLOAT, nframesout);
    if(outmri==NULL){
      printf("ERROR: fMRImatrixMultiply: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(inmri,outmri);
  }
  else{
    if(outmri->width  != inmri->width || 
       outmri->height != inmri->height || 
       outmri->depth  != inmri->depth || 
       outmri->nframes != nframesout){
      printf("ERROR: fMRImatrixMultiply: output dimension mismatch\n");
      return(NULL);
    }
    if(outmri->type != MRI_FLOAT){
      printf("ERROR: fMRImatrixMultiply: structure passed is not MRI_FLOAT\n");
      return(NULL);
    }
  }

  MRIclear(outmri);
  for(fout=0; fout < outmri->nframes; fout++){
    nout0 = fout*outmri->depth;
    for(fin=0; fin < inmri->nframes; fin++){
      val = M->rptr[fout+1][fin+1];
      nin0 = fin*inmri->depth;
      for(s=0; s < outmri->depth; s++){
	nin  = s + nin0;
	nout = s + nout0;
	for(r=0; r < outmri->height; r++){
	  pin = (float*)inmri->slices[nin][r];
	  pout = (float*)outmri->slices[nout][r];
	  for(c=0; c < outmri->width; c++)
	    (*pout++) += val*(*pin++);
	}
      }
    }
  }

  return(outmri);
}
/*--------------------------------------------------------*/
MRI *fMRIvariance(MRI *fmri, float DOF, int RmMean, MRI *var)
{
  int c, r, s, f;
  float val,sumsqval, sumval;

  if(DOF < 0) DOF = fmri->nframes;

  if(var==NULL){
    var = MRIallocSequence(fmri->width, fmri->height, fmri->depth, 
			      MRI_FLOAT, 1);
    if(var==NULL){
      printf("ERROR: fMRIvariance: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(fmri,var);
  }
  else{
    if(var->width  != fmri->width || 
       var->height != fmri->height || 
       var->depth  != fmri->depth){
      printf("ERROR: fMRIvariance: output dimension mismatch\n");
      return(NULL);
    }
    if(var->type != MRI_FLOAT){
      printf("ERROR: fMRIvariance: structure passed is not MRI_FLOAT\n");
      return(NULL);
    }
  }

  for(c=0; c < fmri->width; c++){
    for(r=0; r < fmri->height; r++){
      for(s=0; s < fmri->depth; s++){
	sumval = 0;
	sumsqval = 0;
	for(f=0; f < fmri->nframes; f++){
	  val = MRIgetVoxVal(fmri, c, r, s, f);
	  sumsqval += (val*val);
	  if(RmMean) sumval += val;
	}
	MRIFseq_vox(var,c,r,s,0) = sumsqval/DOF;
	if(RmMean)
	  MRIFseq_vox(var,c,r,s,0) -= ((sumval/DOF)*(sumval/DOF));
      }
    }
  }

  return(var);
}
/*--------------------------------------------------------
  fMRIsumSquare() - computes the sum of the squares over the
  frames. If the Update flag is set, then the sum of the 
  squares is added to that already in sumsqr. If sumsqr
  is NULL, it will be allocated.
  --------------------------------------------------------*/
MRI *fMRIsumSquare(MRI *fmri, int Update, MRI *sumsqr)
{
  int c, r, s, f, n;
  float v;
  float *pfmri=NULL, *psumsqr = NULL;

  if(sumsqr==NULL){
    sumsqr = MRIallocSequence(fmri->width, fmri->height, fmri->depth, 
			      MRI_FLOAT, 1);
    if(sumsqr==NULL){
      printf("ERROR: fMRIsumSquare: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(fmri,sumsqr);
  }
  else{
    if(sumsqr->width  != fmri->width || 
       sumsqr->height != fmri->height || 
       sumsqr->depth  != fmri->depth){
      printf("ERROR: fMRIsumsqriance: output dimension mismatch\n");
      return(NULL);
    }
    if(sumsqr->type != MRI_FLOAT){
      printf("ERROR: fMRIsumsqriance: structure passed is not MRI_FLOAT\n");
      return(NULL);
    }
  }

  if(Update) MRIclear(sumsqr);
  n = 0;
  for(f=0; f < fmri->nframes; f++){
    for(s=0; s < fmri->depth; s++){
      for(r=0; r < fmri->height; r++){
	pfmri   = (float *)   fmri->slices[n][r];
	psumsqr = (float *) sumsqr->slices[s][r];
	for(c=0; c < fmri->width; c++){
	  v = (*pfmri++);
	  (*psumsqr++) += (v*v);
	}
      }
      n++;
    }
  }

  return(sumsqr);
}
/*--------------------------------------------------------------------*/
MRI *fMRIcomputeT(MRI *ces, MATRIX *X, MATRIX *C, MRI *var, MRI *t)
{
  int c, r, s;
  MATRIX *Xt, *XtX, *iXtX, *CiXtX, *Ct, *CiXtXCt;
  float srf, cesval, std;

  if(C->rows != 1){
    printf("ERROR: fMRIcomputeT: contrast matrix has more than 1 row.\n");
    return(NULL);
  }

  if(ces->nframes != 1){
    printf("ERROR: fMRIcomputeT: contrast effect size has "
	   "more than 1 frame.\n");
    return(NULL);
  }

  if(t==NULL){
    t = MRIallocSequence(ces->width, ces->height, ces->depth, 
			      MRI_FLOAT, 1);
    if(t==NULL){
      printf("ERROR: fMRIcomputeT: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(ces,t);
  }
  else{
    if(t->width  != ces->width || 
       t->height != ces->height || 
       t->depth  != ces->depth){
      printf("ERROR: fMRIcomputeT: output dimension mismatch\n");
      return(NULL);
    }
    if(t->type != MRI_FLOAT){
      printf("ERROR: fMRIcomputeT: structure passed is not MRI_FLOAT\n");
      return(NULL);
    }
  }

  Xt      = MatrixTranspose(X,NULL);
  XtX     = MatrixMultiply(Xt,X,NULL);
  iXtX    = MatrixInverse(XtX,NULL);
  CiXtX   = MatrixMultiply(C,iXtX,NULL);
  Ct      = MatrixTranspose(C,NULL);
  CiXtXCt = MatrixMultiply(CiXtX,Ct,NULL);
  srf = sqrt(CiXtXCt->rptr[1][1]);
  //printf("fMRIcomputeT: srf = %g\n",srf);

  for(c=0; c < ces->width; c++){
    for(r=0; r < ces->height; r++){
      for(s=0; s < ces->depth; s++){
	std = sqrt(MRIgetVoxVal(var, c, r, s, 0));
	if(std == 0)
	  MRIFseq_vox(t,c,r,s,0) = 0;
	else{
	  cesval = MRIgetVoxVal(ces, c, r, s, 0);
	  MRIFseq_vox(t,c,r,s,0) = cesval/(srf*std);
	}
      }
    }
  }

  MatrixFree(&Xt);
  MatrixFree(&XtX);
  MatrixFree(&iXtX);
  MatrixFree(&CiXtX);
  MatrixFree(&Ct);
  MatrixFree(&CiXtXCt);

  return(t);
}
/*--------------------------------------------------------*/
MRI *fMRIsigT(MRI *t, float DOF, MRI *sig)
{
  int c, r, s, f;
  float tval, sigtval;

  if(sig==NULL){
    sig = MRIallocSequence(t->width, t->height, t->depth, 
			   MRI_FLOAT, t->nframes);
    if(sig==NULL){
      printf("ERROR: fMRIsigT: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(t,sig);
  }
  else{
    if(t->width   != sig->width  || 
       t->height  != sig->height || 
       t->depth   != sig->depth  ||
       t->nframes != sig->nframes){
      printf("ERROR: fMRIsigT: output dimension mismatch\n");
      return(NULL);
    }
    if(sig->type != MRI_FLOAT){
      printf("ERROR: fMRIsigT: structure passed is not MRI_FLOAT\n");
      return(NULL);
    }
  }

  for(c=0; c < t->width; c++){
    for(r=0; r < t->height; r++){
      for(s=0; s < t->depth; s++){
	for(f=0; f < t->nframes; f++){
	  tval = MRIFseq_vox(t,c,r,s,f);
          sigtval = sigt(tval, rint(DOF));
	  if(tval < 0) sigtval *= -1;
	  MRIFseq_vox(sig,c,r,s,f) = sigtval;
	}
      }
    }
  }

  return(sig);
}
/*--------------------------------------------------------------------*/
MRI *fMRIcomputeF(MRI *ces, MATRIX *X, MATRIX *C, MRI *var, MRI *F)
{
  int c, r, s, f, J;
  MATRIX *Xt, *XtX, *iXtX, *CiXtX, *Ct, *CiXtXCt, *iCiXtXCt;
  MATRIX *M, *cesvect, *cesvectt, *voxF;
  float cesval, voxvar;

  if(ces->nframes != C->rows ){
    printf("ERROR: fMRIcomputeT: contrast effect size and contrast matrix "
	   "have inconsistent dimensions.\n");
    return(NULL);
  }

  if(F==NULL){
    F = MRIallocSequence(ces->width, ces->height, ces->depth, 
			      MRI_FLOAT, 1);
    if(F==NULL){
      printf("ERROR: fMRIcomputeF: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(ces,F);
  }
  else{
    if(F->width  != ces->width || 
       F->height != ces->height || 
       F->depth  != ces->depth){
      printf("ERROR: fMRIcomputeT: output dimension mismatch\n");
      return(NULL);
    }
    if(F->type != MRI_FLOAT){
      printf("ERROR: fMRIcomputeT: structure passed is not MRI_FLOAT\n");
      return(NULL);
    }
  }

  Xt      = MatrixTranspose(X,NULL);
  XtX     = MatrixMultiply(Xt,X,NULL);
  iXtX    = MatrixInverse(XtX,NULL);
  CiXtX   = MatrixMultiply(C,iXtX,NULL);
  Ct      = MatrixTranspose(C,NULL);
  CiXtXCt = MatrixMultiply(CiXtX,Ct,NULL);
  iCiXtXCt = MatrixInverse(CiXtXCt,NULL);
  J = C->rows;
  cesvect = MatrixAlloc(ces->nframes,1,MATRIX_REAL);
  cesvectt = MatrixAlloc(1,ces->nframes,MATRIX_REAL);
  M = NULL;
  voxF = NULL;

  for(c=0; c < ces->width; c++){
    for(r=0; r < ces->height; r++){
      for(s=0; s < ces->depth; s++){
	voxvar = MRIgetVoxVal(var, c, r, s, 0);
	if(voxvar == 0)
	  MRIFseq_vox(F,c,r,s,0) = 0;
	else{
	  for(f=0; f < ces->nframes; f++){
	    cesval = MRIgetVoxVal(ces, c, r, s, f);
	    cesvect->rptr[f+1][1]  = cesval;
	    cesvectt->rptr[1][f+1] = cesval;
	  }
	  M = MatrixMultiply(iCiXtXCt,cesvect,M);
	  voxF = MatrixMultiply(cesvectt,M,voxF);

	  MRIFseq_vox(F,c,r,s,0) = (voxF->rptr[1][1])/(J*voxvar);
	}
      }
    }
  }

  MatrixFree(&Xt);
  MatrixFree(&XtX);
  MatrixFree(&iXtX);
  MatrixFree(&CiXtX);
  MatrixFree(&Ct);
  MatrixFree(&CiXtXCt);
  MatrixFree(&iCiXtXCt);
  MatrixFree(&cesvect);
  MatrixFree(&cesvectt);
  MatrixFree(&M);

  return(F);
}

/*--------------------------------------------------------*/
// DOF1 = dof of den (same as t DOF)
// DOF2 = dof of num (number of rows in C)
// Note: order is rev relative to fsfast's FTest.m
MRI *fMRIsigF(MRI *F, float DOFDen, float DOFNum, MRI *sig)
{
  int c, r, s, f;
  float Fval, sigFval;

  if(sig==NULL){
    sig = MRIallocSequence(F->width, F->height, F->depth, 
			   MRI_FLOAT, F->nframes);
    if(sig==NULL){
      printf("ERROR: fMRIsigF: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(F,sig);
  }
  else{
    if(F->width   != sig->width  || 
       F->height  != sig->height || 
       F->depth   != sig->depth  ||
       F->nframes != sig->nframes){
      printf("ERROR: fMRIsigF: output dimension mismatch\n");
      return(NULL);
    }
    if(sig->type != MRI_FLOAT){
      printf("ERROR: fMRIsigF: structure passed is not MRI_FLOAT\n");
      return(NULL);
    }
  }

  for(c=0; c < F->width; c++){
    for(r=0; r < F->height; r++){
      for(s=0; s < F->depth; s++){
	for(f=0; f < F->nframes; f++){
	  Fval = MRIFseq_vox(F,c,r,s,f);
	  sigFval = gsl_cdf_fdist_Q(Fval,DOFNum,DOFDen);
	  MRIFseq_vox(sig,c,r,s,f) = sigFval;
	}
      }
    }
  }

  return(sig);
}

/*--------------------------------------------------------
  fMRInskip() - skip the first nskip frames
  --------------------------------------------------------*/
MRI *fMRInskip(MRI *inmri, int nskip, MRI *outmri)
{
  int c, r, s, fin, fout;
  int nframesout;
  float val;

  if(inmri->nframes <= nskip){
    printf("ERROR: fMRInskip: nskip >= nframes\n");
    return(NULL);
  }

  nframesout = inmri->nframes - nskip;
  if(outmri==NULL){
    outmri = MRIallocSequence(inmri->width, inmri->height, inmri->depth, 
			      inmri->type, nframesout);
    if(outmri==NULL){
      printf("ERROR: fMRInskip: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(inmri,outmri);
  }
  else{
    if(outmri->width  != inmri->width || 
       outmri->height != inmri->height || 
       outmri->depth  != inmri->depth || 
       outmri->nframes != nframesout){
      printf("ERROR: fMRInskip: output dimension mismatch\n");
      return(NULL);
    }
    if(outmri->type != inmri->type){
      printf("ERROR: fMRInskip: structure type mismatch\n");
      return(NULL);
    }
  }

  MRIclear(outmri);
  for(fout=0; fout < outmri->nframes; fout++){
    fin = fout + nskip;
    for(s=0; s < outmri->depth; s++){
      for(r=0; r < outmri->height; r++){
	for(c=0; c < outmri->width; c++){
	  val = MRIgetVoxVal(inmri, c, r, s, fin);
	  MRIsetVoxVal(outmri,c, r, s, fout, val);
	}
      }
    }
  }

  return(outmri);
}

/*--------------------------------------------------------
  fMRIndrop() - drop the last ndrop frames
  --------------------------------------------------------*/
MRI *fMRIndrop(MRI *inmri, int ndrop, MRI *outmri)
{
  int c, r, s, fin, fout;
  int nframesout;
  float val;

  if(inmri->nframes <= ndrop){
    printf("ERROR: fMRIndrop: ndrop >= nframes\n");
    return(NULL);
  }

  nframesout = inmri->nframes - ndrop;
  if(outmri==NULL){
    outmri = MRIallocSequence(inmri->width, inmri->height, inmri->depth, 
			      inmri->type, nframesout);
    if(outmri==NULL){
      printf("ERROR: fMRIndrop: could not alloc\n");
      return(NULL);
    }
    MRIcopyHeader(inmri,outmri);
  }
  else{
    if(outmri->width  != inmri->width || 
       outmri->height != inmri->height || 
       outmri->depth  != inmri->depth || 
       outmri->nframes != nframesout){
      printf("ERROR: fMRIndrop: output dimension mismatch\n");
      return(NULL);
    }
    if(outmri->type != inmri->type){
      printf("ERROR: fMRIndrop: structure type mismatch\n");
      return(NULL);
    }
  }

  MRIclear(outmri);
  for(fout=0; fout < outmri->nframes; fout++){
    fin = fout;
    for(s=0; s < outmri->depth; s++){
      for(r=0; r < outmri->height; r++){
	for(c=0; c < outmri->width; c++){
	  val = MRIgetVoxVal(inmri, c, r, s, fin);
	  MRIsetVoxVal(outmri,c, r, s, fout, val);
	}
      }
    }
  }

  return(outmri);
}
/*---------------------------------------------------------------*/
MATRIX *MRItoMatrix(MRI *mri, int c, int r, int s, 
		    int Mrows, int Mcols, MATRIX *M)
{
  int mr, mc, f;

  if(M==NULL) M = MatrixAlloc(Mrows,Mcols,MATRIX_REAL);
  else{
    if(M->rows != Mrows || M->cols != Mcols){
      printf("ERROR: Matrix dim mismatch\n");
    }
  }

  if(mri->nframes != Mrows*Mcols){
    printf("ERROR: MRItoMatrix: MRI frames = %d, does not equal\n",
	   mri->nframes);
    printf("       matrix dim = %dx%d = %d",Mrows,Mcols,Mrows*Mcols);
    return(NULL);
  }

  f = 0;
  for(mr=1; mr <= Mrows; mr++){
    for(mc=1; mc <= Mcols; mc++){
      M->rptr[mr][mc] = MRIgetVoxVal(mri,c,r,s,f);
      f++;
    }
  }
  return(M);
}

/*---------------------------------------------------------------*/
MATRIX *MRItoSymMatrix(MRI *mri, int c, int r, int s, MATRIX *M)
{
  int mr, mc, f, Msize, nframesexp;

  if(M==NULL){
    Msize = (int)(round( (sqrt(8.0*mri->nframes + 1.0) - 1.0 )/2.0 ));
    printf("Msize = %d\n",Msize);
    M = MatrixAlloc(Msize,Msize,MATRIX_REAL);
  }

  nframesexp = M->rows*(M->rows+1)/2;
  if(mri->nframes != nframesexp){
    printf("ERROR: MRItoSymMatrix: MRI frames = %d, does not support sym\n",
	   mri->nframes);
    return(NULL);
  }

  f = 0;
  for(mr=1; mr <= M->rows; mr++){
    for(mc=mr; mc <= M->cols; mc++){
      M->rptr[mr][mc] = MRIgetVoxVal(mri,c,r,s,f);
      M->rptr[mc][mr] = MRIgetVoxVal(mri,c,r,s,f);
      f++;
    }
  }
  return(M);
}

/*---------------------------------------------------------------*/
int MRIfromMatrix(MRI *mri, int c, int r, int s, MATRIX *M)
{
  int mr,mc,f;

  if(mri->nframes != M->rows*M->cols){
    printf("ERROR: MRIfromMatrix: MRI frames = %d, does not equal\n",
	   mri->nframes);
    printf("       matrix dim = %dx%d = %d",M->rows,M->cols,M->rows*M->cols);
    return(1);
  }

  f = 0;
  for(mr=1; mr <= M->rows; mr++){
    for(mc=1; mc <= M->cols; mc++){
      MRIsetVoxVal(mri,c,r,s,f,M->rptr[mr][mc]);
      f++;
    }
  }
  return(0);
}

/*---------------------------------------------------------------*/
int MRIfromSymMatrix(MRI *mri, int c, int r, int s, MATRIX *M)
{
  int mr,mc,f, nframesexp;

  nframesexp = M->rows*(M->rows+1)/2;
  if(mri->nframes != nframesexp){
    printf("ERROR: MRIfromSumMatrix: MRI frames = %d, does not equal\n",
	   mri->nframes);
    printf("       matrix dim = %dx%d = %d",M->rows,M->cols,M->rows*M->cols);
    return(1);
  }

  f = 0;
  for(mr=1; mr <= M->rows; mr++){
    for(mc=mr; mc <= M->cols; mc++){
      MRIsetVoxVal(mri,c,r,s,f,M->rptr[mr][mc]);
      f++;
    }
  }
  return(0);
}

/*------------------------------------------------------------------------
  MRInormWeights() - rescales each voxel so that the sum across all
  frames equals nframes. If sqrtFlag=1, then computes the sqrt(w)
  before normalzing.  If invFlag=1, then computes the 1/w before
  normalzing.  The sqrt and inv can be used if the weights are
  variances for WLMS. Can be done in-place. If mask, then ignores
  voxels where mask<0.5. Weights must be >  0.
  *------------------------------------------------------*/
MRI *MRInormWeights(MRI *w, int sqrtFlag, int invFlag, MRI *mask, MRI *wn)
{
  int c,r,s,f;
  double v, vsum, m;

  //-------------------------------------------
  if(wn == NULL){
    wn = MRIallocSequence(w->width,w->height,w->depth,
			  MRI_FLOAT,w->nframes);
    if(wn == NULL){
      printf("ERROR: MRInormWeights(): could not alloc weights\n");
      return(NULL);
    }
    MRIcopyHeader(wn,w);
  }

  //-------------------------------------------
  for(c=0; c < w->width; c++){
    for(r=0; r < w->height; r++){
      for(s=0; s < w->depth; s++){

	if(mask != NULL){
	  m = MRIgetVoxVal(mask,c,r,s,0);
	  if(m < 0.5) continue;
	}

	// First go through and compute the sum
	vsum = 0;
	for(f = 0; f < w->nframes; f++){
	  v = MRIgetVoxVal(w,c,r,s,f);
	  if(v <= 0){
	    printf("ERROR: MRInormWeights: value less than or eq to 0.\n");
	    printf("  c=%d, r=%d, s=%d, v=%g\n",c,r,s,v);
	    // Should do a free here, I guess
	    return(NULL);
	  }
	  if(sqrtFlag) v = sqrt(v);
	  if(invFlag)  v = 1/v;
	  vsum += v;
	}

	// So that the sum = nframes
	vsum /= w->nframes; 

	// Now rescale
	for(f = 0; f < w->nframes; f++){
	  v = MRIgetVoxVal(w,c,r,s,f);
	  if(sqrtFlag) v = sqrt(v);
	  if(invFlag)  v = 1/v;
	  v = v/vsum;
	  MRIsetVoxVal(wn,c,r,s,f,v);
	}
      }
    }
  }

  return(wn);
}
/*---------------------------------------------------------------*/
int MRIglmFit(MRIGLM *mriglm)
{
  int c,r,s,n,nc,nr,ns,nf,pctdone;
  float m,Xcond;
  long nvoxtot, nthvox;

  nc = mriglm->y->width;
  nr = mriglm->y->height;
  ns = mriglm->y->depth;
  nf = mriglm->y->nframes;
  nvoxtot = nc*nr*ns;

  mriglm->nregtot = MRIglmNRegTot(mriglm);
  GLMallocX(mriglm->glm, nf, mriglm->nregtot);
  GLMallocY(mriglm->glm);

  if(mriglm->w != NULL || mriglm->npvr != 0) mriglm->pervoxflag = 1;
  else                                       mriglm->pervoxflag = 0;

  GLMcMatrices(mriglm->glm);

  if(! mriglm->pervoxflag) {
    MatrixCopy(mriglm->Xg,mriglm->glm->X);
    mriglm->XgLoaded = 1;
    GLMxMatrices(mriglm->glm);
  }

  mriglm->beta = MRIallocSequence(nc, nr, ns, MRI_FLOAT, mriglm->nregtot) ;
  mriglm->eres = MRIallocSequence(nc, nr, ns, MRI_FLOAT, nf);
  mriglm->rvar = MRIallocSequence(nc, nr, ns, MRI_FLOAT, 1);

  if(mriglm->yhatsave)
    mriglm->yhat = MRIallocSequence(nc,nr,ns,MRI_FLOAT,nf);
  if(mriglm->condsave)
    mriglm->cond = MRIallocSequence(nc,nr,ns,MRI_FLOAT, 1) ;

  for(n = 0; n < mriglm->glm->ncontrasts; n++){
    mriglm->gamma[n] = MRIallocSequence(nc,nr,ns,MRI_FLOAT, 
					mriglm->glm->C[n]->rows);
    mriglm->F[n] = MRIallocSequence(nc, nr, ns,MRI_FLOAT, 1);
    mriglm->p[n] = MRIallocSequence(nc, nr, ns,MRI_FLOAT, 1);
    if(mriglm->glm->ypmfflag[n])
      mriglm->ypmf[n] = MRIallocSequence(nc,nr,ns,MRI_FLOAT,nf);
  }

  //--------------------------------------------
  pctdone = 0; 
  nthvox = 0;
  mriglm->n_ill_cond = 0;
  for(c=0; c < nc; c++){
    for(r=0; r < nr; r++){
      for(s=0; s< ns; s++){
	nthvox ++;
	if(nthvox == (long) floor(.1*nvoxtot) ){
	  pctdone += 10;
	  printf("%2d%% ",pctdone);
	  fflush(stdout);
	  nthvox = 0;
	}

	// Check the mask -----------
	if(mriglm->mask != NULL){
	  m = MRIgetVoxVal(mriglm->mask,c,r,s,0);
	  if(m < 0.5) continue;
	}

	// Get data from mri and put in GLM
	MRIglmLoadVox(mriglm, c, r, s);

	// Compute intermediate matrices
	GLMxMatrices(mriglm->glm);

	// Compute condition
	if(mriglm->condsave){
	  Xcond = MatrixConditionNumber(mriglm->glm->XtX);
	  MRIsetVoxVal(mriglm->cond,c,r,s,0,Xcond);
	}

	// Test condition
	if(mriglm->glm->ill_cond_flag){
	  mriglm->n_ill_cond ++;
	  continue;
	}

	GLMfit(mriglm->glm);
	GLMtest(mriglm->glm);

	// Pack data back into MRI
	MRIsetVoxVal(mriglm->rvar,c,r,s,0,mriglm->glm->rvar);
	MRIfromMatrix(mriglm->beta, c, r, s, mriglm->glm->beta);
	MRIfromMatrix(mriglm->eres, c, r, s, mriglm->glm->eres);
	if(mriglm->yhatsave)
	  MRIfromMatrix(mriglm->yhat, c, r, s, mriglm->glm->yhat);
	for(n = 0; n < mriglm->glm->ncontrasts; n++){
	  MRIfromMatrix(mriglm->gamma[n], c, r, s, mriglm->glm->gamma[n]);
	  MRIsetVoxVal(mriglm->F[n],c,r,s,0,mriglm->glm->F[n]);
	  MRIsetVoxVal(mriglm->p[n],c,r,s,0,mriglm->glm->p[n]);
	  if(mriglm->glm->ypmfflag[n])
	    MRIfromMatrix(mriglm->ypmf[n], c, r, s, mriglm->glm->ypmf[n]);
	}

      }
    }
  }
  printf("\n");

  printf("n_ill_cond = %d\n",mriglm->n_ill_cond);
  return(0);
}

/* ----------------------------------------------------------------------------------
   MRIglmLoadVox() - loads the data (X and y) for the voxel at crs
   into the GLM matrix struct and applies the weights (if applicable). 
   X and y are allocated if they are NULL. mriglm->nregtot is also computed
   here. If Xg has already been loaded into X, then it is not loaded again
   unless mriglm->w is non-null.
   ----------------------------------------------------------------------------------*/
int MRIglmLoadVox(MRIGLM *mriglm, int c, int r, int s)
{
  int f, n, nthreg;
  double v;

  if(mriglm->glm->X == NULL){
    mriglm->nregtot  = mriglm->Xg->cols + mriglm->npvr;
    mriglm->glm->X   = MatrixAlloc(mriglm->y->nframes,mriglm->nregtot,MATRIX_REAL);
  }
  if(mriglm->glm->y == NULL)
    mriglm->glm->y = MatrixAlloc(mriglm->y->nframes,1,MATRIX_REAL);

  // Load y, Xg, and the per-vox reg --------------------------
  for(f = 1; f <= mriglm->y->nframes; f++){

    // Load y
    mriglm->glm->y->rptr[f][1] = MRIgetVoxVal(mriglm->y,c,r,s,f-1);

    // Load Xg->X the global design matrix if needed
    if(mriglm->w != NULL || !mriglm->XgLoaded){
      nthreg = 1;
      for(n = 1; n <= mriglm->Xg->cols; n++){
	mriglm->glm->X->rptr[f][nthreg] = mriglm->Xg->rptr[f][n]; // X=Xg
	nthreg++;
      }
    }
    else nthreg = mriglm->Xg->cols;

    // Load the global per-voxel regressors matrix, X = [X pvr]
    for(n = 1; n <= mriglm->npvr; n++){
      mriglm->glm->X->rptr[f][nthreg] = MRIgetVoxVal(mriglm->pvr[n-1],c,r,s,f-1);
      nthreg++;
    }
  }
  mriglm->XgLoaded = 1; // Set flag that Xg has been loaded
  
  // Weight X and y, X = w.*X, y = w.*y
  if(mriglm->w != NULL && ! mriglm->skipweight){
    for(f = 1; f <= mriglm->glm->X->rows; f++){
      v = MRIgetVoxVal(mriglm->w,c,r,s,f-1);	    
      mriglm->glm->y->rptr[f][1] *= v;
      for(n = 1; n <= mriglm->glm->X->cols; n++) 
	mriglm->glm->X->rptr[f][n] *= v;
    }
  }
  
  return(0);
}
/*----------------------------------------------------------------
  MRIglmNRegTot() - computes the total number of regressors based
  on the number of columns in Xg + number of per-voxel regressors
  ----------------------------------------------------------------*/
int MRIglmNRegTot(MRIGLM *mriglm)
{
  mriglm->nregtot = mriglm->Xg->cols + mriglm->npvr;
  return(mriglm->nregtot);
}

/*----------------------------------------------------------------
  MRItoVector() - copies all the frames from the given voxel
  in to a vector.
  ----------------------------------------------------------------*/
VECTOR *MRItoVector(MRI *mri, int c, int r, int s, VECTOR *v)
{
  int f;
  if(v == NULL) v = MatrixAlloc(mri->nframes,1,MATRIX_REAL);

  for(f=1; f <= v->rows; f++)
    v->rptr[f][1] = MRIgetVoxVal(mri,c,r,s,f-1);
  return(v);
}
