#ifndef mriVolume_h
#define mriVolume_h

#include "volume_io.h"
#include "mri.h"
#include "xTypes.h"
#include "xVoxel.h"
#include "mriTypes.h"
#include "mriTransform.h"
#include "transform.h"


/* Enable this to turn macros on, see details below. */
#define VOLM_USE_MACROS

typedef enum {
  Volm_tErr_Invalid = -1,
  Volm_tErr_NoErr = 0,
  Volm_tErr_InvalidSignature,
  Volm_tErr_InvalidParamater,
  Volm_tErr_InvalidIdx,
  Volm_tErr_AllocationFailed,
  Volm_tErr_CouldntReadVolume,
  Volm_tErr_CouldntCopyVolume,
  Volm_tErr_CouldntReadTransform,
  Volm_tErr_CouldntCopyTransform,
  Volm_tErr_CouldntNormalizeVolume,
  Volm_tErr_CouldntExportVolumeToCOR,
  Volm_tErr_MRIVolumeNotPresent,
  Volm_tErr_ScannerTransformNotPresent,
  Volm_tErr_IdxToRASTransformNotPresent,
  
  Volm_knNumErrorCodes
} Volm_tErr;

#define Volm_knErrStringLen 1024

typedef BUFTYPE Volm_tValue;
typedef Volm_tValue *Volm_tValueRef;
#define Volm_knMaxValue   255
#define Volm_knNumValues  256

#define Volm_kSignature 0xd9d6b2a9

#define Volm_kfDefaultBrightness 0.35
#define Volm_kfDefaultContrast   12.0

typedef struct {
  
  long mSignature;
  
  int mnDimensionX;
  int mnDimensionY;
  int mnDimensionZ;
  int mnDimensionFrame;

  MRI*     mpMriValues;     /* MRI struct */
  float*   mpSnapshot;      /* copy of values */
  float*   mpMaxValues;     /* max projection. 3 planes, each dimension
			       of a slice. 
			       mpMaxValues[orientation][x][y] */
  
  mriTransformRef    mIdxToRASTransform;           /* idx -> ras (by MRI) */
  //  LTA*               mDisplayTransform;            /* buf -> index */
  mriTransformRef    mDisplayTransform;            /* buf -> index */
  mriTransformRef    mMNITalLtzToRealTalTransform; /* tal (z<0) -> real tal */
  mriTransformRef    mMNITalGtzToRealTalTransform; /* tal (z>0) -> real tal */
  mriTransformRef    mScannerTransform;            /* idx -> scnaner */
  
  char msSubjectName[mri_knSubjectNameLen];
  char msVolumeName[mri_knSubjectNameLen];
  char msOriginalPath[mri_knPathLen];
  
  float mfColorBrightness;                 /* threshold */
  float mfColorContrast;                   /* squash */
  xColor3f mafColorTable[Volm_knNumValues]; /* color for each value */
  xColor3n manColorTable[Volm_knNumValues]; /* color for each value */
  MATRIX   *m_resample_orig ;              /* 256^3->original volume */
  MATRIX   *m_resample ;                   /* 256^3->original volume */
  MATRIX   *m_resample_inv ;               /* original volume->256^3 */
  float     max_val ;                      /* max value in mpMriValues */
  float     min_val ;                      /* min value in mpMriValues */

  VECTOR* mpTmpScreenIdx;     /* Used as tmp variables in macros. */
  VECTOR* mpTmpMRIIdx;
  xVoxel  mTmpVoxel;
} mriVolume, *mriVolumeRef;

typedef enum {
  Volm_tVisitComm_Continue = 0,
  Volm_tVisitComm_SkipRestOfRow,
  Volm_tVisitComm_SkipRestOfPlane,
  Volm_tVisitComm_Stop,
  Volm_knNumVisitCommands
} Volm_tVisitCommand;
typedef Volm_tVisitCommand(*Volm_tVisitFunction)(xVoxelRef  iAnaIdx,
						 float      iValue,
						 void*      ipData);

Volm_tErr Volm_New        ( mriVolumeRef* opVolume );
Volm_tErr Volm_Delete     ( mriVolumeRef* iopVolume );
Volm_tErr Volm_DeepClone  ( mriVolumeRef  this, 
			    mriVolumeRef* opVolume );

Volm_tErr Volm_CreateFromVolume( mriVolumeRef this, 
				 mriVolumeRef iVolume );
Volm_tErr Volm_ImportData      ( mriVolumeRef this,
				 char*        isSource );
Volm_tErr Volm_ExportNormToCOR ( mriVolumeRef this,
				 char*        isPath );

Volm_tErr Volm_LoadDisplayTransform ( mriVolumeRef this,
				      char*        isSource );
Volm_tErr Volm_UnloadDisplayTransform ( mriVolumeRef this );


/* Use the GetColor functions when you just need to display a color on
   the screen. */
void Volm_GetIntColorAtIdx        ( mriVolumeRef     this,
				    xVoxelRef        iIdx,
				    xColor3nRef      oColor );
void Volm_GetIntColorAtXYSlice    ( mriVolumeRef     this,
				    mri_tOrientation iOrientation,
				    xPoint2nRef      iPoint,
				    int              inSlice,
				    xColor3nRef      oColor );
void Volm_GetMaxIntColorAtIdx     ( mriVolumeRef     this,
				    xVoxelRef        iIdx,
				    mri_tOrientation iOrientation,
				    xColor3nRef      oColor );
void Volm_GetMaxIntColorAtXYSlice ( mriVolumeRef     this,
				    mri_tOrientation iOrientation,
				    xPoint2nRef      iPoint,
				    int              inSlice,
				    xColor3nRef      oColor );

Volm_tErr Volm_GetDimensions        ( mriVolumeRef this,
				      int*         onDimensionX, 
				      int*         onDimensionY, 
				      int*         onDimensionZ );
Volm_tErr Volm_GetNumberOfFrames    ( mriVolumeRef this,
				      int*         onDimensionFrames );
Volm_tErr Volm_GetType              ( mriVolumeRef this,
				      int*         onType );

/* Use the GetValue functions when you want the real value of the
   voxel. Before calling the unsafe version, make sure the volume is
   valid, the index is in bounds, and you're passing a valid
   pointer. */
Volm_tErr Volm_GetValueAtIdx       ( mriVolumeRef this,
				     xVoxelRef    iIdx,
				     float*       oValue );
Volm_tErr Volm_GetValueAtIdxUnsafe ( mriVolumeRef this,
				     xVoxelRef    iIdx,
				     float*       oValue );
Volm_tErr Volm_SetValueAtIdx       ( mriVolumeRef this,
				     xVoxelRef    iIdx,
				     float        iValue );

Volm_tErr Volm_GetValueAtIdxFrame       ( mriVolumeRef this,
					  xVoxelRef    iIdx,
					  int          iFrame,
					  float*       oValue );
Volm_tErr Volm_GetValueAtIdxFrameUnsafe ( mriVolumeRef this,
					  xVoxelRef    iIdx,
					  int          iFrame,
					  float*       oValue );
Volm_tErr Volm_SetValueAtIdxFrame       ( mriVolumeRef this,
					  xVoxelRef    iIdx,
					  int          iFrame,
					  float        iValue );

/* coordinate conversion. idx stands for index and is the 0->dimension-1
   index of the volume. RAS space, aka world space, is centered on the
   center of the volume and is in mm. mni tal is mni's version of 
   talairach space. the other tal is mni tal with a small modification. */
Volm_tErr Volm_ConvertIdxToRAS     ( mriVolumeRef this,
				     xVoxelRef    iIdx,
				     xVoxelRef    oRAS );
Volm_tErr Volm_ConvertRASToIdx     ( mriVolumeRef this,
				     xVoxelRef    iRAS,
				     xVoxelRef    oIdx );
Volm_tErr Volm_ConvertIdxToMNITal  ( mriVolumeRef this,
				     xVoxelRef    iIdx,
				     xVoxelRef    oMNITal );
Volm_tErr Volm_ConvertIdxToTal     ( mriVolumeRef this,
				     xVoxelRef    iIdx,
				     xVoxelRef    oTal );
Volm_tErr Volm_ConvertTalToIdx     ( mriVolumeRef this,
				     xVoxelRef    iTal,
				     xVoxelRef    oIdx );
Volm_tErr Volm_ConvertIdxToScanner ( mriVolumeRef this,
				     xVoxelRef    iIdx,
				     xVoxelRef    oScanner );
Volm_tErr Volm_ConvertIdxToMRIIdx  ( mriVolumeRef this,
				     xVoxelRef    iIdx,
				     xVoxelRef    oMRIIdx );

Volm_tErr Volm_GetIdxToRASTransform ( mriVolumeRef     this,
				      mriTransformRef* opTransform );

/* calls the parameter function for every voxel, passing the voxel, the
   value, and the pointer passed to it. */
Volm_tErr Volm_VisitAllVoxels ( mriVolumeRef        this,
				Volm_tVisitFunction iFunc,
				void*               ipData );


Volm_tErr Volm_FindMaxValues ( mriVolumeRef this );

Volm_tErr Volm_MakeColorTable ( mriVolumeRef this );

Volm_tErr Volm_SetBrightnessAndContrast ( mriVolumeRef this,
					  float        ifBrightness,
					  float        ifContrast );

Volm_tErr Volm_SaveToSnapshot      ( mriVolumeRef this );
Volm_tErr Volm_RestoreFromSnapshot ( mriVolumeRef this );

Volm_tErr Volm_Rotate    ( mriVolumeRef     this,
			   mri_tOrientation iAxis,
			   float            ifDegrees );
Volm_tErr Volm_Threshold ( mriVolumeRef     this,
			   float            iThreshold,
			   tBoolean         ibAbove,
			   float            iNewValue );
Volm_tErr Volm_Flip      ( mriVolumeRef     this,
			   mri_tOrientation iAxis );
Volm_tErr Volm_SetAllValues ( mriVolumeRef     this,
			      float            iValue );

Volm_tErr Volm_CopySubjectName ( mriVolumeRef this,
				 char*        oSubjectName,
				 int          inDestLen );
Volm_tErr Volm_CopyVolumeName  ( mriVolumeRef this,
				 char*        oVolumeName,
				 int          inDestLen );
Volm_tErr Volm_CopySourceDir   ( mriVolumeRef this,
				 char*        oSourceDir,
				 int          inDestLen );

Volm_tErr Volm_SetSubjectName           ( mriVolumeRef this,
					  char*        isName );
Volm_tErr Volm_SetVolumeName            ( mriVolumeRef this,
					  char*        isName );
Volm_tErr Volm_ExtractAndSetSubjectName ( mriVolumeRef this,
					  char*        isSource );
Volm_tErr Volm_ExtractAndSetVolumeName  ( mriVolumeRef this,
					  char*        isSource );

/* Sets up internal data structures from an MRI volume and starts
   using the given MRI as its voxel values. */
Volm_tErr Volm_SetFromMRI_ ( mriVolumeRef this,
			     MRI*        iMRI );

/* Note that the functions in this section are implemented as
   functions and macros. The functions are slower but safer, and the
   macros are faster but don't make any checks. So you should test
   with the functions and then build with the macros turned on. And
   keep the macro and functions version synced in
   development. _grin_ Turn the flag on at the top of this file. */
#ifndef VOLM_USE_MACROS

/* Note that all the public functions that refer to 'idx' refer to
   screen idx, or the index in 256^3 space. The local MRI idx might
   not be 256^3, so this conversion takes the idx from 256^3 space to
   the local MRI space. This is done using the m_resample matrix. All
   the access functions of course need to use the local MRI idx. */
void Volm_ConvertScreenIdxToMRIIdx_ ( mriVolumeRef this,
				      xVoxelRef    iScreenIdx,
				      xVoxelRef    oMRIIdx );
void Volm_ConvertMRIIdxToScreenIdx_ ( mriVolumeRef this,
				      xVoxelRef    iMRIIdx,
				      xVoxelRef    oScreenIdx );

/* safer function versions of main accessing and setting functions */
void Volm_ConvertIdxToXYSlice_  ( xVoxelRef         iIdx,
				  mri_tOrientation  iOrientation,
				  xPoint2nRef       oPoint,
				  int*              onSlice );
void Volm_ConvertXYSliceToIdx_  ( mri_tOrientation  iOrientation,
				  xPoint2nRef       iPoint,
				  int               inSlice,
				  xVoxelRef         oIdx );

void Volm_GetValueAtXYSlice_     ( mriVolumeRef this,
				   mri_tOrientation  iOrientation,
				   xPoint2nRef       iPoint,
				   int               inSlice,
				   float*            oValue );
void Volm_GetValueAtIdx_         ( mriVolumeRef      this,
				   xVoxelRef         iIdx,
				   float*            oValue );
void Volm_SetValueAtIdx_         ( mriVolumeRef      this,
				   xVoxelRef         iIdx,
				   float             iValue );
void Volm_GetSincValueAtIdx_     ( mriVolumeRef      this,
				   xVoxelRef         iIdx,
				   Real*             irValue);

void Volm_GetValueAtIdxFrame_    ( mriVolumeRef      this,
				   xVoxelRef         iIdx,
				   int               iFrame,
				   float*            oValue );
void Volm_SetValueAtIdxFrame_    ( mriVolumeRef      this,
				   xVoxelRef         iIdx,
				   int               iFrame,
				   float             iValue );

void Volm_GetMaxValueAtXYSlice_  ( mriVolumeRef this,
				   mri_tOrientation iOrientation, 
				   xPoint2nRef      iPoint,
				   float*           oValue );
void Volm_SetMaxValueAtXYSlice_  ( mriVolumeRef this,
				   mri_tOrientation iOrientation, 
				   xPoint2nRef      iPoint,
				   float            iValue );
int  Volm_GetMaxValueIndex_      ( mriVolumeRef     this,
				   mri_tOrientation iOrientation, 
				   xPoint2nRef      iPoint );


void Volm_ApplyDisplayTransform_ ( mriVolumeRef     this,
				   xVoxelRef        iIdx,
				   xVoxelRef        oIdx );

#else /* VOLM_USE_MACROS */

/* macro version  of main accessing and setting functions */

#define Volm_ConvertScreenIdxToMRIIdx_(this,iScreenIdx,oMRIIdx) \
  *MATRIX_RELT(this->mpTmpScreenIdx,1,1) = (iScreenIdx)->mfX; \
  *MATRIX_RELT(this->mpTmpScreenIdx,2,1) = (iScreenIdx)->mfY; \
  *MATRIX_RELT(this->mpTmpScreenIdx,3,1) = (iScreenIdx)->mfZ; \
  *MATRIX_RELT(this->mpTmpScreenIdx,4,1) = 1.0 ; \
  \
 MatrixMultiply( this->m_resample, this->mpTmpScreenIdx, this->mpTmpMRIIdx ); \
  \
  (oMRIIdx)->mfX = *MATRIX_RELT(this->mpTmpMRIIdx,1,1); \
  (oMRIIdx)->mfY = *MATRIX_RELT(this->mpTmpMRIIdx,1,2); \
  (oMRIIdx)->mfZ = *MATRIX_RELT(this->mpTmpMRIIdx,1,3); \
  \
  if( floor((oMRIIdx)->mfX+0.5) < 0 || floor((oMRIIdx)->mfX+0.5) >= this->mnDimensionX || \
      floor((oMRIIdx)->mfY+0.5) < 0 || floor((oMRIIdx)->mfY+0.5) >= this->mnDimensionY || \
      floor((oMRIIdx)->mfZ+0.5) < 0 || floor((oMRIIdx)->mfZ+0.5) >= this->mnDimensionZ ) { \
    xVoxl_Set( oMRIIdx, 0, 0, 0 ); \
  }

#define Volm_ConvertMRIIdxToScreenIdx_(this,iMRIIdx,oScreenIdx) \
  *MATRIX_RELT(this->mpTmpMRIIdx,1,1) = (iMRIIdx)->mfX; \
  *MATRIX_RELT(this->mpTmpMRIIdx,2,1) = (iMRIIdx)->mfY; \
  *MATRIX_RELT(this->mpTmpMRIIdx,3,1) = (iMRIIdx)->mfZ; \
  *MATRIX_RELT(this->mpTmpMRIIdx,4,1) = 1.0 ; \
 \
  MatrixMultiply( this->m_resample_inv,  \
		  this->mpTmpMRIIdx, this->mpTmpScreenIdx ); \
 \
  (oScreenIdx)->mfX = *MATRIX_RELT(this->mpTmpScreenIdx,1,1); \
  (oScreenIdx)->mfY = *MATRIX_RELT(this->mpTmpScreenIdx,1,2); \
  (oScreenIdx)->mfZ = *MATRIX_RELT(this->mpTmpScreenIdx,1,3); \
 \
  if( (floor((oScreenIdx)->mfX+0.5) < 0 || floor((oScreenIdx)->mfX+0.5) >= 256 || \
       floor((oScreenIdx)->mfY+0.5) < 0 || floor((oScreenIdx)->mfY+0.5) >= 256 || \
       floor((oScreenIdx)->mfZ+0.5) < 0 || floor((oScreenIdx)->mfZ+0.5) >= 256) ) { \
 \
    xVoxl_Set( oScreenIdx, 0, 0, 0 ); \
  }


#define Volm_ConvertIdxToXYSlice_(iIdx,iOrientation,oPoint,onSlice) \
  switch( iOrientation ) {                          \
  case mri_tOrientation_Coronal:                    \
    (oPoint)->mnX = (iIdx)->mfX;                    \
    (oPoint)->mnY = (iIdx)->mfY;                    \
    *(onSlice)    = (iIdx)->mfZ;                    \
    break;                                          \
  case mri_tOrientation_Horizontal:                 \
    (oPoint)->mnX = (iIdx)->mfX;                    \
    (oPoint)->mnY = (iIdx)->mfZ;                    \
    *(onSlice)    = (iIdx)->mfY;                    \
    break;                                          \
  case mri_tOrientation_Sagittal:                   \
    (oPoint)->mnX = (iIdx)->mfZ;                    \
    (oPoint)->mnY = (iIdx)->mfY;                    \
    *(onSlice)    = (iIdx)->mfX;                    \
    break;                                          \
  default:                                          \
    DebugPrintStack;                                \
    DebugPrint( ("Volm_ConvertIdxToXYSlice_ called with invalid " \
     "orientation %d", iOrientation) );               \
    (oPoint)->mnX = (oPoint)->mnY = *(onSlice) = 0;               \
    break;                                          \
  }


#define Volm_ConvertXYSliceToIdx_(iOrientation,iPoint,inSlice,oIdx) \
  switch( iOrientation ) {                                          \
  case mri_tOrientation_Coronal:                                    \
    (oIdx)->mfX = (iPoint)->mnX;                                    \
    (oIdx)->mfY = (iPoint)->mnY;                                    \
    (oIdx)->mfZ = (inSlice);                                        \
    break;                                                          \
  case mri_tOrientation_Horizontal:                                 \
    (oIdx)->mfX = (iPoint)->mnX;                                    \
    (oIdx)->mfY = (inSlice);                                        \
    (oIdx)->mfZ = (iPoint)->mnY;                                    \
    break;                                                          \
  case mri_tOrientation_Sagittal:                                   \
    (oIdx)->mfX = (inSlice);                                        \
    (oIdx)->mfY = (iPoint)->mnY;                                    \
    (oIdx)->mfZ = (iPoint)->mnX;                                    \
    break;                                                          \
  default:                                                          \
    DebugPrintStack;                                                \
    DebugPrint( ("Volm_ConvertXYSliceToIdx_ called with invalid "   \
     "orientation %d", iOrientation) );                 \
    xVoxl_Set( (oIdx), 0, 0, 0 );                                   \
    break;                                                          \
  }

#define Volm_GetValueAtXYSlice_(this,iOrientation,iPoint,inSlice,oValue) \
  switch( iOrientation ) {                                          \
  case mri_tOrientation_Coronal:                                    \
    *(oValue) = MRIvox( this->mpMriValues, (iPoint)->mnX,          \
            (iPoint)->mnY, (inSlice) );                 \
    break;                                                          \
  case mri_tOrientation_Horizontal:                                 \
    *(oValue) = MRIvox( this->mpMriValues, (iPoint)->mnX,          \
            (inSlice), (iPoint)->mnY );                 \
    break;                                                          \
  case mri_tOrientation_Sagittal:                                   \
    *(oValue) = MRIvox( this->mpMriValues, (inSlice),              \
            (iPoint)->mnY, (iPoint)->mnX );             \
    break;                                                          \
  default:                                                          \
    *(oValue) = 0;                                                  \
    break;                                                          \
  }


#define Volm_GetValueAtIdx_(this,iIdx,oValue) \
  Volm_ConvertScreenIdxToMRIIdx_( this, iIdx, &this->mTmpVoxel ); \
 \
  switch( this->mpMriValues->type ) { \
    case MRI_UCHAR: \
      *oValue =  \
	MRIvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		(int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ); \
      break; \
    case MRI_INT: \
      *oValue =  \
	MRIIvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		 (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ); \
      break; \
    case MRI_LONG: \
      *oValue =  \
	MRILvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		 (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ); \
      break; \
    case MRI_FLOAT: \
      *oValue =  \
	MRIFvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		 (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ); \
      break; \
    case MRI_SHORT: \
      *oValue =  \
	MRISvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		 (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ); \
      break; \
    default: \
      *oValue = 0; \
      break ; \
    }

#define Volm_SetValueAtIdx_(this,iIdx,iValue) \
  Volm_ConvertScreenIdxToMRIIdx_( this, iIdx, &this->mTmpVoxel ); \
 \
  switch (this->mpMriValues->type) \
    { \
    default: \
      break ; \
    case MRI_UCHAR: \
      MRIvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	      (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ) =  \
	(BUFTYPE) iValue; \
      break ; \
    case MRI_SHORT: \
      MRISvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	       (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ) =  \
	(short) iValue; \
      break ; \
    case MRI_FLOAT: \
      MRIFvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	       (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ) =  \
	(float) iValue; \
      break ; \
    case MRI_LONG: \
      MRILvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	       (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ) =  \
	(long) iValue; \
      break ; \
    case MRI_INT: \
      MRIIvox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	       (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5) ) =  \
	(int) iValue; \
      break ; \
    }

#define Volm_GetValueAtIdxFrame_(this,iIdx,iFrame,oValue) \
  Volm_ConvertScreenIdxToMRIIdx_( this, iIdx, &this->mTmpVoxel ); \
 \
  switch( this->mpMriValues->type ) { \
    case MRI_UCHAR: \
      *oValue =  \
	MRIseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	   	    (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                    iFrame ); \
      break; \
    case MRI_INT: \
      *oValue =  \
	MRIIseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		     (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                     iFrame ); \
      break; \
    case MRI_LONG: \
      *oValue =  \
	MRILseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		     (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                     iFrame ); \
      break; \
    case MRI_FLOAT: \
      *oValue =  \
	MRIFseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		     (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                     iFrame ); \
      break; \
    case MRI_SHORT: \
      *oValue =  \
	MRISseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
		     (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                     iFrame ); \
      break; \
    default: \
      *oValue = 0; \
      break ; \
    }

#define Volm_SetValueAtIdxFrame_(this,iIdx,iFrame,iValue) \
  Volm_ConvertScreenIdxToMRIIdx_( this, iIdx, &this->mTmpVoxel ); \
 \
  switch (this->mpMriValues->type) \
    { \
    default: \
      break ; \
    case MRI_UCHAR: \
      MRIseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	          (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                  iFrame ) =  \
	(BUFTYPE) iValue; \
      break ; \
    case MRI_SHORT: \
      MRISseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	           (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                   iFrame ) =  \
	(short) iValue; \
      break ; \
    case MRI_FLOAT: \
      MRIFseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	           (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                   iFrame ) =  \
	(float) iValue; \
      break ; \
    case MRI_LONG: \
      MRILseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	           (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                   iFrame ) =  \
	(long) iValue; \
      break ; \
    case MRI_INT: \
      MRIIseq_vox( this->mpMriValues, (int)floor((this->mTmpVoxel).mfX+0.5),  \
	           (int)floor((this->mTmpVoxel).mfY+0.5), (int)floor((this->mTmpVoxel).mfZ+0.5), \
                   iFrame ) =  \
	(int) iValue; \
      break ; \
    }

#define Volm_GetSincValueAtIdx_(this,iIdx,irValue) \
  Volm_ConvertScreenIdxToMRIIdx_( this, iIdx, &this->mTmpVoxel ); \
 \
  MRIsincSampleVolume( this->mpMriValues,  \
		       (this->mTmpVoxel).mfX, \
		       (this->mTmpVoxel).mfY, \
		       (this->mTmpVoxel).mfZ, \
		       2, irValue);
#define Volm_GetMaxValueAtXYSlice_(this,iOrientation,iPoint,oValue) \
   *oValue = \
  this->mpMaxValues[(iOrientation * this->mnDimensionX * this->mnDimensionY) \
                       + ((iPoint)->mnY * this->mnDimensionX) + (iPoint)->mnX]
#define Volm_SetMaxValueAtXYSlice_(this,iOrientation,iPoint,iValue ) \
   this->mpMaxValues[(iOrientation * this->mnDimensionX * this->mnDimensionY) \
               + ((iPoint)->mnY * this->mnDimensionX) + (iPoint)->mnX] = iValue

#define Volm_ApplyDisplayTransform_(this,iIdx,oIdx) \
    Trns_ConvertBtoA( this->mDisplayTransform, iIdx, oIdx )

#endif /* VOLM_USE_MACROS */

/* SOMEBODY changed this code so that the
   Volm_ConvertScreenIdxToMRIIdx_ function automatically sets
   out-of-bounds voxels to 0,0,0, so these functions are pretty much
   worthless because voxels always end up being valid. */
Volm_tErr Volm_Verify     ( mriVolumeRef this );
Volm_tErr Volm_VerifyIdx  ( mriVolumeRef this,
			    xVoxelRef    iIdx );
Volm_tErr Volm_VerifyIdx_ ( mriVolumeRef this,
			    xVoxelRef    iIdx );

/* So here is a function that really does return an error when an
   index is out of bounds. This can be used to actually verify indices
   so you don't end up drawing the value at 0,0,0 when you're outside
   the bounds of the volume. -RKT */
Volm_tErr Volm_VerifyIdxInMRIBounds ( mriVolumeRef this,
				      xVoxelRef    iIdx );

Volm_tErr Volm_VerifyFrame_ ( mriVolumeRef this,
			      int          iFrame );
char* Volm_GetErrorString ( Volm_tErr ieCode );


#endif
