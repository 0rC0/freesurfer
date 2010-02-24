/**
 * @file  gcamorphgpu.hpp
 * @brief Holds GCA morph data on the GPU
 *
 * 
 */
/*
 * Original Author: Richard Edgar
 * CVS Revision Info:
 *    $Author: rge21 $
 *    $Date: 2010/02/24 15:23:37 $
 *    $Revision: 1.5 $
 *
 * Copyright (C) 2002-2008,
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


#ifndef GCA_MORPH_GPU_H
#define GCA_MORPH_GPU_H

#include "gcamorph.h"

#include "volumegpu.hpp"


// ==================================================================

namespace GPU {

  namespace Classes {

    //! Class to hold a GCA morph on the GPU
    class GCAmorphGPU {
    public:
      //! Matches x, y and z in GCAmorph
      VolumeGPU<float3> d_r;
      //! Matches invalid flag in GCAmorph
      VolumeGPU<unsigned char> d_invalid;
      //! Matches area field in GCAmorph
      VolumeGPU<float> d_area;
      //! Matches area1 field in GCAmorph
      VolumeGPU<float> d_area1;
      //! Matches area2 field in GCAmorph
      VolumeGPU<float> d_area2;

      // -----------------------------------------
      // Constructors & Destructor

      //! Default constructor
      GCAmorphGPU( void ) : d_r(),
			    d_invalid(),
			    d_area(),
			    d_area1(),
			    d_area2() {};

      //! Destructor
      ~GCAmorphGPU( void ) {};

      // -------------------------------------------

      //! Checks integrity of members
      void CheckIntegrity( void ) const;

      // -------------------------------------------
      // Memory management
      
      //! Allocates GPU memory for volume of given size
      void AllocateAll( const dim3& dims );

      //! Releases all the GPU memory
      void ReleaseAll( void );

      // -------------------------------------------
      // Transfer routines

      //! Sends all data to the GPU
      void SendAll( const GCAM* src );

      //! Receives all data from the GPU
      void RecvAll( GCAM* dst ) const;

    private:

    };

  }
}



#endif
