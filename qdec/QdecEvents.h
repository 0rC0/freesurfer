/**
 * @file  QDECEvents.h
 * @brief Event constants
 *
 * These are the events used in our program.
 */
/*
 * Original Author: Kevin Teich
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2008/06/13 00:24:27 $
 *    $Revision: 1.1.2.2 $
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

#ifndef QdecEvents_h
#define QdecEvents_h

#include "vtkCommand.h"

class QdecEvents {

 public:

  // Description:
  // Our event constants.
  enum { UserSelectedVertex = vtkCommand::UserEvent + 1 };

};

#endif
