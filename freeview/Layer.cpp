/**
 * @file  Layer.cpp
 * @brief Base Layer class. A layer is an independent data object with 2D and 3D graphical representations.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2010/05/06 21:17:12 $
 *    $Revision: 1.15 $
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

#include "wx/wx.h"
#include "Layer.h"
#include "LayerProperties.h"
#include <math.h>

#define CLOSE_DISTANCE 1e-6

Layer::Layer() : Listener( "Layer" ), Broadcaster( "Layer" )
{
  for ( int i = 0; i < 3; i++ )
  {
    m_dSlicePosition[i] = 0;
    m_dWorldOrigin[i] = 0;
    m_dWorldVoxelSize[i] = 1;
    m_dWorldSize[i] = 0;
  }
  m_bLocked = false;
  mProperties = NULL;
}

Layer::~Layer()
{
  if ( mProperties )
    delete mProperties;
  
  SendBroadcast( "LayerObjectDeleted", this );
}

void Layer::SetName( const char* name )
{
  if ( m_strName != name )
  {
    m_strName = name;
    SendBroadcast( "LayerNameChanged", this );
  }
}

bool Layer::IsTypeOf( std::string tname )
{
  for ( size_t i = 0; i < m_strTypeNames.size(); i++ )
  {
    if ( m_strTypeNames[i] == tname )
      return true;
  }
  return false;
}

std::string Layer::GetEndType()
{
  if ( m_strTypeNames.size() > 0 )
    return m_strTypeNames[ m_strTypeNames.size()-1 ];
  else
    return "";
}

double* Layer::GetWorldOrigin()
{
  return m_dWorldOrigin;
}

void Layer::GetWorldOrigin( double* origin )
{
  for ( int i = 0; i < 3; i++ )
    origin[i] = m_dWorldOrigin[i];
}

void Layer::SetWorldOrigin( double* origin )
{
  for ( int i = 0; i < 3; i++ )
    m_dWorldOrigin[i] = origin[i];
}

double* Layer::GetWorldSize()
{
  return m_dWorldSize;
}

void Layer::GetWorldSize( double* size )
{
  for ( int i = 0; i < 3; i++ )
    size[i] = m_dWorldSize[i];
}

void Layer::SetWorldSize( double* size )
{
  for ( int i = 0; i < 3; i++ )
    m_dWorldSize[i] = size[i];
}

double* Layer::GetWorldVoxelSize()
{
  return m_dWorldVoxelSize;
}

void Layer::GetWorldVoxelSize( double* vs )
{
  for ( int i = 0; i < 3; i++ )
    vs[i] = m_dWorldVoxelSize[i];
}

void Layer::SetWorldVoxelSize( double* vs )
{
  for ( int i = 0; i < 3; i++ )
    m_dWorldVoxelSize[i] = vs[i];
}

double* Layer::GetSlicePosition()
{
  return m_dSlicePosition;
}

void Layer::GetSlicePosition( double* slicePos )
{
  for ( int i = 0; i < 3; i++ )
  {
    slicePos[i] = m_dSlicePosition[i];
  }
}

void Layer::SetSlicePosition( double* slicePos )
{
  this->BlockBroadcast( true );
  for ( int i = 0; i < 3; i++ )
  {
    SetSlicePosition( i, slicePos[i] );
  }
  this->BlockBroadcast( false );
//  this->SendBroadcast( "SlicePositionChanged", this );
//  this->SendBroadcast( "LayerActorUpdated", this );
}

void Layer::SetSlicePosition( int nPlane, double slicePos )
{
  wxASSERT( nPlane >= 0 && nPlane <= 2 );

  if ( fabs( slicePos - m_dSlicePosition[ nPlane ] ) > CLOSE_DISTANCE )
  {
    m_dSlicePosition[nPlane] = slicePos;
//    this->SendBroadcast( "SlicePositionChanged", this );
    OnSlicePositionChanged( nPlane );
//    this->SendBroadcast( "LayerActorUpdated", this );
  }
}

void Layer::RASToVoxel( const double* pos, int* n )
{
  for ( int i = 0; i < 3; i++ )
  {
    n[i] = ( int )( ( pos[i] - m_dWorldOrigin[i] ) / m_dWorldVoxelSize[i] + 0.5 );
  }
}

void Layer::VoxelToRAS( const int* n, double* pos )
{
  for ( int i = 0; i < 3; i++ )
  {
    pos[i] = m_dWorldOrigin[i] + m_dWorldVoxelSize[i] * n[i];
  }
}

void Layer::Lock( bool bLock )
{
  m_bLocked = bLock;
  this->SendBroadcast( "LayerLockChanged", this );
}

void Layer::DoListenToMessage( std::string const iMessage, void* iData, void* sender )
{
  if ( iMessage == _( "ShowInfoChanged" ) )
    this->SendBroadcast( "LayerShowInfoChanged", iData, this );
}

void Layer::GetBounds( double* bounds )
{
  double* origin = GetWorldOrigin();
  double* size = GetWorldSize();
  for ( int i = 0; i < 3; i++ )
  {
    bounds[i*2] = origin[i];
    bounds[i*2+1] = origin[i] + size[i];
  }
}
