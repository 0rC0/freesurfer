#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "error.h"
#include "fio.h"
#include "diag.h"
#include "svm.h"
#include "proto.h"
#include "utils.h"

static double svm_rbf_dot(int ninputs, float *xi, float *xj, double sigma) ;
static double svm_linear_dot(int ninputs, float *xi, float *xj) ;
static double SVMlagrangian(SVM *svm, int ntraining, float **x, 
                            float *y, double **dot_matrix) ;
#if 0
SVM   *
SVMalloc(int ninputs, char *c1_name, char *c2_name)
{
  SVM *svm ;

  svm = (SVM *)calloc(1, sizeof(SVM)) ;
  if (NULL == svm)
    ErrorExit(ERROR_NOMEMORY, "SVMalloc(%d, %s, %s) - calloc failed",
              ninputs, c1_name, c2_name) ;
  strcpy(svm->class1_name, c1_name) ;
  strcpy(svm->class2_name, c2_name) ;

  svm->ninputs = ninputs ;
  svm->type = SVM_KERNEL_LINEAR ;
  svm->sigma = DEFAULT_SVM_SIGMA ;
  svm->w = (double *)calloc(ninputs, sizeof(double)) ;
  if (NULL == svm->w)
    ErrorExit(ERROR_NOMEMORY, "SVMalloc(%d, %s, %s) - calloc failed",
              ninputs, c1_name, c2_name) ;
  return(svm) ;
}

int
SVMtrain(SVM *svm, float **x, float *y, int ntraining, double C, 
         double tol, int max_iter)
{
  int    iter, i, j, k, Nsupport, *permutation, *moved, *failed, nmoved, dir, keep_alpha ;
  double **dot_matrix, dot, step_size, L, Lold, Lprev, *da_old, da, b, constraint, ai, aj,
         dai, daj, min_step_size ;

  permutation = (int *)calloc(ntraining, sizeof(int)) ;
  moved = (int *)calloc(ntraining, sizeof(int)) ;
  failed = (int *)calloc(ntraining, sizeof(int)) ;
  if (!permutation || !moved || !failed)
    ErrorExit(ERROR_NOMEMORY, "SVMtrain(%d): could not allocate internal buffers", ntraining);

  svm->C = C ;
  svm->ntraining = ntraining ;
  dot_matrix = (double **)calloc(ntraining, sizeof(double *)) ;
  keep_alpha = svm->alpha != NULL ;
  if (!svm->alpha)
    svm->alpha = (double *)calloc(ntraining, sizeof(double)) ;
  da_old = (double *)calloc(ntraining, sizeof(double)) ;
  if (NULL == dot_matrix || NULL == svm->alpha || NULL == da_old)
    ErrorExit(ERROR_NOMEMORY, "SVMtrain: could not allocated dot_matrix") ;

  for (i = 0 ; i < ntraining ; i++)
  {
    dot_matrix[i] = (double *)calloc(ntraining, sizeof(double)) ;
    if (NULL == dot_matrix[i])
      ErrorExit(ERROR_NOMEMORY, "SVMtrain: could not allocated dot_matrix[i]",i) ;
    if (keep_alpha == 0)
      svm->alpha[i] = 0.0 ;
  }

  for (i = 0 ; i < ntraining ; i++)
  {
    for (j = 0 ; j <= i ; j++)
    {
      switch (svm->type)
      {
      default:
      case SVM_KERNEL_LINEAR:
        dot = svm_linear_dot(svm->ninputs, x[i], x[j]) ;
        break ;
      case SVM_KERNEL_RBF:
        dot = svm_rbf_dot(svm->ninputs, x[i], x[j], svm->sigma) ;
        break ;
      }
        
      dot_matrix[i][j] = dot_matrix[j][i] = dot ;
    }
  }


  step_size = 0.01/svm->ninputs ; min_step_size = step_size/10000 ;

  /* use gradient ascent to maximize lagrangian */
  L = Lold = SVMlagrangian(svm, ntraining, x, y, dot_matrix) ;
  printf("iter %d: L = %f\r", 0, Lold);
  dir = 1 ;
  for (iter = 0 ; iter < max_iter ; iter++)
  {
    /* compute gradient*/
    for (k = 0 ; k < ntraining ; k++)
    {
      da = 1;
      for (i = 0 ; i < ntraining ; i++)
         da = da - svm->alpha[i] * y[i] * y[k] * dot_matrix[i][k] ;

      da_old[k] = step_size*da ;
    }

    /* build permutation */
    for (i = 0 ; i < ntraining ; i++)
      permutation[i] = i ;
    for (i = 0 ; i < ntraining ; i++)
    {
      int tmp ;
#if 1
      j = (int)randomNumber(0.0, (double)(ntraining-0.0001)) ;
#else
      j = (int)random() % ntraining ;
#endif

      tmp = permutation[i] ; permutation[i] = permutation[j] ; permutation[j] = tmp ;
    }

    /* try to change each alpha at least once */
    memset(failed, 0, ntraining*sizeof(int)) ; Lprev = Lold ;
    memset(moved, 0, ntraining*sizeof(int)) ; nmoved = 0 ;
    if (iter == Gdiag_no)
      DiagBreak() ;
    for (k = 0 ; k < ntraining ; k++)
    {
      i = permutation[k] ;
      if (i == Gdiag_no)
        DiagBreak() ;

      /* pick a second training vector that wants to move in the opposite direction
         to preserve sum(yi*alpha(i)) == 0.
      */
      if (dir > 0)  /* pick from start */
      {
        for (j = 0 ; j < ntraining ; j++)
        {
          if (failed[j] || moved[j])
            continue ;
          if (da_old[j]*y[j] * da_old[i] * y[i] < 0)  /* have different signs */
            break ;
        }
        if (j >= ntraining)  /* try to find one, even if it's already moved */
        {
          for (j = 0 ; j < ntraining ; j++)
          {
            if (failed[j])
              continue ;
            if (da_old[j]*y[j] * da_old[i] * y[i] < 0)  /* have different signs */
              break ;
          }
        }
      }
      else   /* search from end */
      {
        for (j = ntraining-1 ; j >= 0 ; j--)
        {
          if (failed[j] || moved[j])
            continue ;
          if (da_old[j]*y[j] * da_old[i] * y[i] < 0)  /* have different signs */
            break ;
        }
        if (j < 0)
        {
          for (j = ntraining-1 ; j >= 0 ; j--)
          {
            if (failed[j])
              continue ;
            if (da_old[j]*y[j] * da_old[i] * y[i] < 0)  /* have different signs */
              break ;
          }
        }
      }
      if (j < 0 || j >= ntraining)  /* couldn't find one */
      {
        memset(failed, 0, ntraining*sizeof(int)) ;
        memset(moved, 0, ntraining*sizeof(int)) ;
        continue ;
      }
      if (i < 0 || i >= ntraining)
        continue ;

      /* update ith alpha and make sure it stays in the feasible region */
      ai = svm->alpha[i] + da_old[i] ;
      if (ai < 0)
        ai = 0 ;
      if (ai >= C)
        ai = C ;
      dai = ai - svm->alpha[i] ;

      /* move i and j by equal and opposite amounts */
      daj = -dai * y[i] / y[j] ;

      /* update jth alpha and make sure it stays in the feasible region */
      aj = svm->alpha[j] + daj ;
      if (aj < 0)
        aj = 0 ;
      if (aj >= C)
        aj = C ;
      daj = aj - svm->alpha[j] ;
      dai = -daj * y[j] / y[i] ; /* in case bounds checking changed daj */

      if (dai*da_old[i] < 0 || daj*da_old[j] < 0)
      {
        DiagBreak() ;
        k-- ;            /* try it again with a different j */
        failed[j] = 1 ;  /* couldn't move them in dir we wanted to */
        continue ;
      }

      svm->alpha[i] += dai ;
      svm->alpha[j] += daj ;

      L = SVMlagrangian(svm, ntraining, x, y, dot_matrix) ;
      
      if (svm->alpha[i] < 0 || svm->alpha[j] < 0 || L < Lprev)
      {
        DiagBreak() ;
        svm->alpha[i] -= dai ; svm->alpha[j] -= daj ;
        failed[j] = 1 ; k-- ; continue ;
      }


      if (Lprev >= L)  /* Lagrangian didn't change */
      {
        failed[j] = 1 ; k-- ; continue ;
      }

      /* Lagrangian increased */
      nmoved += 2 ;
      Lprev = L ;
      constraint = SVMconstraint(svm, y, ntraining) ;
      if (!DZERO(constraint))
        DiagBreak() ;
      moved[i] = moved[j] = 1 ;
      memset(failed, 0, ntraining*sizeof(int)) ;
    }


    printf("iter %d: L = %f\r", iter, L) ;
    Lold = L ;
    dir *= -1 ;
    if (!nmoved)
    {
      step_size /= 10 ;
      if (step_size < min_step_size)
        break ;
    }
  }

  for (Nsupport = 0, i = 0 ; i < ntraining ; i++)
  {
    if (!DZERO(svm->alpha[i]))
      Nsupport++ ;

    for (k = 0 ; k < svm->ninputs ; k++)
      svm->w[k] += svm->alpha[i]*y[i]*x[i][k] ;
  }

  printf("\n") ;
  /*  printf("%d support vectors found\n", Nsupport) ;*/

  if (Nsupport == 0)   /* should never happen */
    Nsupport = 1 ;

  for (b = 0.0, i = 0 ; i < ntraining ; i++)
  {
    if (DZERO(svm->alpha[i]))
      continue ;   /* only use support vectors */

    for (k = 0 ; k < svm->ninputs ; k++)
      b += svm->w[k]*x[i][k] ;
    b -= 1/y[i] ;
  }
  
  svm->threshold = b/Nsupport ;

  svm->nsupport = Nsupport ;
  svm->ysupport = (float *)calloc(Nsupport, sizeof(float)) ;
  svm->xsupport = (float **)calloc(Nsupport, sizeof(float *)) ;
  svm->asupport = (double *)calloc(Nsupport, sizeof(double)) ;

  for (k = i = 0 ; i < ntraining ; i++)
  {
    if (!DZERO(svm->alpha[i]))  /* it's a support vector */
    {
      svm->ysupport[k] = y[i] ;
      svm->asupport[k] = svm->alpha[i] ;
      svm->xsupport[k] = (float *)calloc(svm->ninputs, sizeof(float)) ;
      for (j = 0 ; j < svm->ninputs ; j++)
        svm->xsupport[k][j] = x[i][j] ;
      k++ ;
    }

    free(dot_matrix[i]) ;
  }
  free(da_old) ; free(dot_matrix) ;
  free(permutation) ; free(moved) ; free(failed) ;
  return(NO_ERROR) ;
}

static double
SVMlagrangian(SVM *svm, int ntraining, float **x, float *y, double **dot_matrix)
{
  int  i, j ;
  double L ;

  L = 0 ;
   
  for (i = 0 ; i < ntraining ; i++)
  {
    L += svm->alpha[i] ;
    for (j = 0 ; j < ntraining ; j++)
      L -=  0.5 * svm->alpha[i]*svm->alpha[j]*y[i]*y[j]*dot_matrix[i][j];
  }

  return(L) ;
}

int
SVMwrite(SVM *svm, char *fname)
{
  FILE    *fp ;
  int     i, j ;

  fp = fopen(fname, "w") ;
  if (!fp)
    ErrorReturn(ERROR_BADPARM, 
                (ERROR_BADPARM, "SVMwrite(%s): could not open file", fname)) ;

  fprintf(fp, "NINPUTS\t%d\n", svm->ninputs) ;
  fprintf(fp, "CLASS1\t%s\n", svm->class1_name) ; 
  fprintf(fp, "CLASS2\t%s\n", svm->class2_name) ; 
  fprintf(fp, "C\t%f\n", svm->C) ;
  fprintf(fp, "TYPE\t%d\n", svm->type) ;
  if (svm->extra_args > 0)
  {
    fprintf(fp, "EXTRA\t%d\n", svm->extra_args) ;
    for (i = 0 ; i < svm->extra_args ; i++)
      fprintf(fp, "%s\t", svm->args[i]) ;
  }

  fprintf(fp, "THRESHOLD\t%e\n", svm->threshold) ;
  fprintf(fp, "WEIGHTS\n") ;
  for (i = 0 ; i < svm->ninputs ; i++)
    fprintf(fp, "%e\n", svm->w[i]) ;

  fprintf(fp, "NSUPPORT\t%d\n", svm->nsupport) ; 
  for (i = 0 ; i < svm->nsupport ; i++)
  {
    fprintf(fp, "%e %f\n", svm->asupport[i], svm->ysupport[i]) ;
    for (j = 0 ; j < svm->ninputs ; j++)
      fprintf(fp, "%f\n", svm->xsupport[i][j]) ;
  }

  fclose(fp) ;
  return(NO_ERROR) ;
}

SVM *
SVMread(char *fname)
{
  FILE    *fp ;
  int     i, ninputs, retval, j ;
  SVM     *svm ;
  char    class1_name[STRLEN], class2_name[STRLEN], token[STRLEN], line[STRLEN], *cp ;

  fp = fopen(fname, "r") ;
  if (!fp)
    ErrorReturn(NULL, 
                (ERROR_BADPARM, "SVMread(%s): could not open file", fname)) ;

  retval = fscanf(fp, "NINPUTS\t%d\n", &ninputs) ;
  retval = fscanf(fp, "CLASS1\t%s\n", class1_name) ; 
  retval = fscanf(fp, "CLASS2\t%s\n", class2_name) ; 

  svm = SVMalloc(ninputs, class1_name, class2_name) ;
  if (!svm)
    ErrorReturn(NULL, 
                (ERROR_BADPARM, "SVMread(%s): could not create svm", fname)) ;

  cp = fgetl(line, STRLEN-1, fp) ;
  while (NULL != cp)
  {
    retval = sscanf(cp, "%s", token) ;
    if (stricmp(token, "C") == 0)
    {
      retval = sscanf(cp, "%*s\t%lf", &svm->C) ;
    }
    else if (stricmp(token, "TYPE") == 0)
    {
      retval = sscanf(cp, "%*s\t%d", &svm->type) ;
    }
    else if (stricmp(token, "NEXTRA") == 0)
    {
      retval = sscanf(cp, "%*s\t%d", &svm->extra_args) ;
      svm->args = (char **)calloc(svm->extra_args, sizeof(char *)) ;
      if (!svm->args)
        ErrorExit(ERROR_BADPARM, "SVMread(%s): could not allocate %d extra args",
                  svm->extra_args) ;
      for (i = 0 ; i < svm->extra_args ; i++)
      {
        cp = fgetl(line, STRLEN-1, fp) ;
        if (!cp)
          ErrorExit(ERROR_BADFILE, "SVMread(%s): couldn't read %dth extra arg from file",
                    fname, i) ;

        svm->args[i] = (char *)calloc(strlen(cp)+1, sizeof(char)) ;
        if (!svm->args[i])
          ErrorExit(ERROR_BADPARM, "SVMread(%s): could not allocate %dth extra arg",i) ;
        strcpy(svm->args[i], cp) ;
      }        
    }
    else if (stricmp(token, "THRESHOLD") == 0)
    {
      retval = sscanf(cp, "%*s\t%le", &svm->threshold) ;
    }
    else if (stricmp(token, "NSUPPORT") == 0)
    {
      retval = sscanf(cp, "%*s\t%d", &svm->nsupport) ;
      svm->asupport = (double *)calloc(svm->nsupport, sizeof(double)) ;
      svm->ysupport = (float *)calloc(svm->nsupport, sizeof(double)) ;
      svm->xsupport = (float **)calloc(svm->nsupport, sizeof(float *)) ;
      for (i = 0 ; i < svm->nsupport ; i++)
      {
        
        retval = fscanf(fp, "%le %f\n", &svm->asupport[i], &svm->ysupport[i]) ;
        svm->xsupport[i] = (float *)calloc(svm->ninputs, sizeof(float)) ;
        for (j = 0 ; j < svm->nsupport ; j++)
          retval = fscanf(fp, "%f\n", &svm->xsupport[i][j]) ;
      }
    }
    else if (stricmp(token, "WEIGHTS") == 0)
    {
      for (i = 0 ; i < svm->ninputs ; i++)
      {
        retval = fscanf(fp, "%le\n", &svm->w[i]) ;
      }
    }
    cp = fgetl(line, STRLEN-1, fp) ;
  }
  fclose(fp) ;
  return(svm) ;
}
int
SVMfree(SVM **psvm)
{
  SVM *svm ;
  int i ;

  svm = *psvm ; *psvm = NULL ;

  if (svm->alpha)
    free(svm->alpha) ;

  if (svm->w)
    free(svm->w) ;
  if (svm->extra_args)
  {
    for (i = 0 ; i < svm->extra_args ; i++)
      free(svm->args[i]) ;

    free(svm->args) ;
  }
  free(svm) ;
  return(NO_ERROR) ;
}

double
SVMclassify(SVM *svm, float *x)
{
  int    i ;
  double classification, dot ;

  for (classification = 0.0, i = 0 ; i < svm->nsupport ; i++)
  {
    switch (svm->type)
    {
    default:
    case SVM_KERNEL_LINEAR:
      dot = svm_linear_dot(svm->ninputs, svm->xsupport[i], x) ;
      break ;
    case SVM_KERNEL_RBF:
      dot = svm_rbf_dot(svm->ninputs, svm->xsupport[i], x, svm->sigma) ;
      break ;
    }
    classification += (svm->ysupport[i] * svm->asupport[i] * dot) ;
  }
  classification -= svm->threshold ;

  for (classification = 0.0, i = 0 ; i < svm->ninputs ; i++)
    classification += svm->w[i]*x[i] ;
  classification -= svm->threshold ;
  return(classification) ;
}

static double
svm_linear_dot(int ninputs, float *xi, float *xj)
{
  int k ;
  double dot ;

  for (dot = 0.0, k = 0 ; k < ninputs ; k++)
    dot += xi[k] * xj[k] ;
  return(dot) ;
}

static double
svm_rbf_dot(int ninputs, float *xi, float *xj, double sigma)
{
  int k ;
  double norm, two_sigma_squared, xdiff, dot ;

  two_sigma_squared = 2*sigma*sigma ;

  for (norm = 0.0, k = 0 ; k < ninputs ; k++)
  {
    xdiff = xi[k] - xj[k] ;
    norm += xdiff*xdiff ;
  }
  dot = exp(-norm / two_sigma_squared) ;
  return(dot) ;
}
double
SVMconstraint(SVM *svm, float *y, int ntraining)
{
  double constraint ;
  int    i ;

  for (constraint = 0.0, i = 0 ;  i < ntraining ; i++)
    constraint += y[i] * svm->alpha[i] ;
  return(constraint) ;
}
#endif
