/**
 * @file  vtkFSSurfaceLabelSource.h
 * @brief Reads a label, maps it to a surface, and outputs PolyData
 *
 * A FreeSurfer label file consists of a list of vertices that may
 * also have associated vertex indices. This will read in a label, map
 * it to a surface, fill in any holes, and output PolyData, which will
 * appear to be a subset of the surface PolyData.
 */
/*
 * Original Author: Kevin Teich
 * CVS Revision Info:
 *    $Author: kteich $
 *    $Date: 2007/05/15 19:07:07 $
 *    $Revision: 1.1 $
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

#include <map>
#include <vector>
#include <stdexcept>
#include "vtkFSSurfaceLabelSource.h"
#include "vtkFSSurfaceSource.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"

using namespace std;

vtkStandardNewMacro( vtkFSSurfaceLabelSource );
vtkCxxRevisionMacro( vtkFSSurfaceLabelSource, "$Revision: 1.1 $" );

vtkFSSurfaceLabelSource::vtkFSSurfaceLabelSource() {

  this->vtkSource::SetNthOutput(0, vtkPolyData::New());

  // Releasing data for pipeline parallism.  Filters will know it is
  // empty.
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

vtkFSSurfaceLabelSource::~vtkFSSurfaceLabelSource () {

}

void
vtkFSSurfaceLabelSource::ReadLabel ( MRIS* iSurface, char const* ifnLabel ) {

  assert( iSurface );
  assert( ifnLabel );

  LABEL* label;

  try {

    MRIS* mris = iSurface;
    
    // Load the white vertex positions in the surface.
    int eMRIS = MRISreadWhiteCoordinates( mris, "white" );
    if( eMRIS != 0 )
      throw runtime_error( "Couldn't read the white surface file, so unable to load a label." );
    
    // Load the label file.
    char* fnLabel = strdup( ifnLabel );
    label = LabelRead( NULL, fnLabel );
    free( fnLabel );
    if( NULL == label )
      throw runtime_error( "Couldn't read label" );
    
    // Map it to the surface using the white coordinates.
    LabelToWhite( label, mris );
    
    // See if it's completely unassigned. If so, there are likely to
    // be lots of holes in the label after we sample it to the
    // vertices, so we'll fill it in afterwards.
    int unassigned;
    LabelIsCompletelyUnassigned( label, &unassigned );
    
    // Assign the mris vertex numbers to unnumbered vertices in the
    // label.
    LabelFillUnassignedVertices( mris, label, WHITE_VERTICES );
    
    // If we were unassigned before, fill the holes now.
    if( unassigned ) {
      LABEL* filledLabel = LabelFillHoles( label, mris, WHITE_VERTICES );
      LabelFree( &label );
      label = filledLabel;
    }
    
    // We use marks to figure out where the label is on the surface.
    MRISclearMarks( mris );
    LabelMark( label, mris );
    
    // Now we have a marked surface. What we want to do is create a
    // new poly data object without only marked faces in it. To do
    // that, we need a list of points in the faces, and make faces
    // that index those points. However we need to map the surface
    // based indices to a new range of indices that only have the
    // selected points. So we'll save a list of points and faces that
    // are selected using the old indices, and then create a mapping.
    
    // For each face in the surface, if all its vertices are marked,
    // put those points and the face in our list of ones that will go
    // into the new poly data.
    map<vtkIdType,map<int,double> > aPoints;
    map<vtkIdType,map<int,vtkIdType> > aPolys;
    int nPoly = 0;
    for( int nFace = 0; nFace < mris->nfaces; nFace++ ) {
      if( mris->vertices[mris->faces[nFace].v[0]].marked &&
	  mris->vertices[mris->faces[nFace].v[1]].marked &&
	  mris->vertices[mris->faces[nFace].v[2]].marked ) {
	
	// We're going to make an entry for a poly with a 0-based face
	// index, but using the same vertex index as the original
	// surface.
	for( int nFaceVertex = 0; 
	     nFaceVertex < VERTICES_PER_FACE; nFaceVertex++ ) {
	  
	  int nVertex = mris->faces[nFace].v[nFaceVertex];
	  
	  // Save the poly using the 0 based index.
	  aPolys[nPoly][nFaceVertex] = nVertex;
	  
	  // Save the point with the original index. This can be
	  // redundant, we don't care.
	  aPoints[nVertex][0] = mris->vertices[nVertex].x;
	  aPoints[nVertex][1] = mris->vertices[nVertex].y;
	  aPoints[nVertex][2] = mris->vertices[nVertex].z;
	}
	
	nPoly++;
      }
    }
    
    // Now lets make point and poly lists out of the maps we just
    // made.
    vtkPoints* labelPoints = vtkPoints::New();
    labelPoints->SetNumberOfPoints( aPoints.size() );
    
    vtkCellArray* labelPolys = vtkCellArray::New();
    labelPolys->Allocate( aPolys.size() );
    
    vtkIdType nNextNewID = 0;
    map<vtkIdType,vtkIdType> aOldIDToNewID;
    
    // For each point we saved, we need to map its surface based index
    // to a new 0 based index.
    map<vtkIdType,map<int,double> >::iterator tPoint;
    for( tPoint = aPoints.begin(); tPoint != aPoints.end(); ++tPoint ) {
      
      // Get the old ID.
      vtkIdType oldID = tPoint->first;
      
      // Build a point.
      double point[3];
      for( int n = 0; n < 3; n++ )
	point[n] = tPoint->second[n];
      
      // Map the old ID to a new ID and save it.
      aOldIDToNewID[oldID] = nNextNewID++;
      
      // Insert the point with the new ID.
      labelPoints->SetPoint( aOldIDToNewID[oldID], point );
    }
    
    // Now for each poly, add it to the polys array using the new 0
    // based point indices.
    map<vtkIdType,map<int,int> >::iterator tPoly;
    for( tPoly = aPolys.begin(); tPoly != aPolys.end(); ++tPoly ) {
      
      // Make a poly using the new IDs.
      vtkIdType poly[3];
      for( int n = 0; n < 3; n++ )
	poly[n] = aOldIDToNewID[tPoly->second[n]];
      
      // Insert the poly.
      labelPolys->InsertNextCell( 3, poly );
    }
    
    // Set the points and polys in our output
    this->GetOutput()->SetPoints( labelPoints );
    labelPoints->Delete();
    
    this->GetOutput()->SetPolys( labelPolys );
    labelPolys->Delete();
    
  }
  catch( exception& e ) {
    
    if( label )
      LabelFree( &label );
    
    throw e;
  }
  
  if( label )
    LabelFree( &label );
}

void
vtkFSSurfaceLabelSource::ReadLabel ( vtkFSSurfaceSource* iSurface,
				     char const* ifnLabel ) {

  assert( iSurface );
  assert( ifnLabel );

  this->ReadLabel( iSurface->GetMRIS(), ifnLabel );
}

vtkPolyData*
vtkFSSurfaceLabelSource::GetOutput () {

  if ( this->NumberOfOutputs < 1 ) {
    return NULL;
  }

  return (vtkPolyData *)(this->Outputs[0]);
}

vtkPolyData*
vtkFSSurfaceLabelSource::GetOutput ( int inOutput ) {

  return (vtkPolyData *)this->vtkSource::GetOutput( inOutput );
}

void
vtkFSSurfaceLabelSource::SetOutput ( vtkPolyData* iOutput ) {

  this->vtkSource::SetNthOutput( 0, iOutput );
}
