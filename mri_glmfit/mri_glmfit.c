// mri_glmfit.c

/* 

BEGINUSAGE --------------------------------------------------------------

USAGE: ./mri_glmfit 

   --glmdir dir : save outputs to dir

   --y inputfile
   --fsgd FSGDF <gd2mtx> : freesurfer descriptor file
   --X design matrix file
   --C contrast1.mat <--C contrast2.mat ...>

   --pvr pvr1 <--prv pvr2 ...> : per-voxel regressors
   --selfreg col row slice   : self-regressor from index col row slice

   --w weightfile : weight for each input at each voxel
   --w-inv : invert weights
   --w-sqrt : sqrt of (inverted) weights

   --fwhm fwhm : smooth input by fwhm
   --var-fwhm fwhm : smooth variance by fwhm

   --mask maskfile : binary mask
   --label labelfile : use label as mask, surfaces only
   --mask-inv : invert mask
   --save-mask : save final mask to glmdir/mask.mgh

   --surf subject hemi : needed for some flags (uses white by default)

   --sim nulltype nsim thresh csdbasename : simulation perm, null-full, null-z

   --pca : perform pca/svd analysis on residual
   --save-yhat : flag to save signal estimate
   --save-cond  : flag to save design matrix condition at each voxel
   --voxdump col row slice  : dump voxel GLM and exit 

   --seed seed : used for synthesizing noise
   --synth : replace input with gaussian 

   --resynthtest niters : test GLM by resynthsis
   --profile     niters : test speed

   --diag Gdiag_no : set diagnositc level 
   --debug     turn on debugging
   --checkopts don't run anything, just check options and exit
   --help      print out information on how to use this program
   --version   print out version and exit


ENDUSAGE --------------------------------------------------------------

BEGINHELP --------------------------------------------------------------

OUTLINE:
  SUMMARY
  MATHEMATICAL BACKGROUND
  COMMAND-LINE ARGUMENTS
  MONTE CARLO SIMULATION AND CORRECTION FOR MULTIPLE COMPARISONS

SUMMARY

Performs general linear model (GLM) analysis in the volume or the
surface.  Options include simulation for correction for multiple
comparisons, weighted LMS, variance smoothing, PCA/SVD analysis of
residuals, per-voxel design matrices, and 'self' regressors. This
program performs both the estimation and inference. This program
is meant to replace mris_glm (which only operated on surfaces).
This program can be run in conjunction with mris_preproc.

MATHEMATICAL BACKGROUND

This brief intoduction to GLM theory is provided to help the user
understand what the inputs and outputs are and how to set the 
various parameters. These operations are performed at each voxel
or vertex separately (except with --var-fwhm).

The forward model is given by:

    y = W*X*B + n

where X is the Ns-by-Nb design matrix, y is the Ns-by-Nv input data
set, B is the Nb-by-Nv regression parameters, and n is noise. Ns is
the number of inputs, Nb is the number of regressors, and Nv is the
number of voxels/vertices (all cols/rows/slices). y may be surface
or volume data and may or may not have been spatially smoothed. W
is a diagonal weighing matrix. 

During the estimation stage, the forward model is inverted to
solve for B:

    B = inv(X'W'*W*X)*X'W'y

The signal estimate is computed as 

    yhat = B*X

The residual error is computed as 

    eres = y - yhat

The noise variance estimate (rvar) is computed as the sum of the
squares of the residual error divided by the DOF.  The DOF equals the
number of rows of X minus the number of columns.

A contrast matrix C has J rows and as many columns as columns of
X. The contrast is then computed as:

   G = C*B

The F-ratio for the contrast is then given by:

   F = G'*inv(C*inv(X'W'*W*X))*C')*G/(J*rvar)

The F is then used to compute a p-value.  Note that when J=1, this
reduces to a two-tailed t-test.


COMMAND-LINE ARGUMENTS

--glmdir dir

Directory where output will be saved. Not needed with --sim.

The outputs will be saved in mgh format as:
  mri_glmfit.log - execution parameters (send with bug reports)
  beta.mgh - all regression coefficients (B above)
  eres.mgh - residual error
  rvar.mgh - residual error variance
  rstd.mgh - residual error stddev (just sqrt of rvar)
  y.fsgd - fsgd file (if one was input)
  wn.mgh - normalized weights (with --w)
  yhat.mgh - signal estimate (with --save-yhat)
  mask.mgh - final mask (with --save-mask)
  cond.mgh - design matrix condition at each voxel (with --save-cond)
  contrast1name/ - directory for each contrast (see --C)
    C.dat - copy of contrast matrix
    gamma.mgh - contrast (G above)
    F.mgh - F-ratio
    sig.mgh - significance from F-test (actually -log10(p))

--y inputfile

Path to input file with each frame being a separate input. This can be
volume or surface-based, but the file must be one of the 'volume'
formats (eg, mgh, img, nii, etc) accepted by mri_convert. See
mris_preproc for an easy way to generate this file for surface data.

--fsgd fname <gd2mtx>

Specify the global design matrix with a FreeSurfer Group Descriptor
File (FSGDF).  See http://surfer.nmr.mgh.harvard.edu/docs/fsgdf.txt
for more info.  The gd2mtx is the method by which the group
description is converted into a design matrix. Legal values are doss
(Different Offset, Same Slope) and dods (Different Offset, Different
Slope). doss will create a design matrix in which each class has it's
own offset but forces all classes to have the same slope. dods models
each class with it's own offset and slope. In either case, you'll need
to know the order of the regressors in order to correctly specify the
contrast vector. For doss, the first NClass columns refer to the
offset for each class.  The remaining columns are for the continuous
variables. In dods, the first NClass columns again refer to the offset
for each class.  However, there will be NClass*NVar more columns (ie,
one column for each variable for each class). The first NClass columns
are for the first variable, etc. If neither of these models works for
you, you will have to specify the design matrix manually (with --X).

--X design matrix file

Specify the design matrix in matlab4 format. Within matlab, you can
save a matrix with save('X.mat','X','-v4');

--C contrast1.mat <--C contrast2.mat ...>

Specify one or more contrasts to test. The contrast.mat file is an
ASCII text file with the contrast matrix in it (make sure the last
line is blank). The output will be saved in glmdir/contrast1,
glmdir/contrast2, etc. Eg, if --C norm-v-cont.mat, then the ouput
will be glmdir/norm-v-cont.

--pvr pvr1 <--prv pvr2 ...>

Per-voxel (or vertex) regressors (PVR). Normally, the design matrix is
'global', ie, the same matrix is used at each voxel. This option allows the
user to specify voxel-specific regressors to append to the design
matrix. Note: the contrast matrices must include columns for these
components.

--selfreg col row slice

Create a 'self-regressor' from the input data based on the waveform at 
index col row slice. This waveform is residualized and then added as a 
column to the design matrix. Note: the contrast matrices must include 
columns for this component.

--w weightfile
--w-inv
--w-sqrt

Perform weighted LMS using per-voxel weights from the weightfile. The
data in weightfile must have the same dimensions as the input y
file. If --w-inv is flagged, then the inverse of each weight is used
as the weight.  If --w-sqrt is flagged, then the square root of each
weight is used as the weight.  If both are flagged, the inverse is
done first. The final weights are normalized so that the sum at each
voxel equals 1. The normalized weights are then saved in
glmdir/wn.mgh.  The --w-inv and --w-sqrt flags are useful when passing
contrast variances from a lower level analysis to a higher level
analysis (as is often done in fMRI).

--fwhm fwhm

Smooth input with a Gaussian kernel with the given full-width/half-maximum
(fwhm) specified in mm. If the data are surface-based, then you must
specify --surf, otherwise mri_glmfit assumes that the input is a volume
and will perform volume smoothing.

--var-fwhm fwhm

Smooth residual variance map with a Gaussian kernel with the given 
full-width/half-maximum (fwhm) specified in mm. If the data are 
surface-based, then you must specify --surf, otherwise mri_glmfit 
assumes that the input is a volume and will perform volume smoothing.

--mask maskfile
--label labelfile
--mask-inv
--save-mask

Only perform analysis where mask=1. All other voxels will be set to 0. 
If using surface, then labelfile will be converted to a binary mask 
(requires --surf). If --mask-inv is flagged, then performs analysis 
only where mask=0. If performing a simulation (--sim), map maximums
and clusters will only be searched for in the mask. --save-mask
will save final mask in glmdir/mask.mgh

--surf subject hemi

Specify that the input has a surface geometry from the hemisphere of the
given FreeSurfer subject. This is necessary for smoothing surface data
(--fwhm or --var-fwhm), specifying a label as a mask (--label), or
running a simulation (--sim) on surface data. If --surf is not specified,
then mri_glmfit will assume that the data are volume-based and use
the geometry as specified in the header to make spatial calculations.

--pca

Flag to perform PCA/SVD analysis on the residual. The result is stored
in glmdir/pca-eres as v.mgh (spatial eigenvectors), u.mat (frame 
eigenvectors), sdiag.mat (singular values). eres = u*s*v'. The matfiles
are just ASCII text. The spatial EVs can be loaded as overlays in 
tkmedit or tksurfer. In addition, there is stats.dat with 5 columns:
  (1) component number 
  (2) variance spanned by that component
  (3) cumulative variance spanned up to that component
  (4) percent variance spanned by that component
  (5) cumulative percent variance spanned up to that component

--save-yhat

Flag to save the signal estimate (yhat) as glmdir/yhat.mgh. Normally, this
pis not very useful except for debugging. 

--save-cond

Flag to save the condition number of the design matrix at eaach voxel.
Normally, this is not very useful except for debugging. It is totally
useless if not using weights or PVRs.

--seed seed

Use seed as the seed for the random number generator. By default, mri_glmfit
will select a seed based on time-of-day. This only has an effect with
--sim or --synth.

--synth

Replace input data with whise gaussian noise. This is good for testing.

--voxdump col row slice

Save GLM data for a single voxel in directory glmdir/voxdump-col-row-slice.
Exits immediately. Good for debugging.


MONTE CARLO SIMULATION AND CORRECTION FOR MULTIPLE COMPARISONS

One method for correcting for multiple comparisons is to perform simulations
under the null hypothesis and see how often the value of a statistic 
from the 'true' analysis is exceeded. This frequency is then interpreted
as a p-value which has been corrected for multiple comparisons. This
is especially useful with surface-based data as traditional random
field theory is harder to implement. This simulator is roughly based
on FSLs permuation simulator (randomise) and AFNIs null-z simulator
(AlphaSim). Note that FreeSurfer also offers False Discovery Rate (FDR) 
correction in tkmedit and tksurfer. 

The estimation, simulation, and correction are done in three distinct
phases:
  1. Estimation: run the analysis on your data without simulation. 
     At this point you can view your results (see if FDR is 
     sufficient:). 
  2. Simulation: run the simulator with the same parameters 
     as the estimation to get the Cluster Simulation Data (CSD). 
  3. Clustering: run mri_surfcluster or mri_volcluster with the CSD 
     from the simulator and the output of the estimation. These
     programs will print out clusters along with their p-values.

The Estimation step is described in detail above. The simulation
is invoked by calling mri_glmfit with the following arguments:

--sim nulltype nsim thresh csdbasename 

It is not necessary to specify --glmdir (it will be ignored). If 
you are analyzing surface data, then include --surf.

nulltype is the method of generating the null data. Legal values are: 
  (1) perm - perumation (cf FSL randomise)
  (2) null-full - replace input with white gaussian noise
  (3) null-z - do not actually do analysis, just assume the output 
      is z-distributed (cf ANFI AlphaSim)
nsim - number of simulation iterations to run (see below)
thresh - threshold, specified as -log10(pvalue) to use for clustering
csdbasename - base name of the file to store the CSD data in. Each
  contrast will get its own file (created by appending the contrast
  name to the base name). A '.csd' is appended to each file name.

Multiple simulations can be run in parallel by specifying different
csdbasenames. Then pass the multiple CSD files to mri_surfcluster 
and mri_volcluster. The Full CSD file is written on each iteration,
which means that the CSD file will be valid if the simulation
is aborted or crashes.

ENDHELP --------------------------------------------------------------

*/

// Things to do:
// Set up an actual CSD struct
// Package volume cluster better

// Save some sort of config in output dir.

// Leave-one-out
// Copies or Links to source data?

// Check to make sure no two contrast names are the same
// Check to make sure no two contrast mtxs are the same
// p-to-z
// Rewrite MatrixReadTxt to ignore # and % and empty lines
// Auto-det/read matlab4 matrices

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
double round(double x);
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "macros.h"
#include "utils.h"
#include "mrisurf.h"
#include "mrisutils.h"
#include "error.h"
#include "diag.h"
#include "mri.h"
#include "mri2.h"
#include "fio.h"
#include "version.h"
#include "label.h"
#include "matrix.h"
#include "annotation.h"
#include "fmriutils.h"
#include "cmdargs.h"
#include "fsglm.h"
#include "gsl/gsl_cdf.h"
#include "pdf.h"
#include "fsgdf.h"
#include "timer.h"
#include "matfile.h"
#include "volcluster.h"
#include "surfcluster.h"
#include "randomfields.h"

int MRISmaskByLabel(MRI *y, MRIS *surf, LABEL *lb, int invflag);

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void dump_options(FILE *fp);
static int SmoothSurfOrVol(MRIS *surf, MRI *mri, double SmthLevel);

int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_glmfit.c,v 1.51 2006/01/18 23:28:33 greve Exp $";
char *Progname = NULL;

int SynthSeed = -1;

char *yFile = NULL, *XFile=NULL, *betaFile=NULL, *rvarFile=NULL;
char *yhatFile=NULL, *eresFile=NULL, *wFile=NULL, *maskFile=NULL;
char *condFile=NULL;
char *GLMDir=NULL;
char *pvrFiles[50];
int yhatSave=0;
int condSave=0;
int maskSave=0;

char *labelFile=NULL;
LABEL *clabel=NULL;
int   maskinv = 0;
int   nmask, nvoxels, saveMask=0;
float maskfraction, voxelsize;

MRI *mritmp=NULL, *sig=NULL, *rstd;

int debug = 0, checkoptsonly = 0;
char tmpstr[2000];

int nContrasts=0;
char *CFile[100];
int err,c,r,s;
double Xcond;

int npvr=0;
MRIGLM *mriglm=NULL, *mriglmtmp=NULL;

double FWHM=0;
double SmoothLevel=0;
double VarFWHM=0;
double VarSmoothLevel=0;
double ResFWHM;

char voxdumpdir[1000];
int voxdump[3];
int voxdumpflag = 0;

char *fsgdfile = NULL;
FSGD *fsgd=NULL;
char  *gd2mtx_method = "none";

int nSelfReg = 0;
int crsSelfReg[100][3];
char *SUBJECTS_DIR;
int cmax, rmax, smax;
double Fmax, sigmax;

int pcaSave=0;
int npca = -1;
MATRIX *Upca=NULL,*Spca=NULL;
MRI *Vpca=NULL;

struct utsname uts;
char *cmdline, cwd[2000];

char *MaxVoxBase = NULL;
int DontSave = 0;

int DoSim=0;
int synth = 0;

SURFCLUSTERSUM *SurfClustList;
int nClusters;
char *subject=NULL, *hemi=NULL, *simbase=NULL;
MRI_SURFACE *surf=NULL;
int nsim,nthsim;
double csize;

int DiagCluster=0;
double DiagClusterSize=0;

double  InterVertexDistAvg, InterVertexDistStdDev, avgvtxarea, ar1mn, ar1std, ar1max;
double eresgstd, eresfwhm, searchspace;
MRI *ar1=NULL, *z=NULL;

CSD *csd;
RFS *rfs;
int weightinv=0, weightsqrt=0;

/*--------------------------------------------------*/
int main(int argc, char **argv)
{
  int nargs,n;
  struct timeb  mytimer;
  int msecFitTime;
  MATRIX *wvect=NULL, *Mtmp=NULL, *Xselfreg=NULL, *Xnorm=NULL;
  FILE *fp;

  eresfwhm = -1;
  csd = CSDalloc();
  csd->threshsign = 0; //0=abs,+1,-1

  /* rkt: check for and handle version tag */
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
  mriglm = (MRIGLM *) calloc(sizeof(MRIGLM),1);
  mriglm->glm = GLMalloc();

  if(argc == 0) usage_exit();

  parse_commandline(argc, argv);
  check_options();
  if(checkoptsonly) return(0);

  // Seed the random number generator just in case
  if(SynthSeed < 0) SynthSeed = PDFtodSeed();
  srand48(SynthSeed);
  
  if(surf != NULL){
    MRIScomputeMetricProperties(surf);
    InterVertexDistAvg    = surf->avg_vertex_dist;
    InterVertexDistStdDev = surf->std_vertex_dist;
    avgvtxarea = surf->avg_vertex_area;
    printf("Number of vertices %d\n",surf->nvertices);
    printf("Number of faces    %d\n",surf->nfaces);
    printf("Total area         %lf\n",surf->total_area);
    printf("AvgVtxArea       %lf\n",avgvtxarea);
    printf("AvgVtxDist       %lf\n",InterVertexDistAvg);
    printf("StdVtxDist       %lf\n",InterVertexDistStdDev);
  }

  // Compute number of iterations for surface smoothing
  if(FWHM > 0 && surf != NULL){
    SmoothLevel = MRISfwhm2nitersSubj(FWHM, subject, hemi, "white");
    printf("Surface smoothing by fwhm=%lf, niters=%lf\n",FWHM,SmoothLevel);
  } else SmoothLevel = FWHM;

  if(VarFWHM > 0 && surf != NULL){
    VarSmoothLevel = MRISfwhm2nitersSubj(VarFWHM, subject, hemi, "white");
    printf("Variance surface smoothing by fwhm=%lf, niters=%lf\n",VarFWHM,VarSmoothLevel);
  } else VarSmoothLevel = VarFWHM;

  dump_options(stdout);

  // Create the output directory 
  if(! DontSave){
    if(GLMDir != NULL){
      printf("Creating output directory %s\n",GLMDir);
      err = mkdir(GLMDir,(mode_t)-1);
      if(err != 0 && errno != EEXIST){
	printf("ERROR: creating directory %s\n",GLMDir);
	perror(NULL);    
	return(1);
      }
    }
    sprintf(tmpstr,"%s/mri_glmfit.log",GLMDir);
    fp = fopen(tmpstr,"w");
    dump_options(fp);
    fclose(fp);
  }

  mriglm->npvr     = npvr;
  mriglm->yhatsave = yhatSave;
  mriglm->condsave = condSave;

  // X ---------------------------------------------------------
  //Load global X------------------------------------------------
  if(XFile != NULL){  
    mriglm->Xg = MatrixReadTxt(XFile, NULL);
    if(mriglm->Xg==NULL){
      printf("ERROR: loading X %s\n",XFile);
      exit(1);
    }
  }
  else{
    mriglm->Xg = gdfMatrix(fsgd,gd2mtx_method,NULL);
    if(mriglm->Xg==NULL) exit(1);
  }

  // Check the condition of the global matrix -----------------
  Xnorm = MatrixNormalizeCol(mriglm->Xg,NULL);
  Xcond = MatrixNSConditionNumber(Xnorm);
  printf("Matrix condition is %g\n",Xcond);
  if(Xcond > 10000){
    printf("ERROR: matrix is ill-conditioned or badly scaled, condno = %g\n",
	   Xcond);
    MatrixPrint(stdout,mriglm->Xg);
    exit(1);
  }
  // Load Per-Voxel Regressors -----------------------------------
  if(mriglm->npvr > 0){
    for(n=0; n < mriglm->npvr; n++){
      mriglm->pvr[n] = MRIread(pvrFiles[n]);
      if(mriglm->pvr[n] == NULL) exit(1);
      if(mriglm->pvr[n]->nframes != mriglm->Xg->rows){
	printf("ERROR: dimension mismatch between pvr and X.\n");
	printf("  pvr has %d frames, X has %d rows.\n",
	       mriglm->pvr[n]->nframes,mriglm->Xg->rows);
	printf("PVR %d %s\n",n,pvrFiles[n]);
	exit(1);
      }
    }
  }

  // Load input--------------------------------------
  printf("Loading y from %s\n",yFile);
  mriglm->y = MRIread(yFile);
  if(mriglm->y == NULL){
    printf("ERROR: loading y %s\n",yFile);
    exit(1);
  }
  nvoxels = mriglm->y->width*mriglm->y->height*mriglm->y->depth;

  mriglm->mask = NULL;
  // Load the mask file ----------------------------------
  if(maskFile != NULL){
    mriglm->mask = MRIread(maskFile);
    if(mriglm->mask  == NULL){
      printf("ERROR: reading mask file %s\n",maskFile);
      exit(1);
    }
  }
  // Load the label mask file ----------------------------------
  if(labelFile != NULL){
    clabel = LabelRead(NULL, labelFile);
    if(clabel == NULL){
      printf("ERROR reading %s\n",labelFile);
      exit(1);
    }
    printf("Found %d points in label.\n",clabel->n_points);
    mriglm->mask = MRISlabel2Mask(surf, clabel, NULL);
    mritmp = mri_reshape(mriglm->mask, mriglm->y->width, 
			 mriglm->y->height, mriglm->y->depth, 1);
    MRIfree(&mriglm->mask);
    mriglm->mask = mritmp;
  }
  if(mriglm->mask && maskinv) MRImaskInvert(mriglm->mask,mriglm->mask);
  if(surf && mriglm->mask) MRISremoveRippedFromMask(surf, mriglm->mask, mriglm->mask);

  if(mriglm->mask){
    nmask = MRInMask(mriglm->mask);
    printf("Found %d voxels in mask\n",nmask);
    if(!DontSave && saveMask){
      sprintf(tmpstr,"%s/mask.mgh",GLMDir);
      MRIwrite(mriglm->mask,tmpstr);
    }
  }
  else nmask = nvoxels;
  maskfraction = (double)nmask/nvoxels;

  if(surf != NULL)  {
    searchspace = surf->total_area * maskfraction;
    if(surf->group_avg_surface_area > 0)
      searchspace *= (surf->group_avg_surface_area/surf->total_area);
  }
  else  searchspace = nmask * voxelsize;

  // Check number of frames ----------------------------------
  if(mriglm->y->nframes != mriglm->Xg->rows){
    printf("ERROR: dimension mismatch between y and X.\n");
    printf("  y has %d inputs, X has %d rows.\n",
	   mriglm->y->nframes,mriglm->Xg->rows);
    exit(1);
  }
  // Load the weight file ----------------------------------
  if(wFile != NULL){
    mriglm->w = MRIread(wFile);
    if(mriglm->w  == NULL){
      printf("ERROR: reading weight file %s\n",wFile);
      exit(1);
    }
    // Check number of frames
    if(mriglm->y->nframes != mriglm->w->nframes){
      printf("ERROR: dimension mismatch between y and w.\n");
      printf("  y has %d frames, w has %d frames.\n",
	     mriglm->y->nframes,mriglm->w->nframes);
      exit(1);
    }
    // Invert, Sqrt, and Normalize the weights
    mritmp = MRInormWeights(mriglm->w, weightsqrt, weightinv, 
			    mriglm->mask, mriglm->w);
    if(mritmp==NULL) exit(1);
    if(weightsqrt || weightinv){
      sprintf(tmpstr,"%s/wn.mgh",GLMDir);
      if(!DontSave) MRIwrite(mriglm->w,tmpstr);
    }
  }
  else mriglm->w = NULL;

  // Handle self-regressors
  if(nSelfReg > 0){
    for(n=0; n<nSelfReg; n++){
      c = crsSelfReg[n][0];
      r = crsSelfReg[n][1];
      s = crsSelfReg[n][2];
      printf("Self regressor %d     %d %d %d\n",n,c,r,s);
      if(c < 0 || c >= mriglm->y->width || 
	 r < 0 || r >= mriglm->y->height ||
	 s < 0 || s >= mriglm->y->depth){
	printf("ERROR: %d self regressor is out of the volume (%d,%d,%d)\n",
	       n,c,r,s);
	exit(1);
      }
      MRIglmLoadVox(mriglm,c,r,s,0);
      GLMxMatrices(mriglm->glm);
      GLMfit(mriglm->glm);
      GLMdump("selfreg",mriglm->glm);
      Mtmp = MatrixHorCat(Xselfreg,mriglm->glm->eres,NULL);
      if(n > 0) MatrixFree(&Xselfreg);
      Xselfreg = Mtmp;
    }

    // Need new mriglm, so alloc and copy the old one
    mriglmtmp = (MRIGLM *) calloc(sizeof(MRIGLM),1);
    mriglmtmp->glm = GLMalloc();
    mriglmtmp->y = mriglm->y;
    mriglmtmp->w = mriglm->w;
    mriglmtmp->Xg = MatrixHorCat(mriglm->Xg,Xselfreg,NULL);
    for(n=0; n < mriglm->npvr; n++) mriglmtmp->pvr[n] = mriglmtmp->pvr[n];
    //MRIglmFree(&mriglm);
    mriglm = mriglmtmp;
  }
  MRIglmNRegTot(mriglm);

  // Load the contrast matrices ---------------------------------
  mriglm->glm->ncontrasts = nContrasts;
  if(nContrasts > 0){
    for(n=0; n < nContrasts; n++){
      mriglm->glm->C[n] = MatrixReadTxt(CFile[n], NULL);
      if(mriglm->glm->C[n] == NULL){
	printf("ERROR: loading C %s\n",CFile[n]);
	exit(1);
      }
      if(mriglm->glm->C[n]->cols != mriglm->nregtot){
	printf("ERROR: dimension mismatch between X and contrast %s",CFile[n]);
	printf("       X has %d cols, C has %d cols\n",
	       mriglm->nregtot,mriglm->glm->C[n]->cols);
	exit(1);
      }
      // Get its name
      mriglm->glm->Cname[n] = fio_basename(CFile[n],".mat");
      // Should check to make sure no two are the same
    }
  }

  // At this point, y, X, and C have been loaded, now pre-alloc
  GLMallocX(mriglm->glm,mriglm->y->nframes,mriglm->nregtot);
  GLMallocY(mriglm->glm);

  // Check DOF
  GLMdof(mriglm->glm);
  printf("DOF = %g\n",mriglm->glm->dof);
  if(mriglm->glm->dof < 1){
    printf("ERROR: DOF = %g\n",mriglm->glm->dof);
    exit(1);
  }

  // Compute Contrast-related matrices
  GLMcMatrices(mriglm->glm);

  // Dump a voxel
  if(voxdumpflag){
    sprintf(voxdumpdir,"%s/voxdump-%d-%d-%d",GLMDir,
	    voxdump[0],voxdump[1],voxdump[2]);
    printf("Dumping voxel %d %d %d to %s\n",
	   voxdump[0],voxdump[1],voxdump[2],voxdumpdir);
    MRIglmLoadVox(mriglm,voxdump[0],voxdump[1],voxdump[2],0);
    GLMxMatrices(mriglm->glm);
    GLMfit(mriglm->glm);
    GLMtest(mriglm->glm);
    GLMdump(voxdumpdir,mriglm->glm);
    if(mriglm->w){
      wvect = MRItoVector(mriglm->w,voxdump[0],voxdump[1],voxdump[2],NULL);
      sprintf(tmpstr,"%s/w.dat",voxdumpdir);
      MatrixWriteTxt(tmpstr,wvect);
    }
    exit(0);
  }

  if(pcaSave){
    if(npca < 0) npca = mriglm->y->nframes;
    if(npca > mriglm->y->nframes){
      printf("ERROR: npca = %d, max can be %d\n",npca,mriglm->y->nframes);
      exit(1);
    }
  }

  if(synth){
    printf("Replacing input data with synthetic white noise\n");
    MRIrandn(mriglm->y->width,mriglm->y->height,mriglm->y->depth,mriglm->y->nframes,
	     0,1,mriglm->y);
  }
  if(FWHM > 0 && (!DoSim || !strcmp(csd->simtype,"perm")) ){
    printf("Smoothing input by fwhm %lf \n",FWHM);
    SmoothSurfOrVol(surf, mriglm->y, SmoothLevel);
    printf("   ... done\n");
  }

  // Don't do sim --------------------------------------------------------
  if(!DoSim){
    // Now do the estimation and testing
    TimerStart(&mytimer) ;

    if(VarFWHM > 0){
      printf("Starting fit\n");
      MRIglmFit(mriglm);
      printf("Variance smoothing\n");
      SmoothSurfOrVol(surf, mriglm->rvar, VarSmoothLevel);
      printf("Starting test\n");
      MRIglmTest(mriglm);
    }
    else{
      printf("Starting fit and test\n");
      MRIglmFitAndTest(mriglm);
    }
    msecFitTime = TimerStop(&mytimer) ;
    printf("Fit completed in %g minutes\n",msecFitTime/(1000*60.0));
  }

  //--------------------------------------------------------------------------
  //--------------------------------------------------------------------------
  //--------------------------------------------------------------------------
  if(DoSim){
    csd->seed = SynthSeed;
    if(surf != NULL){
      strcpy(csd->anattype,"surface");
      strcpy(csd->subject,subject);
      strcpy(csd->hemi,hemi);
    }
    else {
      strcpy(csd->anattype,"volume");
      voxelsize = mriglm->y->xsize * mriglm->y->ysize * mriglm->y->zsize;
    }
    csd->searchspace = searchspace;
    csd->nreps = nsim;
    CSDallocData(csd);
    if(!strcmp(csd->simtype,"null-z")){
      rfs = RFspecInit(SynthSeed,NULL);
      rfs->name = strcpyalloc("gaussian");
      rfs->params[0] = 0;
      rfs->params[1] = 1;
      z = MRIalloc(mriglm->y->width,mriglm->y->height,mriglm->y->depth,MRI_FLOAT);
    }

    printf("Staring simulation sim over %d trials\n",nsim);
    TimerStart(&mytimer) ;
    for(nthsim=0; nthsim < nsim; nthsim++){
      msecFitTime = TimerStop(&mytimer) ;
      printf("%4d/%d t=%g ----------------------\n",
	     nthsim+1,nsim,msecFitTime/(1000*60.0));

      if(!strcmp(csd->simtype,"null-full")){
	MRIrandn(mriglm->y->width,mriglm->y->height,mriglm->y->depth,
		 mriglm->y->nframes,0,1,mriglm->y);
	if(FWHM > 0){
	  printf("Smoothing Input ... \n");
	  SmoothSurfOrVol(surf, mriglm->y, SmoothLevel);
	  printf("   ... done\n");
	}
      }
      if(!strcmp(csd->simtype,"perm")) MatrixRandPermRows(mriglm->Xg);

      // Variance smoothing
      if(strcmp(csd->simtype,"null-z")){ // not a null z
	// If variance smoothing, then need to test and fit separately
	if(VarFWHM > 0){
	  printf("Starting fit\n");
	  MRIglmFit(mriglm);
	  printf("Variance smoothing\n");
	  SmoothSurfOrVol(surf, mriglm->rvar, VarSmoothLevel);
	  printf("Starting test\n");
	  MRIglmTest(mriglm);
	}
	else{
	  printf("Starting fit and test\n");
	  MRIglmFitAndTest(mriglm);
	}
      }
      
      // Go through each contrast
      for(n=0; n < mriglm->glm->ncontrasts; n++){
	if(strcmp(csd->simtype,"null-z")){
	  // not a null z
	  sig  = MRIlog10(mriglm->p[n],sig,1);
	  // If it is t-test (ie, one row) then apply the sign
	  if(mriglm->glm->C[n]->rows == 1) MRIsetSign(sig,mriglm->gamma[n],0);
	  sigmax = MRIframeMax(sig,0,mriglm->mask,1,&cmax,&rmax,&smax);
	  Fmax = MRIgetVoxVal(mriglm->F[n],cmax,rmax,smax,0);
	}
	else{
	  // null-z: synth z-field, smooth, rescale, compute p, compute sig
	  // This should do the same thing as AFNI's AlphaSim
	  RFsynth(z,rfs,mriglm->mask);
	  if(SmoothLevel > 0) SmoothSurfOrVol(surf, z, SmoothLevel);
	  RFrescale(z,rfs,mriglm->mask,z);
	  if(csd->threshsign == 0) MRIabs(z,z); // two-tailed
	  mriglm->p[n] = RFstat2P(z,rfs,mriglm->mask,mriglm->p[n]);
	  sig = MRIlog10(mriglm->p[n],sig,1);
	  if(mriglm->glm->C[n]->rows == 1) MRIsetSign(sig,z,0);
	  sigmax = MRIframeMax(sig,0,mriglm->mask,1,&cmax,&rmax,&smax);
	  Fmax = MRIgetVoxVal(z,cmax,rmax,smax,0);
	}
	if(mriglm->mask) MRImask(sig,mriglm->mask,sig,0.0,0.0);
	MRIScopyMRI(surf, sig, 0, "val");
	SurfClustList = sclustMapSurfClusters(surf,csd->thresh,-1,csd->threshsign,
					      0,&nClusters,NULL);
	csize = sclustMaxClusterArea(SurfClustList, nClusters);
	printf("%s %d %d   %g  %g  %g\n",mriglm->glm->Cname[n],nthsim,
	       nClusters,csize,sigmax,Fmax);

	// Re-write the full CSD file each time. Should not take that
	// long and assures output can be used immediately regardless
	// of whether the job terminated properly or not
	strcpy(csd->contrast,mriglm->glm->Cname[n]);
	sprintf(tmpstr,"%s-%s.csd",simbase,mriglm->glm->Cname[n]);
	//printf("csd %s \n",tmpstr);
	fflush(stdout);
	fp = fopen(tmpstr,"w");
	fprintf(fp,"# mri_glmfit simulation sim\n");
	fprintf(fp,"# hostname %s\n",uts.nodename);
	fprintf(fp,"# machine  %s\n",uts.machine);
	//fprintf(fp,"# label  %s\n",labelFile);
	//fprintf(fp,"# label-inv  %d\n",clabelinv);
	csd->nreps = nthsim+1;
	csd->nClusters[nthsim] = nClusters;
	csd->MaxClusterSize[nthsim] = csize;
	csd->MaxSig[nthsim] = sigmax;
	CSDprint(fp, csd);
	fclose(fp);

	if(DiagCluster && csize > DiagClusterSize){
	  sprintf(tmpstr,"./%s-sig.mgh",mriglm->glm->Cname[n]);
	  printf("Found cluster size =%lf > %lf, saving into %s\n",
		 csize,DiagClusterSize,tmpstr);
	  MRIwrite(sig,tmpstr);
	  exit(1);
	}

	MRIfree(&sig);
	free(SurfClustList);
      }
    }// simulation loop
    exit(0);
  }
  //--------------------------------------------------------------------------

  if(MaxVoxBase != NULL){
    for(n=0; n < mriglm->glm->ncontrasts; n++){
      sig    = MRIlog10(mriglm->p[n],sig,1);
      sigmax = MRIframeMax(sig,0,mriglm->mask,1,&cmax,&rmax,&smax);
      Fmax = MRIgetVoxVal(mriglm->F[n],cmax,rmax,smax,0);
      sprintf(tmpstr,"%s-%s",MaxVoxBase,mriglm->glm->Cname[n]);
      fp = fopen(tmpstr,"a");
      fprintf(fp,"%e  %e    %d %d %d     %d\n",
	      sigmax,Fmax,cmax,rmax,smax,SynthSeed);
      fclose(fp);
      MRIfree(&sig);
    }
  }

  if(DontSave) exit(0);

  // Compute fwhm of residual
  if(surf != NULL){
    printf("Computing spatial AR1 \n");
    ar1 = MRISar1(surf, mriglm->eres, mriglm->mask, NULL);
    sprintf(tmpstr,"%s/ar1.mgh",GLMDir);
    MRIwrite(ar1,tmpstr);
    RFglobalStats(ar1, mriglm->mask, &ar1mn, &ar1std, &ar1max);
    eresgstd = InterVertexDistAvg/sqrt(-4*log(ar1mn));
    eresfwhm = eresgstd*sqrt(log(256.0));
    printf("Residual: ar1mn=%lf, ar1std=%lf, gstd=%lf, fwhm=%lf\n",
	   ar1mn,ar1std,eresgstd,eresfwhm);
    //printf("Residual: ar1mn=%lf, ar1std=%lf, ar1max=%lf, gstd=%lf, fwhm=%lf\n",
    //   ar1mn,ar1std,ar1max,eresgstd,eresfwhm);
  }

  // Save estimation results
  printf("Writing results\n");
  MRIwrite(mriglm->beta,betaFile);
  MRIwrite(mriglm->rvar,rvarFile);

  rstd = MRIsqrt(mriglm->rvar,NULL);
  sprintf(tmpstr,"%s/rstd.mgh",GLMDir);
  MRIwrite(rstd,tmpstr);

  if(mriglm->yhatsave) MRIwrite(mriglm->yhat,yhatFile);
  if(mriglm->condsave) MRIwrite(mriglm->cond,condFile);
  if(eresFile) MRIwrite(mriglm->eres,eresFile);    

  sprintf(tmpstr,"%s/Xg.dat",GLMDir);
  MatrixWriteTxt(tmpstr, mriglm->Xg);
  
  // Save the contrast results
  for(n=0; n < mriglm->glm->ncontrasts; n++){
    printf("  %s\n",mriglm->glm->Cname[n]);
    
    // Create output directory for contrast
    sprintf(tmpstr,"%s/%s",GLMDir,mriglm->glm->Cname[n]);
    mkdir(tmpstr,(mode_t)-1);
    
    // Dump contrast matrix
    sprintf(tmpstr,"%s/%s/C.dat",GLMDir,mriglm->glm->Cname[n]);
    MatrixWriteTxt(tmpstr, mriglm->glm->C[n]);

    // Save gamma 
    sprintf(tmpstr,"%s/%s/gamma.mgh",GLMDir,mriglm->glm->Cname[n]);
    MRIwrite(mriglm->gamma[n],tmpstr);

    // Save F
    sprintf(tmpstr,"%s/%s/F.mgh",GLMDir,mriglm->glm->Cname[n]);
    MRIwrite(mriglm->F[n],tmpstr);
    
    // Compute and Save -log10 p-values
    sig=MRIlog10(mriglm->p[n],sig,1);
    if(mriglm->mask) MRImask(sig,mriglm->mask,sig,0.0,0.0);


    // If it is t-test (ie, one row) then apply the sign
    if(mriglm->glm->C[n]->rows == 1) MRIsetSign(sig,mriglm->gamma[n],0);

    // Write out the sig
    sprintf(tmpstr,"%s/%s/sig.mgh",GLMDir,mriglm->glm->Cname[n]);
    MRIwrite(sig,tmpstr);

    // Find and save the max sig
    sigmax = MRIframeMax(sig,0,mriglm->mask,1,&cmax,&rmax,&smax);
    Fmax = MRIgetVoxVal(mriglm->F[n],cmax,rmax,smax,0);
    printf("    maxvox sig=%g  F=%g  at  index %d %d %d    seed=%d\n",
	   sigmax,Fmax,cmax,rmax,smax,SynthSeed);

    sprintf(tmpstr,"%s/%s/maxvox.dat",GLMDir,mriglm->glm->Cname[n]);
    fp = fopen(tmpstr,"w");
    fprintf(fp,"%e  %e    %d %d %d     %d\n",
	    sigmax,Fmax,cmax,rmax,smax,SynthSeed);
    fclose(fp);
    
    MRIfree(&sig);
  }

  // --------- Save FSGDF stuff --------------------------------
  if(fsgd != NULL){
    strcpy(fsgd->measname,"external");
    sprintf(fsgd->datafile,"%s",yFile);

    sprintf(tmpstr,"%s/fsgd.X.mat",GLMDir);
    MatlabWrite(mriglm->Xg,tmpstr,"X");
    sprintf(fsgd->DesignMatFile,"fsgd.X.mat");
    sprintf(tmpstr,"%s/y.fsgd",GLMDir);
    fsgd->ResFWHM = eresfwhm;

    fp = fopen(tmpstr,"w");
    gdfPrintHeader(fp,fsgd);
    fprintf(fp,"Creator          %s\n",Progname);
    fprintf(fp,"SUBJECTS_DIR     %s\n",SUBJECTS_DIR);
    fprintf(fp,"SynthSeed        %d\n",SynthSeed);
    fclose(fp);
  }

  // Compute and save PCA
  if(pcaSave){
    printf("Computing PCA (%d)\n",npca);
    sprintf(tmpstr,"%s/pca-eres",GLMDir);
    mkdir(tmpstr,(mode_t)-1);
    err=MRIpca(mriglm->eres, &Upca, &Spca, &Vpca, mriglm->mask);
    if(err) exit(1);
    sprintf(tmpstr,"%s/eres-pca/v.mgh",GLMDir);
    MRIwrite(Vpca,tmpstr);
    sprintf(tmpstr,"%s/eres-pca/u.mat",GLMDir);
    MatrixWriteTxt(tmpstr, Upca);
    sprintf(tmpstr,"%s/eres-pca/sdiag.mat",GLMDir);
    MatrixWriteTxt(tmpstr, Spca);
    sprintf(tmpstr,"%s/eres-pca/stats.dat",GLMDir);
    WritePCAStats(tmpstr,Spca);
  }

  // re-write the log file, adding a few things
  sprintf(tmpstr,"%s/mri_glmfit.log",GLMDir);
  fp = fopen(tmpstr,"w");
  dump_options(fp);
  fprintf(fp,"ResidualFWHM %lf\n",eresfwhm);
  fprintf(fp,"SearchSpace %lf\n",searchspace);
  if(surf != NULL)  fprintf(fp,"anattype surface\n");
  else              fprintf(fp,"anattype volume\n");
  fclose(fp);

  printf("mri_glmfit done\n");
  return(0); 
  exit(0);

}
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/

/* --------------------------------------------- */
static int parse_commandline(int argc, char **argv)
{
  int  nargc , nargsused, msec, niters;
  char **pargv, *option ;
  double rvartmp;

  if(argc < 1) usage_exit();

  nargc   = argc;
  pargv = argv;
  while(nargc > 0){

    option = pargv[0];
    if(debug) printf("%d %s\n",nargc,option);
    nargc -= 1;
    pargv += 1;

    nargsused = 0;

    if (!strcasecmp(option, "--help"))  print_help() ;
    else if (!strcasecmp(option, "--version")) print_version() ;
    else if (!strcasecmp(option, "--debug"))   debug = 1;
    else if (!strcasecmp(option, "--checkopts"))   checkoptsonly = 1;
    else if (!strcasecmp(option, "--nocheckopts")) checkoptsonly = 0;
    else if (!strcasecmp(option, "--save-yhat")) yhatSave = 1;
    else if (!strcasecmp(option, "--save-cond")) condSave = 1;
    else if (!strcasecmp(option, "--save-mask")) maskSave = 1;
    else if (!strcasecmp(option, "--dontsave")) DontSave = 1;
    else if (!strcasecmp(option, "--synth"))   synth = 1;
    else if (!strcasecmp(option, "--mask-inv"))  maskinv = 1;
    else if (!strcasecmp(option, "--w-inv"))  weightinv = 1;
    else if (!strcasecmp(option, "--w-sqrt")) weightsqrt = 1;

    else if (!strcasecmp(option, "--diag")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%d",&Gdiag_no);
      nargsused = 1;
    }
    else if (!strcasecmp(option, "--diag-cluster")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%lf",&DiagClusterSize);
      DiagCluster=1;
      nargsused = 1;
    }
    else if (!strcasecmp(option, "--sim")){
      if(nargc < 4) CMDargNErr(option,4);
      if(CSDcheckSimType(pargv[0])){
	printf("ERROR: simulation type %s unrecognized, supported values are\n"
	       "  perm, null-full, null-z\n", pargv[0]);
	exit(1);
      }
      strcpy(csd->simtype,pargv[0]);
      sscanf(pargv[1],"%d",&nsim);
      sscanf(pargv[2],"%lf",&csd->thresh);
      simbase = pargv[3]; // basename
      printf("simbase %s\n",simbase);
      DoSim = 1;
      DontSave = 1;
      nargsused = 4;
    }
    else if (!strcasecmp(option, "--surf")){
      if(nargc < 2) CMDargNErr(option,1);
      SUBJECTS_DIR = getenv("SUBJECTS_DIR");
      if(SUBJECTS_DIR == NULL){
	printf("ERROR: SUBJECTS_DIR not defined in environment\n");
	exit(1);
      }
      subject = pargv[0];
      hemi    = pargv[1];
      nargsused = 2;
      sprintf(tmpstr,"%s/%s/surf/%s.white",SUBJECTS_DIR,subject,hemi);
      printf("Reading source surface %s\n",tmpstr);
      surf = MRISread(tmpstr) ;
      if (!surf)
	ErrorExit(ERROR_NOFILE, "%s: could not read surface %s", Progname, tmpstr) ;
    }
    else if (!strcasecmp(option, "--seed")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%d",&SynthSeed);
      nargsused = 1;
    }
    else if(!strcasecmp(option, "--smooth") || !strcasecmp(option, "--fwhm")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%lf",&FWHM);
      csd->nullfwhm = FWHM;
      nargsused = 1;
    }
    else if(!strcasecmp(option, "--var-smooth") || !strcasecmp(option, "--var-fwhm")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%lf",&VarFWHM);
      csd->varfwhm = VarFWHM;
      nargsused = 1;
    }
    else if (!strcasecmp(option, "--voxdump")){
      if(nargc < 3) CMDargNErr(option,3);
      sscanf(pargv[0],"%d",&voxdump[0]);
      sscanf(pargv[1],"%d",&voxdump[1]);
      sscanf(pargv[2],"%d",&voxdump[2]);
      voxdumpflag = 1;
      nargsused = 3;
    }
    else if (!strcasecmp(option, "--selfreg")){
      if(nargc < 3) CMDargNErr(option,3);
      sscanf(pargv[0],"%d",&crsSelfReg[nSelfReg][0]);
      sscanf(pargv[1],"%d",&crsSelfReg[nSelfReg][1]);
      sscanf(pargv[2],"%d",&crsSelfReg[nSelfReg][2]);
      nSelfReg++;
      nargsused = 3;
    }
    else if (!strcasecmp(option, "--profile")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%d",&niters);
      if(SynthSeed < 0) SynthSeed = PDFtodSeed();
      srand48(SynthSeed);
      printf("Starting GLM profile over %d iterations. Seed=%d\n",
	     niters,SynthSeed);
      msec = GLMprofile(200, 20, 5, niters);
      nargsused = 1;
      exit(0);
    }
    else if (!strcasecmp(option, "--resynthtest")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%d",&niters);
      if(SynthSeed < 0) SynthSeed = PDFtodSeed();
      srand48(SynthSeed);
      printf("Starting GLM resynth test over %d iterations. Seed=%d\n",
	     niters,SynthSeed);
      err = GLMresynthTest(niters, &rvartmp);
      if(err){
	printf("Failed. rvar = %g\n",rvartmp);
	exit(1);
      }
      printf("Passed. rvarmax = %g\n",rvartmp);
      exit(0);
      nargsused = 1;
    }
    else if (!strcmp(option, "--y")){
      if(nargc < 1) CMDargNErr(option,1);
      yFile = fio_fullpath(pargv[0]);
      nargsused = 1;
    }
    else if (!strcmp(option, "--mask")){
      if(nargc < 1) CMDargNErr(option,1);
      maskFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--label")){
      if(nargc < 1) CMDargNErr(option,1);
      labelFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--w")){
      if(nargc < 1) CMDargNErr(option,1);
      wFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--X")){
      if(nargc < 1) CMDargNErr(option,1);
      XFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--pvr")){
      if(nargc < 1) CMDargNErr(option,1);
      pvrFiles[npvr] = pargv[0];
      npvr++;
      nargsused = 1;
    }
    else if (!strcmp(option, "--glmdir")){
      if(nargc < 1) CMDargNErr(option,1);
      GLMDir = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--beta")){
      if(nargc < 1) CMDargNErr(option,1);
      betaFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--rvar")){
      if(nargc < 1) CMDargNErr(option,1);
      rvarFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--yhat")){
      if(nargc < 1) CMDargNErr(option,1);
      yhatFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--eres")){
      if(nargc < 1) CMDargNErr(option,1);
      eresFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--C")){
      if(nargc < 1) CMDargNErr(option,1);
      CFile[nContrasts] = pargv[0];
      nContrasts++;
      nargsused = 1;
    }
    else if (!strcmp(option, "--pca")){
      if(CMDnthIsArg(nargc, pargv, 0)){
	sscanf(pargv[0],"%d",&niters);
	nargsused = 1;
      }
      pcaSave = 1;
    }
    else if ( !strcmp(option, "--fsgd") ){
      if(nargc < 1) CMDargNErr(option,1);
      fsgdfile = pargv[0];
      nargsused = 1;
      fsgd = gdfRead(fsgdfile,0);
      if(fsgd==NULL) exit(1);
      if(CMDnthIsArg(nargc, pargv, 1)){
	gd2mtx_method = pargv[1]; nargsused ++;
	if(gdfCheckMatrixMethod(gd2mtx_method)) exit(1);
      }
      else gd2mtx_method = "dods";
      printf("INFO: gd2mtx_method is %s\n",gd2mtx_method);
      strcpy(fsgd->DesignMatMethod,gd2mtx_method);
    }
    else if (!strcmp(option, "--maxvox")){
      if(nargc < 1) CMDargNErr(option,1);
      MaxVoxBase = pargv[0];
      nargsused = 1;
    }
    else{
      fprintf(stderr,"ERROR: Option %s unknown\n",option);
      if(CMDsingleDash(option))
	fprintf(stderr,"       Did you really mean -%s ?\n",option);
      exit(-1);
    }
    nargc -= nargsused;
    pargv += nargsused;
  }
  return(0);
}
/* ------------------------------------------------------ */
static void usage_exit(void)
{
  print_usage() ;
  exit(1) ;
}
/* --------------------------------------------- */
static void print_usage(void)
{
printf("\n");
printf("USAGE: ./mri_glmfit \n");
printf("\n");
printf("   --glmdir dir : save outputs to dir\n");
printf("\n");
printf("   --y inputfile\n");
printf("   --fsgd FSGDF <gd2mtx> : freesurfer descriptor file\n");
printf("   --X design matrix file\n");
printf("   --C contrast1.mat <--C contrast2.mat ...>\n");
printf("\n");
printf("   --pvr pvr1 <--prv pvr2 ...> : per-voxel regressors\n");
printf("   --selfreg col row slice   : self-regressor from index col row slice\n");
printf("\n");
printf("   --w weightfile : weight for each input at each voxel\n");
printf("   --w-inv : invert weights\n");
printf("   --w-sqrt : sqrt of (inverted) weights\n");
printf("\n");
printf("   --fwhm fwhm : smooth input by fwhm\n");
printf("   --var-fwhm fwhm : smooth variance by fwhm\n");
printf("\n");
printf("   --mask maskfile : binary mask\n");
printf("   --label labelfile : use label as mask, surfaces only\n");
printf("   --mask-inv : invert mask\n");
printf("   --save-mask : save final mask to glmdir/mask.mgh\n");
printf("\n");
printf("   --surf subject hemi : needed for some flags (uses white by default)\n");
printf("\n");
printf("   --sim nulltype nsim thresh csdbasename : simulation perm, null-full, null-z\n");
printf("\n");
printf("   --pca : perform pca/svd analysis on residual\n");
printf("   --save-yhat : flag to save signal estimate\n");
printf("   --save-cond  : flag to save design matrix condition at each voxel\n");
printf("   --voxdump col row slice  : dump voxel GLM and exit \n");
printf("\n");
printf("   --seed seed : used for synthesizing noise\n");
printf("   --synth : replace input with gaussian \n");
printf("\n");
printf("   --resynthtest niters : test GLM by resynthsis\n");
printf("   --profile     niters : test speed\n");
printf("\n");
printf("   --diag Gdiag_no : set diagnositc level \n");
printf("   --debug     turn on debugging\n");
printf("   --checkopts don't run anything, just check options and exit\n");
printf("   --help      print out information on how to use this program\n");
printf("   --version   print out version and exit\n");
printf("\n");
printf("\n");


}
/* --------------------------------------------- */
static void print_help(void)
{
  print_usage() ;
printf("\n");
printf("OUTLINE:\n");
printf("  SUMMARY\n");
printf("  MATHEMATICAL BACKGROUND\n");
printf("  COMMAND-LINE ARGUMENTS\n");
printf("  MONTE CARLO SIMULATION AND CORRECTION FOR MULTIPLE COMPARISONS\n");
printf("\n");
printf("SUMMARY\n");
printf("\n");
printf("Performs general linear model (GLM) analysis in the volume or the\n");
printf("surface.  Options include simulation for correction for multiple\n");
printf("comparisons, weighted LMS, variance smoothing, PCA/SVD analysis of\n");
printf("residuals, per-voxel design matrices, and 'self' regressors. This\n");
printf("program performs both the estimation and inference. This program\n");
printf("is meant to replace mris_glm (which only operated on surfaces).\n");
printf("This program can be run in conjunction with mris_preproc.\n");
printf("\n");
printf("MATHEMATICAL BACKGROUND\n");
printf("\n");
printf("This brief intoduction to GLM theory is provided to help the user\n");
printf("understand what the inputs and outputs are and how to set the \n");
printf("various parameters. These operations are performed at each voxel\n");
printf("or vertex separately (except with --var-fwhm).\n");
printf("\n");
printf("The forward model is given by:\n");
printf("\n");
printf("    y = W*X*B + n\n");
printf("\n");
printf("where X is the Ns-by-Nb design matrix, y is the Ns-by-Nv input data\n");
printf("set, B is the Nb-by-Nv regression parameters, and n is noise. Ns is\n");
printf("the number of inputs, Nb is the number of regressors, and Nv is the\n");
printf("number of voxels/vertices (all cols/rows/slices). y may be surface\n");
printf("or volume data and may or may not have been spatially smoothed. W\n");
printf("is a diagonal weighing matrix. \n");
printf("\n");
printf("During the estimation stage, the forward model is inverted to\n");
printf("solve for B:\n");
printf("\n");
printf("    B = inv(X'W'*W*X)*X'W'y\n");
printf("\n");
printf("The signal estimate is computed as \n");
printf("\n");
printf("    yhat = B*X\n");
printf("\n");
printf("The residual error is computed as \n");
printf("\n");
printf("    eres = y - yhat\n");
printf("\n");
printf("The noise variance estimate (rvar) is computed as the sum of the\n");
printf("squares of the residual error divided by the DOF.  The DOF equals the\n");
printf("number of rows of X minus the number of columns.\n");
printf("\n");
printf("A contrast matrix C has J rows and as many columns as columns of\n");
printf("X. The contrast is then computed as:\n");
printf("\n");
printf("   G = C*B\n");
printf("\n");
printf("The F-ratio for the contrast is then given by:\n");
printf("\n");
printf("   F = G'*inv(C*inv(X'W'*W*X))*C')*G/(J*rvar)\n");
printf("\n");
printf("The F is then used to compute a p-value.  Note that when J=1, this\n");
printf("reduces to a two-tailed t-test.\n");
printf("\n");
printf("\n");
printf("COMMAND-LINE ARGUMENTS\n");
printf("\n");
printf("--glmdir dir\n");
printf("\n");
printf("Directory where output will be saved. Not needed with --sim.\n");
printf("\n");
printf("The outputs will be saved in mgh format as:\n");
printf("  mri_glmfit.log - execution parameters (send with bug reports)\n");
printf("  beta.mgh - all regression coefficients (B above)\n");
printf("  eres.mgh - residual error\n");
printf("  rvar.mgh - residual error variance\n");
printf("  rstd.mgh - residual error stddev (just sqrt of rvar)\n");
printf("  y.fsgd - fsgd file (if one was input)\n");
printf("  wn.mgh - normalized weights (with --w)\n");
printf("  yhat.mgh - signal estimate (with --save-yhat)\n");
printf("  mask.mgh - final mask (with --save-mask)\n");
printf("  cond.mgh - design matrix condition at each voxel (with --save-cond)\n");
printf("  contrast1name/ - directory for each contrast (see --C)\n");
printf("    C.dat - copy of contrast matrix\n");
printf("    gamma.mgh - contrast (G above)\n");
printf("    F.mgh - F-ratio\n");
printf("    sig.mgh - significance from F-test (actually -log10(p))\n");
printf("\n");
printf("--y inputfile\n");
printf("\n");
printf("Path to input file with each frame being a separate input. This can be\n");
printf("volume or surface-based, but the file must be one of the 'volume'\n");
printf("formats (eg, mgh, img, nii, etc) accepted by mri_convert. See\n");
printf("mris_preproc for an easy way to generate this file for surface data.\n");
printf("\n");
printf("--fsgd fname <gd2mtx>\n");
printf("\n");
printf("Specify the global design matrix with a FreeSurfer Group Descriptor\n");
printf("File (FSGDF).  See http://surfer.nmr.mgh.harvard.edu/docs/fsgdf.txt\n");
printf("for more info.  The gd2mtx is the method by which the group\n");
printf("description is converted into a design matrix. Legal values are doss\n");
printf("(Different Offset, Same Slope) and dods (Different Offset, Different\n");
printf("Slope). doss will create a design matrix in which each class has it's\n");
printf("own offset but forces all classes to have the same slope. dods models\n");
printf("each class with it's own offset and slope. In either case, you'll need\n");
printf("to know the order of the regressors in order to correctly specify the\n");
printf("contrast vector. For doss, the first NClass columns refer to the\n");
printf("offset for each class.  The remaining columns are for the continuous\n");
printf("variables. In dods, the first NClass columns again refer to the offset\n");
printf("for each class.  However, there will be NClass*NVar more columns (ie,\n");
printf("one column for each variable for each class). The first NClass columns\n");
printf("are for the first variable, etc. If neither of these models works for\n");
printf("you, you will have to specify the design matrix manually (with --X).\n");
printf("\n");
printf("--X design matrix file\n");
printf("\n");
printf("Specify the design matrix in matlab4 format. Within matlab, you can\n");
printf("save a matrix with save('X.mat','X','-v4');\n");
printf("\n");
printf("--C contrast1.mat <--C contrast2.mat ...>\n");
printf("\n");
printf("Specify one or more contrasts to test. The contrast.mat file is an\n");
printf("ASCII text file with the contrast matrix in it (make sure the last\n");
printf("line is blank). The output will be saved in glmdir/contrast1,\n");
printf("glmdir/contrast2, etc. Eg, if --C norm-v-cont.mat, then the ouput\n");
printf("will be glmdir/norm-v-cont.\n");
printf("\n");
printf("--pvr pvr1 <--prv pvr2 ...>\n");
printf("\n");
printf("Per-voxel (or vertex) regressors (PVR). Normally, the design matrix is\n");
printf("'global', ie, the same matrix is used at each voxel. This option allows the\n");
printf("user to specify voxel-specific regressors to append to the design\n");
printf("matrix. Note: the contrast matrices must include columns for these\n");
printf("components.\n");
printf("\n");
printf("--selfreg col row slice\n");
printf("\n");
printf("Create a 'self-regressor' from the input data based on the waveform at \n");
printf("index col row slice. This waveform is residualized and then added as a \n");
printf("column to the design matrix. Note: the contrast matrices must include \n");
printf("columns for this component.\n");
printf("\n");
printf("--w weightfile\n");
printf("--w-inv\n");
printf("--w-sqrt\n");
printf("\n");
printf("Perform weighted LMS using per-voxel weights from the weightfile. The\n");
printf("data in weightfile must have the same dimensions as the input y\n");
printf("file. If --w-inv is flagged, then the inverse of each weight is used\n");
printf("as the weight.  If --w-sqrt is flagged, then the square root of each\n");
printf("weight is used as the weight.  If both are flagged, the inverse is\n");
printf("done first. The final weights are normalized so that the sum at each\n");
printf("voxel equals 1. The normalized weights are then saved in\n");
printf("glmdir/wn.mgh.  The --w-inv and --w-sqrt flags are useful when passing\n");
printf("contrast variances from a lower level analysis to a higher level\n");
printf("analysis (as is often done in fMRI).\n");
printf("\n");
printf("--fwhm fwhm\n");
printf("\n");
printf("Smooth input with a Gaussian kernel with the given full-width/half-maximum\n");
printf("(fwhm) specified in mm. If the data are surface-based, then you must\n");
printf("specify --surf, otherwise mri_glmfit assumes that the input is a volume\n");
printf("and will perform volume smoothing.\n");
printf("\n");
printf("--var-fwhm fwhm\n");
printf("\n");
printf("Smooth residual variance map with a Gaussian kernel with the given \n");
printf("full-width/half-maximum (fwhm) specified in mm. If the data are \n");
printf("surface-based, then you must specify --surf, otherwise mri_glmfit \n");
printf("assumes that the input is a volume and will perform volume smoothing.\n");
printf("\n");
printf("--mask maskfile\n");
printf("--label labelfile\n");
printf("--mask-inv\n");
printf("--save-mask\n");
printf("\n");
printf("Only perform analysis where mask=1. All other voxels will be set to 0. \n");
printf("If using surface, then labelfile will be converted to a binary mask \n");
printf("(requires --surf). If --mask-inv is flagged, then performs analysis \n");
printf("only where mask=0. If performing a simulation (--sim), map maximums\n");
printf("and clusters will only be searched for in the mask. --save-mask\n");
printf("will save final mask in glmdir/mask.mgh\n");
printf("\n");
printf("--surf subject hemi\n");
printf("\n");
printf("Specify that the input has a surface geometry from the hemisphere of the\n");
printf("given FreeSurfer subject. This is necessary for smoothing surface data\n");
printf("(--fwhm or --var-fwhm), specifying a label as a mask (--label), or\n");
printf("running a simulation (--sim) on surface data. If --surf is not specified,\n");
printf("then mri_glmfit will assume that the data are volume-based and use\n");
printf("the geometry as specified in the header to make spatial calculations.\n");
printf("\n");
printf("--pca\n");
printf("\n");
printf("Flag to perform PCA/SVD analysis on the residual. The result is stored\n");
printf("in glmdir/pca-eres as v.mgh (spatial eigenvectors), u.mat (frame \n");
printf("eigenvectors), sdiag.mat (singular values). eres = u*s*v'. The matfiles\n");
printf("are just ASCII text. The spatial EVs can be loaded as overlays in \n");
printf("tkmedit or tksurfer. In addition, there is stats.dat with 5 columns:\n");
printf("  (1) component number \n");
printf("  (2) variance spanned by that component\n");
printf("  (3) cumulative variance spanned up to that component\n");
printf("  (4) percent variance spanned by that component\n");
printf("  (5) cumulative percent variance spanned up to that component\n");
printf("\n");
printf("--save-yhat\n");
printf("\n");
printf("Flag to save the signal estimate (yhat) as glmdir/yhat.mgh. Normally, this\n");
printf("pis not very useful except for debugging. \n");
printf("\n");
printf("--save-cond\n");
printf("\n");
printf("Flag to save the condition number of the design matrix at eaach voxel.\n");
printf("Normally, this is not very useful except for debugging. It is totally\n");
printf("useless if not using weights or PVRs.\n");
printf("\n");
printf("--seed seed\n");
printf("\n");
printf("Use seed as the seed for the random number generator. By default, mri_glmfit\n");
printf("will select a seed based on time-of-day. This only has an effect with\n");
printf("--sim or --synth.\n");
printf("\n");
printf("--synth\n");
printf("\n");
printf("Replace input data with whise gaussian noise. This is good for testing.\n");
printf("\n");
printf("--voxdump col row slice\n");
printf("\n");
printf("Save GLM data for a single voxel in directory glmdir/voxdump-col-row-slice.\n");
printf("Exits immediately. Good for debugging.\n");
printf("\n");
printf("\n");
printf("MONTE CARLO SIMULATION AND CORRECTION FOR MULTIPLE COMPARISONS\n");
printf("\n");
printf("One method for correcting for multiple comparisons is to perform simulations\n");
printf("under the null hypothesis and see how often the value of a statistic \n");
printf("from the 'true' analysis is exceeded. This frequency is then interpreted\n");
printf("as a p-value which has been corrected for multiple comparisons. This\n");
printf("is especially useful with surface-based data as traditional random\n");
printf("field theory is harder to implement. This simulator is roughly based\n");
printf("on FSLs permuation simulator (randomise) and AFNIs null-z simulator\n");
printf("(AlphaSim). Note that FreeSurfer also offers False Discovery Rate (FDR) \n");
printf("correction in tkmedit and tksurfer. \n");
printf("\n");
printf("The estimation, simulation, and correction are done in three distinct\n");
printf("phases:\n");
printf("  1. Estimation: run the analysis on your data without simulation. \n");
printf("     At this point you can view your results (see if FDR is \n");
printf("     sufficient:). \n");
printf("  2. Simulation: run the simulator with the same parameters \n");
printf("     as the estimation to get the Cluster Simulation Data (CSD). \n");
printf("  3. Clustering: run mri_surfcluster or mri_volcluster with the CSD \n");
printf("     from the simulator and the output of the estimation. These\n");
printf("     programs will print out clusters along with their p-values.\n");
printf("\n");
printf("The Estimation step is described in detail above. The simulation\n");
printf("is invoked by calling mri_glmfit with the following arguments:\n");
printf("\n");
printf("--sim nulltype nsim thresh csdbasename \n");
printf("\n");
printf("It is not necessary to specify --glmdir (it will be ignored). If \n");
printf("you are analyzing surface data, then include --surf.\n");
printf("\n");
printf("nulltype is the method of generating the null data. Legal values are: \n");
printf("  (1) perm - perumation (cf FSL randomise)\n");
printf("  (2) null-full - replace input with white gaussian noise\n");
printf("  (3) null-z - do not actually do analysis, just assume the output \n");
printf("      is z-distributed (cf ANFI AlphaSim)\n");
printf("nsim - number of simulation iterations to run (see below)\n");
printf("thresh - threshold, specified as -log10(pvalue) to use for clustering\n");
printf("csdbasename - base name of the file to store the CSD data in. Each\n");
printf("  contrast will get its own file (created by appending the contrast\n");
printf("  name to the base name). A '.csd' is appended to each file name.\n");
printf("\n");
printf("Multiple simulations can be run in parallel by specifying different\n");
printf("csdbasenames. Then pass the multiple CSD files to mri_surfcluster \n");
printf("and mri_volcluster. The Full CSD file is written on each iteration,\n");
printf("which means that the CSD file will be valid if the simulation\n");
printf("is aborted or crashes.\n");
printf("\n");
printf("\n");
printf("\n");
printf("\n");
printf("\n");
printf("\n");


  exit(1) ;
}
/* --------------------------------------------- */
static void print_version(void)
{
  printf("%s\n", vcid) ;
  exit(1) ;
}
/* --------------------------------------------- */
static void check_options(void)
{
  if(yFile == NULL){
    printf("ERROR: must specify input y file\n");
    exit(1);
  }
  if(XFile == NULL && fsgdfile == NULL){
    printf("ERROR: must specify an input X file or fsgd file\n");
    exit(1);
  }
  if(XFile && fsgdfile ){
    printf("ERROR: cannot specify both X file and fsgd file\n");
    exit(1);
  }

  if(GLMDir == NULL && !DontSave){
    printf("ERROR: must specify GLM output dir\n");
    exit(1);
  }
  if(GLMDir != NULL){
    sprintf(tmpstr,"%s/beta.mgh",GLMDir);
    betaFile = strcpyalloc(tmpstr);
    sprintf(tmpstr,"%s/rvar.mgh",GLMDir);
    rvarFile = strcpyalloc(tmpstr);
    sprintf(tmpstr,"%s/eres.mgh",GLMDir);
    eresFile = strcpyalloc(tmpstr);
    if(yhatSave){
      sprintf(tmpstr,"%s/yhat.mgh",GLMDir);
      yhatFile = strcpyalloc(tmpstr);
    }
    if(condSave){
      sprintf(tmpstr,"%s/cond.mgh",GLMDir);
      condFile = strcpyalloc(tmpstr);
    }
  }

  if(nContrasts == 0){
    printf("ERROR: no contrasts specified\n");
    exit(1);
  }

  if(SUBJECTS_DIR == NULL){
    SUBJECTS_DIR = getenv("SUBJECTS_DIR");
    if(SUBJECTS_DIR==NULL){
      fprintf(stderr,"ERROR: SUBJECTS_DIR not defined in environment\n");
      exit(1);
    }
  }

  if(labelFile != NULL && surf==NULL){
    printf("ERROR: need --surf with --label\n");
    exit(1);
  }

  if(DoSim && VarFWHM > 0 && !strcmp(csd->simtype,"null-z")){
    printf("ERROR: cannot use variance smoothing with null-z simulation\n");
    exit(1);
  }

  if(saveMask && (maskFile==NULL && labelFile==NULL)){
    printf("ERROR: need mask with --save-mask\n");
    exit(1);
  }


  return;
}

/* --------------------------------------------- */
static void dump_options(FILE *fp)
{
  int n;

  fprintf(fp,"\n");
  fprintf(fp,"%s\n",vcid);
  fprintf(fp,"cwd %s\n",cwd);
  fprintf(fp,"cmdline %s\n",cmdline);
  fprintf(fp,"sysname  %s\n",uts.sysname);
  fprintf(fp,"hostname %s\n",uts.nodename);
  fprintf(fp,"machine  %s\n",uts.machine);
  fprintf(fp,"user     %s\n",VERuser());

  if(FWHM > 0){
    fprintf(fp,"fwhm     %lf\n",FWHM);
    if(surf != NULL) fprintf(fp,"niters    %lf\n",SmoothLevel);
  }
  if(VarFWHM > 0){
    fprintf(fp,"varfwhm  %lf\n",VarFWHM);
    if(surf != NULL) fprintf(fp,"varniters %lf\n",VarSmoothLevel);
  }

  if(synth) fprintf(fp,"SynthSeed = %d\n",SynthSeed);

  fprintf(fp,"y    %s\n",yFile);
  if(XFile)     fprintf(fp,"X    %s\n",XFile);
  if(fsgdfile)  fprintf(fp,"FSGD %s\n",fsgdfile);
  if(labelFile) fprintf(fp,"labelmask  %s\n",labelFile);
  if(maskFile)  fprintf(fp,"mask %s\n",maskFile);
  if(labelFile || maskFile) fprintf(fp,"maskinv %d\n",maskinv);

  fprintf(fp,"glmdir %s\n",GLMDir);

  for(n=0; n < nSelfReg; n++){
    fprintf(fp,"SelfRegressor %d  %4d %4d %4d\n",n+1,
	   crsSelfReg[n][0],crsSelfReg[n][1],crsSelfReg[n][2]);
  }

  return;
}

/*--------------------------------------------------------------------*/
static int SmoothSurfOrVol(MRIS *surf, MRI *mri, double SmthLevel)
{
  double gstd;

  if(surf == NULL){
    gstd = SmthLevel/sqrt(log(256.0));
    printf("  Volume Smoothing by FWHM=%lf, Gstd=%lf\n",SmthLevel,gstd);
    MRIgaussianSmooth(mri, gstd, 1, mri); /* 1 = normalize */
  }
  else{
    printf("  Surface Smoothing by %d iterations\n",(int)SmthLevel);
    MRISsmoothMRI(surf, mri, SmthLevel, mri);
  }
  return(0);
}
/*--------------------------------------------------------------------*/
int MRISmaskByLabel(MRI *y, MRIS *surf, LABEL *lb, int invflag)
{
  int **crslut, *lbmask, vtxno, n, c, r, s, f;

  lbmask = (int*) calloc(surf->nvertices,sizeof(int));

  // Set each label vertex in lbmask to 1
  for(n=0; n<lb->n_points; n++){
    vtxno = lb->lv[n].vno;
    lbmask[vtxno] = 1;
  }

  crslut = MRIScrsLUT(surf, y);
  for(vtxno = 0; vtxno < surf->nvertices; vtxno++){
    if(lbmask[vtxno] && !invflag) continue; // in-label and not inverting
    if(!lbmask[vtxno] && invflag) continue; // out-label but inverting
    c = crslut[0][vtxno];
    r = crslut[1][vtxno];
    s = crslut[2][vtxno];
    for(f=0; f < y->nframes; f++) MRIsetVoxVal(y,c,r,s,f,0);
  }

  free(lbmask);
  MRIScrsLUTFree(crslut);
  return(0);
}
