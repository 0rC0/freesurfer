/*----------------------------------------------------------
  Name: mri_annotation2label.c
  $Id: mri_annotation2label.c,v 1.1 2001/05/08 21:52:35 greve Exp $
  Author: Douglas Greve
  Purpose: Converts an annotation to a labels.

  User specifies the subject, hemisphere, label base, and 
  (optionally) the annotation base. By default, the annotation
  base is aparc. The program will get the annotations from
  SUBJECTS_DIR/subject/label/hemi_annotbase.annot. A separate
  label file is created for each annotation index; the name of
  the file conforms to labelbase-XXX.label, where XXX is the
  zero-padded 3 digit annotation index. If labelbase includes
  a directory path, that directory must already exist. If there
  are no points in the annotation file for a particular index,
  no label file is created.

  Example:

  mri_annotation2label --subject LW --hemi rh --labelbase ./labels/aparc-rh

  This will get annotations from $SUBJECTS_DIR/LW/label/rh_aparc.annot
  and then create about 94 label files: aparc-rh-001.label, 
  aparc-rh-002.label, ... Note that the directory labels must already
  exist. 

  The human readable names that correspond to the annotation index can
  be found in:

     $MRI_DIR/christophe_parc.txt

  Testing:
  1. Load annotations in tksurfer:  
       tksurfer -LW lh inflated
       % read_annotations lh_aparc
     When a point is clicked on, it prints out a lot of info, including
       annot = S_temporalis_sup (93, 3988701) (221, 220, 60)
     This indicates that annotion number 93 was hit. Save this point.
   
  2. Load label into tksurfer (new job):
       tksurfer -LW lh inflated
       [edit label field and read]
     Verify that label pattern looks like that from the annotation

  3. Load label into tkmedit
       tkmedit LW T1
       [Load the label]
       [Goto the point saved from step 1]
  -----------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <string.h>

#include "MRIio.h"
#include "error.h"
#include "diag.h"
#include "mrisurf.h"
#include "label.h"
#include "annotation.h"

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void argnerr(char *option, int n);
static void dump_options(FILE *fp);
static int  singledash(char *flag);

int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_annotation2label.c,v 1.1 2001/05/08 21:52:35 greve Exp $";
char *Progname = NULL;

char  *subject   = NULL;
char  *annotation = "aparc";
char  *hemi       = NULL;
char  *labelbase = NULL;

MRI_SURFACE *Surf;
LABEL *label     = NULL;

int debug = 0;

char *SUBJECTS_DIR = NULL;
FILE *fp;

char tmpstr[2000];
char annotfile[1000];
char labelfile[1000];
int  nperannot[1000];

/*-------------------------------------------------*/
/*-------------------------------------------------*/
/*-------------------------------------------------*/
int main(int argc, char **argv)
{
  VERTEX *vtx;
  int nthpoint,err,vtxno,ano,ani,vtxani,animax;

  printf("\n");

  Progname = argv[0] ;
  argc --;
  argv++;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  if(argc == 0) usage_exit();

  parse_commandline(argc, argv);
  check_options();
  dump_options(stdout);

  /*--- Get environment variables ------*/
  SUBJECTS_DIR = getenv("SUBJECTS_DIR");
  if(SUBJECTS_DIR==NULL){
    fprintf(stderr,"ERROR: SUBJECTS_DIR not defined in environment\n");
    exit(1);
  }

  /* ------ Load subject's surface ------ */
  sprintf(tmpstr,"%s/%s/surf/%s.%s",SUBJECTS_DIR,subject,hemi,"white"); 
  printf("Reading surface \n %s\n",tmpstr);
  Surf = MRISread(tmpstr);
  if(Surf == NULL){
    fprintf(stderr,"ERROR: could not read %s\n",tmpstr);
    exit(1);
  }

  /* ------ Load annotation ------ */
  sprintf(annotfile,"%s/%s/label/%s_%s.annot",
	  SUBJECTS_DIR,subject,hemi,annotation);
  printf("Loading annotations from %s\n",annotfile);
  err = MRISreadAnnotation(Surf, annotfile);
  if(err){
    fprintf(stderr,"ERROR: MRISreadAnnotation() failed\n");
    exit(1);
  }

  /* Loop through each vertex */
  animax = -1;
  for(vtxno = 0; vtxno < Surf->nvertices; vtxno++){
    vtx = &(Surf->vertices[vtxno]);
    ano = Surf->vertices[vtxno].annotation;
    vtxani = annotation_to_index(ano);
    nperannot[vtxani] ++;    
    if(animax < vtxani) animax = vtxani;
  }

  printf("animax = %d\n",animax);
  for(ani=0; ani < animax; ani++){

    if(nperannot[ani] == 0){
      printf("%3d  %5d   --- skipping \n",ani,nperannot[ani]);
      continue;
    }
    printf("%3d  %5d\n",ani,nperannot[ani]);

    sprintf(labelfile,"%s-%03d.label",labelbase,ani);
    label = LabelAlloc(nperannot[ani],subject,labelfile);
    label->n_points = nperannot[ani];

    nthpoint = 0;
    for(vtxno = 0; vtxno < Surf->nvertices; vtxno++){
      vtx = &(Surf->vertices[vtxno]);
      ano = Surf->vertices[vtxno].annotation;
      vtxani = annotation_to_index(ano);
      if(vtxani == ani){
	label->lv[nthpoint].vno = vtxno;
	label->lv[nthpoint].x = vtx->x;
	label->lv[nthpoint].y = vtx->y;
	label->lv[nthpoint].z = vtx->z;
	nthpoint ++;
      }
    }

    LabelWrite(label,labelfile);
    LabelFree(&label);
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

    if (!strcmp(option, "--help"))  print_help() ;

    else if (!strcmp(option, "--version")) print_version() ;

    else if (!strcmp(option, "--debug"))   debug = 1;

    /* -------- source inputs ------ */
    else if (!strcmp(option, "--subject")){
      if(nargc < 1) argnerr(option,1);
      subject = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--labelbase")){
      if(nargc < 1) argnerr(option,1);
      labelbase = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--annotation")){
      if(nargc < 1) argnerr(option,1);
      annotation = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--hemi")){
      if(nargc < 1) argnerr(option,1);
      hemi = pargv[0];
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
  fprintf(stderr, "   --subject    source subject\n");
  fprintf(stderr, "   --hemi       hemisphere (lh or rh) (with surface)\n");
  fprintf(stderr, "   --labelbase  output will be base-XXX.label \n");
  fprintf(stderr, "   --annotation as found in SUBJDIR/labels <aparc>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "   --help       display help\n");  
  fprintf(stderr, "   --version    display version\n");  
  fprintf(stderr, "\n");
}
/* --------------------------------------------- */
static void dump_options(FILE *fp)
{
  fprintf(fp,"subject = %s\n",subject);
  fprintf(fp,"annotation = %s\n",annotation);
  fprintf(fp,"hemi = %s\n",hemi);
  fprintf(fp,"labelbase = %s\n",  labelbase);
  fprintf(fp,"\n");
  return;
}
/* --------------------------------------------- */
static void print_help(void)
{
  print_usage() ;
  printf("This program will convert an annotation into multiple label files.\n");

  printf(" 
  User specifies the subject, hemisphere, label base, and 
  (optionally) the annotation base. By default, the annotation
  base is aparc. The program will get the annotations from
  SUBJECTS_DIR/subject/label/hemi_annotbase.annot. A separate
  label file is created for each annotation index; the name of
  the file conforms to labelbase-XXX.label, where XXX is the
  zero-padded 3 digit annotation index. If labelbase includes
  a directory path, that directory must already exist. If there
  are no points in the annotation file for a particular index,
  no label file is created. The human-readable names that correspond 
  to the annotation indices for aparc can be found in
  $MRI_DIR/christophe_parc.txt.  This annotation key also appears 
  at the end of this help.

  Example:

  mri_annotation2label --subject LW --hemi rh --labelbase ./labels/aparc-rh

  This will get annotations from $SUBJECTS_DIR/LW/label/rh_aparc.annot
  and then create about 94 label files: aparc-rh-001.label, 
  aparc-rh-002.label, ... Note that the directory labels must already
  exist. 

  Testing:
  1. Load annotations in tksurfer:  
       tksurfer -LW lh inflated
       % read_annotations lh_aparc
     When a point is clicked on, it prints out a lot of info, including
       annot = S_temporalis_sup (93, 3988701) (221, 220, 60)
     This indicates that annotion number 93 was hit. Save this point.
   
  2. Load label into tksurfer (new job):
       tksurfer -LW lh inflated
       [edit label field and read]
     Verify that label pattern looks like that from the annotation

  3. Load label into tkmedit
       tkmedit LW T1
       [Load the label]
       [Goto the point saved from step 1] \n\n");

  printf("Annotation Key\n");
  print_annotation_table(stdout);

  exit(1) ;
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
  if(subject == NULL){
    fprintf(stderr,"ERROR: no source subject specified\n");
    exit(1);
  }
  if(labelbase == NULL){
    fprintf(stderr,"ERROR: No label base specified\n");
    exit(1);
  }
  if(hemi == NULL){
    fprintf(stderr,"ERROR: No hemisphere specified\n");
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
