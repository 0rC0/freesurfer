#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>


#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "mrisurf.h"
#include "macros.h"
#include "fio.h"

static char vcid[] = "$Id: mris_convert.c,v 1.4 1998/07/02 22:57:18 fischl Exp $";


/*-------------------------------- CONSTANTS -----------------------------*/

/*-------------------------------- PROTOTYPES ----------------------------*/

int main(int argc, char *argv[]) ;

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;
static int convertWFile(char *in_fname, char *out_fname) ;

/*-------------------------------- DATA ----------------------------*/

char *Progname ;

static int talairach_flag = 0 ;
static int patch_flag = 0 ;
static int read_orig_positions = 0 ;
static int w_file_flag = 0 ;

/*-------------------------------- FUNCTIONS ----------------------------*/

int
main(int argc, char *argv[])
{
  MRI_SURFACE  *mris ;
  char         **av, *in_fname, *out_fname, fname[100], hemi[10],
               *cp, path[100], *dot, ext[100] ;
  int          ac, nargs ;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;


  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 2)
    usage_exit() ;

  in_fname = argv[1] ;
  out_fname = argv[2] ;

  dot = strrchr(out_fname, '.') ;
  if (dot)
  {
    strcpy(ext, dot+1) ;
    if (!stricmp(ext, "W")) 
      w_file_flag = 1 ;
  }

  if (w_file_flag)
  {
    convertWFile(in_fname, out_fname) ;
    exit(0) ;
  }

  if (patch_flag)   /* read in orig surface before reading in patch */
  {
    FileNamePath(in_fname, path) ;
    cp = strrchr(in_fname, '/') ;
    if (cp)
    {
      cp = strchr(cp, '.') ;
      if (cp)
      {
        strncpy(hemi, cp-2, 2) ;
        hemi[2] = 0 ;
      }
      else
        strcpy(hemi, "lh") ;
    }
    else
      strcpy(hemi, "lh") ;
    sprintf(fname, "%s/%s.orig", path, hemi) ;
    mris = MRISread(fname) ;
    if (!mris)
      ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
                Progname, fname) ;
    if (MRISreadPatch(mris, in_fname) != NO_ERROR)
      ErrorExit(ERROR_NOFILE, "%s: could not read patch file %s",
                Progname, in_fname) ;
    if (read_orig_positions)
    {
      if (MRISreadVertexPositions(mris, "smoothwm") != NO_ERROR)
        ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
                  Progname, fname) ;
    }
  }
  else
  {
    mris = MRISread(in_fname) ;
    if (!mris)
      ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
                Progname, in_fname) ;
  }

  if (mris->patch)
    MRISwritePatch(mris, out_fname) ;
  else
    MRISwrite(mris, out_fname) ;

  MRISfree(&mris) ;

  exit(0) ;
  return(0) ;  /* for ansi */
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int
get_option(int argc, char *argv[])
{
  int  nargs = 0 ;
  char *option ;
  
  option = argv[1] + 1 ;            /* past '-' */
  if (!stricmp(option, "-help"))
    print_help() ;
  else if (!stricmp(option, "-version"))
    print_version() ;
  else switch (toupper(*option))
  {
  case 'O':
    read_orig_positions = 1 ;
    break ;
  case 'P':
    patch_flag = 1 ;
    nargs = 0 ;
    break ;
  case 'T':
    talairach_flag = 1 ;
    break ;
  case '?':
  case 'U':
    print_usage() ;
    exit(1) ;
    break ;
  default:
    fprintf(stderr, "unknown option %s\n", argv[1]) ;
    exit(1) ;
    break ;
  }

  return(nargs) ;
}

static void
usage_exit(void)
{
  print_usage() ;
  exit(1) ;
}

static void
print_usage(void)
{
  fprintf(stderr, 
          "usage: %s [options] <input surface file> <output surface file>\n",
          Progname) ;
}

static void
print_help(void)
{
  print_usage() ;
  fprintf(stderr, 
          "\nThis program will convert an MRI surface to ascii.\n") ;
  fprintf(stderr, "\nvalid options are:\n\n") ;
  exit(1) ;
}

static void
print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}
static int
convertWFile(char *in_fname, char *out_fname)
{
  FILE   *infp, *outfp ;
  char   line[300], *cp ;
  int    vno, l = 0, num ;
  float  val ;

  fprintf(stderr, "writing w file %s...\n", out_fname) ;
  outfp = fopen(out_fname,"wb");
  if (outfp==NULL) 
    ErrorExit(ERROR_NOFILE, "%s: Can't create file %s\n",Progname,out_fname) ;

  infp = fopen(in_fname,"rb");
  if (infp==NULL) 
    ErrorExit(ERROR_NOFILE, "%s: Can't create file %s\n",Progname,in_fname) ;


  cp = fgetl(line, 299, infp) ;
  num = atoi(cp) ;
  fwrite2(0,outfp);
  fwrite3(num,outfp);

  while ((cp = fgetl(line, 299, infp)) != NULL)
  {
    l++ ;
    if (sscanf(cp, "%d %f", &vno, &val) != 2)
    {
      ErrorPrintf(ERROR_BADFILE,"%s: could not scan parms from line %d: %s.\n",
                Progname, l, cp) ;
      val = 0.0f ;   /* don't know what it is... */
    }
    fwrite3(vno,outfp);
    fwriteFloat(val, outfp) ;
  }
  fclose(infp) ;
  fclose(outfp) ;
  return(NO_ERROR) ;
}

