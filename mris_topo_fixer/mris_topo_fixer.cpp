#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

extern "C" {
#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "mrisurf.h"
#include "mri.h"
#include "macros.h"
#include "version.h"
#include "timer.h"
}
#include "mris_topology.h"
#include "topology/patchdisk.h"

int main(int argc, char *argv[]) ;

static int noint = 1 ;

static int  get_option(int argc, char *argv[]) ;
static void initTopoFixerParameters();
static void freeTopoFixerParameters();

char *Progname ;


static char *brain_name    = "brain" ;
static char *wm_name       = "wm" ;
static char *orig_name     = "orig" ;
static char *defect_name  = "defects" ;

static char sdir[STRLEN] = "" ;
static TOPOFIX_PARMS parms;
static int MGZ = 0; // set to 1 for MGZ

static double pct_over = 1.1;

static void initTopoFixerParameters(){
	parms.verbose = 1; //minimal mode
	parms.smooth = 0;    //smoothing
	parms.match = 1;     //matching using local intensity estimates
	parms.nminattempts = 10;
	parms.l_mri = 1.0;
  parms.l_curv = 1.0 ;
  parms.l_qcurv = 1.0;
  parms.l_unmri = 0.0; //not implemented yet
	parms.nattempts_percent=0.1;
  parms.minimal_loop_percent = 0.4;
	parms.no_self_intersections = 1;

	//creation of the patching surfaces
	PatchDisk *disk = new PatchDisk[4];
  for(int n = 0 ; n < 4 ; n++)
    disk[n].Create(n);
	parms.patchdisk = (void*)disk;

	//mri
  parms.mri = NULL;
	parms.mri_wm = NULL; 
  parms.mri_gray_white = NULL;
  parms.mri_k1_k2 = NULL;
	//histo
  parms.h_k1 = NULL;
	parms.h_k2 = NULL;
	parms.h_gray = NULL;
	parms.h_white = NULL;
	parms.h_dot = NULL;
	parms.h_border = NULL;
	parms.h_grad = NULL;;
	parms.transformation_matrix=NULL;
}

static void freeTopoFixerParameters(){
	PatchDisk *disk = (PatchDisk*)parms.patchdisk;
	delete [] disk;
	//mri
	if(parms.mri) MRIfree(&parms.mri);
	if(parms.mri_wm) MRIfree(&parms.mri_wm);
	if(parms.mri_gray_white) MRIfree(&parms.mri_gray_white);
	if(parms.mri_k1_k2) MRIfree(&parms.mri_k1_k2);
	//histo
	if(parms.h_k1) HISTOfree(&parms.h_k1);
  if(parms.h_k2) HISTOfree(&parms.h_k2);
  if(parms.h_gray) HISTOfree(&parms.h_gray);
  if(parms.h_white) HISTOfree(&parms.h_white);
  if(parms.h_dot) HISTOfree(&parms.h_dot);
  if(parms.h_border) HISTOfree(&parms.h_border);
  if(parms.h_grad) HISTOfree(&parms.h_grad);
	if(parms.transformation_matrix)  MatrixFree(&parms.transformation_matrix);
} 


int main(int argc, char *argv[])
{

	char          **av, *hemi, *sname, *cp, fname[STRLEN] ;
  int           ac, nargs ;
  MRI_SURFACE   *mris, *mris_corrected ;
	// MRI           *mri, *mri_wm ;
  int           msec, nvert, nfaces, nedges, eno ;
  struct timeb  then ;

	char cmdline[CMD_LINE_LEN] ;

  make_cmd_version_string
    (argc,
     argv,
     "$Id: mris_topo_fixer.cpp,v 1.1 2006/04/21 18:31:53 segonne Exp $",
     "$Name:  $",
     cmdline);

  /* rkt: check for and handle version tag */
  nargs =
    handle_version_option
    (argc,
     argv,
		 "$Id: mris_topo_fixer.cpp,v 1.1 2006/04/21 18:31:53 segonne Exp $",
     "$Name:  $");

	if (nargs && argc - nargs == 1)
    exit (0);
  argc -= nargs;

	
  //parameters for mris_topo_fixer
	initTopoFixerParameters();

	Progname = argv[0] ;
	ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;
	ac = argc ;
  av = argv ;
	for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++){
   nargs = get_option(argc, argv) ;
   argc -= nargs ;
   argv += nargs ;
	}

	/*if(argc < 2)  usage_exit() ;

  print_parameters();

  printf("%s\n",vcid);
  printf("  %s\n",MRISurfSrcVersion());
  fflush(stdout); */

	TimerStart(&then) ;
  sname = argv[1] ;
  hemi = argv[2] ;
  if(strlen(sdir) == 0){
    cp = getenv("SUBJECTS_DIR") ;
    if (!cp)
      ErrorExit(ERROR_BADPARM,
								"%s: SUBJECTS_DIR not defined in environment.\n", Progname) ;
    strcpy(sdir, cp) ;
  }

	sprintf(fname, "%s/%s/surf/%s.%s", sdir, sname, hemi,orig_name) ;
  printf("reading input surface %s...\n", fname) ;
  mris = MRISreadOverAlloc(fname,pct_over) ;
  if (!mris)
    ErrorExit(ERROR_NOFILE, "%s: could not read input surface %s",
              Progname, fname) ;
  MRISaddCommandLine(mris, cmdline) ;
  strcpy(mris->subject_name, sname) ;

	eno = MRIScomputeEulerNumber(mris, &nvert, &nfaces, &nedges) ;
  fprintf(stderr, "Before topology correction, eno=%d (nv=%d, nf=%d, ne=%d,"
					" g=%d)\n", eno, nvert, nfaces, nedges, (2-eno)/2) ;
  if(eno == 2 ) {
    fprintf(stderr,"The Euler Number of this surface is 2"
            "\nNothing to do\nProgram exiting\n");
    exit(0);
  }

	if(parms.no_self_intersections){
		int self_intersect =  IsMRISselfIntersecting(mris);
		if(self_intersect){
			fprintf(stderr,"\nThe original surface self-intersects!!!\n");
			fprintf(stderr,"\nAbort !!!\n");
			MRISfree(&mris);
			exit(-1);
		}else
			fprintf(stderr,"\nThe original surface does not self-intersect\n");
	}

	//MRISmarkOrientationChanges(mris);


	//number of loops 
	int nloops = (1-eno)/2;
	fprintf(stderr,"The surface has %d loops (X=%d)\n\n",nloops,eno);
	
	//read the topological defects 
  sprintf(fname, "%s/%s/surf/%s.%s", sdir, sname, hemi, defect_name) ;
	MRISreadCurvature(mris,fname);
	for(int n = 0 ; n < mris->nvertices ; n++)
    mris->vertices[n].marked2 = int(mris->vertices[n].curv);

	//read the mri volume
	sprintf(fname, "%s/%s/mri/%s", sdir, sname, brain_name) ;
  if(MGZ) sprintf(fname, "%s.mgz", fname);
  printf("reading brain volume from %s...\n", brain_name) ;
  parms.mri = MRIread(fname) ;
  if (!parms.mri)
    ErrorExit(ERROR_NOFILE,
              "%s: could not read brain volume from %s", Progname, fname) ;

  sprintf(fname, "%s/%s/mri/%s", sdir, sname, wm_name) ;
  if(MGZ) sprintf(fname, "%s.mgz", fname);
  printf("reading wm segmentation from %s...\n", wm_name) ;
  parms.mri_wm = MRIread(fname) ;
  if (!parms.mri_wm)
    ErrorExit(ERROR_NOFILE,
              "%s: could not read wm volume from %s", Progname, fname) ;

	mris_corrected=MRISduplicateOver(mris);

	//mris becomes useless!
	MRISfree(&mris) ;

	MRISinitTopoFixParameters(mris_corrected,&parms); 


	int def = atoi(argv[3]);
	if(def<0){
		int n = 1 ;
		while(MRISgetEuler(mris_corrected)<2){
			MRIScorrectDefect(mris_corrected,n++,parms);
			if(n>1000) break;
		}
	}else
		MRIScorrectDefect(mris_corrected,def,parms);

	//checking if we have some self-intersections (should not happen) 
	if(parms.no_self_intersections){
    int self_intersect =  IsMRISselfIntersecting(mris_corrected);
    if(self_intersect){
      fprintf(stderr,"\nThe final surface self-intersects !\n");
			if (noint) MRISremoveIntersections(mris_corrected) ;
    }else
			fprintf(stderr,"\nThe final surface does not self-intersect !\n");
  }

	eno = MRIScomputeEulerNumber(mris_corrected, &nvert, &nfaces, &nedges) ;
  fprintf(stderr, "after topology correction, eno=%d (nv=%d, nf=%d, ne=%d,"
          " g=%d)\n", eno, nvert, nfaces, nedges, (2-eno)/2) ;

	/* compute the orientation changes */
	//  MRISmarkOrientationChanges(mris_corrected);

	MRISwrite(mris_corrected,argv[4]);
	
	fprintf(stderr,"\n\n");

	msec = TimerStop(&then) ;
  fprintf(stderr,"topology fixing took %2.1f minutes\n",
          (float)msec/(60*1000.0f));

	freeTopoFixerParameters();

  return(0) ;  /* for ansi */
}


static int
get_option(int argc, char *argv[])
{
  int  nargs = 0 ;
  char *option ;

  option = argv[1] + 1 ;            /* past '-' */
  if (!stricmp(option, "-help"))
    exit(1);//print_help() ;
  else if (!stricmp(option, "mgz")){
    printf("INFO: assuming .mgz format\n");
    MGZ = 1;
  }else if (!stricmp(option , "verbose")){
		Gdiag = DIAG_VERBOSE;
	}
	else if (!stricmp(option, "verbose_low"))
    {
      parms.verbose=VERBOSE_MODE_LOW;
      fprintf(stderr,"verbose mode on (default+low mode)\n");
      nargs = 0 ;
    }
  else if (!stricmp(option, "warnings"))
    {
      parms.verbose=VERBOSE_MODE_MEDIUM;
      fprintf(stderr,"verbose mode on (medium mode): printing warnings\n");
      nargs = 0 ;
    }
  else if (!stricmp(option, "errors"))
    {
      parms.verbose=VERBOSE_MODE_HIGH;
      fprintf(stderr,"verbose mode on (high mode): "
              "exiting when warnings appear\n");
      nargs = 0 ;
    }
	else if (!stricmp(option, "mri"))
    {
      parms.l_mri = atof(argv[2]) ;
      fprintf(stderr,"setting l_mri = %2.2f\n", parms.l_mri) ;
      nargs = 1 ;
    }
  else if (!stricmp(option, "curv"))
    {
      parms.l_curv = atof(argv[2]) ;
      fprintf(stderr,"setting l_curv = %2.2f\n", parms.l_curv) ;
      nargs = 1 ;
    }
  else if (!stricmp(option, "qcurv"))
    {
      parms.l_qcurv = atof(argv[2]) ;
      fprintf(stderr,"setting l_qcurv = %2.2f\n", parms.l_qcurv) ;
      nargs = 1 ;
    }
  else if (!stricmp(option, "unmri"))
    {
      parms.l_unmri = atof(argv[2]) ;
			parms.l_unmri = 0.0;
			fprintf(stderr,"setting l_unmri = %2.2f\n", parms.l_unmri) ;
			//      if(parms.volume_resolution<1)
			//parms.volume_resolution = 2;
      nargs = 1 ;
    }
	else if (!stricmp(option, "pct"))
    {
      parms.nattempts_percent = atof(argv[2]) ;
      fprintf(stderr,"setting pct of attempts = %2.2f\n", parms.nattempts_percent) ;
      nargs = 1 ;
    }
	else if (!stricmp(option, "nmin"))
    {
      parms.nminattempts = __MAX(1,atoi(argv[2])) ;
      fprintf(stderr,"setting minimal number of attempts = %2.2f\n", parms.nminattempts) ;
      nargs = 1 ;
    }
	else if (!stricmp(option, "int"))
    {
      noint = 0 ;
      nargs = 0 ;
    }
	else if (!stricmp(option, "no_intersection"))
    {
      parms.no_self_intersections = 1;
      fprintf(stderr,"avoiding self-intersecting patches\n") ;
    }
	else if (!stricmp(option, "minimal"))
    {
			parms.nminattempts = 1;
			parms.nattempts_percent=0.0f;
			fprintf(stderr,"cuting minimal loop only\n") ;
		}
	else if (!stricmp(option, "loop_pct"))
    {
      parms.minimal_loop_percent = atof(argv[2]) ;
      fprintf(stderr,"setting loop_pct = %2.2f\n", parms.minimal_loop_percent) ;
      nargs = 1 ;
    }
	else if (!stricmp(option, "smooth"))
    {
      parms.smooth = atoi(argv[2]) ;
      fprintf(stderr,"smoothing defect with mode %d\n", parms.smooth) ;
      nargs = 1 ;
    }
	else if (!stricmp(option, "match"))
    {
			if(atoi(argv[2])) parms.match = 1 ;
			else  parms.match = 0;
      fprintf(stderr,"matching mode : %d \n", parms.match) ;
      nargs = 1 ;
    }
	else switch (toupper(*option))
    {
		case '?':
    case 'U':
			//      print_usage() ;
      exit(1) ;
      break ;
		case 'V':
			Gdiag_no = atoi(argv[2]) ;
			nargs = 1 ;
			break ;
		}

	return(nargs) ;
}
