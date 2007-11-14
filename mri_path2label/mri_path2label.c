/**
 * @file  mri_path2label.c
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: greve $
 *    $Date: 2007/11/14 04:22:02 $
 *    $Revision: 1.14 $
 *
 * Copyright (C) 2002-2007,
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


/**
 * @file   mri_path2label.c
 * @author Kevin Teich
 * @date   $Date: 2007/11/14 04:22:02 $
 *
 * @brief  Converts scuba's path file format to a label file.
 *
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "version.h"
#include "label.h"
#include "path.h"
#include "error.h"
#include "mrisutils.h"
#include "cmdargs.h"
#include "diag.h"
#include "error.h"

#ifdef Darwin
#include "getline.h"
#endif
#ifdef SunOS
#include "getline.h"
#endif

char *Progname = NULL;

static void print_usage (void) ;
static void print_help (void) ;
static int  guess_file_type (char* fname, int* is_path, int *is_label);
static int  convert_path_to_label (char* fname, char* ofname);
static int  convert_label_to_path (char* fname, char* ofname);
static int  convert_single_path_to_label (char* fname, char* ofname);
static int  convert_single_label_to_path (char* fname, char* ofname);
static int connect_single_path(char* fname, char* ofname, char *subject, char *hemi) ;
static int fill_single_path(char* fname, char* ofname, char *subject, char *hemi, int seed) ;

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void usage_exit(void);
static void print_version(void) ;
static void dump_options(FILE *fp);

static char vcid[] = "$Id: mri_path2label.c,v 1.14 2007/11/14 04:22:02 greve Exp $";

char* source_file          = NULL;
char* dest_file            = NULL;
int   path_to_label  = 0;
int   label_to_path  = 0;
int   single_path          = 0;
int connect = 0;
int fill = 0, fillseed = -1;
char *subject=NULL, *hemi=NULL;
int debug=0;
int checkoptsonly=0;
int MRISfill(MRIS *mris, int seedvtxno);

/*-------------------------------------------------------------*/
int main(int argc, char *argv[]) 
{
  int   nargs;
  int   source_is_label      = 0;
  int   source_is_path       = 0;
  int   err                  = 0;
  FILE* fp                   = NULL;

  nargs = handle_version_option (argc, argv, 
      "$Id: mri_path2label.c,v 1.14 2007/11/14 04:22:02 greve Exp $", "$Name:  $");
  if(nargs && argc - nargs == 1) exit (0);
  argc -= nargs;

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

  if(!label_to_path && !path_to_label) {
    err = guess_file_type (source_file, &source_is_path, &source_is_label);
    if (err) {
      printf ("ERROR: Couldn't determing source file type.\n");
      exit (1);
    }

    if(source_is_path)  path_to_label = 1;
    if(source_is_label) label_to_path = 1;
  }

  fp = fopen (dest_file, "w");
  if (NULL == fp) {
    printf ("ERROR: Couldn't open dest file for writing.\n");
    exit (1);
  }
  fclose (fp);

  if(connect){
    // Still under construction
    printf("Connecting vertices in path\n");
    connect_single_path(source_file, dest_file, subject, hemi) ;
    exit(0);
  }
  if(fill){
    // Still under construction
    printf("Filling vertices in path\n");
    fill_single_path(source_file, dest_file, subject, hemi, fillseed) ;
    exit(0);
  }


  printf ("INFO: Converting %s\n", source_file);
  printf ("INFO:         to %s\n", dest_file);
  if(path_to_label) printf ("INFO: Path to label\n");
  else              printf ("INFO: Label to path\n");
  printf ("\n");

  if (single_path) printf ("INFO: Converting a single path\n");

  if (single_path) {
    if(path_to_label) convert_single_path_to_label(source_file, dest_file);
    else if (label_to_path) convert_single_label_to_path (source_file, dest_file);
  } else {
    if (path_to_label)      convert_path_to_label (source_file, dest_file);
    else if (label_to_path) convert_label_to_path (source_file, dest_file);
  }

  return 0;
}
/*--------------------------------------------------------------------*/
/*------ end main ------>>>>*<<<<<<-----------------------------------*/
/*--------------------------------------------------------------------*/
/*-----------------------------------------------------------*/
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
    else if (!strcasecmp(option, "--single")) single_path = 1;
    else if (!strcasecmp(option, "--label2path")) label_to_path = 1;
    else if (!strcasecmp(option, "--path2label")) path_to_label = 1;

    else if (!strcasecmp(option, "--connect")){
      if(nargc < 2) CMDargNErr(option,2);
      connect = 1;
      subject = pargv[0];
      hemi    = pargv[1];
      nargsused = 2;
    } 
    else if (!strcasecmp(option, "--fill")){
      if(nargc < 2) CMDargNErr(option,3);
      fill = 1;
      subject = pargv[0];
      hemi    = pargv[1];
      sscanf(pargv[2],"%d",&fillseed);
      nargsused = 3;
    } 
    else if (!strcasecmp(option, "--i")){
      if(nargc < 1) CMDargNErr(option,1);
      source_file = pargv[0];
      nargsused = 1;
    } 
    else if (!strcasecmp(option, "--o")){
      if(nargc < 1) CMDargNErr(option,1);
      dest_file = pargv[0];
      nargsused = 1;
    } 
    else {
      if(source_file == NULL) source_file = option;
      else if(dest_file == NULL) dest_file = option;
      else {
	fprintf(stderr,"ERROR: Option %s unknown\n",option);
	if(CMDsingleDash(option))
	  fprintf(stderr,"       Did you really mean -%s ?\n",option);
	exit(-1);
      }
    }
    nargc -= nargsused;
    pargv += nargsused;
  }
  return(0);
}
/*-----------------------------------------------------------*/
static void usage_exit(void) {
  print_usage() ;
  exit(1) ;
}
/*-----------------------------------------------------------*/
static void print_version(void) {
  printf("%s\n", vcid) ;
  exit(1) ;
}
/*-----------------------------------------------------------*/
static void check_options(void) 
{
  return;
}
/*-----------------------------------------------------------*/
static void dump_options(FILE *fp) 
{
  return;
}

/*--------------------------------------------------------------------*/
static void print_usage(void) {
  printf("USAGE: %s [options] input output\n", Progname);
  printf("\n");
  printf("   --single : only convert a single path, and don't use sentinel values\n");
  printf("   --path2label : will treat input as a path and output a label\n");
  printf("   --label2path : will treat input as a label and output a path\n");
  printf("   --connect subject hemi : connect path (input and output must be paths)\n");
  printf("   --fill subject hemi seedvtx : fill closed, connected path\n");
  printf("      input must be a path, output must be a label\n");
  printf("\n");
}

/*--------------------------------------------------------------------*/
static void print_help(void) {
  printf (
    "  Purpose: Converts a path file to a label or a label file to a path\n"
    "  file. Attempts to guess the correct format by looking for\n"
    "  .path and .label suffixes and by looking at the first line in\n"
    "  the file. Use one of the options to override this behavior.\n"
    "  Will return an error if it cannot guess a format and none is\n"
    "  explicitly supplied.\n"
    "\n"
    "  Multiple paths will be encoded in a label file separated by a\n"
    "  line with all columns -99999. This is a sentinel dividing lists\n"
    "  of path points from each other. If the option --single is provided,\n"
    "  then sentinel values won't be used, and the label file will look\n"
    "  like a normal label file.\n"
  );
  printf ("\n");
}
/*--------------------------------------------------------------------*/
static int guess_file_type (char* fname, int* is_path, int *is_label) {
  FILE*  fp         = NULL;
  char*  line       = NULL;
  size_t size       = 1024;
  char*  needle     = NULL;
  int    found      = 0;

  *is_path = 0;
  *is_label = 0;
  found = 0;

  /* First just check the path checker. */
  if (PathIsPathFile (fname)) {
    *is_path = 1;
    return (ERROR_NONE);
  }

  /* Open the file. */
  fp = fopen (fname, "r");
  if (NULL == fp) {
    printf ("ERROR: Couldn't open %s\n", fname);
    return 1;
  }

  /* Line buffer. */
  line = (char*) malloc (size);

  while (!feof(fp) && !found) {
    /* Get a line. */
    getline (&line, &size, fp);

    /* If it's a comment line. */
    if (line[0] == '#') {
      /* Look for the label string. It's a label file if so. */
      needle = strstr( line, "label" );
      if ( NULL != needle ) {
        *is_label = 1;
        found = 1;
        break;
      }
    }
  }

  fclose( fp );

  free (line);

  if (!found)
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't identify %s", fname));

  return (ERROR_NONE);
}
/*--------------------------------------------------------------------*/
static int convert_path_to_label (char* fname, char* ofname) {
  int     err;
  int     num_paths;
  PATH**  paths       = NULL;
  int     path_index;
  int     label_size;
  int     pno;
  LABEL*  label       = NULL;
  int     label_vno;

  /* Read the paths file. */
  err = PathReadMany (fname, &num_paths, &paths);
  if (ERROR_NONE != err) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't read %s", fname));
  }

  printf ("INFO: Got %d paths\n\n", num_paths);

  /* Go through the paths we can and build a sum of number of points
     we'll need to write to the label, including an extra one per path
     for the sentinel value. */
  label_vno = 0;
  label_size = 0;
  for (path_index = 0; path_index < num_paths; path_index++) {
    label_size += paths[path_index]->n_points + 1;
  }

  /* Allocate a label of that size. */
  label = LabelAlloc (label_size, NULL, NULL);
  if (NULL == label) {
    ErrorReturn (ERROR_NO_MEMORY,
                 (ERROR_NO_MEMORY, "Couldn't allocate label of %d points",
                  label_size));
  }
  label->n_points = label_size;

  /* For each path...*/
  for (path_index = 0; path_index < num_paths; path_index++) {
    /* Write all the path points to the label. */
    for (pno = 0; pno < paths[path_index]->n_points; pno++) {
      label->lv[label_vno].x = paths[path_index]->points[pno].x;
      label->lv[label_vno].y = paths[path_index]->points[pno].y;
      label->lv[label_vno].z = paths[path_index]->points[pno].z;
      label->lv[label_vno].vno = paths[path_index]->points[pno].vno;
      label_vno++;
    }

    /* Write the sentinel value. */
    label->lv[label_vno].x = -99999;
    label->lv[label_vno].y = -99999;
    label->lv[label_vno].z = -99999;
    label->lv[label_vno].vno = -99999;
    label_vno++;

    /* Go ahead and delte the path now. */
    PathFree (&paths[path_index]);
  }

  /* Free our paths variable. */
  free (paths);

  /* Write the label file. */
  LabelWrite (label, ofname);

  /* Free the label. */
  LabelFree (&label);

  return (ERROR_NONE);
}
/*--------------------------------------------------------------------*/
static int convert_label_to_path (char* fname, char* ofname) {
  LABEL* label           = NULL;
  int    label_vno;
  int    num_paths;
  PATH** paths           = NULL;
  int    label_vno_test;
  int    path_size;
  int    path_index;
  int    pno;
  int    err;

  /* Read the label file. */
  label = LabelRead (NULL, fname);
  if (NULL == label) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't read %s", fname));
  }

  /* Count the number of sentinels, -99999 vnos in the label; this is
     the number of labels. */
  num_paths = 0;
  for (label_vno = 0; label_vno < label->n_points; label_vno++) {
    if (-99999 == label->lv[label_vno].vno)
      num_paths++;
  }

  /* Make sure we got some paths. */
  if (0 == num_paths) {
    LabelFree (&label);
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "No encoded paths found in label file"));
  }

  printf ("INFO: Found %d paths in label file.\n\n", num_paths);

  /* Allocate path objects. */
  paths = (PATH**) calloc (num_paths, sizeof(PATH*));
  if (NULL == paths) {
    ErrorReturn (ERROR_NO_MEMORY,
                 (ERROR_NO_MEMORY, "Couldn't allocate %d paths", num_paths));
  }

  /* For each path we're goint o read.. */
  path_index = 0;
  label_vno = 0;
  for (path_index = 0; path_index < num_paths; path_index++) {
    /* Count the size of the path, the number of points between here
    and the next sentinel. */
    path_size = 0;
    label_vno_test = label_vno;
    while (-99999 != label->lv[label_vno_test].vno) {
      path_size++;
      label_vno_test++;
    }

    /* Make the path. */
    paths[path_index] = PathAlloc (path_size, NULL);
    if (NULL == paths) {
      ErrorReturn (ERROR_NO_MEMORY,
                   (ERROR_NO_MEMORY, "Couldn't allocate path of %d points",
                    path_size));
    }

    /* Read points into the path from the label. */
    pno = 0;
    while (-99999 != label->lv[label_vno].vno) {
      paths[path_index]->points[pno].x = label->lv[label_vno].x;
      paths[path_index]->points[pno].y = label->lv[label_vno].y;
      paths[path_index]->points[pno].z = label->lv[label_vno].z;
      paths[path_index]->points[pno].vno = label->lv[label_vno].vno;
      label_vno++;
      pno++;
    }

    /* Now we're at the sentinel, so skip past it. */
    label_vno++;
  }

  /* Write the path file. */
  err = PathWriteMany (ofname, num_paths, paths);
  if (0 != err) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't write to %s", ofname));
  }

  return (ERROR_NONE);
}

/*--------------------------------------------------------------------*/
static int convert_single_path_to_label (char* fname, char* ofname) {
  int     err;
  int     num_paths;
  PATH**  paths       = NULL;
  LABEL*  label       = NULL;
  int     path_index;

  /* Read the paths file. */
  err = PathReadMany (fname, &num_paths, &paths);
  if (ERROR_NONE != err) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't read %s", fname));
  }

  /* Warn if we have more than one path. */
  if (num_paths != 1) {
    printf ("WARNING: Found multiple paths in label file.\n"
	    "Maybe you didn't mean to use the single option?\n"
	    "Converting it to a single path.\n");
  }

  /* Convert our first path. */
  err = PathConvertToLabel (paths[0], &label);
  if (ERROR_NONE != err) {
    for (path_index = 0; path_index < num_paths; path_index++)
      PathFree (&paths[path_index]);
    free (paths);
    return err;
  }

  /* Delete all the paths. */
  for (path_index = 0; path_index < num_paths; path_index++) {
    PathFree (&paths[path_index]);
  }

  /* Free our paths variable. */
  free (paths);

  /* Write the label file. */
  LabelWrite (label, ofname);

  /* Free the label. */
  LabelFree (&label);

  return (ERROR_NONE);
}

static int convert_single_label_to_path (char* fname, char* ofname) {
  LABEL* label           = NULL;
  int    label_vno;
  int    num_paths;
  PATH** paths           = NULL;
  int    err;

  /* Read the label file. */
  label = LabelRead (NULL, fname);
  if (NULL == label) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't read %s", fname));
  }

  /* Count the number of sentinels, -99999 vnos in the label; we only
     want one path, so this is just to check if this is a
     multiple-path label file. */
  num_paths = 0;
  for (label_vno = 0; label_vno < label->n_points; label_vno++) {
    if (-99999 == label->lv[label_vno].vno)
      num_paths++;
  }

  /* Make sure we only got one paths. */
  if (num_paths > 1) {
    printf ("WARNING: Found multiple paths in label file.\n"
	    "Maybe you didn't mean to use the single option?\n"
	    "Converting it to a single path.\n");
  }

  /* Allocate path objects. */
  paths = (PATH**) calloc (1, sizeof(PATH*));
  if (NULL == paths) {
    ErrorReturn (ERROR_NO_MEMORY,
                 (ERROR_NO_MEMORY, "Couldn't allocate %d paths", 1));
  }

  err = PathCreateFromLabel (label, &paths[0]);
  if (ERROR_NONE != err) {
    free (paths);
    return err;
  }

  /* Write the path file. */
  err = PathWriteMany (ofname, 1, paths);
  if (0 != err) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't write to %s", ofname));
  }

  PathFree (&paths[0]);
  free (paths);

  return (ERROR_NONE);
}

/*---------------------------------------------------------------------------------*/
static int connect_single_path(char* fname, char* ofname, char *subject, char *hemi) 
{
  int     err;
  int     num_paths;
  PATH **paths = NULL, *newpath;

  int *vtxnolist,*final_path, path_length, k, vtxno;
  char tmpstr[2000];
  MRIS *mris;

  /* Read the paths file. */
  err = PathReadMany (fname, &num_paths, &paths);
  if (ERROR_NONE != err) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't read %s", fname));
  }

  /* Warn if we have more than one path. */
  if (num_paths != 1) {
    printf ("WARNING: Found multiple paths in paths file. \n"
	    "Maybe you didn't mean to use the connect option?\n"
	    "Will only convert first path\n\n");
  }

  sprintf(tmpstr,"%s/%s/surf/%s.orig",getenv("SUBJECTS_DIR"),subject,hemi);
  printf("Reading %s\n",tmpstr);
  mris = MRISread(tmpstr);
  if(mris == NULL) exit(1);

  final_path = (int*) calloc(mris->nvertices,sizeof(int));
  vtxnolist = (int*) calloc(paths[0]->n_points,sizeof(int));
  for(k=0; k < paths[0]->n_points; k++)
    vtxnolist[k] = paths[0]->points[k].vno;

  MRISfindPath(vtxnolist, paths[0]->n_points, mris->nvertices, 
	       final_path, &path_length, mris );

  newpath = PathAlloc(path_length,"");
  newpath->n_points = path_length;
  newpath->points = (PATH_POINT *) calloc(path_length,sizeof(PATH_POINT));
  for(k=0; k < path_length; k++){
    vtxno = final_path[k];
    newpath->points[k].vno = vtxno;
    newpath->points[k].x = mris->vertices[vtxno].x;
    newpath->points[k].y = mris->vertices[vtxno].y;
    newpath->points[k].z = mris->vertices[vtxno].z;
  }

  /* Write the path file. */
  err = PathWriteMany (ofname, 1, &newpath);
  if (0 != err) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't write to %s", ofname));
  }

  PathFree(&paths[0]);
  free (paths);
  PathFree(&newpath);
  MRISfree(&mris);
  free(final_path);
  free(vtxnolist);

  return(ERROR_NONE);
}
/*---------------------------------------------------------------------------------*/
static int fill_single_path(char* fname, char* ofname, char *subject, char *hemi, int seed) 
{
  int     err;
  int     num_paths;
  PATH **paths = NULL;
  LABEL *label;
  int *vtxnolist,*final_path, path_length, k, vtxno, nlabel, nth;
  char tmpstr[2000];
  MRIS *mris;

  /* Read the paths file. */
  err = PathReadMany (fname, &num_paths, &paths);
  if (ERROR_NONE != err) {
    ErrorReturn (ERROR_BADFILE,
                 (ERROR_BADFILE, "Couldn't read %s", fname));
  }

  /* Warn if we have more than one path. */
  if (num_paths != 1) {
    printf ("WARNING: Found multiple paths in paths file. \n"
	    "Maybe you didn't mean to use the connect option?\n"
	    "Will only convert first path\n\n");
  }

  sprintf(tmpstr,"%s/%s/surf/%s.orig",getenv("SUBJECTS_DIR"),subject,hemi);
  printf("Reading %s\n",tmpstr);
  mris = MRISread(tmpstr);
  if(mris == NULL) exit(1);

  final_path = (int*) calloc(mris->nvertices,sizeof(int));
  vtxnolist = (int*) calloc(paths[0]->n_points,sizeof(int));
  for(k=0; k < paths[0]->n_points; k++)
    vtxnolist[k] = paths[0]->points[k].vno;

  MRISfindPath(vtxnolist, paths[0]->n_points, mris->nvertices, 
	       final_path, &path_length, mris );

  // Make sure they are 0
  for(k=0; k < mris->nvertices; k++) mris->vertices[k].val = 0;

  for(k=0; k < path_length; k++){
    vtxno = final_path[k];
    mris->vertices[vtxno].val = 1;
  }

  printf("Filling %d\n",seed);
  MRISfill(mris, seed);

  nlabel = 0;
  for(k=0; k < mris->nvertices; k++) 
    if(mris->vertices[k].val > 0.5) nlabel++;

  printf("nlabel %d\n",nlabel);
  label = LabelAlloc(nlabel, subject, "");
  label->n_points = nlabel;
  nth = 0;
  for(k=0; k < mris->nvertices; k++){
    if(mris->vertices[k].val < 0.5) continue;
    label->lv[nth].vno = k;
    label->lv[nth].x = mris->vertices[k].x;
    label->lv[nth].y = mris->vertices[k].y;
    label->lv[nth].z = mris->vertices[k].z;
    label->lv[nth].stat = 0;
    nth ++;
  }
  printf("Saving label file %s\n",ofname);
  LabelWrite(label, ofname);

  PathFree(&paths[0]);
  free (paths);
  MRISfree(&mris);
  free(final_path);
  free(vtxnolist);
  LabelFree(&label);

  return(ERROR_NONE);
}


/*----------------------------------------------------------------*/

/*!
  \fn int MRISfill(MRIS *mris, int seedvtxno)
  \brief Fills in an area on the surface.
  \param mris - surface
  \param seedvtxno - seed vertex number
  All the val fields on the surface should be 0 except for a CLOSED
  loop which should have val=1. The seedvtxno should be a vertex
  number NOT on the boundary. When finished, the val fields in the
  closed area will equal 1. Recursive.
*/

int MRISfill(MRIS *mris, int seedvtxno)
{
  int nthnbr, nbrvtxno;

  if(mris->vertices[seedvtxno].val) return(0);
  mris->vertices[seedvtxno].val = 1;
  for (nthnbr=0; nthnbr < mris->vertices[seedvtxno].vnum; nthnbr++) {
    nbrvtxno = mris->vertices[seedvtxno].v[nthnbr];
    MRISfill(mris, nbrvtxno);
  }
  return(0);
}
