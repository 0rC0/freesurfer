/**
* @file   mri_copy_params.cpp
* @author Yasunari Tosa
* @date   Tue Sep 28 11:25:08 2004
* 
* @brief  copy acq params from some volue to create a new volume
*         
* 
*/

#include <iostream>
#if (__GNUC__ < 3)
#include "/usr/include/g++-3/alloc.h"
#endif
#include <string>

extern "C" {
#include "mri.h"
  char *Progname = "mri_copy_params";
}

using namespace std;

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    cout << "Usage  : mri_copy_params <input volume> <like volume> [<output volume>]" << endl;
    cout << "         if the output volume is not given, it will overwrite input volume " << endl;
    cout << "Purpose: copies TR, TE, TI, and flip angle from <like volume>" << endl;
    return 0;
  }
  string invol;
  string likevol;
  string outvol;

  if (argc == 2)
  {
    invol = argv[1];
    likevol = argv[2];
    outvol = argv[1];
  }
  else if (argc ==3)
  {
    invol = argv[1];
    likevol = argv[2];
    outvol = argv[1];
  }
  MRI *imri = MRIread(const_cast<char *> (invol.c_str()));
  if (!imri)
  {
    cerr << "could not open " << invol.c_str() << endl;
    return -1;
  }
  MRI *lmri = MRIreadHeader(const_cast<char *> (likevol.c_str()), MRI_VOLUME_TYPE_UNKNOWN);
  if (!lmri)
  {
    MRIfree(&imri);
    cerr << "could not open " << invol.c_str() << endl;
    return -1;
  }
  imri->tr = lmri->tr;
  imri->te = lmri->te;
  imri->ti = lmri->ti;
  imri->flip_angle = lmri->flip_angle;
  
  int val = MRIwrite(imri, const_cast<char *>(outvol.c_str()));
  if (val)
  {
    cerr << "error occured in writing " << outvol.c_str() << endl;
  }
  MRIfree(&imri);
  MRIfree(&lmri);
}
