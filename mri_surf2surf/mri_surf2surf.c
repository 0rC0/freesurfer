/*----------------------------------------------------------
  Name: mri_surf2surf.c
  $Id: mri_surf2surf.c,v 1.6 2002/04/19 21:25:45 greve Exp $
  Author: Douglas Greve
  Purpose: Resamples data from one surface onto another. If
  both the source and target subjects are the same, this is
  just a format conversion. The source or target subject may
  be ico.  Can handle data with multiple frames.
  -----------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mri.h"
#include "icosahedron.h"
#include "fio.h"

#include "MRIio.h"
#include "error.h"
#include "diag.h"
#include "mrisurf.h"
#include "mri2.h"
#include "mri_identify.h"

#include "bfileio.h"
#include "registerio.h"
#include "resample.h"
#include "selxavgio.h"
#include "prime.h"

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void argnerr(char *option, int n);
static void dump_options(FILE *fp);
static int  singledash(char *flag);
int GetNVtxsFromWFile(char *wfile);
int GetICOOrderFromValFile(char *filename, char *fmt);
int GetNVtxsFromValFile(char *filename, char *fmt);

int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_surf2surf.c,v 1.6 2002/04/19 21:25:45 greve Exp $";
char *Progname = NULL;

char *surfreg = "sphere.reg";
char *hemi    = NULL;

char *srcsubject = NULL;
char *srcvalfile = NULL;
char *srctypestring = NULL;
int   srctype = MRI_VOLUME_TYPE_UNKNOWN;
MRI  *SrcVals, *SrcHits, *SrcDist;
MRI_SURFACE *SrcSurfReg;
char *SrcHitFile = NULL;
char *SrcDistFile = NULL;
int nSrcVtxs = 0;
int SrcIcoOrder;

char *trgsubject = NULL;
char *trgvalfile = NULL;
char *trgtypestring = NULL;
int   trgtype = MRI_VOLUME_TYPE_UNKNOWN;
MRI  *TrgVals, *TrgValsSmth, *TrgHits, *TrgDist;
MRI_SURFACE *TrgSurfReg;
char *TrgHitFile = NULL;
char *TrgDistFile = NULL;
int TrgIcoOrder;

MRI  *mritmp;
int  reshape = 1;
int  reshapefactor;

char *mapmethod = "nnfr";

int UseHash = 1;
int framesave = 0;
float IcoRadius = 100.0;
int nSmoothSteps = 0;
int nthstep, nnbrs, nthnbr, nbrvtx, frame;

int debug = 0;

char *SUBJECTS_DIR = NULL;
char *MRI_DIR = NULL;
SXADAT *sxa;
FILE *fp;

char tmpstr[2000];

int ReverseMapFlag = 0;
int cavtx = 0; /* command-line vertex -- for debugging */

int main(int argc, char **argv)
{
  int f,vtx,tvtx,svtx,n;
  float *framepower = NULL;
  char fname[2000];
  int nTrg121,nSrc121,nSrcLost;
  int nTrgMulti,nSrcMulti;
  float MnTrgMultiHits,MnSrcMultiHits, val;

  Progname = argv[0] ;
  argc --;
  argv++;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  if(argc == 0) usage_exit();

  parse_commandline(argc, argv);
  check_options();
  dump_options(stdout);

  SUBJECTS_DIR = getenv("SUBJECTS_DIR");
  if(SUBJECTS_DIR==NULL){
    fprintf(stderr,"ERROR: SUBJECTS_DIR not defined in environment\n");
    exit(1);
  }
  MRI_DIR = getenv("MRI_DIR") ;
  if(MRI_DIR==NULL){
    fprintf(stderr,"ERROR: MRI_DIR not defined in environment\n");
    exit(1);
  }

  /* --------- Load the registration surface for source subject --------- */
  if(!strcmp(srcsubject,"ico")){ /* source is ico */
    SrcIcoOrder = GetICOOrderFromValFile(srcvalfile,srctypestring);
    sprintf(fname,"%s/lib/bem/ic%d.tri",MRI_DIR,SrcIcoOrder);
    SrcSurfReg = ReadIcoByOrder(SrcIcoOrder, IcoRadius);
    printf("Source Ico Order = %d\n",SrcIcoOrder);
  }
  else{
    sprintf(fname,"%s/%s/surf/%s.%s",SUBJECTS_DIR,srcsubject,hemi,surfreg);
    printf("Reading source surface reg %s\n",fname);
    SrcSurfReg = MRISread(fname) ;
  }
  if (!SrcSurfReg)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface %s", Progname, fname) ;
  printf("Done\n");

  /* ------------------ load the source data ----------------------------*/
  printf("Loading source data\n");
  if(!strcmp(srctypestring,"curv")){ /* curvature file */
    sprintf(fname,"%s/%s/surf/%s.%s",SUBJECTS_DIR,srcsubject,hemi,srcvalfile);
    printf("Reading curvature file %s\n",fname);
    MRISreadCurvatureFile(SrcSurfReg, fname);
    SrcVals = MRIallocSequence(SrcSurfReg->nvertices, 1, 1,MRI_FLOAT,1);
    for(vtx = 0; vtx < SrcSurfReg->nvertices; vtx++){
      MRIFseq_vox(SrcVals,vtx,0,0,0) = SrcSurfReg->vertices[vtx].curv;
      if(vtx == cavtx){
  printf("vtx = %d, curv = %g, val = %g\n",vtx,
         SrcSurfReg->vertices[vtx].curv,
         MRIFseq_vox(SrcVals,vtx,0,0,0));
  DiagBreak();
      }
    }
  }
  else if(!strcmp(srctypestring,"paint") || !strcmp(srctypestring,"w")){
    MRISreadValues(SrcSurfReg,srcvalfile);
    SrcVals = MRIallocSequence(SrcSurfReg->nvertices, 1, 1,MRI_FLOAT,1);
    for(vtx = 0; vtx < SrcSurfReg->nvertices; vtx++)
      MRIFseq_vox(SrcVals,vtx,0,0,0) = SrcSurfReg->vertices[vtx].val;
  }
  else { /* Use MRIreadType */
    //SrcVals = mri_load_bvolume(srcvalfile); 
    SrcVals =  MRIreadType(srcvalfile,srctype);
    if(SrcVals == NULL){
      printf("ERROR: could not read %s as type %d\n",srcvalfile,srctype);
      exit(1);
    }
    if(SrcVals->height != 1 || SrcVals->depth != 1){
      reshapefactor = SrcVals->height * SrcVals->depth;
      printf("Reshaping %d\n",reshapefactor);
      mritmp = mri_reshape(SrcVals, reshapefactor*SrcVals->width, 
         1, 1, SrcVals->nframes);
      MRIfree(&SrcVals);
      SrcVals = mritmp;
      reshapefactor = 0; /* reset for output */
    }

    if(SrcVals->width != SrcSurfReg->nvertices){
      fprintf(stderr,"ERROR: dimesion inconsitency in source data\n");
      fprintf(stderr,"       Number of surface vertices = %d\n",
        SrcSurfReg->nvertices);
      fprintf(stderr,"       Number of value vertices = %d\n",SrcVals->width);
      exit(1);
    }
    if(is_sxa_volume(srcvalfile)){
      printf("INFO: Source volume detected as selxavg format\n");
      sxa = ld_sxadat_from_stem(srcvalfile);
      if(sxa == NULL) exit(1);
      framepower = sxa_framepower(sxa,&f);
      if(f != SrcVals->nframes){
  fprintf(stderr," number of frames is incorrect (%d,%d)\n",
    f,SrcVals->nframes);
  exit(1);
      }
      printf("INFO: Adjusting Frame Power\n");  fflush(stdout);
      mri_framepower(SrcVals,framepower);
    }
  }
  if(SrcVals == NULL){
    fprintf(stderr,"ERROR loading source values from %s\n",srcvalfile);
    exit(1);
  }
  printf("Done\n");

  if(strcmp(srcsubject,trgsubject)){
    /* ------- Source and Target Subjects are different -------------- */
    /* ------- Load the registration surface for target subject ------- */
    if(!strcmp(trgsubject,"ico")){
      sprintf(fname,"%s/lib/bem/ic%d.tri",MRI_DIR,TrgIcoOrder);
      TrgSurfReg = ReadIcoByOrder(TrgIcoOrder, IcoRadius);
      reshapefactor = 6;
    }
    else{
      sprintf(fname,"%s/%s/surf/%s.%s",SUBJECTS_DIR,trgsubject,hemi,surfreg);
      printf("Reading target surface reg %s\n",fname);
      TrgSurfReg = MRISread(fname) ;
    }
    if (!TrgSurfReg)
      ErrorExit(ERROR_NOFILE, "%s: could not read surface %s", 
    Progname, fname) ;
    printf("Done\n");
    
    if(!strcmp(mapmethod,"nnfr")) ReverseMapFlag = 1;
    else                          ReverseMapFlag = 0;
    
    /*-------------------------------------------------------------*/
    /* Map the values from the surface to surface */
    printf("Mapping Source Volume onto Source Subject Surface\n");
    TrgVals = surf2surf_nnfr(SrcVals, SrcSurfReg,TrgSurfReg,
           &SrcHits,&SrcDist,&TrgHits,&TrgDist,
           ReverseMapFlag,UseHash);
    
    
    /* Compute some stats on the mapping number of srcvtx mapping to a 
       target vtx*/
    nTrg121 = 0;
    MnTrgMultiHits = 0.0;
    for(tvtx = 0; tvtx < TrgSurfReg->nvertices; tvtx++){
      n = MRIFseq_vox(TrgHits,tvtx,0,0,0);
      if(n == 1) nTrg121++;
      else MnTrgMultiHits += n;
    }
    nTrgMulti = TrgSurfReg->nvertices - nTrg121;
    if(nTrgMulti > 0) MnTrgMultiHits = (MnTrgMultiHits/nTrgMulti);
    else              MnTrgMultiHits = 0;
    printf("nTrg121 = %5d, nTrgMulti = %5d, MnTrgMultiHits = %g\n",
     nTrg121,nTrgMulti,MnTrgMultiHits);
    
    /* Compute some stats on the mapping number of trgvtxs mapped from a 
       source vtx*/
    nSrc121 = 0;
    nSrcLost = 0;
    MnSrcMultiHits = 0.0;
    for(svtx = 0; svtx < SrcSurfReg->nvertices; svtx++){
      n = MRIFseq_vox(SrcHits,svtx,0,0,0);
      if(n == 1) nSrc121++;
      else if(n == 0) nSrcLost++;
      else MnSrcMultiHits += n;
    }
    nSrcMulti = SrcSurfReg->nvertices - nSrc121;
    if(nSrcMulti > 0) MnSrcMultiHits = (MnSrcMultiHits/nSrcMulti);
    else              MnSrcMultiHits = 0;
    
    printf("nSrc121 = %5d, nSrcLost = %5d, nSrcMulti = %5d, MnSrcMultiHits = %g\n",
     nSrc121,nSrcLost,nSrcMulti,MnSrcMultiHits);
    
    /* save the Source Hits into a .w file */
    if(SrcHitFile != NULL){
      printf("INFO: saving source hits to %s\n",SrcHitFile);
      for(vtx = 0; vtx < SrcSurfReg->nvertices; vtx++)
  SrcSurfReg->vertices[vtx].val = MRIFseq_vox(SrcHits,vtx,0,0,0) ;
      MRISwriteValues(SrcSurfReg, SrcHitFile) ;
    }
    /* save the Source Distance into a .w file */
    if(SrcDistFile != NULL){
      printf("INFO: saving source distance to %s\n",SrcDistFile);
      for(vtx = 0; vtx < SrcSurfReg->nvertices; vtx++)
  SrcSurfReg->vertices[vtx].val = MRIFseq_vox(SrcDist,vtx,0,0,0) ;
      MRISwriteValues(SrcSurfReg, SrcDistFile) ;
    }
    /* save the Target Hits into a .w file */
    if(TrgHitFile != NULL){
      printf("INFO: saving target hits to %s\n",TrgHitFile);
      for(vtx = 0; vtx < TrgSurfReg->nvertices; vtx++)
  TrgSurfReg->vertices[vtx].val = MRIFseq_vox(TrgHits,vtx,0,0,0) ;
      MRISwriteValues(TrgSurfReg, TrgHitFile) ;
    }
    /* save the Target Hits into a .w file */
    if(TrgDistFile != NULL){
      printf("INFO: saving target distance to %s\n",TrgDistFile);
      for(vtx = 0; vtx < TrgSurfReg->nvertices; vtx++)
  TrgSurfReg->vertices[vtx].val = MRIFseq_vox(TrgDist,vtx,0,0,0) ;
      MRISwriteValues(TrgSurfReg, TrgDistFile) ;
    }
  }
  else{
    /* --- Source and Target Subjects are the same --- */
    printf("INFO: trgsubject = srcsubject\n");
    TrgSurfReg = SrcSurfReg;
    TrgVals = SrcVals;
  }
       
  /* Smooth if desired */
  if(nSmoothSteps > 0){
    nnbrs = TrgSurfReg->vertices[0].vnum;
    printf("INFO: Smoothing, NSteps = %d, NNbrs = %d\n",nSmoothSteps,nnbrs);

    TrgValsSmth = MRIcopy(TrgVals,NULL);

    for(nthstep = 0; nthstep < nSmoothSteps; nthstep ++){
      printf("Step = %d\n",nthstep);
      fflush(stdout);
      for(vtx = 0; vtx < TrgSurfReg->nvertices; vtx++){
  nnbrs = TrgSurfReg->vertices[vtx].vnum;
  //if(nnbrs != 5) printf("%4d %d\n",vtx,nnbrs);
  for(frame = 0; frame < TrgVals->nframes; frame ++){
    val = MRIFseq_vox(TrgVals,vtx,0,0,frame);
    for(nthnbr = 0; nthnbr < nnbrs; nthnbr++){
      nbrvtx = TrgSurfReg->vertices[vtx].v[nthnbr];
      val += MRIFseq_vox(TrgVals,nbrvtx,0,0,frame) ;
    }/* end loop over neighbor */
    MRIFseq_vox(TrgValsSmth,vtx,0,0,frame) = (val/(nnbrs+1));
  }/* end loop over frame */
      } /* end loop over vertex */
      MRIcopy(TrgValsSmth,TrgVals);
    }/* end loop over smooth step */
    MRIfree(&TrgValsSmth);
  }


  /* readjust frame power if necessary */
  if(is_sxa_volume(srcvalfile)){
    printf("INFO: Readjusting Frame Power\n");  fflush(stdout);
    for(f=0; f < TrgVals->nframes; f++) framepower[f] = 1.0/framepower[f];
    mri_framepower(TrgVals,framepower);
    sxa->nrows = 1;
    sxa->ncols = TrgVals->width;
  }

  /* ------------ save the target data -----------------------------*/
  printf("Saving target data\n");
  if(!strcmp(trgtypestring,"paint") || !strcmp(trgtypestring,"w")){
    for(vtx = 0; vtx < TrgSurfReg->nvertices; vtx++)
      TrgSurfReg->vertices[vtx].val = MRIFseq_vox(TrgVals,vtx,0,0,framesave);
    MRISwriteValues(TrgSurfReg,trgvalfile);
  }
  else {
    /*mri_save_as_bvolume(TrgVals,trgvalfile,0,BF_FLOAT); */
    if(reshape){
      if(reshapefactor == 0) 
  reshapefactor = GetClosestPrimeFactor(TrgVals->width,6);

      printf("Reshaping %d (nvertices = %d)\n",reshapefactor,TrgVals->width);
      mritmp = mri_reshape(TrgVals, TrgVals->width / reshapefactor, 
         1, reshapefactor,TrgVals->nframes);
      if(mritmp == NULL){
  printf("ERROR: mri_reshape could not alloc\n");
  return(1);
      }
      MRIfree(&TrgVals);
      TrgVals = mritmp;
    }
    MRIwriteType(TrgVals,trgvalfile,trgtype);
    if(is_sxa_volume(srcvalfile)) sv_sxadat_by_stem(sxa,trgvalfile);
  }

  return(0);
}
/* --------------------------------------------- */
static int parse_commandline(int argc, char **argv)
{
  int  nargc , nargsused;
  char **pargv, *option ;

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
    else if (!strcasecmp(option, "--usehash")) UseHash = 1;
    else if (!strcasecmp(option, "--hash")) UseHash = 1;
    else if (!strcasecmp(option, "--dontusehash")) UseHash = 0;
    else if (!strcasecmp(option, "--nohash")) UseHash = 0;
    else if (!strcasecmp(option, "--noreshape")) reshape = 0;
    else if (!strcasecmp(option, "--reshape"))   reshape = 1;

    /* -------- source value inputs ------ */
    else if (!strcmp(option, "--srcsubject")){
      if(nargc < 1) argnerr(option,1);
      srcsubject = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--srcsurfval")){
      if(nargc < 1) argnerr(option,1);
      srcvalfile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--srcfmt") ||
       !strcmp(option, "--src_type")){
      if(nargc < 1) argnerr(option,1);
      srctypestring = pargv[0];
      srctype = string_to_type(srctypestring);
      nargsused = 1;
    }
    else if (!strcmp(option, "--nsmooth")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%d",&nSmoothSteps);
      if(nSmoothSteps < 1){
  fprintf(stderr,"ERROR: number of smooth steps (%d) must be >= 1\n",
    nSmoothSteps);
      }
      nargsused = 1;
    }

    /* -------- target value inputs ------ */
    else if (!strcmp(option, "--trgsubject")){
      if(nargc < 1) argnerr(option,1);
      trgsubject = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--trgicoorder")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%d",&TrgIcoOrder);
      nargsused = 1;
    }
    else if (!strcmp(option, "--trgsurfval")){
      if(nargc < 1) argnerr(option,1);
      trgvalfile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--trgfmt") ||
       !strcmp(option, "--trg_type")){
      if(nargc < 1) argnerr(option,1);
      trgtypestring = pargv[0];
      if(!strcmp(trgtypestring,"curv")){
  fprintf(stderr,"ERROR: Cannot select curv as target format\n");
  exit(1);
      }
      trgtype = string_to_type(trgtypestring);
      nargsused = 1;
    }

    else if (!strcmp(option, "--frame")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%d",&framesave);
      nargsused = 1;
    }
    else if (!strcmp(option, "--cavtx")){
      /* command-line vertex -- for debugging */
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%d",&cavtx);
      nargsused = 1;
    }
    else if (!strcmp(option, "--hemi")){
      if(nargc < 1) argnerr(option,1);
      hemi = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--mapmethod")){
      if(nargc < 1) argnerr(option,1);
      mapmethod = pargv[0];
      if(strcmp(mapmethod,"nnfr") && strcmp(mapmethod,"nnf")){
  fprintf(stderr,"ERROR: mapmethod must be nnfr or nnf\n");
  exit(1);
      }
      nargsused = 1;
    }
    else if (!strcmp(option, "--srchits")){
      if(nargc < 1) argnerr(option,1);
      SrcHitFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--srcdist")){
      if(nargc < 1) argnerr(option,1);
      SrcDistFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--trghits")){
      if(nargc < 1) argnerr(option,1);
      TrgHitFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--trgdist")){
      if(nargc < 1) argnerr(option,1);
      TrgDistFile = pargv[0];
      nargsused = 1;
    }
    else{
      fprintf(stderr,"ERROR: Option %s unknown\n",option);
      if(singledash(option))
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
  fprintf(stderr, "USAGE: %s \n",Progname) ;
  fprintf(stderr, "\n");
  fprintf(stderr, "   --srcsubject source subject\n");
  fprintf(stderr, "   --srcsurfval path of file with input values \n");
  fprintf(stderr, "   --src_type   source format\n");
  fprintf(stderr, "   --trgsubject target subject\n");
  fprintf(stderr, "   --trgicoorder when trgsubject=ico\n");
  fprintf(stderr, "   --trgsurfval path of file in which to store output values\n");
  fprintf(stderr, "   --trg_type   target format\n");
  fprintf(stderr, "   --hemi       hemisphere (lh or rh) \n");
  fprintf(stderr, "   --surfreg    surface registration (sphere.reg)  \n");
  fprintf(stderr, "   --mapmethod  nnfr or nnf\n");
  fprintf(stderr, "   --frame      save only nth frame (with --trg_type paint)\n");
  fprintf(stderr, "   --nsmooth    number of smoothing steps\n");  
  fprintf(stderr, "   --noreshape  do not reshape output to multiple 'slices'\n");  

  fprintf(stderr, "\n");
  printf("%s\n", vcid) ;
  printf("\n");

}
/* --------------------------------------------- */
static void print_help(void)
{
  print_usage() ;
  printf(

"This program will resample one surface onto another. The source and \n"
"target subjects can be any subject in $SUBJECTS_DIR and/or the  \n"
"icosahedron (ico). The source and target file formats can be anything \n"
"supported by mri_convert. The source format can also be a curvature \n"
"file or a paint (.w) file. The user also has the option of smoothing \n"
"on the surface. \n"
"\n"
"OPTIONS\n"
"\n"
"  --srcsubject subjectname\n"
"\n"
"    Name of source subject as found in $SUBJECTS_DIR or ico for icosahedron.\n"
"    The input data must have been sampled onto this subject's surface (eg, \n"
"    using mri_vol2surf)\n"
"\n"
"  --srcsurfval sourcefile\n"
"\n"
"    Name of file where the data on the source surface is located.\n"
"\n"
"  --src_type typestring\n"
"\n"
"    Format type string. Can be either curv (for FreeSurfer curvature file), \n"
"    paint or w (for FreeSurfer paint files), or anything accepted by \n"
"    mri_convert. If no type string  is given, then the type is determined \n"
"    from the sourcefile (if possible).\n"
"\n"
"  --trgsubject subjectname\n"
"\n"
"    Name of target subject as found in $SUBJECTS_DIR or ico for icosahedron.\n"
"\n"
"  --trgicoorder order\n"
"\n"
"    Icosahedron order number. This specifies the size of the\n"
"    icosahedron according to the following table: \n"
"              Order  Number of Vertices\n"
"                0              12 \n"
"                1              42 \n"
"                2             162 \n"
"                3             642 \n"
"                4            2562 \n"
"                5           10242 \n"
"                6           40962 \n"
"                7          163842 \n"
"    In general, it is best to use the largest size available.\n"
"\n"
"  --trgsurfval targetfile\n"
"\n"
"    Name of file where the data on the target surface will be stored.\n"
"    BUG ALERT: for trg_type w or paint, use the full path.\n"
"\n"
"  --trg_type typestring\n"
"\n"
"    Format type string. Can be paint or w (for FreeSurfer paint files) or anything\n"
"    accepted by mri_convert. NOTE: output cannot be stored in curv format\n"
"    If no type string  is given, then the type is determined from the sourcefile\n"
"    (if possible). If using paint or w, see also --frame.\n"
"\n"
"  --hemi hemifield (lh or rh)\n"
"\n"
"  --surfreg registration_surface"
"\n"
"    If the source and target subjects are not the same, this surface is used \n"
"    to register the two surfaces. sphere.reg is used as the default. Don't change\n"
"    this unless you know what you are doing.\n"
"\n"
"  --mapmethod methodname\n"
"\n"
"    Method used to map from the vertices in one subject to those of another.\n"
"    Legal values are: nnfr (neighest-neighbor, forward and reverse) and nnf\n"
"    (neighest-neighbor, forward only). Default is nnfr. The mapping is done\n"
"    in the following way. For each vertex on the target surface, the closest\n"
"    vertex in the source surface is found, based on the distance in the \n"
"    registration space (this is the forward map). If nnf is chosen, then the\n"
"    the value at the target vertex is set to that of the closest source vertex.\n"
"    This, however, can leave some source vertices unrepresented in target (ie,\n"
"    'holes'). If nnfr is chosen, then each hole is assigned to the closest\n"
"    target vertex. If a target vertex has multiple source vertices, then the\n"
"    source values are averaged together. It does not seem to make much difference. \n"
"\n"
"  --nsmooth niterations\n"
"\n"
"    Number of smoothing iterations. Each iteration consists of averaging each\n"
"    vertex with its neighbors. When only smoothing is desired, just set the \n"
"    the source and target subjects to the same subject.\n"
"\n"
"  --frame framenumber\n"
"\n"
"    When using paint/w output format, this specifies which frame to output. This\n"
"    format can store only one frame. The frame number is zero-based (default is 0).\n"
"\n"
"  --noreshape"
"\n"
"    By default, mri_surf2surf will save the output as multiple\n"
"    'slices'; has no effect for paint/w output format. For ico, the output\n"
"    will appear to be a 'volume' with Nv/R colums, 1 row, R slices and Nf \n"
"    frames, where Nv is the number of vertices on the surface. For icosahedrons, \n"
"    R=6. For others, R will be the prime factor of Nv closest to 6. Reshaping \n"
"    is for logistical purposes (eg, in the analyze format the size of a dimension \n"
"    cannot exceed 2^15). Use this flag to prevent this behavior. This has no \n"
"    effect when the output type is paint.\n"
"\n"
"EXAMPLES:\n"
"\n"
"1. Resample a subject's thickness of the left cortical hemisphere on to a \n"
"   7th order icosahedron and save in analyze4d format:\n"
"\n"
"   mri_surf2surf --hemi lh --srcsubject bert \n"
"      --srcsurfval $SUBJECTS_DIR/bert/surf/lh.thickness --src_type curv \n"
"      --trgsubject ico --trgicoorder 7 \n"
"      --trgsurfval bert-thickness-lh.img --trg_type analyze4d \n"
"\n"
"2. Resample data on the icosahedron to the right hemisphere of subject bert.\n"
"   Save in paint so that it can be viewed as an overlay in tksurfer. The \n"
"   source data is stored in bfloat format (ie, icodata_000.bfloat, ...)\n"
"\n"
"   mri_surf2surf --hemi rh --srcsubject ico \n"
"      --srcsurfval icodata-rh --src_type bfloat \n"
"      --trgsubject bert \n"
"      --trgsurfval ./bert-ico-rh.w --trg_type paint \n"
"\n"
"BUG REPORTS: send bugs to analysis-bugs@nmr.mgh.harvard.edu. Make sure \n"
"    to include the version and full command-line and enough information to\n"
"    be able to recreate the problem. Not that anyone does.\n"
"\n"
"\n"
"BUGS:\n"
"\n"
"  When the output format is paint, the output file must be specified with\n"
"  a partial path (eg, ./data-lh.w) or else the output will be written into\n"
"  the subject's anatomical directory.\n"
"\n"
"\n"
"AUTHOR: Douglas N. Greve, Ph.D., MGH-NMR Center (greve@nmr.mgh.harvard.edu)\n"
"\n");

  exit(1) ;
}

/* --------------------------------------------- */
static void dump_options(FILE *fp)
{
  fprintf(fp,"srcsubject = %s\n",srcsubject);
  fprintf(fp,"srcval     = %s\n",srcvalfile);
  fprintf(fp,"srctype    = %s\n",srctypestring);
  fprintf(fp,"trgsubject = %s\n",trgsubject);
  fprintf(fp,"trgval     = %s\n",trgvalfile);
  fprintf(fp,"trgtype    = %s\n",trgtypestring);
  fprintf(fp,"surfreg    = %s\n",surfreg);
  fprintf(fp,"hemi       = %s\n",hemi);
  fprintf(fp,"frame      = %d\n",framesave);
  return;
}
/* --------------------------------------------- */
static void print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}
/* --------------------------------------------- */
static void argnerr(char *option, int n)
{
  if(n==1)
    fprintf(stderr,"ERROR: %s flag needs %d argument\n",option,n);
  else
    fprintf(stderr,"ERROR: %s flag needs %d arguments\n",option,n);
  exit(-1);
}
/* --------------------------------------------- */
static void check_options(void)
{
  if(srcsubject == NULL){
    fprintf(stderr,"ERROR: no source subject specified\n");
    exit(1);
  }
  if(srcvalfile == NULL){
    fprintf(stderr,"A source value path must be supplied\n");
    exit(1);
  }

  if(srctypestring == NULL){
    srctypestring = "bfloat";
    srctype = BFLOAT_FILE;
  }
  if( strcasecmp(srctypestring,"w") != 0 &&
      strcasecmp(srctypestring,"curv") != 0 &&
      strcasecmp(srctypestring,"paint") != 0 ){
    if(srctype == MRI_VOLUME_TYPE_UNKNOWN) {
  srctype = mri_identify(srcvalfile);
  if(srctype == MRI_VOLUME_TYPE_UNKNOWN){
    fprintf(stderr,"ERROR: could not determine type of %s\n",srcvalfile);
    exit(1);
  }
    }
  }

  if(trgsubject == NULL){
    fprintf(stderr,"ERROR: no target subject specified\n");
    exit(1);
  }
  if(trgvalfile == NULL){
    fprintf(stderr,"A target value path must be supplied\n");
    exit(1);
  }

  if(trgtypestring == NULL){
    trgtypestring = "bfloat";
    trgtype = BFLOAT_FILE;
  }
  if( strcasecmp(trgtypestring,"w") != 0 &&
      strcasecmp(trgtypestring,"curv") != 0 &&
      strcasecmp(trgtypestring,"paint") != 0 ){
    if(trgtype == MRI_VOLUME_TYPE_UNKNOWN) {
  trgtype = mri_identify(trgvalfile);
  if(trgtype == MRI_VOLUME_TYPE_UNKNOWN){
    fprintf(stderr,"ERROR: could not determine type of %s\n",trgvalfile);
    exit(1);
  }
    }
  }

  if(hemi == NULL){
    fprintf(stderr,"ERROR: no hemifield specified\n");
    exit(1);
  }

  return;
}

/*---------------------------------------------------------------*/
static int singledash(char *flag)
{
  int len;
  len = strlen(flag);
  if(len < 2) return(0);

  if(flag[0] == '-' && flag[1] != '-') return(1);
  return(0);
}

/*---------------------------------------------------------------*/
int GetNVtxsFromWFile(char *wfile)
{
  FILE *fp;
  int i,ilat, num, nvertices;
  int *vtxnum;
  float *wval;

  fp = fopen(wfile,"r");
  if (fp==NULL) {
    fprintf(stderr,"ERROR: Progname: GetNVtxsFromWFile():\n");
    fprintf(stderr,"Could not open %s\n",wfile);
    fprintf(stderr,"(%s,%d,%s)\n",__FILE__, __LINE__,__DATE__);
    exit(1);
  }
  
  fread2(&ilat,fp);
  fread3(&num,fp);
  vtxnum = (int *)   calloc(sizeof(int),   num);
  wval   = (float *) calloc(sizeof(float), num);

  for (i=0;i<num;i++){
    fread3(&vtxnum[i],fp);
    wval[i] = freadFloat(fp) ;
  }
  fclose(fp);

  nvertices = vtxnum[num-1] + 1;

  free(vtxnum);
  free(wval);

  return(nvertices);
}
//MRI *MRIreadHeader(char *fname, int type);
/*---------------------------------------------------------------*/
int GetNVtxsFromValFile(char *filename, char *typestring)
{
  //int err,nrows, ncols, nslcs, nfrms, endian;
  int nVtxs=0;
  int type;
  MRI *mri;

  printf("GetNVtxs: %s %s\n",filename,typestring);

  if(!strcmp(typestring,"curv")){
    fprintf(stderr,"ERROR: cannot get nvertices from curv format\n");
    exit(1);
  }

  if(!strcmp(typestring,"paint") || !strcmp(typestring,"w")){
    nVtxs = GetNVtxsFromWFile(filename);
    return(nVtxs);
  }

  type = string_to_type(typestring);
  mri = MRIreadHeader(filename, type);
  if(mri == NULL) exit(1);
  
  nVtxs = mri->width*mri->height*mri->depth;

  MRIfree(&mri);

  return(nVtxs);
}
/*---------------------------------------------------------------*/
int GetICOOrderFromValFile(char *filename, char *fmt)
{
  int nIcoVtxs,IcoOrder;

  nIcoVtxs = GetNVtxsFromValFile(filename, fmt);

  IcoOrder = IcoOrderFromNVtxs(nIcoVtxs);
  if(IcoOrder < 0){
    fprintf(stderr,"ERROR: number of vertices = %d, does not mach ico\n",
      nIcoVtxs);
    exit(1);

  }
  
  return(IcoOrder);
}
#if 0
/* --------------------------------------------- */
int check_format(char *trgfmt)
{
  if( strcasecmp(trgfmt,"bvolume") != 0 &&
      strcasecmp(trgfmt,"bfile") != 0 &&
      strcasecmp(trgfmt,"bshort") != 0 &&
      strcasecmp(trgfmt,"bfloat") != 0 &&
      strcasecmp(trgfmt,"w") != 0 &&
      strcasecmp(trgfmt,"curv") != 0 &&
      strcasecmp(trgfmt,"paint") != 0 ){
    fprintf(stderr,"ERROR: format %s unrecoginized\n",trgfmt);
    fprintf(stderr,"Legal values are: bvolume, bfile, bshort, bfloat, and \n");
    fprintf(stderr,"                  paint, w\n");
    exit(1);
  }
  return(0);
}
#endif
