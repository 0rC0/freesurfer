/**
 * @file  MyUtils.h
 * @brief Misc utility class.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2008/04/01 22:18:49 $
 *    $Revision: 1.3 $
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

#ifndef MyUtils_h
#define MyUtils_h

#include <math.h>
#include <wx/wx.h>

class MyUtils 
{
public:
	MyUtils() {}
	
	static void FloodFill( char** data, int x, int y, int min_x, int min_y, int max_x, int max_y, int fill_value, int border_value );
	
	template <class T> static double GetDistance( T* pt1, T* pt2 );
	template <class T> static void GetVector( T* pt1, T* pt2, double* v_out ); 
	template <class T> static bool Equal( T* pt1, T* p2, int n = 3 );
	template <class T> static T** AllocateMatrix(int ny, int nx);
	template <class T> static void FreeMatrix(T** p, int ny);
	
	static bool HasExtension( const wxString& filename, const wxString& ext );
	
	static wxString GetNormalizedPath( const wxString& filename );
	static wxString GetNormalizedFullPath( const wxString& filename );
};
	

template <class T>
		double MyUtils::GetDistance( T* pt1, T* pt2 )
{
	double dRes = 0;
	for ( int i = 0; i < 3; i++ )
	{
		dRes += ( pt2[i] - pt1[i] ) * ( pt2[i] - pt1[i] );
	}
	
	return sqrt( dRes );
}

template <class T> 
		void MyUtils::GetVector( T* pt1_in, T* pt2_in, double* v_out )
{
	T pt1[3], pt2[3];
	for ( int i = 0; i < 3; i++ )
	{
		pt1[i] = pt1_in[i];
		pt2[i] = pt2_in[i];
	}
	
	double dist = GetDistance( pt1, pt2 );
	if ( dist == 0 )
		return;
	
	for ( int i = 0; i < 3; i++ )
	{
		v_out[i] = ( (double)pt2[i] - (double)pt1[i] ) / dist;
	}
} 

template <class T>
		bool MyUtils::Equal( T* pt1, T* pt2, int nLength )
{
	for ( int i = 0; i < nLength; i++ )
	{
		if ( pt1[i] != pt2[i] )
			return false;
	}
	
	return true;
}

template <class T>
		T** MyUtils::AllocateMatrix(int ny, int nx)
{
	T** p = new T*[ny];
	if (!p)
		return 0;
		
	for (int i = 0; i < ny; i++)
	{
		p[i] = new T[nx];
		if (!p[i])
		{
			for (int j = 0; j < i; j++)
				delete[] p[j];
			delete[] p;
			return 0;
		}
	}

	return p;
}

template <class T>
		void MyUtils::FreeMatrix(T** p, int ny)
{
	if (p == 0)
		return;
	for (int i = 0; i < ny; i++)
		delete[] p[i];
	delete[] p;
	p = 0;
}

#endif
