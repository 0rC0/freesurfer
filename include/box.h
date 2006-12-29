/**
 * @file  box.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 02:08:59 $
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


#ifndef BOX_H
#define BOX_H

typedef struct
{
  int  x0 ;
  int  y0 ;
  int  z0 ;
  int  x1 ;
  int  y1 ;
  int  z1 ;
  int  width ;
  int  height ;
  int  depth ;
}
BOX ;


int   BoxPrint(BOX *box, FILE *fp) ;
int   BoxExpand(BOX *box_src, BOX *box_dst, int dx, int dy, int dz) ;

#endif
