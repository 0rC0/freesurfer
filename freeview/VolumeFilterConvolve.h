/**
 * @file  VolumeFilterConvolve.h
 * @brief Base VolumeFilterConvolve class. 
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2010/03/26 19:04:05 $
 *    $Revision: 1.1 $
 *
 * Copyright (C) 2008-2009,
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

#ifndef VolumeFilterConvolve_h
#define VolumeFilterConvolve_h

#include "VolumeFilter.h"

class LayerMRI;

class VolumeFilterConvolve : public VolumeFilter
{
public:
  VolumeFilterConvolve( LayerMRI* input = 0, LayerMRI* output = 0 );
  virtual ~VolumeFilterConvolve();
  
  void SetSigma( double dvalue )
  {
    m_dSigma = dvalue;
  }
  
  double GetSigma()
  {
    return m_dSigma;
  }
  
  std::string GetName()
  {
    return "Convolve";
  }
  
protected:
  bool Execute();
  
  double m_dSigma;
};

#endif


