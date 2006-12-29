/**
 * @file  MRIio_old.c
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 01:49:30 $
 *    $Revision: 1.2 $
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


/* MRI I/O - routines for reading and writing large files fast */
/* 2/1/91 - AD */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "MRIio_old.h"
#include "proto.h"

static void MGHprint_error(char *str) ;

static void
MGHprint_error(char *str)
{
  printf(str);
  exit(0);
}

char *
lmalloc(unsigned long size)
{
  char *p;

  p = malloc(size);
  if (p==NULL) MGHprint_error("Cannot malloc()\n");
  return p;
}

char *
lcalloc(size_t nmemb,size_t size)
{
  char *p;

  p = calloc(nmemb,size);
  if (p==NULL) MGHprint_error("Cannot calloc()\n");
  return p;
}

void
file_name(char *fpref, char *fname, int num, char *form)
{
  char ext[10];

  sprintf(ext,form,num);
  strcpy(fname,fpref);
  strcat(fname,ext);
}

void
buffer_to_image(unsigned char *buf, unsigned char **im,int ysize,int xsize)
{
  int i,j;
  unsigned long k;
  float sum;

  k=0;
  sum = 0;
  for (i=0;i<ysize;i++)
    for (j=0;j<xsize;j++)
    {
      im[i][j] = buf[k++];
      sum += im[i][j];
    }
  /*
    printf("avg=%f\n",sum/(ysize*xsize));
  */
}

void
image_to_buffer(unsigned char **im, unsigned char *buf,int ysize,int xsize)
{
  int i,j;
  unsigned long k;

  k=0;
  for (i=0;i<ysize;i++)
    for (j=0;j<xsize;j++)
      buf[k++] = im[i][j];
}
