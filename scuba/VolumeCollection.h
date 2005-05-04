#ifndef VolumeCollection_H
#define VolumeCollection_H

#include "string_fixed.h"
#include "DataCollection.h"
#include "ScubaROIVolume.h"
extern "C" {
#ifdef Status
#undef Status
#endif
#include "mri.h"
#ifdef X
  #undef X 
#endif
#ifdef Y
  #undef Y
#endif
#ifdef Z
  #undef Z
#endif
}
#include "Volume3.h"
#include "Point3.h"
#include "ScubaTransform.h"
#include "Broadcaster.h"
#include "VectorOps.h"

class VolumeCollection;

class VolumeLocation : public DataLocation {
  friend class VolumeCollectionTester;
  friend class VolumeCollection;
 public:
  VolumeLocation ( VolumeCollection& iVolume, float const iRAS[3] );
  VolumeLocation ( VolumeCollection& iVolume, int const iIndex[3] );
  ~VolumeLocation () {}
  int* Index() { return mIdxi; }
  float* IndexF() { return mIdxf; }
  void SetFromRAS( float const iRAS[3] );
 protected:
  VolumeCollection& mVolume;
  int mIdxi[3];
  float mIdxf[3];
};

class VolumeCollection : public DataCollection {

  friend class VolumeCollectionTester;
  friend class VolumeCollectionFlooder;

 public:
  VolumeCollection ();
  virtual ~VolumeCollection ();

  virtual DataLocation& MakeLocationFromRAS ( float const iRAS[3] );
  DataLocation& MakeLocationFromIndex ( int const iIndex[3] );

  // Should return a type description unique to the subclass.
  virtual std::string GetTypeDescription() { return "Volume"; }

  // Set the file name for this ROI. This will be used to load the MRI
  // and to save it.
  void SetFileName ( std::string& ifnMRI );
  std::string GetFileName () { return mfnMRI; }

  // Creates an MRI using an existing data collection as a template.
  void MakeUsingTemplate ( int iCollectionID );

  // Load an MRI from the currently named volume.
  void LoadVolume ();

  // Get the MRI. If we're loading an MRI, this requires the MRI to be
  // set first.
  MRI* GetMRI ();

  // Saves the current volume to a file name. If none is given, uses
  // the set file name.
  void Save ();
  void Save ( std::string ifn );
  

  // Accessors.
  float GetMRIMinValue () { return mMRIMinValue; }
  float GetMRIMaxValue () { return mMRIMaxValue; }

  float GetMRIMagnitudeMinValue (); // (Calls MakeMagnitudeVolume if necessary)
  float GetMRIMagnitudeMaxValue ();

  float GetVoxelXSize () { return mVoxelSize[0]; }
  float GetVoxelYSize () { return mVoxelSize[1]; }
  float GetVoxelZSize () { return mVoxelSize[2]; }

  void GetRASRange ( float oRASRange[6] );
  void GetMRIIndexRange ( int oMRIIndexRange[3] );

  // Coordinate conversion.
  void RASToMRIIndex ( float const iRAS[3], int oIndex[3] );
  void RASToMRIIndex ( float const iRAS[3], float oIndex[3] );
  void MRIIndexToRAS ( int const iIndex[3], float oRAS[3] );
  void MRIIndexToRAS ( float const iIndex[3], float oRAS[3] );
  void RASToDataRAS  ( float const iRAS[3], float oDataRAS[3] );

  // Bounds testing.
  bool IsInBounds ( VolumeLocation& iLoc );

  // Calculates values.
  float GetMRINearestValue   ( VolumeLocation& iLoc );
  float GetMRITrilinearValue ( VolumeLocation& iLoc );
  float GetMRISincValue      ( VolumeLocation& iLoc );
  float GetMRIMagnitudeValue ( VolumeLocation& iLoc );
  
  // Sets value.
  void SetMRIValue ( VolumeLocation& iLoc, float iValue );
  
  virtual TclCommandResult
    DoListenToTclCommand ( char* isCommand, int iArgc, char** iasArgv );

  virtual void
    DoListenToMessage ( std::string isMessage, void* iData );

  // Create an ROI for this data collection.
  virtual ScubaROI* DoNewROI ();


  // Select or unselect in the current ROI.
  void Select   ( VolumeLocation& iLoc );
  void Unselect ( VolumeLocation& iLoc );

  // Return whether or not an ROI is at this point, and if so, returns
  // the color. If multiple ROIs are present, blends the color.
  bool IsSelected ( VolumeLocation& iLoc, int oColor[3] );

  // Return whether or not an ROI is present other than the one passed
  // in.
  bool IsOtherRASSelected ( float iRAS[3], int iThisROIID );


  // Writes an ROI to a label file.
  void WriteROIToLabel ( int iROIID, std::string ifnLabel );

  // Creates a new ROI in this data collection from a label file.
  int  NewROIFromLabel ( std::string ifnLabel );

  // Writes all structure ROIs to a segmentation volume file.
  void WriteROIsToSegmentation ( std::string ifnVolume );


  // Sets the data to world transform, basically a user settable
  // display transform.
  virtual void SetDataToWorldTransform ( int iTransformID );

  // Returns the combined world to index transform.
  Matrix44& GetWorldToIndexTransform ();

  // Enable or disable world to index transform.
  void SetUseWorldToIndexTransform ( bool ibUse );
  bool GetUseWorldToIndexTransform () { return mbUseDataToIndexTransform; }

  // Finds and returns RAS points in a square area. Each RAS will
  // translate to a unique voxel. INPUT POINTS MUST BE IN CLOCKWISE OR
  // COUNTERCLOCKWISE ORDER.
  void FindRASPointsInSquare ( float iCenter[3],
			       float iPointA[3], float iPointB[3],
			       float iPointC[3], float iPointD[3],
			       float iMaxDistance, // optional
			       std::list<Point3<float> >& oPoints );
  void FindRASPointsInCircle ( float iPointA[3], float iPointB[3],
			       float iPointC[3], float iPointD[3],
			       float iMaxDistance, // optional
			       float iCenter[3], float iRadius,
			       std::list<Point3<float> >& oPoints );

  // Import and export points to a control point file.
  void ImportControlPoints ( std::string ifnControlPoints,
			     std::list<Point3<float> >& oControlPoints);
  void ExportControlPoints ( std::string ifnControlPoints,
			     std::list<Point3<float> >& iControlPoints );


  // Return a histogram from the RAS voxels passed in.
  void MakeHistogram ( std::list<Point3<float> >& iRASPoints, int icBins,
		       float& oMinBinValue, float& oBinIncrement,
		       std::map<int,int>& oBinCounts );

  // Return a histogram for the entire volume.
  void MakeHistogram ( int icBins, 
		       float iMinThresh, float iMaxThresh,
		       float& oMinBinValue, float& oBinIncrement,
		       std::map<int,int>& oBinCounts );

  virtual void DataChanged ();

  // For autosaving.
  void SetAutoSaveOn ( bool ibAutosave ) { mbAutosave = ibAutosave; }
  bool GetAutoSaveOn () { return mbAutosave; }

  bool IsAutosaveDirty () { return mbAutosaveDirty; }
  void AutosaveIfDirty ();

  // Given an MRI voxel index and a plane in RAS coords and the
  // direction to extend the voxel, returns whether or not the voxel
  // intersects the plane. Tests each of the voxel's edges against the
  // plane.
  VectorOps::IntersectionResult VoxelIntersectsPlane
    ( Point3<int>& iMRIIndex, int iIncrement,
      Point3<float>& iPlaneRAS, Point3<float>& iPlaneRASNormal,
      Point3<float>& oIntersectionRAS ); 

  void PrintVoxelCornerCoords ( std::ostream& iStream,
				Point3<int>& iMRIIdx );

protected:

  // Gets information from the MRI structure.
  void InitializeFromMRI ();

  // Update the value range.
  void UpdateMRIValueRange ();

  // Calculates the final world to index transfrom from the data to
  // index transform and the world to index transform.
  void CalcWorldToIndexTransform ();

  // Calculates the magnitude volume and saves it in mMagnitudeMRI.
  void MakeMagnitudeVolume ();

  // Faster way of getting values.
  float GetMRINearestValueAtIndexUnsafe ( int iIndex[3] );


  // Filename to use when reading or writing.
  std::string mfnMRI;

  // The actual MRI volume.
  MRI* mMRI;

  // The magnitude volume, used for the edge line tool. This is
  // generated by MakeMagnitudeVolume whenever a magnitude value is
  // set.
  MRI* mMagnitudeMRI;

  // We're dealing with two transforms here, data->world which is
  // defined in DataCollection, and data->index which is defined
  // here. Actually it goes world->data->index and
  // index->data->world. We composite these in the last transform.
  Transform44 mDataToIndexTransform;
  Transform44 mWorldToIndexTransform;

  bool mbUseDataToIndexTransform;

  // For the autosave volume.
  bool mbAutosave;
  std::string MakeAutosaveFileName ( std::string& ifn );
  void DeleteAutosave();
  bool mbAutosaveDirty;
  std::string mfnAutosave;

  // Voxel sizes.
  float mVoxelSize[3];

  // MRI and magnitude volume ranges.
  float mMRIMinValue, mMRIMaxValue;
  float mMRIMagMinValue, mMRIMagMaxValue;

  // The selected voxel cache.
  void InitSelectionVolume ();
  Volume3<bool>* mSelectedVoxels;
};

class VolumeCollectionFlooder {
 public:
  VolumeCollectionFlooder ();
  virtual ~VolumeCollectionFlooder ();

  // Override these to do startup and shutdown stuff, also allow the
  // user or program to cancel the flood.
  virtual void DoBegin ();  
  virtual void DoEnd ();
  virtual bool DoStopRequested ();

  virtual bool CompareVoxel ( float iRAS[3] );
  virtual void DoVoxel ( float iRAS[3] );

  class Params {
  public:
    Params ();
    int mSourceCollection;
    bool mbStopAtPaths;
    bool mbStopAtROIs;
    bool mb3D;
    bool mbWorkPlaneX;
    bool mbWorkPlaneY;
    bool mbWorkPlaneZ;
    float mViewNormal[3];
    int mFuzziness;
    int mMaxDistance;
    bool mbDiagonal;
    bool mbOnlyZero;
    enum FuzzinessType { seed, gradient };
    FuzzinessType mFuzzinessType;
  };

  class CheckPair {
  public:
    CheckPair() {}
    CheckPair( Point3<int>& iSource, Point3<int>& iCheck ) {
      mSourceIndex = iSource; mCheckIndex = iCheck;
    }
    Point3<int> mSourceIndex;
    Point3<int> mCheckIndex;
  };    

  void Flood ( VolumeCollection& iVolume, float iRASSeed[3], Params& iParams );
  VolumeCollection* mVolume;
  Params* mParams;
};


#endif
