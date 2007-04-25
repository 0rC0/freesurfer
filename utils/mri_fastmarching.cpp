/**
 * @file  mri_fastmarching.cpp
 * @brief Creates a distance transform.
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: Florent Segonne
 * CVS Revision Info:
 *    $Author$
 *    $Date$
 *    $Revision$
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

#include "fastmarching.h"

extern "C" MRI *MRIextractDistanceMap( MRI *mri_src, MRI *mri_dst, int label, float max_distance, int mode ) {

  MRI *mri_distance = NULL;
  
  // make sure that the max distance is greater than 0
  if( max_distance <= 0 ) {    
    max_distance = 2*MAX(MAX(mri_src->width,mri_src->height),mri_src->depth);    
  }
  
  if( mri_dst==NULL ) {
    mri_dst = MRIalloc( mri_src->width,mri_src->height,mri_src->depth,MRI_FLOAT );
    MRIcopyHeader(mri_src, mri_dst);
  }

  mri_distance = mri_dst;

  // make sure that the member variables are all the same
  if ( mri_dst->width != mri_src->width || mri_dst->height != mri_src->height || mri_dst->depth != mri_src->depth || mri_dst->type != MRI_FLOAT) {
    fprintf(stderr,"Exiting : incompatible structure with mri_src\n");
  } else {


    // set values to zero
    for(int z =0 ; z < mri_dst->depth ; z++)
      for(int y = 0 ; y < mri_dst->height ; y++)
        for(int x = 0 ; x < mri_dst->width ; x++)
          MRIFvox( mri_dst, x, y, z) = 0.0f;
  
    const int outside = 1;
    const int inside = 2;
    const int both = 3;
    
    // positive inside and positive outside
    const int bothUnsigned = 4;
  
    if ( mode == outside || mode == both || mode == bothUnsigned ) {
      FastMarching< +1 > fastmarching_out( mri_distance );
      fastmarching_out.SetLimit( max_distance );
      fastmarching_out.InitFromMRI( mri_src, label );
      fastmarching_out.Run( max_distance );
    }
    
    if ( mode == inside || mode == both || mode == bothUnsigned ) {
      FastMarching< -1 > fastmarching_in( mri_distance );
      fastmarching_in.SetLimit( -max_distance );
      fastmarching_in.InitFromMRI( mri_src, label );
      fastmarching_in.Run( -max_distance );
    }
    
    // if unsigned, then we want than the inside and out to be positive
    if( bothUnsigned ) {
      for(int z =0 ; z < mri_distance->depth ; z++)
        for(int y = 0 ; y < mri_distance->height ; y++)
          for(int x = 0 ; x < mri_distance->width ; x++)
            MRIFvox( mri_distance, x, y, z) = fabs( MRIFvox( mri_distance, x, y, z) );
    }
        
  }

  return mri_distance;
}

