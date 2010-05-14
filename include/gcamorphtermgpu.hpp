/**
 * @file  gcamorphtermgpu.hpp
 * @brief Holds routines to compute GCAmorph terms on the GPU (header)
 *
 * This file holds a variety of routines which compute terms for
 * mri_ca_register
 */
/*
 * Original Author: Richard Edgar
 * CVS Revision Info:
 *    $Author: rge21 $
 *    $Date: 2010/05/14 17:50:11 $
 *    $Revision: 1.2 $
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


#ifndef GCA_MORPH_TERM_GPU_HPP
#define GCA_MORPH_TERM_GPU_HPP

#include "chronometer.hpp"

#include "mriframegpu.hpp"
#include "gcamorphgpu.hpp"

namespace GPU {
  namespace Algorithms {

    //! Class to hold GCAMorph 'term' computations
    class GCAmorphTerm {

    public:

      //! Constructor (stream defaults to zero)
      GCAmorphTerm( const cudaStream_t s = 0 ) : stream(s) {};

      //! Destructor
      ~GCAmorphTerm( void ) {};

      //! Routine to print timing information to std.out
      static void ShowTimings( void );


      //! Computes the smoothness term
      void Smoothness( GPU::Classes::GCAmorphGPU& gcam,
		       const float l_smoothness ) const;

      // ######################################################
    private:

      //! Stream to use for operations
      cudaStream_t stream;

      //! Timer for smoothness term
      static SciGPU::Utilities::Chronometer tSmoothTot;
      //! Timer for smoothness term subtraction
      static SciGPU::Utilities::Chronometer tSmoothSubtract;
      //! Timer for smoothness computation itself
      static SciGPU::Utilities::Chronometer tSmoothCompute;
    };

  }
}

#endif
