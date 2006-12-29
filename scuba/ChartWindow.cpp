/**
 * @file  ChartWindow.cpp
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 02:09:13 $
 *    $Revision: 1.7 $
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


#include <stdexcept>
#include "ChartWindow.h"
#include <fstream>

using namespace std;

ChartWindowFactory* ChartWindow::sFactory = NULL;

void
ChartWindow::SetFactory ( ChartWindowFactory* iFactory ) {

  sFactory = iFactory;
}

ChartWindow*
ChartWindow::NewChartWindow () {

  if ( NULL != sFactory ) {
    return sFactory->NewChartWindow();
  } else {
    throw runtime_error( "No factory defined" );
  }

  return NULL;
}

ChartWindow::ChartWindow ():
    Broadcaster( "ChartWindow" ),
    mbShowLegend( false ) {}

ChartWindow::~ChartWindow () {
  int id = GetID();
  SendBroadcast( "chartDeleted", (void*)&id );
}

void
ChartWindow::ClearData () {

  mPointData.clear();
  mXAxisMarkerData.clear();
}

void
ChartWindow::SetPointData ( list<PointData>& iaData ) {

  SetPointData( 0, iaData );
}

void
ChartWindow::AddPointData ( PointData& iData ) {

  AddPointData( 0, iData );
}

void
ChartWindow::SetPointData ( int inGroup, list<PointData>& iaData ) {

  InitGroupDataIfNotSet( inGroup );

  mPointData[inGroup] = iaData;
}

void
ChartWindow::AddPointData ( int inGroup,PointData& iData ) {

  InitGroupDataIfNotSet( inGroup );

  mPointData[inGroup].push_back( iData );
}

void
ChartWindow::SetGroupLabel ( int inGroup, string isLabel ) {

  InitGroupDataIfNotSet( inGroup );

  mGroupData[inGroup].msLabel = isLabel;
}

void
ChartWindow::SetGroupConnected ( int inGroup, bool ibConnected ) {

  InitGroupDataIfNotSet( inGroup );

  mGroupData[inGroup].mbConnected = ibConnected;
}

void
ChartWindow::SetGroupColor ( int inGroup, int iColorRGBi[3] ) {

  InitGroupDataIfNotSet( inGroup );

  mGroupData[inGroup].mColorRGBi[0] = iColorRGBi[0];
  mGroupData[inGroup].mColorRGBi[1] = iColorRGBi[1];
  mGroupData[inGroup].mColorRGBi[2] = iColorRGBi[2];
}

void
ChartWindow::GetGroupColor ( int inGroup, int oColorRGBi[3] ) {

  InitGroupDataIfNotSet( inGroup );

  oColorRGBi[0] = mGroupData[inGroup].mColorRGBi[0];
  oColorRGBi[1] = mGroupData[inGroup].mColorRGBi[1];
  oColorRGBi[2] = mGroupData[inGroup].mColorRGBi[2];
}

void
ChartWindow::SetTitle ( string isTitle ) {

  msTitle = isTitle;
}

void
ChartWindow::SetXAxisLabel ( string isLabel ) {

  msXLabel = isLabel;
}

void
ChartWindow::SetYAxisLabel ( string isLabel ) {

  msYLabel = isLabel;
}

void
ChartWindow::SetInfo ( string isInfo ) {

  msInfo = isInfo;
}

void
ChartWindow::SetXAxisMarkers ( list<MarkerData>& iaData ) {

  mXAxisMarkerData = iaData;
}

void
ChartWindow::AddXAxisMarker  ( MarkerData& iData ) {

  mXAxisMarkerData.push_back( iData );
}

void
ChartWindow::GenerateReport ( string ifnReport,
                              bool ibIncludeGroupColumn,
                              bool ibIncludeLabelColumn,
                              bool ibIncludeXColumn,
                              bool ibIncludeYColumn ) {

  try {
    // Check file name first.
    ofstream fReport( ifnReport.c_str(), ios::out );

    map<int,list<PointData> >::iterator tGroup;
    for ( tGroup = mPointData.begin();
          tGroup != mPointData.end();
          ++tGroup ) {

      int nGroup = tGroup->first;
      list<PointData>& lPoints = tGroup->second;

      list<PointData>::iterator tPoint;
      for ( tPoint = lPoints.begin();
            tPoint != lPoints.end();
            ++tPoint ) {

        PointData& point = (*tPoint);

        // Output the data they want.
        if ( ibIncludeGroupColumn &&
             mGroupData[nGroup].msLabel != "" ) {
          fReport << mGroupData[nGroup].msLabel << "\t";
        }
        if ( ibIncludeLabelColumn &&
             point.msLabel != "" ) {
          fReport << point.msLabel << "\t";
        }
        if ( ibIncludeXColumn ) {
          fReport << point.mX << "\t";
        }
        if ( ibIncludeYColumn ) {
          fReport << point.mY;
        }
        fReport << endl;
      }
    }
  } catch ( exception& e ) {
    throw runtime_error( "Error writing " + ifnReport + ": " + e.what() );
  } catch ( ... ) {
    throw runtime_error( "Error writing " + ifnReport );
  }
}

void
ChartWindow::InitGroupDataIfNotSet ( int inGroup ) {

  if ( mGroupData.find( inGroup ) == mGroupData.end() ) {
    GroupData groupData;
    groupData.mbConnected = false;
    // Default colors
    switch ( inGroup % 8 ) {
    case 0:
      // Red
      groupData.mColorRGBi[0] = 255;
      groupData.mColorRGBi[1] = 0;
      groupData.mColorRGBi[2] = 0;
      break;
    case 1:
      // Green
      groupData.mColorRGBi[0] = 0;
      groupData.mColorRGBi[1] = 255;
      groupData.mColorRGBi[2] = 0;
      break;
    case 2:
      // Blue
      groupData.mColorRGBi[0] = 0;
      groupData.mColorRGBi[1] = 0;
      groupData.mColorRGBi[2] = 255;
      break;
    case 3:
      // Purple
      groupData.mColorRGBi[0] = 255;
      groupData.mColorRGBi[1] = 0;
      groupData.mColorRGBi[2] = 255;
      break;
    case 4:
      // Brown
      groupData.mColorRGBi[0] = 181;
      groupData.mColorRGBi[1] = 42;
      groupData.mColorRGBi[2] = 42;
      break;
    case 5:
      // Pink
      groupData.mColorRGBi[0] = 255;
      groupData.mColorRGBi[1] = 200;
      groupData.mColorRGBi[2] = 200;
      break;
    case 6:
      // Lightblue
      groupData.mColorRGBi[0] = 174;
      groupData.mColorRGBi[1] = 230;
      groupData.mColorRGBi[2] = 240;
      break;
    case 7:
      // Yellow
      groupData.mColorRGBi[0] = 255;
      groupData.mColorRGBi[1] = 255;
      groupData.mColorRGBi[2] = 0;
      break;
    }
    SetGroupData( inGroup, groupData );
  }
}

void
ChartWindow::SetGroupData ( int inGroup, GroupData& iData ) {

  mGroupData[inGroup] = iData;
}
