/**
 * @file  talairach_afd.c
 * @brief Automatically detect Talairach alignment failures.
 *
 * Detect bad Talairach alignment by checking subjects Talairach transform
 * against a range of known good values.
 * See: dev/docs/Automatic_Failure_Detection.doc
 */
/*
 * Original Author: Laurence Wastiaux
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2007/02/09 00:53:08 $
 *    $Revision: 1.5 $
 *
 * Copyright (C) 2007,
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "mri.h"
#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "mrimorph.h"
#include "mri_conform.h"
#include "utils.h"
#include "const.h"
#include "timer.h"
#include "version.h"
#include "stats.h"
#include "fio.h"

static char vcid[] =
"$Id: talairach_afd.c,v 1.5 2007/02/09 00:53:08 nicks Exp $";
static int get_option(int argc, char *argv[]) ;
static void usage(int exit_value) ;
static char *subject_name = NULL;
#define DEFAULT_THRESHOLD 0.01f
static float threshold = DEFAULT_THRESHOLD;
static int verbose=0;
VECTOR *ReadMeanVect(char *fname);
MATRIX *ReadCovMat(char *fname);
VECTOR *Load_xfm(char *fname);
double *LoadProbas(char *fname, int *nb_samples);
float mvnpdf(VECTOR *t, VECTOR *v_mu, MATRIX *m_sigma);
float ComputeArea(HISTO *h, int nbin);

int main(int argc, char *argv[]) ;

char *Progname ;

int main(int argc, char *argv[])
{
  char **av;
  char *fsenv, *sname;
  char fsafd[1000], tmf[1000], cvf[1000], xfm[1000], probasf[1000];
  int ac, nargs;
  int b, msec, minutes, seconds ;
  int nsamples, bin;
  struct timeb start ;
  float p, pval;
  double *ts_probas;
  VECTOR *mu;
  MATRIX *sigma;
  VECTOR *txfm;
  HISTO *h;

  mu=NULL;
  sigma=NULL;
  txfm=NULL;

  /* rkt: check for and handle version tag */
  nargs = handle_version_option
    (argc, argv,
     "$Id: talairach_afd.c,v 1.5 2007/02/09 00:53:08 nicks Exp $",
     "$Name:  $");
  if (nargs && argc - nargs == 1)
    exit (0);
  argc -= nargs;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  TimerStart(&start) ;

  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 1)
    usage(1);

  if (subject_name==NULL) {
    printf("ERROR: a subject name must be provided!\n");
    usage(1);
  }

  fsenv=getenv("FREESURFER_HOME");
  if(!fsenv) {
    printf("ERROR: FREESURFER_HOME not defined!\n");
    exit(1);
  }

  sprintf(fsafd, "%s/fsafd", fsenv);
  sprintf(tmf, "%s/TalairachingMean.adf", fsafd);
  sprintf(cvf, "%s/TalairachingCovariance.adf", fsafd);
  sprintf(xfm, "%s/mri/transforms/talairach.xfm", subject_name);
  sprintf(probasf, "%s/TalairachingProbas.adf", fsafd);
  sname=fio_basename(subject_name, NULL);

  /*----- load 1x9 mean vector computed from training set -----*/
  mu = ReadMeanVect(tmf);

  /*----- load 9x9 cov matrix computed from training set -----*/
  sigma = ReadCovMat(cvf);

  /*--- Load training set's probas ---*/
  ts_probas = LoadProbas(probasf, &nsamples);

  h = HISTObins(21, 0,1);
  HISTOcount(h, ts_probas, nsamples);

  for (b = 0 ; b < h->nbins ; b++)
    h->counts[b] = (float)h->counts[b]/(float)nsamples ;

  /*---Load Talairach.xfm---*/
  txfm = Load_xfm(xfm);

  /*--- compute mvnpdf ---*/
  p = mvnpdf(txfm, mu, sigma);

  bin = HISTOvalToBin(h,p);

  pval = ComputeArea(h, bin);

  if(pval < threshold)
    printf("ERROR: talairach_afd: Talairach Transform: %s ***FAILED***"
           " (p=%f, pval=%f < threshold=%f)\n",
           sname, p, pval, threshold);
  else{
    printf("talairach_afd: Talairach Transform: %s OK "
           "(p=%f, pval=%f >= threshold=%f)\n",
           sname, p, pval, threshold);
  }

  if (verbose) {
    int i,j;
    printf("\nFREESURFER_HOME=%s\n", fsenv);
    printf("\nfsafdDir=%s\n", fsafd);
    
    printf("\nmu:\n");
    for (i=1;i<=9;i++){
      printf("%f  ", mu->rptr[1][i]);
    }
    printf("\n\n");
    printf("sigma:\n");
    for(i=1;i<=9;i++){
      for(j=1;j<=9;j++){
        printf("%f  ", sigma->rptr[i][j]);
      }
      printf("\n");
    }
    printf("\n");
    printf("xfm:\n");
    for(i=1;i<=9;i++){
      printf("%f  ", txfm->rptr[1][i]);
    }
    printf("\n\n");

    printf("proba = %f\n\n", p);

    msec = TimerStop(&start) ;
    seconds = nint((float)msec/1000.0f) ;
    minutes = seconds / 60 ;
    seconds = seconds % 60 ;
    fprintf(stderr, "Talairach failure detection took %d minutes"
            " and %d seconds.\n", minutes, seconds) ;
  }

  exit(0) ;
  return(0) ;
}


/*----------------------------------------------------------------------
  Parameters:

  Description:
  ----------------------------------------------------------------------*/
static void print_version(void)
{
  fprintf(stdout, "%s\n", vcid) ;
  exit(1) ;
}

static int
get_option(int argc, char *argv[])
{
  int  nargs = 0 ;
  char *option;

  option = argv[1] + 1 ;            /* past '-' */
  if (!stricmp(option, "help"))
    usage(1) ;
  else if (!stricmp(option, "version"))
    print_version() ;
  if (!stricmp(option, "subj"))
  {
    subject_name = argv[2];
    nargs = 1;
  }
  else if (!stricmp(option, "T") || !stricmp(option, "threshold")){
    threshold = (float)atof(argv[2]);
    nargs = 1;
  }
  else switch (toupper(*option))
  {
  case 'T':
  case '?':
  case 'U':
    usage(0) ;
    break ;
  case 'V':
    verbose=1;
    break;
  default:
    fprintf(stderr, "unknown option %s\n", argv[1]) ;
    exit(1) ;
    break ;
  }

  return(nargs) ;
}


/*----------------------------------------------------------------------
  Parameters:

  Description:
  ----------------------------------------------------------------------*/
static void
usage(int exit_value)
{
  FILE *fout;

  fout = (exit_value ? stderr : stdout);

  fprintf(fout,
          "Usage: %s -subj <subject_name> [-T <p-value threshold>]\n",
          Progname);
  fprintf(fout,
          "This program detects Talairach alignment failures.\n") ;

  fprintf(fout, "Required:\n") ;
  fprintf(fout,
          "   -subj %%s  specify subject's name \n");
  fprintf(fout, "Optional:\n") ;
  fprintf(fout,
          "   -T #      threshold the p-values at #\n"
          "             (i.e., Talairach transforms for subjects with\n"
          "             p-values <= T are considered as very unlikely)\n"
          "             default=%2.3f\n",DEFAULT_THRESHOLD);
  fprintf(fout,
          "   -V        verbose\n");

  exit(exit_value) ;
}


/*----------------------------------------------------------------------
  Parameters:char *fname

  Description:reads the 9 componants of Mean Vector mu
  from "$FREESURFER_HOME/fsafd/TalairachingMean.adf"
  ----------------------------------------------------------------------*/
VECTOR *
ReadMeanVect(char *fname)
{
  FILE *fp;
  char tag[1000], tmpstr[1000];
  int nth, r;
  VECTOR *v;

  v = MatrixAlloc(1,9, MATRIX_REAL) ;

  fp = fopen(fname,"r");
  if(fp == NULL){
    printf("ERROR: talairach_afd::ReadMeanVect(): could not open %s\n",
           fname);
    exit(1);
  }

  nth=1;
  while(1) {
    r = fscanf(fp,"%s",tag);
    if(r==EOF) break;
    if(!strcmp(tag,"#")){  // line starts with #:
                           // header info -> skip the whole line
      fgets(tmpstr,1000,fp);
    }
    else{
      v->rptr[1][nth]=(float)atof(tag);
      nth++;
    }
  }
  if (nth!=10){
    printf("ERROR: talairach_afd::ReadMeanVect(): "
           "9 components could not be loaded\n");
    exit(1);
  }

  fclose(fp);
  return(v);
}


/*----------------------------------------------------------------------
  Parameters:char *fname

  Description:reads the 9x9 componants of covariance matrix sigma[9][9]
  from "$FREESURFER_HOME/fsafd/TalairachingCovariance.adf"
  ----------------------------------------------------------------------*/
MATRIX *
ReadCovMat(char *fname)
{
  FILE *fp;
  char tag[1000], tmpstr[1000];
  int nth, r, i, j;
  MATRIX *s;

  s = MatrixAlloc(9,9, MATRIX_REAL) ;

  fp = fopen(fname,"r");
  if(fp == NULL){
    printf("ERROR: talairach_afd::ReadCovMAt(): could not open %s\n",fname);
    exit(1);
  }

  nth=0;
  while(1) {
    r = fscanf(fp,"%s",tag);
    if(r==EOF) break;
    if(!strcmp(tag,"#")){  // line starts with #:
                           // header info -> skip the whole line
      fgets(tmpstr,1000,fp);
    }
    else{
      i=nth/9;
      j=nth-(i*9);
      s->rptr[i+1][j+1]=(float)atof(tag);
      nth++;
    }
  }
  if (nth!=81){
    printf("ERROR: talairach_afd::ReadCovMAt(): "
           "9x9 components could not be loaded\n");
    exit(1);
  }

  fclose(fp);
  return(s);
}


/*----------------------------------------------------------------------
  Parameters:char *fname

  Description:Load xfm matrix
  ----------------------------------------------------------------------*/
VECTOR *
Load_xfm(char *fname)
{
  FILE *fp;
  char *sdir;
  char line[MAX_LINE_LEN], tmp_name[1000];
  char tag[1000];
  int nth, r, i, j;
  float M[3][4];
  VECTOR *v;

  v = MatrixAlloc(1,9, MATRIX_REAL) ;

  fp = fopen(fname,"r");
  if(!fp){
    // try to find subject in SUBJECTS_DIR
    sdir=getenv("SUBJECTS_DIR");
    if(sdir){
      sprintf(tmp_name,"%s/%s", sdir, fname);
      fp = fopen(tmp_name,"r");
      if(!fp){
        printf("ERROR: talairach_afd::Load_xfm(): could not open %s\n",fname);
        exit(1);
      }
    }
    else{
      printf("ERROR: talairach_afd::Load_xfm(): SUBJECTS_DIR is not defined, "
             "could not open %s\n",fname);
      exit(1);
    }
  }
  while(1){
    fgetl(line, MAX_LINE_LEN-1, fp) ;
    if (!strcmp(line, "Linear_Transform =" ))
      break;
  }
  nth=0;
  while(1) {
    r = fscanf(fp,"%s",tag);
    if(r==EOF) break;
    i=nth/4;
    j=nth-(i*4);
    M[i][j]=(float)atof(tag);
    nth++;
  }
  if(nth!=12){
    printf("ERROR: talairach_afd::Load_xfm(%s): 3x4 components could "
           "not be loaded\n", fname);
    exit(1);
  }
  for(i=0;i<3;i++){
    for(j=0;j<3;j++) {
      v->rptr[1][i*3+j+1]=M[i][j];
    }
  }

  fclose(fp);
  return(v);
}


/*----------------------------------------------------------------------
  Parameters:char *fname

  Description:Load probas computed from the training data set
  and contained in probas table
  ----------------------------------------------------------------------*/
double *
LoadProbas(char *fname, int *nb_samples)
{
  FILE *fp;
  char tag[1000], tmpstr[1000], c[1000];
  int  nth, r,  len;
  double *p=NULL;

  fp = fopen(fname,"r");
  if(fp == NULL){
    printf("ERROR: talairach_afd::LoadProbas(): could not open %s\n",fname);
    exit(1);
  }

  nth=0;

  while(1) {
    r = fscanf(fp,"%s",tag);
    if(r==EOF) break;
    if(!strcmp(tag,"#")){  // line starts with #:
                           // header info -> skip the whole line
      fgets(tmpstr,1000,fp);
      if(strcmp(tmpstr, " nrows")>0){
        sscanf(tmpstr, "%s %d", c, &len);
        break;
      }
    }
  }

  p=(double *)calloc(len, sizeof(double));
  if(!p){
    printf("ERROR: talairach_afd::LoadProbas(): "
           "could not allocate memory for probas vector\n");
    exit(1);
  }

  fp=freopen(fname,"r", fp);
  if(fp == NULL){
    printf("ERROR: talairach_afd::LoadProbas(): could not re-open %s\n",
           fname);
    exit(1);
  }

  while(1) {
    r = fscanf(fp,"%s",tag);
    if(r==EOF) break;
    if(!strcmp(tag,"#")){  // line starts with #:
                           // header info -> skip the whole line
      fgets(tmpstr,1000,fp);
    }
    else{
      p[nth] = (double)atof(tag);
      nth++;
    }
  }

  if(nth!=len){
    printf("ERROR: talairach_afd::LoadProbas(): "
           "%d components could not be loaded\n", len);
    exit(1);
  }
  *(nb_samples)=len;

  fclose(fp);
  return(p);

}

/*----------------------------------------------------------------------
  Parameters:vector t, mean vector mu, covariance matrix sigma

  Description:computes p = mvnpdf(t, mu, sigma)
  ----------------------------------------------------------------------*/

float
mvnpdf(VECTOR *t, VECTOR *v_mu, MATRIX *m_sigma)
{
  VECTOR *sub, *t_sub;
  MATRIX *m_sinv, *m_tmp;
  float  num, det, den, tmp;
  int d;

  if(!t || !v_mu || !m_sigma){
    fprintf(stderr, "ERROR: talairach_afd::mvnpdf(): cannot compute mvnpdf\n");
    exit(1);
  }

  d = t->cols;

  sub = VectorSubtract(t,v_mu,NULL);
  m_sinv = MatrixInverse(m_sigma, NULL);
  t_sub = VectorTranspose(sub, NULL);

  m_tmp = MatrixMultiply(m_sinv, t_sub, NULL);
  m_tmp = MatrixMultiply(sub, m_tmp, NULL);
  tmp = m_tmp->rptr[1][1];
  num = exp(- (tmp/2));
  det = MatrixDeterminant(m_sigma);
  den = 1/(sqrt(det));

  num = 1/sqrt(pow(2*M_PI,d)) * num * den;

  return(num);
}


/*----------------------------------------------------------------------
  Parameters:

  Description:computes area under the curve for b<=nbin
  ----------------------------------------------------------------------*/
float
ComputeArea(HISTO *h, int nbin)
{
  float a, s;
  int i;

  s=0;
  for(i=1 ; i<=nbin ; i++) {
    a = (h->counts[i] + h->counts[i-1]) /2;
    s += a;
  }

  return(s);
}

