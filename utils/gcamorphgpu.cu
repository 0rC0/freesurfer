/**
 * @file  gcamorphgpu.cu
 * @brief Holds GCA morph data on the GPU
 *
 * 
 */
/*
 * Original Author: Richard Edgar
 * CVS Revision Info:
 *    $Author: rge21 $
 *    $Date: 2010/03/05 18:05:47 $
 *    $Revision: 1.18 $
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

#include "chronometer.hpp"

#include "volumegpucompare.hpp"


#include "gcamorphgpu.hpp"



// ==============================================================

namespace GPU {
  namespace Classes {

    // --------------------------------------------

    void GCAmorphGPU::CheckIntegrity( void ) const {
      /*!
	Checks that all the allocated member arrays have
	the same dimensions.
	Aborts the program if the check fails
      */

      const dim3 myDims = this->d_r.GetDims();

      bool good = ( myDims == this->d_invalid.GetDims() );
      good = ( good && ( myDims == this->d_area.GetDims() ) );
      good = ( good && ( myDims == this->d_origArea.GetDims() ) );
      good = ( good && ( myDims == this->d_area1.GetDims() ) );
      good = ( good && ( myDims == this->d_area2.GetDims() ) );
      good = ( good && ( myDims == this->d_label.GetDims() ) );
      good = ( good && ( myDims == this->d_status.GetDims() ) );
      good = ( good && ( myDims == this->d_mean.GetDims() ) );
      good = ( good && ( myDims == this->d_variance.GetDims() ) );

      if( !good ) {
	std::cerr << __FUNCTION__
		  << ": Dimension mismatch"
		  << std::endl;
	exit( EXIT_FAILURE );
      }
    }

    // --------------------------------------------

    void GCAmorphGPU::AllocateAll( const dim3& dims ) {
      /*!
	Allocates GPU memory to hold a volume
	of the given size.
	If possible, it keeps the current allocation.
      */

      // Start by seeing if the current allocation is consistent
      this->CheckIntegrity();

      // See if we can re-use existing allocation
      if( dims == this->d_r.GetDims() ) {
	return;
      }

      // Release existing memory
      this->ReleaseAll();

      // Allocate anew
      this->d_r.Allocate( dims );
      this->d_invalid.Allocate( dims );
      this->d_area.Allocate( dims );
      this->d_origArea.Allocate( dims );
      this->d_area1.Allocate( dims );
      this->d_area2.Allocate( dims );
      this->d_label.Allocate( dims );
      this->d_status.Allocate( dims );
      this->d_mean.Allocate( dims );
      this->d_variance.Allocate( dims );
    }


    void GCAmorphGPU::ReleaseAll( void ) {
      /*!
	Releases each of the members.
	Recall that the VolumeGPU::Release method
	will also release any CUDA arrays.
      */
      this->d_r.Release();
      this->d_invalid.Release();
      this->d_area.Release();
      this->d_origArea.Release();
      this->d_area1.Release();
      this->d_area2.Release();
      this->d_label.Release();
      this->d_status.Release();
      this->d_mean.Release();
      this->d_variance.Release();
    }

    // --------------------------------------------

    void GCAmorphGPU::SendAll( const GCAM* src ) {
      /*!
	Sends all supported data in the given GCAM
	to the GPU.
	This involves a lot of packing data, and hence
	is going to be painfully slow
      */

      // Check for number of inputs
      if( src->ninputs != 1 ) {
	std::cerr << __FUNCTION__
		  << ": Must have only one input in the GC1D!"
		  << std::endl;
	exit( EXIT_FAILURE );
      }

      // Extract the dimensions
      const dim3 dims = make_uint3( src->width,
				    src->height,
				    src->depth );

      // Allocate device memory
      this->AllocateAll( dims );

      // Allocate some page-locked host buffers
      float3* h_r = this->d_r.AllocateHostBuffer();
      unsigned char* h_invalid = this->d_invalid.AllocateHostBuffer();
      float* h_area = this->d_area.AllocateHostBuffer();
      float* h_origArea = this->d_origArea.AllocateHostBuffer();
      float* h_area1 = this->d_area1.AllocateHostBuffer();
      float* h_area2 = this->d_area2.AllocateHostBuffer();
      int* h_status = this->d_status.AllocateHostBuffer();
      int* h_label = this->d_status.AllocateHostBuffer();
      float* h_mean = this->d_mean.AllocateHostBuffer();
      float* h_variance = this->d_variance.AllocateHostBuffer();

      for( unsigned int i=0; i<dims.x; i++ ) {
	for( unsigned int j=0; j<dims.y; j++ ) {
	  for( unsigned int k=0; k<dims.z; k++ ) {

	    // Get the 1d index (same for all arrays)
	    const unsigned int i1d = this->d_r.Index1D( i, j, k );
	    // Get the current node
	    const GCA_MORPH_NODE& gcamn = src->nodes[i][j][k];
	    
	    // Pack the data
	    h_r[i1d] = make_float3( gcamn.x,
				    gcamn.y,
				    gcamn.z );

	    h_invalid[i1d] = gcamn.invalid;
	    h_area[i1d] = gcamn.area;
	    h_origArea[i1d] = gcamn.orig_area;
	    h_area1[i1d] = gcamn.area1;
	    h_area2[i1d] = gcamn.area2;
	    h_status[i1d] = gcamn.status;
	    h_label[i1d] = gcamn.label;
	    
	    // Deal with the GC1D
	    if( gcamn.gc != NULL ) {
	      /*
		Store the mean and variance.
		Check at top of the routine has ensured
		that there's only one input.
		This means that the covariance is really
		a variance
	      */
	      h_mean[i1d] = gcamn.gc->means[0];
	      h_variance[i1d] = gcamn.gc->covars[0];
	    } else {
	      /*
		Store negative numbers to indicate that
		there is no GC1D here.
		Since a variance must be >=0, this is
		a reliable test
	      */
	      h_mean[i1d] = -1;
	      h_variance[i1d] = -1;
	    }


	  }
	}
      }


      // Send the data
      this->d_r.SendBuffer( h_r );
      this->d_invalid.SendBuffer( h_invalid );
      this->d_area.SendBuffer( h_area );
      this->d_origArea.SendBuffer( h_origArea );
      this->d_area1.SendBuffer( h_area1 );
      this->d_area2.SendBuffer( h_area2 );
      this->d_status.SendBuffer( h_status );
      this->d_label.SendBuffer( h_label );
      this->d_mean.SendBuffer( h_mean );
      this->d_variance.SendBuffer( h_variance );

      // Wait for the copies to complete
      CUDA_SAFE_CALL( cudaThreadSynchronize() );

      // Release page-locked host memory
      CUDA_SAFE_CALL( cudaFreeHost( h_r ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_invalid ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_area ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_origArea ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_area1 ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_area2 ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_status ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_label ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_mean ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_variance ) );

    }

    // --------------------------------------------

    void GCAmorphGPU::RecvAll( GCAM* dst ) const {
      /*!
	Retrieves all supported data in the given GCAM
	from the GPU.
	This involves a lot of packing data, and hence
	is going to be painfully slow
      */

      // Check for number of inputs
      if( dst->ninputs != 1 ) {
	std::cerr << __FUNCTION__
		  << ": Must have only one input in the GC1D!"
		  << std::endl;
	exit( EXIT_FAILURE );
      }


      // Extract the dimensions
      const dim3 dims = this->d_r.GetDims();

      // Allocate some page-locked host buffers
      float3* h_r = this->d_r.AllocateHostBuffer();
      unsigned char* h_invalid = this->d_invalid.AllocateHostBuffer();
      float* h_area = this->d_area.AllocateHostBuffer();
      float* h_origArea = this->d_origArea.AllocateHostBuffer();
      float* h_area1 = this->d_area1.AllocateHostBuffer();
      float* h_area2 = this->d_area2.AllocateHostBuffer();
      int* h_status = this->d_status.AllocateHostBuffer();
      int* h_label = this->d_status.AllocateHostBuffer();
      float* h_mean = this->d_mean.AllocateHostBuffer();
      float* h_variance = this->d_variance.AllocateHostBuffer();

      // Fetch the data
      this->d_r.RecvBuffer( h_r );
      this->d_invalid.RecvBuffer( h_invalid );
      this->d_area.RecvBuffer( h_area );
      this->d_origArea.RecvBuffer( h_origArea );
      this->d_area1.RecvBuffer( h_area1 );
      this->d_area2.RecvBuffer( h_area2 );
      this->d_status.RecvBuffer( h_status );
      this->d_label.RecvBuffer( h_label );
      this->d_mean.RecvBuffer( h_mean );
      this->d_variance.RecvBuffer( h_variance );
      CUDA_SAFE_CALL( cudaThreadSynchronize() );

      for( unsigned int i=0; i<dims.x; i++ ) {
	for( unsigned int j=0; j<dims.y; j++ ) {
	  for( unsigned int k=0; k<dims.z; k++ ) {

	    // Get the 1d index (same for all arrays)
	    const unsigned int i1d = this->d_r.Index1D( i, j, k );
	    // Get the current node
	    GCA_MORPH_NODE* gcamn = &(dst->nodes[i][j][k]);

	    gcamn->x = h_r[i1d].x;
	    gcamn->y = h_r[i1d].y;
	    gcamn->z = h_r[i1d].z;

	    gcamn->invalid = h_invalid[i1d];
	    gcamn->area = h_area[i1d];
	    gcamn->orig_area = h_origArea[i1d];
	    gcamn->area1 = h_area1[i1d];
	    gcamn->area2 = h_area2[i1d];
	    gcamn->label = h_label[i1d];
	    gcamn->status = h_status[i1d];

	    // We now have a quandary... how to test for validity
	    if( gcamn->gc != NULL ) {
	      // We know there's only one input from test at the top
	      gcamn->gc->means[0] = h_mean[i1d];
	      gcamn->gc->covars[0] = h_variance[i1d];
	    } else {
	      if( h_variance[i1d] >= 0 ) {
		std::cerr << __FUNCTION__
			  << ": Host has no GC1D but GPU has valid variance"
			  << std::endl;
		exit( EXIT_FAILURE );
	      }
	    }

	  }
	}
      }


      // Release page-locked host memory
      CUDA_SAFE_CALL( cudaFreeHost( h_r ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_invalid ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_area ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_origArea ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_area1 ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_area2 ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_status ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_label ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_mean ) );
      CUDA_SAFE_CALL( cudaFreeHost( h_variance ) );

    }




    // --------------------------------------------

    const unsigned int kCMPKernelSize = 16;
    const unsigned int iCMPGlobalsInvalid = 0;
    const unsigned int iCMPGlobalsNeg = 1;

    //! Kernel to perform work of gcamComputeMetricProperties
    __global__
    void CompMetPropKernel( const VolumeArgGPU<float3> r,
			    const VolumeArgGPU<float> origArea,
			    VolumeArgGPU<unsigned char> invalid,
			    VolumeArgGPU<float> area,
			    VolumeArgGPU<float> area1,
			    VolumeArgGPU<float> area2,
			    int *globals ) {
      /*!
	This kernel performs the work of gcamComputeMetricProperties.
	For now, it's unoptimised, and may cause a lot of un-necessary
	memory transations
      */
      // Compute co-ordinates
      const unsigned int ix = threadIdx.x + ( blockIdx.x * blockDim.x );
      const unsigned int iy = threadIdx.y + ( blockIdx.y * blockDim.y );

      // Check if in volume
      if( !r.InVolume( ix, iy, 0 ) ) {
	return;
      }

      // Loop over each z slice
      for( unsigned int iz=0; iz< r.dims.z; iz++ ) {

	int neg = 0;
	int num = 0;

	// Check for invalid node
	if( invalid( ix, iy, iz ) == GCAM_POSITION_INVALID ) {
	  atomicAdd( &(globals[iCMPGlobalsInvalid]), 1 );
	  continue;
	}
	
	// Zero the 'area'
	area(ix,iy,iz) = 0;

	// Compute Jacobean determinants on the 'right'
	if( (ix<r.dims.x-1) && (iy<r.dims.y-1) && (iz<r.dims.z-1) ) {

	  // Check for validity
	  if( (invalid(ix+1,iy,iz) != GCAM_POSITION_INVALID) &&
	      (invalid(ix,iy+1,iz) != GCAM_POSITION_INVALID) &&
	      (invalid(ix,iy,iz+1) != GCAM_POSITION_INVALID) ) {
	    
	    num++;
	    

	    float3 vi = r(ix+1,iy  ,iz  ) - r(ix,iy,iz);
	    float3 vj = r(ix  ,iy+1,iz  ) - r(ix,iy,iz);
	    float3 vk = r(ix  ,iy  ,iz+1) - r(ix,iy,iz);

	    float tmpArea = stp( vj, vk, vi );
	    if( tmpArea <= 0 ) {
	      neg = 1;
	    }

	    area1(ix,iy,iz) = tmpArea;
	    area(ix,iy,iz) += tmpArea;

	  }
	} else {
	  // Going to 'right' would fall out of the volume
	  area1(ix,iy,iz) = 0;
	}


	// Compute Jacobean determinants on the 'left'
	if( (ix>0) && (iy>0) && (iz>0) ) {
	  
	  // Check for validity
	  if( (invalid(ix-1,iy,iz) != GCAM_POSITION_INVALID) &&
	      (invalid(ix,iy-1,iz) != GCAM_POSITION_INVALID) &&
	      (invalid(ix,iy,iz-1) != GCAM_POSITION_INVALID) ) {
	    num++;

	    // I think this ordering preserves handedness
	    // It's different to that in gcamorph.c
	    float3 vi = r(ix,iy,iz) - r(ix-1,iy  ,iz  );
	    float3 vj = r(ix,iy,iz) - r(ix  ,iy-1,iz  );
	    float3 vk = r(ix,iy,iz) - r(ix  ,iy  ,iz-1);

	    float tmpArea = stp( vj, vk, vi );

	    if( tmpArea <= 0 ) {
	      neg = 1;
	    }

	    area2(ix,iy,iz) = tmpArea;
	    area(ix,iy,iz) += tmpArea;
	  }
	} else {
	  area2(ix,iy,iz) = 0;
	}

	// Check if at least one determinant was computed
	if( num > 0 ) {
	  // area is mean of 'left' and 'right' areas
	  area(ix,iy,iz) /= num;
	} else {
	  invalid(ix,iy,iz) = GCAM_AREA_INVALID;
	  area(ix,iy,iz) = 0;
	}

	// Keep track of sign changes
	if( (invalid(ix,iy,iz)==GCAM_VALID) &&
	    neg &&
	    origArea(ix,iy,iz) > 0 ) {
	  atomicAdd( &(globals[iCMPGlobalsNeg]), 1 );
	}

	// Increment invalid counter
	if( invalid(ix,iy,iz) != GCAM_VALID ) {
	  // We need to test again
	  atomicAdd( &(globals[iCMPGlobalsInvalid]), 1 );
	}
      }
    }
    
    void GCAmorphGPU::ComputeMetricProperties( int& invalid, int& neg ) {
      /*!
	Routine to duplicate gcamComputeMetricProperties
	from the file gcamorph.c.
	It essentially computes a lot of jacobean determinants
	and sums them up.
	The argument \a invalid is used to return the number of
	invalid locations found, a task performed by the
	global variable \c Ginvalid in gcamorph.c.
	The argument \a neg is used to keep track of the negative
	determinants, and should be returned to \c gcam->neg
	when called.
      */

      SciGPU::Utilities::Chronometer tTotal;

      tTotal.Start();

      // Sanity check
      this->CheckIntegrity();

      // Allocate temporary on the device to hold invalid and neg
      int *d_globals;
      CUDA_SAFE_CALL( cudaMalloc( (void**)&d_globals, 2*sizeof(int) ) );
      CUDA_SAFE_CALL( cudaMemset( d_globals, 0, 2*sizeof(int) ) );

      // Run the kernel
      dim3 grid, threads;

      threads.x = threads.y = kCMPKernelSize;
      threads.z = 1;

      grid = this->d_r.CoverBlocks( kCMPKernelSize );
      grid.z = 1;

      CompMetPropKernel<<<grid,threads>>>
	( this->d_r, this->d_origArea, this->d_invalid,
	  this->d_area, this->d_area1, this->d_area2,
	  d_globals );
      CUDA_CHECK_ERROR( "CompMetPropKernel failed!\n" );

      // Retrieve global statistics
      int globals[2];
      CUDA_SAFE_CALL( cudaMemcpy( &globals, d_globals,
				  2*sizeof(int),
				  cudaMemcpyDeviceToHost ) );
      invalid = globals[iCMPGlobalsInvalid];
      neg = globals[iCMPGlobalsNeg];

      // Release device temporary
      CUDA_SAFE_CALL( cudaFree( d_globals ) );

      tTotal.Stop();

      //std::cout << __FUNCTION__ << ": Complete in " << tTotal << std::endl;
    }
  }
}



void gcamComputeMetricPropertiesGPU( GCA_MORPH* gcam,
				     int *invalid ) {
  /*!
    This is a wrapper around the CUDA implementation
    of gcamComputeMetricProperties
  */

  GPU::Classes::GCAmorphGPU gcamGPU;
  
  gcamGPU.SendAll( gcam );
  gcamGPU.ComputeMetricProperties( *invalid, gcam->neg );
  gcamGPU.RecvAll( gcam );

}

/*
  The following functions are a bunch of ugly hacks designed
  to permit testing deep within mri_ca_register.
  They should never be included in a release.
  Indeed, if you are reading this in a release version of the
  code, please report it as a bug.
*/

#include "testgpu.h"

static GPU::Classes::GCAmorphGPU compGPU, compCPU;


void GCAMorphSendBefore( const GCAM* src ) {
  int invalid, neg;

  compGPU.SendAll( src );
  compGPU.ComputeMetricProperties( invalid, neg );

  std::cout << __FUNCTION__ << ": invalid = " << invalid << std::endl;
  std::cout << __FUNCTION__ << ": neg = " << neg << std::endl;
}

void GCAMorphSendAfter( const GCAM* src ) {
  compCPU.SendAll( src );
}


void GCAMorphCompareBeforeAfter( GCAM* dst ) {

  GPU::Algorithms::VolumeGPUCompare myComp;
  float areaDiff;
  dim3 loc;

  myComp.MaxDiff( compGPU.d_area, compCPU.d_area, areaDiff, loc );
  
  std::cout << __FUNCTION__
	    << ": area " << areaDiff << " at " << loc << std::endl;

  myComp.MaxDiff( compGPU.d_area1, compCPU.d_area1, areaDiff, loc );
  std::cout << __FUNCTION__
	    << ": area1 " << areaDiff << " at " << loc << std::endl;

  myComp.MaxDiff( compGPU.d_area2, compCPU.d_area2, areaDiff, loc );
  std::cout << __FUNCTION__
	    << ": area2 " << areaDiff << " at " << loc << std::endl;

  unsigned char invalidDiff;
  myComp.MaxDiff( compGPU.d_invalid, compCPU.d_invalid, invalidDiff, loc );
  std::cout << __FUNCTION__
	    << ": invalid " << static_cast<int>(invalidDiff)
	    << " at " << loc << std::endl;

  double errL2;

  errL2 = myComp.ErrL2Norm( compGPU.d_area, compCPU.d_area );
  std::cout << __FUNCTION__ << ": Area L2 = " << errL2 << std::endl;
  errL2 = myComp.ErrL2Norm( compGPU.d_area1, compCPU.d_area1 );
  std::cout << __FUNCTION__ << ": Area1 L2 = " << errL2 << std::endl;
  errL2 = myComp.ErrL2Norm( compGPU.d_area2, compCPU.d_area2 );
  std::cout << __FUNCTION__ << ": Area2 L2 = " << errL2 << std::endl;

  compGPU.RecvAll( dst );
}
