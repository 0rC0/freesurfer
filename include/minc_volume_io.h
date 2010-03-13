/**
 * @file  minc_volume_io.h
 * @brief Wrapper for MNI's volume_io.h, to decouple from MNI lib
 *
 */
/*
 * Original Author: Nick Schmansky
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2010/03/13 01:32:40 $
 *    $Revision: 1.4 $
 *
 * Copyright (C) 2002-2010,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 *
 */


#ifndef MINC_VOLUME_IO_H
#define MINC_VOLUME_IO_H

/*
 * Wrapper for MNI's volume_io.h, which has some annoyances which
 * must be circumvented.
 */

/* remove unwanted warnings between hips_basic.h vs. volume_io/basic.h */
#undef ABS
#undef SIGN
#ifdef Darwin
// The result of not defining __MACTYPES__ is a scuba2 build error
// complaining about QT conflicting with MNI over the Point typedef.
// Notes from the file <mni installation>/include/volume_io/geom_structs.h:
/* Th 'Point' typedef is annoying to Mac OS users, since Point has been
 * a basic type on Macs since the beginning.  Testing __MACTYPES__ should
 * work at least with the OS X codebase, I don't know if it existed in
 * earlier versions of the MacTypes.h header.
 */
#define __MACTYPES__
#endif
#ifdef Status
// avoid conflicts with usage of 'Status' in volume_io/basic.h
#undef Status
#endif
#ifdef Windows_NT
#undef ERROR
#endif // Windows_NT
#include <volume_io.h> //from MNI
/* remove unwanted warnings between hips_basic.h vs. volume_io/basic.h */
#undef ABS
#undef SIGN

#endif // MINC_VOLUME_IO_H
