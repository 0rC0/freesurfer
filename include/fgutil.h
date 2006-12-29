/**
 * @file  fgutil.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 02:08:59 $
 *    $Revision: 1.3 $
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


/*  $Id: fgutil.h,v 1.3 2006/12/29 02:08:59 nicks Exp $   */
#ifndef _FGUTIL_H
#define _FGUTIL_H

#include "h_logz.h"

/* if GRAB_LOGMAP is set, then x is # of rings, and y is # of spokes */
typedef struct
{
  int          sizex, sizey;
  LOGMAP_INFO  *lmi ;
  int          GrabOpts ;     /* bit field of GRAB_ values */
  IMAGE        *Icartesian ;  /* only used if GrabOpts & GRAB_LOGMAP */
  IMAGE        *Ilog ;        /* only used if GrabOpts & GRAB_LOGMAP */
  IMAGE        *Idsp ;        /* image out of DSP */
}
FGINFO;

#define GRAB_DEFAULT 0x00
#define GRAB_FIELD   0x01 /* 640x240 Default is 640x480 */
#define GRAB_LOGMAP  0x02 /* Default is to return real image */
#define GRAB_DBUFF   0x04 /* Grab using double buffer - Default is single */
#define GRAB_EONLY   0x08 /* Grab even field - Default is odd */
#define GRAB_SUBS    0x10 /* Subsample to true field width */

extern int FGInitialize(int GrabOpts, FGINFO *ptr);
extern void FGClose(void);
extern int FGGrab(FGINFO *fgi, char *buffer);
extern int FGsetLogmapType(FGINFO *fgi, int logmap_type) ;

#endif /* _FGUTIL_H */
