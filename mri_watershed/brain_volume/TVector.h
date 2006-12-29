/**
 * @file  TVector.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 02:09:10 $
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


//
// TVector class
//
// writte by y.tosa
// March 14th, 2003
//
// easy vector calculation
//

// multiple inclusion protection
#ifndef c_tvector_h
#define c_tvector_h

#include <iostream>

#ifdef __cplusplus

class TVector {

public:
  double x;
  double y;
  double z;

  explicit TVector():x(0.), y(0.), z(0.) {}
  TVector(double xx, double yy, double zz):x(xx), y(yy), z(zz) {}

  // compound *=
  TVector &operator*=(const double &c) {
    x*=c;
    y*=c;
    z*=c;
    return *this;
  }
  TVector &operator/=(const double &c) {
    x/=c;
    y/=c;
    z/=c;
    return *this;
  }

  // compound +=, -=
  TVector &operator+=(const TVector &v) {
    x+=v.x;
    y+=v.y;
    z+=v.z;
    return *this;
  }
  TVector &operator-=(const TVector &v) {
    x-=v.x;
    y-=v.y;
    z-=v.z;
    return *this;
  }

};

// multiply V*c
TVector operator*(const TVector &a, const double &c) {
  return TVector(a.x*c, a.y*c, a.z*c);
}

// divide V/c
TVector operator/(const TVector &a, const double &c) {
  return TVector(a.x/c, a.y/c, a.z/c);
}

// multiply c*V
TVector operator*(double c, const TVector &a) {
  return TVector(a.x*c, a.y*c, a.z*c);
}

// add vector
TVector operator+(const TVector &a, const TVector &b) {
  double x = a.x + b.x;
  double y = a.y + b.y;
  double z = a.z + b.z;
  return TVector(x,y,z);
}

// subtract vector
TVector operator-(const TVector &a, const TVector &b) {
  double x = a.x - b.x;
  double y = a.y - b.y;
  double z = a.z - b.z;
  return TVector(x,y,z);
}

// inner product
double operator*(const TVector &a, const TVector &b) {
  return (a.x*b.x +a.y*b.y + a.z*b.z);
}

// outer product
TVector operator^(const TVector &a, const TVector &b) {
  return TVector(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

// equality
bool operator==(const TVector &a, const TVector &b) {
  return ((a.x == b.x) && (a.y == b.y) && (a.z == b.z));
}

// output
std::ostream &operator<<(std::ostream &s, const TVector &v) {
  s<< "( " << v.x << ", " << v.y << ", " << v.z << ") ";
  return s;
}

#endif // __cplusplus
#endif // multiple inclusion protection
