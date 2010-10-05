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
 *    $Date: 2010/10/05 18:05:07 $
 *    $Revision: 1.12 $
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
#include "gcamorphcpu.hpp"

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

      //! Computes the Jacobian term
      void Jacobian( GPU::Classes::GCAmorphGPU& gcam,
		     const float l_jacobian,
		     const float jac_scale ) const;

      //! Computes the Log Likelihood term
      template<typename T, typename U>
      void LogLikelihood( GPU::Classes::GCAmorphGPU& gcam,
			  const GPU::Classes::MRIframeGPU<T>& mri,
			  const GPU::Classes::MRIframeGPU<U>& mri_smooth,
			  double l_log_likelihood ) const;

      //! Dispatch routine for the Log Likelihood term
      template<typename T, typename U>
      void LLtermDispatch( GCA_MORPH *gcam,
			   const MRI*  mri,
			   const MRI* mri_smooth,
			   double l_log_likelihood ) const;
      
      //! Basic Dispatch routine for LLT
      void LLTDispatch( GCA_MORPH *gcam,
			const MRI*  mri,
			const MRI* mri_smooth,
			double l_log_likelihood ) const;


      template<typename T>
      void BindMRI( const GPU::Classes::MRIframeGPU<T>& mri ) const;

      template<typename T>
      void UnbindMRI( void ) const;

      template<typename T>
      void BindMRIsmooth( const GPU::Classes::MRIframeGPU<T>& mri ) const;

      template<typename T>
      void UnbindMRIsmooth( void ) const;


      //! Posterior/Anterior consistency check for Label term
      int LabelPostAntConsistency( GPU::Classes::GCAmorphGPU& gcam,
				   GPU::Classes::MRIframeGPU<float>& mri_dist ) const;

      //! Final update stage for Label term
      int LabelFinalUpdate( GPU::Classes::GCAmorphGPU& gcam,
			    const GPU::Classes::MRIframeGPU<float>& mri_dist,
			    const float l_label ) const;


      //! Copy deltas for Label term
      void LabelCopyDeltas( GPU::Classes::GCAmorphGPU& gcam,
			    const GPU::Classes::MRIframeGPU<float>& mri_dist,
			    const float l_label ) const;

      
      //! Wrapper for Remove Label Outliers
      void RemoveLabelOutliersDispatch( GPU::Classes::GCAmorphGPU& gcam,
					MRI *mri_dist,
					const int whalf,
					const double thresh ) const;
      

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

      //! Timer for the Jacobian term
      static SciGPU::Utilities::Chronometer tJacobTot;
      //! Timer for norm calculation of Jacobian term
      static SciGPU::Utilities::Chronometer tJacobMaxNorm;
      //! Timer for jacobian computation itself
      static SciGPU::Utilities::Chronometer tJacobCompute;

      //! Timer for Log likelihood term
      static SciGPU::Utilities::Chronometer tLogLikelihoodTot;
      //! Timer for Log likelihood term computation
      static SciGPU::Utilities::Chronometer tLogLikelihoodCompute;

      //! Timer for the computation portion of RemoveOutliers
      static SciGPU::Utilities::Chronometer tRemoveOutliers;
      //! Timer for copy deltas portion of LabelTerm
      static SciGPU::Utilities::Chronometer tLabelCopyDeltas;
      //! Timer for post/ant consistency check of LabelTerm
      static SciGPU::Utilities::Chronometer tLabelPostAntConsistency;
      //! Timer for final update of Label term
      static SciGPU::Utilities::Chronometer tLabelFinal;

      // ---------------------

      template<typename T>
      void LLTmrismoothDispatch( GCA_MORPH *gcam,
				 const MRI*  mri,
				 const MRI* mri_smooth,
				 double l_log_likelihood ) const;

      //! Remove Label Outliers for Label term
      int RemoveLabelOutliers( Freesurfer::GCAmorphCPU& gcam,
			       MRI *mri_dist,
			       const int whalf,
			       const double thresh ) const;
      
    };

  }
}

#endif
