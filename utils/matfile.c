#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#include <mat.h>
#endif

#include "matfile.h"
#include "matrix.h"
#include "error.h"
#include "proto.h"

static char   *readMatHeader(FILE *fp, MATFILE *mf) ;
static double **matAlloc(int rows, int ncols) ;
static void   matFree(double **matrix, int nrows, int ncols) ;
static int    readMatFile(FILE *fp, MATFILE *mf, double **real_matrix, 
                                     double **imag_matrix) ;

static char *Progname = "matfile" ;

static int (*mat_printf)(const char *szFormat, ...)=NULL;
int Matlab_Install_printf( int (*new_printf)(const char *szFormat, ...) )
{
  mat_printf = new_printf;
  return(0);
}

int
MatlabWrite(MATRIX *mat, const char *fname, char *name)
{
  if (!name)
    name = "matrix" ;

  return(MatFileWrite(fname, mat->data, mat->rows, mat->cols, name)) ;
}

MATRIX *
MatlabRead(const char *fname)
{      
  MATRIX   *mat ;                        
  MATFILE  mf ;
  FILE     *fp ;
  char     *name ;
  double   **real_matrix, **imag_matrix ;
  int      file_type, nrows, ncols, row, col ;
  float    *fptr = NULL ;

(*mat_printf)("MatlabRead: opening file %s\n",fname);

  fp = fopen(fname, "rb") ;
  if (!fp)
      return(NULL) ;

(*mat_printf)("MatlabRead: reading header\n");

  name = readMatHeader(fp, &mf) ;
  if(name==NULL) {
    (*mat_printf)("MatlabRead: readHeader returned NULL\n");
    return(NULL);
  }

(*mat_printf)("MatlabRead: allocating real matrix (r=%d,c=%d)\n",
   (int)mf.mrows,(int)mf.ncols);

  real_matrix = matAlloc((int)mf.mrows, (int)mf.ncols) ;

(*mat_printf)("MatlabRead: done real mtx alloc\n");

  if (mf.imagf)
      imag_matrix = matAlloc((int)mf.mrows, (int)mf.ncols) ;
  else
       imag_matrix = NULL ;

  file_type = readMatFile(fp, &mf, real_matrix, imag_matrix) ;
  nrows = (int)mf.mrows ;
  ncols = (int)mf.ncols ;

  mat = MatrixAlloc(nrows, ncols, 
    mf.imagf ? MATRIX_COMPLEX : MATRIX_REAL) ;

  /* matrix row pointer is 1 based in both row and column */
  for (row = 1 ; row <= nrows ; row++)
  {
#if 0
    fptr = MATRIX_PTR(mat,row,1) ;
#else
    if (mf.imagf)
    {
      fptr = (float *)MATRIX_CELT(mat,row,1) ;
    }
    else
      fptr = MATRIX_RELT(mat,row,1) ;
#endif

    for (col = 1 ; col <= ncols ; col++)
    {
        *fptr++ = (float)real_matrix[row-1][col-1] ;
        if (mf.imagf)
          *fptr++ = (float)imag_matrix[row-1][col-1] ;
    }
  }

  matFree(real_matrix, nrows, ncols) ;
  if (mf.imagf)
      matFree(imag_matrix, nrows, ncols) ;

(*mat_printf)("MatlabRead: done\n");

  return(mat) ;
}

MATFILE *
MatFileRead(const char *fname, int type)
{                              
    MATFILE  *mf ;
    FILE     *fp ;
    char     *name ;
    double   **real_matrix, **imag_matrix ;
    char     bval ;
    int      file_type, nrows, ncols, row, col ;
    char     *cptr = NULL ;
    float    *fptr = NULL ;

    fp = fopen(fname, "rb") ;
    if (!fp)
        return(NULL) ;

    mf = (MATFILE *)calloc(1, sizeof(MATFILE)) ;      
    name = readMatHeader(fp, mf) ;

    real_matrix = matAlloc((int)mf->mrows, (int)mf->ncols) ;
    if (mf->imagf)
        imag_matrix = matAlloc((int)mf->mrows, (int)mf->ncols) ;
    else
        imag_matrix = NULL ;

    file_type = readMatFile(fp, mf, real_matrix, imag_matrix) ;

    nrows = (int)mf->mrows ;
    ncols = (int)mf->ncols ;
/*    type = MAT_BYTE ;*/
    switch (type)
    {
      case MAT_BYTE:
        mf->data = (char *)calloc(nrows*ncols, sizeof(char)) ;
        cptr = mf->data ;
        break ;
      case MAT_FLOAT:
        mf->data = (char *)calloc(nrows*ncols, sizeof(float)) ;
        fptr = (float *)mf->data ;
        break ;
      default:
        (*mat_printf)("MatFileRead: unsupported data format %d\n",
          type) ;
        break ;
    }

    for (row = 0 ; row < nrows ; row++)
    {
        for (col = 0 ; col < ncols ; col++)
        {
            switch (type)
            {
            case MAT_BYTE:
                bval = (char)real_matrix[row][col] ;         
                *cptr++ = bval ;
                break ;
            case MAT_FLOAT:
                *fptr++ = (float)real_matrix[row][col] ;         
                break ;
            }
        }
    }

    matFree(real_matrix, nrows, ncols) ;
    if (mf->imagf)
        matFree(imag_matrix, nrows, ncols) ;

    return(mf) ;
}

int
MatFileWrite(const char *fname, float *data, int rows, int cols, char *name)
{
  int     row, col, nitems, mtype ;
  float   *fptr ;   
  FILE    *fp ;
  double  dval ;
  MATFILE mf ;

  mf.namlen = (long)strlen(name)+1 ;
  mf.mrows = (long)rows ;
  mf.ncols = (long)cols ;
  mf.imagf = 0L ;
#ifdef SPARC
  mtype = MATFILE_SPARC ;
#else
  mtype = MATFILE_PC ;
#endif
  mf.type = mtype + MATFILE_DOUBLE + MATFILE_FULL ;

  fp = fopen(fname, "wb") ;
  if (!fp)
  {
    /*(*mat_printf)("MatFileWrite(%s): could not open file\n", fname) ;
    perror(NULL) ;*/
    return(-1) ;
  }

  nitems = fwrite(&mf, 1, sizeof(MATHD), fp) ;
  if (nitems != sizeof(MATHD))
  {
    fclose(fp) ;
    /*(*mat_printf)("MatFileWrite(%s): could not write header\n", fname) ;
    perror(NULL) ;*/
    return(-2) ;
  }

  nitems = fwrite(name, sizeof(char), (int)mf.namlen, fp) ;
  if (nitems != (int)mf.namlen)
  {
    fclose(fp) ;
    /*(*mat_printf)("MatFileWrite(%s): could not write name\n", fname) ;
    perror(NULL) ;*/
    return(-3) ;
  }

  /* write out in column-major format */
  for (col = 0 ; col < cols ; col++)
  {
    for (row = 0 ; row < rows ; row++)
    {
      fptr = data + row * cols + col ;
      dval = (double)(*fptr) ;
      nitems = fwrite(&dval, 1, sizeof(double), fp) ;
      if (nitems != sizeof(double))
      {
        fclose(fp) ;
        /*(*mat_printf)("MatFileWrite(%s): could not write (%d,%d)\n",
          fname, row, col) ;
        perror(NULL) ;*/
        return(-2) ;
      }
    }
  }
  
  fclose(fp) ;
  return(0) ;
}

static void
matFree(double **matrix, int rows, int cols)
{
    int    i ; i=cols; /* prevents warning */

    for (i = 0 ; i < rows ; i++)
        free(matrix[i]) ;

    free(matrix) ;
}

static double **
matAlloc(int rows, int cols)
{
    double **matrix ;
    int    i ;

    matrix = (double **)calloc(rows, sizeof(double *)) ;
    if (!matrix)
    {
        (*mat_printf)("could not allocate %d x %d matrix\n", rows, cols) ;
        /*exit(3) ;*/
  return(NULL);
    }

    for (i = 0 ; i < rows ; i++)
    {
        matrix[i] = (double *)calloc(cols, sizeof(double)) ;
        if (!matrix[i])
        {
            (*mat_printf)("could not allocate %d x %d matrix\n", rows, cols);
            /*exit(3) ;*/
      return(NULL);
        }
    }

    return(matrix) ;
}

static int
readMatFile(FILE *fp, MATFILE *mf, double **real_matrix, double **imag_matrix)
{
    int     row, col, type, nitems ;
    short   sval ;
    long    lval ;
    double  dval ;
    char    bval ;
    float   fval ;

    type = (int)mf->type - ((int)mf->type / 1000) * 1000 ;   /* remove 1000s from type */
    type = type - type / 100 * 100 ;               /* remove 100s from type */
    type = type / 10 * 10 ;       /* 10s digit specifies data type */
    switch (type)
    {
    case 0:
        type = MAT_DOUBLE ;
        break ;
    case 10:
        type = MAT_FLOAT ;
        break ;
    case 20:
        type = MAT_INT ;
        break ;
    case 30:  /* signed */
    case 40:  /* unsigned */
        type = MAT_SHORT ;
        break ;
    case 50:     /* bytes */
        type = MAT_BYTE ;
        break ;
    default:
        (*mat_printf)("unsupported matlab format %d (%s)\n",
                        type, type == MAT_FLOAT ? "float" : "unknown") ;
        break ;
    }
    
    /* data is stored column by column, real first then imag (if any) */
    for (col = 0 ; col < mf->ncols ; col++)
    {
        for (row = 0 ; row < mf->mrows ; row++)
        {
            switch(type)
            {
            case MAT_BYTE:
                nitems = fread(&bval, 1, sizeof(char), fp) ;
                if (nitems != sizeof(char))
                {
                    (*mat_printf)("%s: could not read val[%d, %d] from .mat file\n",
                                    Progname, row, col) ;
                    /*exit(4)*/return(-1) ;
                }
                real_matrix[row][col] = (double)bval ;
                break ;
            case MAT_SHORT:
                nitems = fread(&sval, 1, sizeof(short), fp) ;
                if (nitems != sizeof(char))
                {
                    (*mat_printf)("%s: could not read val[%d, %d] from .mat file\n",
                                    Progname, row, col) ;
                    /*exit(4)*/return(-1) ;
                }
                real_matrix[row][col] = (double)sval ;
                break ;
            case MAT_INT:   /* 32 bit integer */
                nitems = fread(&lval, 1, sizeof(long), fp) ;
                if (nitems != sizeof(long))
                {
                    (*mat_printf)("%s: could not read val[%d, %d] from .mat file\n",
                                    Progname, row, col) ;
                    /*exit(4)*/return(-1) ;
                }
                real_matrix[row][col] = (double)lval ;
                break ;
            case MAT_FLOAT:
                nitems = fread(&fval, 1, sizeof(float), fp) ;
                if (nitems != sizeof(float))
                {
                    (*mat_printf)("%s: could not read val[%d, %d] from .mat file\n",
                                    Progname, row, col) ;
                    /*exit(4)*/return(-1) ;
                }
                real_matrix[row][col] = (double)fval ;
                break ;
            case MAT_DOUBLE:
                nitems = fread(&dval, 1, sizeof(double), fp) ;
                if (nitems != sizeof(double))
                {
                    (*mat_printf)("%s: could not read val[%d, %d] from .mat file\n",
                                    Progname, row, col) ;
                    /*exit(4)*/return(-1) ;
                }
                real_matrix[row][col] = dval ;
                break ;
            default:
                break ;
            }
        }
    }

    if (imag_matrix) for (col = 0 ; col < mf->ncols ; col++)
    {
        for (row = 0 ; row < mf->mrows ; row++)
        {
            switch(type)
            {
            case MAT_DOUBLE:
                nitems = fread(&dval, 1, sizeof(double), fp) ;
                if (nitems != sizeof(double))
                {
      (*mat_printf)("%s: could not read val[%d, %d]"
        "from .mat file\n",Progname, row, col) ;
                    /*exit(4) ;*/
      return(-1);
                }
                break ;
            default:
                break ;
            }
            imag_matrix[row][col] = dval ;
        }
    }

    return(type) ;
}

static char *
readMatHeader(FILE *fp, MATFILE *mf)
{
    int   nitems ;
    char  *name ;

(*mat_printf)("readMatHeader: fp=%lx, mf=%lx\n",fp,mf);    

    nitems = fread(mf, 1, sizeof(MATHD), fp) ; 
    if (nitems != sizeof(MATHD))
    {
      (*mat_printf)("%s:only read %d bytes of header\n", Progname, nitems) ;
      /*exit(1) ;*/
      return(NULL);
    }

    (*mat_printf)("readMatHeader: nitems = %d, namelen=%d\n",
       nitems,(int)mf->namlen+1);

    name = (char *)calloc((int)mf->namlen+1, sizeof(char)) ;
    if (!name)
      ErrorExit(ERROR_NO_MEMORY, 
                "matReadHeader: couldn't allocate %d bytes for name",
                mf->namlen+1) ;

    nitems = fread(name, sizeof(char), (int)mf->namlen, fp) ;
    if (nitems != mf->namlen)
    {
      (*mat_printf)("%s: only read %d bytes of name (%ld specified)\n", 
                        Progname, nitems, mf->namlen) ;
      /*exit(1) ;*/
      return(NULL);
    }

#if 1
    (*mat_printf)("MATFILE: %ld x %ld, type %ld, imagf %ld, name '%s'\n",
                 mf->mrows, mf->ncols, mf->type, mf->imagf, name) ;
#endif

    return(name) ;
}

