/**
 * @file  Layer.cpp
 * @brief Layer data object.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2008/03/27 18:12:15 $
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
 
#include "wx/wx.h"
#include "Layer.h"
#include <math.h>

#define CLOSE_DISTANCE	1e-6

Layer::Layer() : Listener( "Layer" ), Broadcaster( "Layer" )
{
	for ( int i = 0; i < 3; i++ )
	{
		m_dSlicePosition[i] = 0;
		m_dWorldOrigin[i] = 0;
		m_dWorldVoxelSize[i] = 1;
		m_dWorldSize[i] = 0;
	}
}

Layer::~Layer()
{
}

bool Layer::IsTypeOf( std::string tname )
{
	for ( int i = 0; i < (int)m_strTypeNames.size(); i++ )
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
	this->SendBroadcast( "LayerActorUpdated", this );
}

void Layer::SetSlicePosition( int nPlane, double slicePos )
{
	wxASSERT( nPlane >= 0 && nPlane <= 2 );
	
	char* strPlaneName[] = { "X", "Y", "Z" };
	if ( fabs( slicePos - m_dSlicePosition[ nPlane ] ) > CLOSE_DISTANCE ) 
	{
		m_dSlicePosition[nPlane] = slicePos;
		this->SendBroadcast( std::string("SlicePositionChanged") + strPlaneName[nPlane], this );
		OnSlicePositionChanged( nPlane );
		this->SendBroadcast( "LayerActorUpdated", this );
	}
}


