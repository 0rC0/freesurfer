/**
 * @file  coffin.cxx
 * @brief Container of tractography data and methods
 *
 * Container of tractography data and methods
 */
/*
 * Original Author: Anastasia Yendiki
 * CVS Revision Info:
 *
 * Copyright © 2011 The General Hospital Corporation (Boston, MA) "MGH"
 *
 * Terms and conditions for use, reproduction, distribution and contribution
 * are found in the 'FreeSurfer Software License Agreement' contained
 * in the file 'LICENSE' found in the FreeSurfer distribution, and here:
 *
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferSoftwareLicense
 *
 * Reporting: freesurfer@nmr.mgh.harvard.edu
 *
 */

#include <coffin.h>

using namespace std;

Coffin::Coffin(const char *OutDir, const char *DwiFile,
        const char *GradientFile, const char *BvalueFile,
        const char *MaskFile, const char *BedpostDir,
        const int NumTract, const float FminPath,
        const char *InitFile,
        const char *RoiFile1, const char *RoiFile2,
        const char *RoiMeshFile1, const char *RoiMeshFile2,
        const char *RoiRefFile1, const char *RoiRefFile2,
        const char *PriorFile0, const char *PriorFile1,
        const int AsegPriorType,
        const char *AsegPriorFile0, const char *AsegPriorFile1,
        const char *AsegIdFile,
        const char *AsegTrainFile, const char *PathTrainFile,
        const char *NeighPriorFile, const char *NeighIdFile,
        const char *LocalPriorFile, const char *LocalIdFile,
        const char *AsegFile,
        const char *AffineXfmFile, const char *NonlinXfmFile,
        const int NumBurnIn, const int NumSample,
        const int KeepSampleNth, const int UpdatePropNth,
        const char *PropStdFile,
        const bool Debug) :
        mDebug(Debug),
        mMask(0), mRoi1(0), mRoi2(0),
        mPathPrior0(0), mPathPrior1(0),
        mAsegTrain(0), mPathTrain(0), mAseg(0),
        mPathSamples(0),
        mSpline(InitFile, MaskFile) {
  char fname[PATH_MAX];
  MRI *dwi, *phi[NumTract], *theta[NumTract], *f[NumTract],
      *v0[NumTract], *f0[NumTract], *d0;
  ostringstream infostr;

  // Save input info for logging
  infostr  << "DWIs: " << DwiFile << endl
           << "Gradients: " << GradientFile << endl
           << "B-values: " << BvalueFile << endl
           << "Mask: " << MaskFile << endl
           << "BEDPOST directory: " << BedpostDir << endl
           << "Max number of tracts per voxel: " << NumTract << endl
           << "Tract volume fraction threshold: " << FminPath << endl;
  if (AsegFile) {
    infostr << "Segmentation map: " << AsegFile << endl
            << "Type of aseg prior: " << AsegPriorType << endl;
  }
  if (AffineXfmFile)
    infostr << "DWI-to-atlas affine registration: "
            << AffineXfmFile << endl;
  if (NonlinXfmFile)
    infostr << "DWI-to-atlas nonlinear registration: "
            << NonlinXfmFile << endl;
  mInfoGeneral = infostr.str();

  // Make output directory
  SetOutputDir(OutDir);

  // Read diffusion-weighted images
  cout << "Loading DWIs from " << DwiFile << endl;
  dwi = MRIread(DwiFile);
  if (!dwi) {
    cout << "ERROR: Could not read " << DwiFile << endl;
    exit(1);
  }

  // Size of diffusion-weighted images
  mNx = dwi->width;
  mNy = dwi->height;
  mNz = dwi->depth;
  mNxy = mNx * mNy;

  // Read mask
  cout << "Loading mask from " << MaskFile << endl;
  mMask = MRIread(MaskFile);
  if (!mMask) {
    cout << "ERROR: Could not read " << MaskFile << endl;
    exit(1);
  }

  mPathSamples = MRIclone(mMask, NULL);

  // Read parameter samples from BEDPOST directory
  cout << "Loading BEDPOST parameter samples from " << BedpostDir << endl;
  for (int itract = 0; itract < NumTract; itract++) {
    sprintf(fname, "%s/merged_ph%usamples.nii.gz", BedpostDir, itract+1);
    phi[itract] = MRIread(fname);
    if (!phi[itract]) {
      cout << "ERROR: Could not read " << fname << endl;
      exit(1);
    }
    sprintf(fname, "%s/merged_th%usamples.nii.gz", BedpostDir, itract+1);
    theta[itract] = MRIread(fname);
    if (!theta[itract]) {
      cout << "ERROR: Could not read " << fname << endl;
      exit(1);
    }
    sprintf(fname, "%s/merged_f%usamples.nii.gz", BedpostDir, itract+1);
    f[itract] = MRIread(fname);
    if (!f[itract]) {
      cout << "ERROR: Could not read " << fname << endl;
      exit(1);
    }
    sprintf(fname, "%s/dyads%u.nii.gz", BedpostDir, itract+1);
    v0[itract] = MRIread(fname);
    if (!v0[itract]) {
      cout << "ERROR: Could not read " << fname << endl;
      exit(1);
    }
    sprintf(fname, "%s/mean_f%usamples.nii.gz", BedpostDir, itract+1);
    f0[itract] = MRIread(fname);
    if (!f0[itract]) {
      cout << "ERROR: Could not read " << fname << endl;
      exit(1);
    }
  }

  sprintf(fname, "%s/mean_dsamples.nii.gz", BedpostDir);
  d0 = MRIread(fname);
  if (!d0) {
    cout << "ERROR: Could not read " << fname << endl;
    exit(1);
  }

  // Read segmentation map
  if (AsegFile) {
    cout << "Loading segmentation map from " << AsegFile << endl;
    mAseg = MRIread(AsegFile);
    if (!mAseg) {
      cout << "ERROR: Could not read " << AsegFile << endl;
      exit(1);
    }
  }
  else
    mAseg = 0;

  // Set variables specific to pathway
  SetPathway(InitFile, RoiFile1, RoiFile2,
             RoiMeshFile1, RoiMeshFile2, RoiRefFile1, RoiRefFile2,
             PriorFile0, PriorFile1,
             AsegPriorType, AsegPriorFile0, AsegPriorFile1, AsegIdFile,
             AsegTrainFile, PathTrainFile,
             NeighPriorFile, NeighIdFile,
             LocalPriorFile, LocalIdFile);

  // Initialize voxel-wise diffusion model
  Bite::SetStatic(GradientFile, BvalueFile,
                  NumTract, phi[0]->nframes, FminPath);

  if (Bite::GetNumDir() != dwi->nframes) {
    cout << "ERROR: Dimensions of " << BvalueFile << " and " << DwiFile
         << " do not match" << endl;
    exit(1);
  }

  mData.clear();
  for (int iz = 0; iz < mNz; iz++)
    for (int iy = 0; iy < mNy; iy++)
      for (int ix = 0; ix < mNx; ix++)
        if (MRIgetVoxVal(mMask, ix, iy, iz, 0)) {
          Bite data = Bite(dwi, phi, theta, f, v0, f0, d0, ix, iy, iz);
          mData.push_back(data);
        }

  mDataMask.clear();
  mNumVox = 0;
  for (int iz = 0; iz < mNz; iz++)
    for (int iy = 0; iy < mNy; iy++)
      for (int ix = 0; ix < mNx; ix++)
        if (MRIgetVoxVal(mMask, ix, iy, iz, 0)) {
          mDataMask.push_back(&mData[mNumVox]);
          mNumVox++;
        }
        else
          mDataMask.push_back(0);

  // Free temporary variables
  MRIfree(&dwi);

  for (int itract = 0; itract < NumTract; itract++) {
    MRIfree(&phi[itract]);
    MRIfree(&theta[itract]);
    MRIfree(&f[itract]);
    MRIfree(&v0[itract]);
    MRIfree(&f0[itract]);
  }

  MRIfree(&d0);

  // Read DWI-to-atlas registration
#ifndef NO_CVS_UP_IN_HERE
  if (NonlinXfmFile) {
    mAffineReg.ReadXfm(AffineXfmFile, mMask, 0);
    mNonlinReg.ReadXfm(NonlinXfmFile, mRoi1);
  }
  else
#endif
  if (AffineXfmFile)
    mAffineReg.ReadXfm(AffineXfmFile, mMask, mRoi1);

	/*
vector<float> pt(3);
pt[0] = 75; pt[1] = 55; pt[2] = 51;
cout << "In DWI space: " << pt[0] << " " << pt[1] << " " << pt[2] << endl;
mAffineReg.ApplyXfm(pt, pt.begin());
mNonlinReg.ApplyXfm(pt, pt.begin());
cout << "In atlas space: " << pt[0] << " " << pt[1] << " " << pt[2] << endl;
exit(1);
	*/

  // Set variables specific to MCMC algorithm
  SetMCMCParameters(NumBurnIn, NumSample,
                    KeepSampleNth, UpdatePropNth, PropStdFile);
}

Coffin::~Coffin() {
  MRIfree(&mMask);

  MRIfree(&mRoi1);
  MRIfree(&mRoi2);

  if (mPathPrior0) {
    MRIfree(&mPathPrior0);
    MRIfree(&mPathPrior1);
  }

  for (vector<MRI *>::iterator iseg = mSegPrior0.begin();
                               iseg != mSegPrior0.end(); iseg++)
    MRIfree(&(*iseg));

  for (vector<MRI *>::iterator iseg = mSegPrior1.begin();
                               iseg != mSegPrior1.end(); iseg++)
    MRIfree(&(*iseg));

  if (mAsegTrain)
    MRIfree(&mAsegTrain);

  if (mPathTrain)
    MRIfree(&mPathTrain);

  if (mAseg)
    MRIfree(&mAseg);

  MRIfree(&mPathSamples);
}


//
// Set output directory
//
void Coffin::SetOutputDir(const char *OutDir) {
  char fname[PATH_MAX];

  cout << "Creating output directory " << OutDir << endl;
  sprintf(fname, "mkdir -p %s", OutDir);
  if (system(fname) != 0) {
    cout << "ERROR: Could not create directory " << OutDir << endl;
    exit(1);
  }

  mOutDir = (char *) calloc(strlen(OutDir)+1, sizeof(char));
  strcpy(mOutDir, OutDir);
}

//
// Set variables specific to a given pathway
//
void Coffin::SetPathway(const char *InitFile,
                        const char *RoiFile1, const char *RoiFile2,
                        const char *RoiMeshFile1, const char *RoiMeshFile2,
                        const char *RoiRefFile1, const char *RoiRefFile2,
                        const char *PriorFile0, const char *PriorFile1,
                        const int AsegPriorType,
                        const char *AsegPriorFile0, const char *AsegPriorFile1,
                        const char *AsegIdFile,
                        const char *AsegTrainFile, const char *PathTrainFile,
                        const char *NeighPriorFile, const char *NeighIdFile,
                        const char *LocalPriorFile, const char *LocalIdFile) {
  int dirs[45] = { 0,  0,  0,
                   1,  0,  0,
                  -1,  0,  0,
                   0,  1,  0,
                   0, -1,  0,
                   0,  0,  1,
                   0,  0, -1,
                   1,  1,  1,
                  -1,  1,  1,
                   1, -1,  1,
                  -1, -1,  1,
                   1,  1, -1,
                  -1,  1, -1,
                   1, -1, -1,
                  -1, -1, -1 };
  char fname[PATH_MAX];
  ostringstream infostr;
  vector< vector<unsigned int> > segids, neighids, localids;
  vector< vector<float> > neighpr, localpr;

  // Save input info for logging
  infostr << "Initial control point file: " << InitFile << endl
          << "End ROI 1: " << RoiFile1 << endl;
  if (RoiMeshFile1)
    infostr << "End ROI 1 mesh: " << RoiMeshFile1 << endl
            << "End ROI 1 reference volume: " << RoiRefFile1 << endl;
  infostr << "End ROI 2: " << RoiFile2 << endl;
  if (RoiMeshFile2)
    infostr << "End ROI 2 mesh: " << RoiMeshFile2 << endl
            << "End ROI 2 reference volume: " << RoiRefFile2 << endl;
  if (PriorFile0)
    infostr << "Spatial prior (off path): " << PriorFile0 << endl
            << "Spatial prior (on path): " << PriorFile1 << endl;
  if (AsegPriorFile0)
    infostr << "Aseg prior (off path): " << AsegPriorFile0 << endl
            << "Aseg prior (on path): " << AsegPriorFile1 << endl
            << "Aseg label ID list: " << AsegIdFile << endl;
  if (AsegTrainFile)
    infostr << "Seg. map training set: " << AsegTrainFile << endl
            << "Path training set: " << PathTrainFile << endl;
  if (NeighPriorFile)
    infostr << "Neighbor aseg prior: " << NeighPriorFile << endl
            << "Neighbor aseg label ID list: " << NeighIdFile << endl;
  if (LocalPriorFile)
    infostr << "Local aseg prior: " << LocalPriorFile << endl
            << "Local aseg label ID list: " << LocalIdFile << endl;
  mInfoPathway = infostr.str();

  // Read control point initialization
  ReadControlPoints(InitFile);
  if (!mProposalStdInit.empty() &&
      (mProposalStdInit.size() != mControlPoints.size())) {
    mProposalStdInit.resize(mControlPoints.size(), 1.0);
    cout << "WARN: Resized proposal standard deviations to match new number "
         << "of control points" << endl;
  }

  // Read end ROIs
  if (RoiFile1) {
    if (mRoi1)
      MRIfree(&mRoi1);

    cout << "Loading end ROI from " << RoiFile1 << endl;
    mRoi1 = MRIread(RoiFile1);
    if (!mRoi1) {
      cout << "ERROR: Could not read " << RoiFile1 << endl;
      exit(1);
    }
  }
  else if (RoiMeshFile1) {}

  if (RoiFile2) {
    if (mRoi2)
      MRIfree(&mRoi2);

    cout << "Loading end ROI from " << RoiFile2 << endl;
    mRoi2 = MRIread(RoiFile2);
    if (!mRoi2) {
      cout << "ERROR: Could not read " << RoiFile2 << endl;
      exit(1);
    }
  }
  else if (RoiMeshFile2) {}

  // Size of atlas space
  mNxAtlas = mRoi1->width;
  mNyAtlas = mRoi1->height;
  mNzAtlas = mRoi1->depth;

  // Read spatial path priors
  if (PriorFile0 && PriorFile1) {
    if (mPathPrior0)
      MRIfree(&mPathPrior0);

    cout << "Loading spatial path prior from " << PriorFile0 << endl;
    mPathPrior0 = MRIread(PriorFile0);
    if (!mPathPrior0) {
      cout << "ERROR: Could not read " << PriorFile0 << endl;
      exit(1);
    }

    if (mPathPrior1)
      MRIfree(&mPathPrior1);

    cout << "Loading spatial path prior from " << PriorFile1 << endl;
    mPathPrior1 = MRIread(PriorFile1);
    if (!mPathPrior1) {
      cout << "ERROR: Could not read " << PriorFile1 << endl;
      exit(1);
    }
  }
  else {
    mPathPrior0 = 0;
    mPathPrior1 = 0;
  }

  // Read aseg priors
  if (AsegPriorFile0 && AsegPriorFile1 && AsegIdFile) {
    MRI *tmp;
    vector<string> suff;

    for (vector<MRI *>::iterator iseg = mSegPrior0.begin();
                                 iseg != mSegPrior0.end(); iseg++)
      MRIfree(&(*iseg));

    mSegPrior0.clear();

    for (vector<MRI *>::iterator iseg = mSegPrior1.begin();
                                 iseg != mSegPrior1.end(); iseg++)
      MRIfree(&(*iseg));

    mSegPrior1.clear();

    if (FileExists(AsegPriorFile0))
      suff.push_back("");
    else {
      suff.push_back("_0_0_0.nii.gz");

      if (AsegPriorType == 2) {		// Local neighborhood aseg prior
        suff.push_back("_1_0_0.nii.gz");
        suff.push_back("_-1_0_0.nii.gz");
        suff.push_back("_0_1_0.nii.gz");
        suff.push_back("_0_-1_0.nii.gz");
        suff.push_back("_0_0_1.nii.gz");
        suff.push_back("_0_0_-1.nii.gz");
      }
      else if (AsegPriorType > 2) {	// Nearest-neighbor aseg prior
        suff.push_back("_x.nii.gz");
        suff.push_back("_-x.nii.gz");
        suff.push_back("_y.nii.gz");
        suff.push_back("_-y.nii.gz");
        suff.push_back("_z.nii.gz");
        suff.push_back("_-z.nii.gz");
      }
    }

    for (vector<string>::const_iterator isuff = suff.begin();
                                        isuff < suff.end(); isuff++) {
      unsigned int val;
      vector<unsigned int> idlist;
      sprintf(fname, "%s%s", AsegIdFile, isuff->c_str());
      ifstream idfile(fname, ios::in);

      if (!idfile) {
        cout << "ERROR: Could not open " << fname << endl;
        exit(1);
      }

      cout << "Loading label IDs for aseg prior from " << fname << endl;
      while (idfile >> val)
        idlist.push_back(val);
      segids.push_back(idlist);

      sprintf(fname, "%s%s", AsegPriorFile0, isuff->c_str());
      cout << "Loading aseg prior from " << fname << endl;
      tmp = MRIread(fname);
      if (!tmp) {
        cout << "ERROR: Could not read " << fname << endl;
        exit(1);
      }
      if (tmp->nframes != (int) idlist.size()) {
        cout << "ERROR: Expected " << idlist.size() << " frames in " << fname
             << ", found " << tmp->nframes << endl;
        exit(1);
      }
      mSegPrior0.push_back(tmp);

      sprintf(fname, "%s%s", AsegPriorFile1, isuff->c_str());
      cout << "Loading aseg prior from " << fname << endl;
      tmp = MRIread(fname);
      if (!tmp) {
        cout << "ERROR: Could not read " << fname << endl;
        exit(1);
      }
      if (tmp->nframes != (int) idlist.size()) {
        cout << "ERROR: Expected " << idlist.size() << " frames in " << fname
             << ", found " << tmp->nframes << endl;
        exit(1);
      }
      mSegPrior1.push_back(tmp);
    }
  }

  // Read segmentation map training set
  if (AsegTrainFile) {
    if (mAsegTrain)
      MRIfree(&mAsegTrain);

    cout << "Loading seg. map training set from " << AsegTrainFile << endl;
    mAsegTrain = MRIread(AsegTrainFile);
    if (!mAsegTrain) {
      cout << "ERROR: Could not read " << AsegTrainFile << endl;
      exit(1);
    }
  }
  else
    mAsegTrain = 0;

  // Read path training set
  if (PathTrainFile) {
    if (mPathTrain)
      MRIfree(&mPathTrain);

    cout << "Loading path training set from " << PathTrainFile << endl;
    mPathTrain = MRIread(PathTrainFile);
    if (!mPathTrain) {
      cout << "ERROR: Could not read " << PathTrainFile << endl;
      exit(1);
    }
    if (mPathTrain->nframes != mAsegTrain->nframes) {
      cout << "ERROR: Numbers of training samples in " << PathTrainFile 
           << " and " << AsegTrainFile << " do not match." << endl;
      exit(1);
    }
  }
  else
    mPathTrain = 0;

  mNumArc = 0;

  // Read neighbor aseg priors
  if (NeighPriorFile && NeighIdFile) {
    mPriorNear.clear();
    mIdsNear.clear();
    mDirNear.clear();

    // Directions in which to look for neighboring labels
    mNumNear = 14;
    mDirNear.insert(mDirNear.begin(), dirs+3, dirs+45);

    for (vector<int>::const_iterator idir = mDirNear.begin();
                                     idir < mDirNear.end(); idir += 3) {
      const int idx = idir[0], idy = idir[1], idz = idir[2];
      char prname[PATH_MAX], idname[PATH_MAX];
      string prline, idline;
      ifstream prfile, idfile;

      sprintf(prname, "%s_%d_%d_%d.txt", NeighPriorFile, idx, idy, idz);
      sprintf(idname, "%s_%d_%d_%d.txt", NeighIdFile, idx, idy, idz);

      cout << "Loading nearest neighbor prior from " << prname
           << " with label IDs from " << idname << endl;

      prfile.open(prname, ios::in);
      if (!prfile) {
        cout << "ERROR: Could not open " << prname << endl;
        exit(1);
      }

      idfile.open(idname, ios::in);
      if (!idfile) {
        cout << "ERROR: Could not open " << idname << endl;
        exit(1);
      }

      while (getline(prfile, prline) && getline(idfile, idline)) {
        unsigned int id;
        float pr;
        vector<unsigned int> idlist;
        vector<float> prior;
        istringstream prstr(prline), idstr(idline);

        while (prstr >> pr)
          prior.push_back(pr);

        while (idstr >> id)
          idlist.push_back(id);

        if (prior.size() != idlist.size() + 1) {
          cout << "ERROR: Line length mismatch between "
               << prname << " (" << prline << ") and "
               << idname << " (" << idline << ")" << endl;
          exit(1);
        } 

        mPriorNear.push_back(prior);
        mIdsNear.push_back(idlist);
      }

      prfile.close();
      idfile.close();
    }

    mNumArc = (int) (mPriorNear.size() / mNumNear);
  }

  // Read local aseg priors
  if (LocalPriorFile && LocalIdFile) {
    mPriorLocal.clear();
    mIdsLocal.clear();
    mDirLocal.clear();

    // Directions in which to look for neighboring labels
    if (FileExists(LocalPriorFile)) {
      mNumLocal = 1;
      mDirLocal.insert(mDirLocal.begin(), dirs, dirs+3);
    }
    else {
      mNumLocal = 15;
      mDirLocal.insert(mDirLocal.begin(), dirs, dirs+45);
    }

    for (vector<int>::const_iterator idir = mDirLocal.begin();
                                     idir < mDirLocal.end(); idir += 3) {
      const int idx = idir[0], idy = idir[1], idz = idir[2];
      char prname[PATH_MAX], idname[PATH_MAX];
      string prline, idline;
      ifstream prfile, idfile;

      sprintf(prname, "%s_%d_%d_%d.txt", LocalPriorFile, idx, idy, idz);
      sprintf(idname, "%s_%d_%d_%d.txt", LocalIdFile, idx, idy, idz);

      cout << "Loading local prior from " << prname
           << " with label IDs from " << idname << endl;

      prfile.open(prname, ios::in);
      if (!prfile) {
        cout << "ERROR: Could not open " << prname << endl;
        exit(1);
      }

      idfile.open(idname, ios::in);
      if (!idfile) {
        cout << "ERROR: Could not open " << idname << endl;
        exit(1);
      }

      while (getline(prfile, prline) && getline(idfile, idline)) {
        unsigned int id;
        float pr;
        vector<unsigned int> idlist;
        vector<float> prior;
        istringstream prstr(prline), idstr(idline);

        while (prstr >> pr)
          prior.push_back(pr);

        while (idstr >> id)
          idlist.push_back(id);

        if (prior.size() != idlist.size() + 1) {
          cout << "ERROR: Line length mismatch between "
               << prname << " (" << prline << ") and "
               << idname << " (" << idline << ")" << endl;
          exit(1);
        } 

        mPriorLocal.push_back(prior);
        mIdsLocal.push_back(idlist);
      }

      prfile.close();
      idfile.close();
    }

    if (mNumArc == 0)
      mNumArc = (int) (mPriorLocal.size() / mNumLocal);
    else if (mNumArc != (int) (mPriorLocal.size() / mNumLocal)) {
      cout << "ERROR: Mismatch between the numbers of arc segments in "
           << LocalPriorFile
           << "* (" << mPriorLocal.size() / mNumLocal << ") and "
           << NeighPriorFile
           << "* (" << mPriorNear.size()  / mNumNear  << ")" << endl;
      exit(1);
    }
  }

  // Clear previously used priors, if any
  for (vector<Bite>::iterator idat = mData.begin(); idat < mData.end(); idat++)
    if (idat->IsPriorSet())
      idat->ResetPrior();

  // Initialize prior-related variables that are common to all voxels
  Bite::SetStaticPrior(AsegPriorType, segids,
                       mPathTrain ? mPathTrain->nframes : 0);
}

//
// Set MCMC parameters
//
void Coffin::SetMCMCParameters(const int NumBurnIn, const int NumSample,
                               const int KeepSampleNth, const int UpdatePropNth,
                               const char *PropStdFile) {
  ostringstream infostr;

  // Save input info for logging
  infostr << "Number of burn-in samples: " << NumBurnIn << endl
          << "Number of post-burn-in samples: " << NumSample << endl
          << "Keep every: " << KeepSampleNth << "-th sample" << endl
          << "Update proposal every: " << UpdatePropNth << "-th sample" << endl;
  if (PropStdFile)
    infostr << "Initial proposal SD file: " << PropStdFile << endl;
  mInfoMCMC = infostr.str();

  // Set sampling parameters
  mNumBurnIn = NumBurnIn;
  mNumSample = NumSample;
  mKeepSampleNth = KeepSampleNth;
  mUpdatePropNth = UpdatePropNth;
 
  // Read proposal SD initialization
  ReadProposalStds(PropStdFile);
  if (mProposalStdInit.size() != mControlPoints.size()) {
    cout << "ERROR: Dimensions of " << PropStdFile << " must be "
         << mControlPoints.size()/3 << " x 3 to match control points" << endl;
    exit(1);
  }
}

//
// Read initial control points
//
void Coffin::ReadControlPoints(const char *ControlPointFile) {
  float coord;
  ifstream infile(ControlPointFile, ios::in);

  if (!infile) {
    cout << "ERROR: Could not open " << ControlPointFile << endl;
    exit(1);
  }

  cout << "Loading control points from " << ControlPointFile << endl;
  mControlPoints.clear();
  while (infile >> coord)
    mControlPoints.push_back((int) round(coord));

  if (mControlPoints.size() % 3 != 0) {
    cout << "ERROR: File " << ControlPointFile
         << " must contain triplets of coordinates" << endl;
    exit(1);
  }

  mNumControl = mControlPoints.size()/3;

  // Make sure that initial control points are in mask
  for (vector<int>::iterator icpt = mControlPoints.begin();
                             icpt < mControlPoints.end(); icpt += 3) {
    if (icpt[0] < 0) {
      cout << "WARN: Initial control point "
           << icpt[0] << " " << icpt[1] << " " << icpt[2]
           << " is not in DWI volume - is DWI cropped?" << endl;

      icpt[0] = 0;

      cout << "WARN: Replacing with closest point in volume ("
           << icpt[0] << " " << icpt[1] << " " << icpt[2] << ")" << endl;
    }

    if (icpt[0] >= mMask->width) {
      cout << "WARN: Initial control point "
           << icpt[0] << " " << icpt[1] << " " << icpt[2]
           << " is not in DWI volume - is DWI cropped?" << endl;

      icpt[0] = mMask->width - 1;

      cout << "WARN: Replacing with closest point in volume ("
           << icpt[0] << " " << icpt[1] << " " << icpt[2] << ")" << endl;
    }

    if (icpt[1] < 0) {
      cout << "WARN: Initial control point "
           << icpt[0] << " " << icpt[1] << " " << icpt[2]
           << " is not in DWI volume - is DWI cropped?" << endl;

      icpt[1] = 0;

      cout << "WARN: Replacing with closest point in volume ("
           << icpt[0] << " " << icpt[1] << " " << icpt[2] << ")" << endl;
    }

    if (icpt[1] >= mMask->height) {
      cout << "WARN: Initial control point "
           << icpt[0] << " " << icpt[1] << " " << icpt[2]
           << " is not in DWI volume - is DWI cropped?" << endl;

      icpt[1] = mMask->height - 1;

      cout << "WARN: Replacing with closest point in volume ("
           << icpt[0] << " " << icpt[1] << " " << icpt[2] << ")" << endl;
    }

    if (icpt[2] < 0) {
      cout << "WARN: Initial control point "
           << icpt[0] << " " << icpt[1] << " " << icpt[2]
           << " is not in DWI volume - is DWI cropped?" << endl;

      icpt[2] = 0;

      cout << "WARN: Replacing with closest point in volume ("
           << icpt[0] << " " << icpt[1] << " " << icpt[2] << ")" << endl;
    }

    if (icpt[2] >= mMask->depth) {
      cout << "WARN: Initial control point "
           << icpt[0] << " " << icpt[1] << " " << icpt[2]
           << " is not in DWI volume - is DWI cropped?" << endl;

      icpt[2] = mMask->depth - 1;

      cout << "WARN: Replacing with closest point in volume ("
           << icpt[0] << " " << icpt[1] << " " << icpt[2] << ")" << endl;
    }

    if (! MRIgetVoxVal(mMask, icpt[0], icpt[1], icpt[2], 0)) {
      int dmin = 1000000, ixmin = 0, iymin = 0, izmin = 0;

      cout << "WARN: Initial control point "
           << icpt[0] << " " << icpt[1] << " " << icpt[2]
           << " is not in mask" << endl
           << "WARN: Replacing with closest point in mask (";

      for (int iz = 0; iz < mNz; iz++)
        for (int iy = 0; iy < mNy; iy++)
          for (int ix = 0; ix < mNx; ix++)
            if (MRIgetVoxVal(mMask, ix, iy, iz, 0)) {
              const int dx = icpt[0] - ix,
                        dy = icpt[1] - iy,
                        dz = icpt[2] - iz,
                        dist = dx*dx + dy*dy + dz*dz;

              if (dist < dmin) {
                ixmin = ix;
                iymin = iy;
                izmin = iz;
                dmin = dist;
              }
            }

      icpt[0] = ixmin;
      icpt[1] = iymin;
      icpt[2] = izmin;

      cout << icpt[0] << " " << icpt[1] << " " << icpt[2] << ")" << endl;
    }
  }
}

//
// Read initial proposal standard deviations for control point perturbations
//
void Coffin::ReadProposalStds(const char *PropStdFile) {
  mProposalStdInit.clear();

  if (PropStdFile) {
    float val;
    ifstream infile(PropStdFile, ios::in);

    if (!infile) {
      cout << "ERROR: Could not open " << PropStdFile << endl;
      exit(1);
    }

    cout << "Loading initial proposal SD's from " << PropStdFile << endl;
    while (infile >> val)
      mProposalStdInit.push_back(val);
  }
  else {	// Default values
    mProposalStdInit.resize(mControlPoints.size());
    fill(mProposalStdInit.begin(), mProposalStdInit.begin()+3, 1.0);	//5.0
    fill(mProposalStdInit.begin()+3, mProposalStdInit.end()-3, 1.0);	//1.0
    fill(mProposalStdInit.end()-3, mProposalStdInit.end(), 1.0);	//5.0
  }
}

//
// Run MCMC
//
bool Coffin::RunMCMC() {
  int iprop, ikeep;
  char fname[PATH_MAX];

  // Write input parameters to log file
  sprintf(fname, "%s/log.txt", mOutDir);
  mLog.open(fname, ios::out | ios::app);
  mLog << mInfoGeneral << mInfoPathway << mInfoMCMC;

  cout << "Initializing MCMC" << endl;
  mLog << "Initializing MCMC" << endl;
  if (! InitializeMCMC())
    return false;

  if (mDebug) {
    sprintf(fname, "%s/Finit.nii.gz", mOutDir);
    mSpline.WriteVolume(fname, true);
  }

  cout << "Running MCMC burn-in jumps" << endl;
  mLog << "Running MCMC burn-in jumps" << endl;
  iprop = 1;
  for (int ijump = mNumBurnIn; ijump > 0; ijump--) {
    if (JumpMCMC() || mAcceptF || mAcceptTheta) {	// Accept new path
      UpdatePath();
      UpdateAcceptanceRate();

      if (mDebug) {
        sprintf(fname, "%s/Faccept_b%05d.nii.gz", mOutDir, mNumBurnIn-ijump+1);
        mSpline.WriteVolume(fname, true);
      }
    }
    else {						// Reject new path
      UpdateRejectionRate();

      if (mDebug) {
        sprintf(fname, "%s/Freject_b%05d.nii.gz", mOutDir, mNumBurnIn-ijump+1);
        mSpline.WriteVolume(fname, true);
      }
    }

    if (iprop == mUpdatePropNth) {
      UpdateProposalStd();
      iprop = 1;
    }
    else
      iprop++;
  }

  mPosteriorOnPathMap = numeric_limits<double>::max();

  cout << "Running MCMC main jumps" << endl;
  mLog << "Running MCMC main jumps" << endl;
  iprop = 1;
  ikeep = 1;
  for (int ijump = mNumSample; ijump > 0; ijump--) {
    if (JumpMCMC() || mAcceptF || mAcceptTheta) {	// Accept new path
      UpdatePath();
      UpdateAcceptanceRate();

      if (mDebug) {
        sprintf(fname, "%s/Faccept_%05d.nii.gz", mOutDir, mNumSample-ijump+1);
        mSpline.WriteVolume(fname, true);
      }
    }
    else {						// Reject new path
      UpdateRejectionRate();

      if (mDebug) {
        sprintf(fname, "%s/Freject_%05d.nii.gz", mOutDir, mNumSample-ijump+1);
        mSpline.WriteVolume(fname, true);
      }
    }

    if (iprop == mUpdatePropNth) {
      UpdateProposalStd();
      iprop = 1;
    }
    else
      iprop++;

    if (ikeep == mKeepSampleNth) {
      SavePath();
      ikeep = 1;
    }
    else
      ikeep++;
  }

  mLog.close();
  return true;
}

//
// Run MCMC (single point updates)
//
bool Coffin::RunMCMC1() {
  int iprop, ikeep;
  char fname[PATH_MAX];
  vector<int> cptorder(mNumControl);
  vector<int>::const_iterator icpt;

  // Write input parameters to log file
  sprintf(fname, "%s/log.txt", mOutDir);
  mLog.open(fname, ios::out | ios::app);
  mLog << mInfoGeneral << mInfoPathway << mInfoMCMC;

  cout << "Initializing MCMC" << endl;
  mLog << "Initializing MCMC" << endl;
  if (! InitializeMCMC())
    return false;

  if (mDebug) {
    sprintf(fname, "%s/Finit.nii.gz", mOutDir);
    mSpline.WriteVolume(fname, true);
  }

  cout << "Running MCMC burn-in jumps" << endl;
  mLog << "Running MCMC burn-in jumps" << endl;
  iprop = 1;
  for (int ijump = mNumBurnIn; ijump > 0; ijump--) {
    // Perturb control points in random order
    for (int k = 0; k < mNumControl; k++)
      cptorder[k] = k;
    random_shuffle(cptorder.begin(), cptorder.end());

    fill(mRejectControl.begin(), mRejectControl.end(), false);

    for (icpt = cptorder.begin(); icpt != cptorder.end(); icpt++) {
mRejectSpline = false;
mRejectF = false;
mAcceptF = false;
mRejectTheta = false;
mAcceptTheta = false;
mRejectPosterior = false;

      if (JumpMCMC1(*icpt) || mAcceptF || mAcceptTheta) {	// Accept point
        UpdatePath();

        if (mDebug) {
          sprintf(fname, "%s/Faccept_b%05d_%d.nii.gz",
                  mOutDir, mNumBurnIn-ijump+1, *icpt);
          mSpline.WriteVolume(fname, true);
        }
      }
      else {							// Reject point
        mRejectControl[*icpt] = true;

        if (mDebug) {
          sprintf(fname, "%s/Freject_b%05d_%d.nii.gz",
                  mOutDir, mNumBurnIn-ijump+1, *icpt);
          mSpline.WriteVolume(fname, true);
        }
      }
    }

    UpdateAcceptRejectRate1();

    if (iprop == mUpdatePropNth) {
      UpdateProposalStd();
      iprop = 1;
    }
    else
      iprop++;
  }

  mPosteriorOnPathMap = numeric_limits<double>::max();

  cout << "Running MCMC main jumps" << endl;
  mLog << "Running MCMC main jumps" << endl;
  iprop = 1;
  ikeep = 1;
  for (int ijump = mNumSample; ijump > 0; ijump--) {
    // Perturb control points in random order
    for (int k = 0; k < mNumControl; k++)
      cptorder[k] = k;
    random_shuffle(cptorder.begin(), cptorder.end());

    fill(mRejectControl.begin(), mRejectControl.end(), false);

    for (icpt = cptorder.begin(); icpt != cptorder.end(); icpt++) {
mRejectSpline = false;
mRejectF = false;
mAcceptF = false;
mRejectTheta = false;
mAcceptTheta = false;
mRejectPosterior = false;

      if (JumpMCMC1(*icpt) || mAcceptF || mAcceptTheta) {	// Accept point
        UpdatePath();

        if (mDebug) {
          sprintf(fname, "%s/Faccept_%05d_%d.nii.gz",
                  mOutDir, mNumSample-ijump+1, *icpt);
          mSpline.WriteVolume(fname, true);
        }
      }
      else {							// Reject point
        mRejectControl[*icpt] = true;

        if (mDebug) {
          sprintf(fname, "%s/Freject_%05d_%d.nii.gz",
                  mOutDir, mNumSample-ijump+1, *icpt);
          mSpline.WriteVolume(fname, true);
        }
      }
    }

    UpdateAcceptRejectRate1();

    if (iprop == mUpdatePropNth) {
      UpdateProposalStd();
      iprop = 1;
    }
    else
      iprop++;

    if (ikeep == mKeepSampleNth) {
      SavePath();
      ikeep = 1;
    }
    else
      ikeep++;
  }

  mLog.close();
  return true;
}

//
// Initialize path and MCMC proposals
//
bool Coffin::InitializeMCMC() {
  vector<float>::iterator phi, theta;

  // Interpolate spline from initial control points
  mSpline.SetControlPoints(mControlPoints);
  if (!mSpline.InterpolateSpline()) {
    cout << "ERROR: Path from initial control points is not entirely in mask"
         << "ERROR: Initialization failed" << endl;
    return false;
  }

  mSpline.ComputeTangent();

  // Get initial path points
  mPathPoints.clear();
  mPathPoints.insert(mPathPoints.begin(),
                     mSpline.GetAllPointsBegin(), mSpline.GetAllPointsEnd());

  // Get initial path tangent angles
  mPathPhi.resize(mPathPoints.size());
  phi = mPathPhi.begin();
  mPathTheta.resize(mPathPoints.size());
  theta = mPathTheta.begin();
  for (vector<float>::const_iterator ipt = mSpline.GetTangentBegin();
                                     ipt < mSpline.GetTangentEnd(); ipt += 3) {
    *phi = atan2(ipt[1], ipt[0]);
    *theta = acos(ipt[2] / sqrt(ipt[0]*ipt[0] + ipt[1]*ipt[1] + ipt[2]*ipt[2]));
    if (mDebug)
      mLog << (*phi)/M_PI*180 << " " << (*theta)/M_PI*180 << endl;
    phi++;
    theta++;
  }

  // Compute energy on initial path
  mLikelihoodOnPath = 0;
  mPriorOnPath = 0;
  phi = mPathPhi.begin();
  theta = mPathTheta.begin();
  for (vector<int>::const_iterator ipt = mPathPoints.begin();
                                   ipt < mPathPoints.end(); ipt += 3) {
    Bite *ivox = mDataMask[ipt[0] + ipt[1]*mNx + ipt[2]*mNxy];

    if ( (mPathPrior0 || !mSegPrior0.empty() || mAsegTrain)
         && !ivox->IsPriorSet() ) {
      vector<int> atlaspt(3);

      MapToAtlas(atlaspt, ipt);

      ivox->SetPrior(mPathPrior0, mPathPrior1,
                     mSegPrior0, mSegPrior1, mAsegTrain, mPathTrain, mAseg,
                     atlaspt[0], atlaspt[1], atlaspt[2]);
    }

    ivox->ComputeLikelihoodOnPath(*phi, *theta);
    ivox->ComputePriorOnPath();

    mLikelihoodOnPath += ivox->GetLikelihoodOnPath();
    mPriorOnPath += ivox->GetPriorOnPath();

    phi++;
    theta++;
  }

  mAnatomicalPrior = ComputeAnatomicalPrior(mPathPoints);

  mPosteriorOnPath = mLikelihoodOnPath + mPriorOnPath + mAnatomicalPrior;

  // Initialize control point proposal distribution
  mProposalStd.resize(mProposalStdInit.size());
  copy(mProposalStdInit.begin(), mProposalStdInit.end(), mProposalStd.begin());

  if (mDebug) {
    mLog << "Proposal STDs: ";
    for (vector<float>::const_iterator pstd = mProposalStd.begin();
                                       pstd < mProposalStd.end(); pstd++)
      mLog << *pstd << " ";
    mLog << endl;
  }

  // Initialize jump acceptance statistics
  mRejectControl.resize(mNumControl);

  mAcceptCount.resize(mNumControl);
  fill(mAcceptCount.begin(), mAcceptCount.end(), 0);
  mRejectCount.resize(mNumControl);
  fill(mRejectCount.begin(), mRejectCount.end(), 0);

  mAcceptSpan.resize(mControlPoints.size());
  fill(mAcceptSpan.begin(), mAcceptSpan.end(), 0.0);
  mRejectSpan.resize(mControlPoints.size());
  fill(mRejectSpan.begin(), mRejectSpan.end(), 0.0);

  mControlPointsNew.resize(mControlPoints.size());
  mControlPointJumps.resize(mControlPoints.size());
  mControlPointsMap.resize(mControlPoints.size());

  // Clear saved path samples
  mControlPointSamples.clear();
  mPathLengthSamples.clear();
  mLikelihoodOnPathSamples.clear();
  mPriorOnPathSamples.clear();
  mPosteriorOnPathSamples.clear();
  MRIclear(mPathSamples);

  return true;
}

//
// Perform a single MCMC jump
//
bool Coffin::JumpMCMC() {
  fill(mRejectControl.begin(), mRejectControl.end(), false);
  mRejectSpline = false;
  mRejectF = false;
  mAcceptF = false;
  mRejectTheta = false;
  mAcceptTheta = false;
  mRejectPosterior = false;

  if (!ProposePath()) {
    if (mDebug)
      mLog << "mLikelihoodOnPathNew=NaN mPriorOnPathNew=NaN" << endl
           << "mAnatomicalPriorNew=NaN" << endl
           << "mLikelihoodOffPathNew=NaN mPriorOffPathNew=NaN" << endl
           << "mLikelihoodOn-OffPathNew=NaN mPriorOn-OffPathNew=NaN" << endl
           << "mPathLengthNew=NaN" << endl
           << "mLikelihoodOnPath=NaN mPriorOnPath=NaN" << endl
           << "mAnatomicalPrior=NaN" << endl
           << "mLikelihoodOffPath=NaN mPriorOffPath=NaN" << endl
           << "mLikelihoodOff-OnPath=NaN mPriorOff-OnPath=NaN" << endl
           << "mPathLength=NaN" << endl;

    return false;
  }

  ProposeDiffusionParameters();

  if (!AcceptPath()) {
    if (mDebug && !mRejectPosterior)
      mLog << "mLikelihoodOnPathNew=NaN mPriorOnPathNew=NaN" << endl
           << "mAnatomicalPriorNew=NaN" << endl
           << "mLikelihoodOffPathNew=NaN mPriorOffPathNew=NaN" << endl
           << "mLikelihoodOn-OffPathNew=NaN mPriorOn-OffPathNew=NaN" << endl
           << "mPathLengthNew=NaN" << endl
           << "mLikelihoodOnPath=NaN mPriorOnPath=NaN" << endl
           << "mAnatomicalPrior=NaN" << endl
           << "mLikelihoodOffPath=NaN mPriorOffPath=NaN" << endl
           << "mLikelihoodOff-OnPath=NaN mPriorOff-OnPath=NaN" << endl
           << "mPathLength=NaN" << endl;

    return false;
  }

  return true;
}

//
// Perform a single MCMC jump (single point updates)
//
bool Coffin::JumpMCMC1(int ControlIndex) {
  if (!ProposePath1(ControlIndex)) {
    if (mDebug)
      mLog << "mLikelihoodOnPathNew=NaN mPriorOnPathNew=NaN" << endl
           << "mAnatomicalPriorNew=NaN" << endl
           << "mLikelihoodOffPathNew=NaN mPriorOffPathNew=NaN" << endl
           << "mLikelihoodOn-OffPathNew=NaN mPriorOn-OffPathNew=NaN" << endl
           << "mPathLengthNew=NaN" << endl
           << "mLikelihoodOnPath=NaN mPriorOnPath=NaN" << endl
           << "mAnatomicalPrior=NaN" << endl
           << "mLikelihoodOffPath=NaN mPriorOffPath=NaN" << endl
           << "mLikelihoodOff-OnPath=NaN mPriorOff-OnPath=NaN" << endl
           << "mPathLength=NaN" << endl;

    return false;
  }

  ProposeDiffusionParameters();

  if (!AcceptPath())
    return false;

  return true;
}

//
// Propose path by perturbing spline control points
//
bool Coffin::ProposePath() {
  vector<bool>::iterator isrej;
  vector<int>::const_iterator coord = mControlPoints.begin();
  vector<int>::const_iterator cpoint = mControlPointsNew.begin();
  vector<int>::iterator newcoord = mControlPointsNew.begin();
  vector<float>::const_iterator pstd = mProposalStd.begin();
  vector<float>::iterator jump = mControlPointJumps.begin();
  vector<float>::iterator phi, theta;

  // Perturb current control points
  if (mDebug)
    mLog << "Jump norms: ";
  for (isrej = mRejectControl.begin(); isrej < mRejectControl.end(); isrej++) {
    double norm = 0;

    for (int ii = 0; ii < 3; ii++) {
      *jump = round((*pstd) * PDFgaussian());
      *newcoord = *coord + (int) *jump;

      *jump *= *jump;
      norm += *jump;

      jump++;
      pstd++;
      coord++;
      newcoord++;
    }

    if (mDebug)
      mLog << sqrt(norm) << " ";

    if (norm > 0) {
      jump -= 3;
      for (int ii = 0; ii < 3; ii++) {
        *jump /= norm;
        jump++;
      }
    }

    // Check that new control point is in mask
    if (!IsInMask(cpoint))
      *isrej = true;

    cpoint += 3;
  }

  if (mDebug)
    mLog << endl;

  // Check that new end points are in end ROIs
  if (!IsInRoi(mControlPointsNew.begin(), mRoi1))
    mRejectControl[0] = true;

  if (!IsInRoi(mControlPointsNew.end() - 3, mRoi2))
    mRejectControl[mNumControl-1] = true;

  for (isrej = mRejectControl.begin(); isrej < mRejectControl.end(); isrej++)
    if (*isrej)
      return false;

  // Interpolate spline from proposed control points
  mSpline.SetControlPoints(mControlPointsNew);
  if (!mSpline.InterpolateSpline()) {
    mRejectSpline = true;
    return false;
  }
    
  mSpline.ComputeTangent();

  // Get proposed path points
  mPathPointsNew.clear();
  mPathPointsNew.insert(mPathPointsNew.begin(),
                        mSpline.GetAllPointsBegin(), mSpline.GetAllPointsEnd());

  // Get proposed path tangent angles
  mPathPhiNew.resize(mPathPointsNew.size());
  phi = mPathPhiNew.begin();
  mPathThetaNew.resize(mPathPointsNew.size());
  theta = mPathThetaNew.begin();
  for (vector<float>::const_iterator ipt = mSpline.GetTangentBegin();
                                     ipt < mSpline.GetTangentEnd(); ipt += 3) {
    *phi = atan2(ipt[1], ipt[0]);
    *theta = acos(ipt[2] / sqrt(ipt[0]*ipt[0] + ipt[1]*ipt[1] + ipt[2]*ipt[2]));
    phi++;
    theta++;
  }

  return true;
}

//
// Propose path by perturbing spline control points (single point updates)
//
bool Coffin::ProposePath1(int ControlIndex) {
  const int offset = ControlIndex * 3;
  double norm = 0;
  vector<int>::const_iterator coord = mControlPoints.begin() + offset;
  vector<int>::const_iterator cpoint = mControlPointsNew.begin() + offset;
  vector<int>::iterator newcoord = mControlPointsNew.begin() + offset;
  vector<float>::const_iterator pstd = mProposalStd.begin() + offset;
  vector<float>::iterator jump = mControlPointJumps.begin() + offset;
  vector<float>::iterator phi, theta;

  copy(mControlPoints.begin(), mControlPoints.end(), mControlPointsNew.begin());

  // Perturb current control point
  for (int ii = 0; ii < 3; ii++) {
    *jump = round((*pstd) * PDFgaussian());
    *newcoord = *coord + (int) *jump;

    *jump *= *jump;
    norm += *jump;

    jump++;
    pstd++;
    coord++;
    newcoord++;
  }

  if (norm > 0) {
    jump -= 3;
    for (int ii = 0; ii < 3; ii++) {
      *jump /= norm;
      jump++;
    }
  }

  // Check that new control point is in mask and, if end point, in end ROI
  if (!IsInMask(cpoint) ||
      ((ControlIndex == 0) && !IsInRoi(cpoint, mRoi1)) ||
      ((ControlIndex == mNumControl-1) && !IsInRoi(cpoint, mRoi2))) {
    mRejectControl[ControlIndex] = true;
    return false;
  }

  // Interpolate spline from proposed control points
  mSpline.SetControlPoints(mControlPointsNew);
  if (!mSpline.InterpolateSpline()) {
    mRejectSpline = true;
    return false;
  }
    
  mSpline.ComputeTangent();

  // Get proposed path points
  mPathPointsNew.clear();
  mPathPointsNew.insert(mPathPointsNew.begin(),
                        mSpline.GetAllPointsBegin(), mSpline.GetAllPointsEnd());

  // Get proposed path tangent angles
  mPathPhiNew.resize(mPathPointsNew.size());
  phi = mPathPhiNew.begin();
  mPathThetaNew.resize(mPathPointsNew.size());
  theta = mPathThetaNew.begin();
  for (vector<float>::const_iterator ipt = mSpline.GetTangentBegin();
                                     ipt < mSpline.GetTangentEnd(); ipt += 3) {
    *phi = atan2(ipt[1], ipt[0]);
    *theta = acos(ipt[2] / sqrt(ipt[0]*ipt[0] + ipt[1]*ipt[1] + ipt[2]*ipt[2]));
    phi++;
    theta++;
  }

  return true;
}

//
// Propose diffusion parameters by sampling from their marginal posteriors
//
void Coffin::ProposeDiffusionParameters() {
  vector<int>::const_iterator ipt;

  // Sample parameters on proposed path
  for (ipt = mPathPointsNew.begin(); ipt < mPathPointsNew.end(); ipt += 3) {
    Bite *ivox = mDataMask[ipt[0] + ipt[1]*mNx + ipt[2]*mNxy];
    ivox->SampleParameters();
  }

  // Sample parameters on current path
  for (ipt = mPathPoints.begin(); ipt < mPathPoints.end(); ipt += 3) {
    Bite *ivox = mDataMask[ipt[0] + ipt[1]*mNx + ipt[2]*mNxy];
    ivox->SampleParameters();
  }
}

//
// Determine if proposed path will be accepted
//
bool Coffin::AcceptPath() {
  double neglogratio;
  vector<int>::const_iterator ipt;
  vector<float>::const_iterator phi, theta;

  mLikelihoodOnPathNew = 0;
  mPriorOnPathNew = 0;
  mLikelihoodOffPathNew = 0;
  mPriorOffPathNew = 0;

  // Compute posterior on proposed path
  phi = mPathPhiNew.begin();
  theta = mPathThetaNew.begin();
  for (ipt = mPathPointsNew.begin(); ipt < mPathPointsNew.end(); ipt += 3) {
    Bite *ivox = mDataMask[ipt[0] + ipt[1]*mNx + ipt[2]*mNxy];

    if ( (mPathPrior0 || !mSegPrior0.empty() || mAsegTrain)
         && !ivox->IsPriorSet() ) {
      vector<int> atlaspt(3);

      MapToAtlas(atlaspt, ipt);

      ivox->SetPrior(mPathPrior0, mPathPrior1,
                     mSegPrior0, mSegPrior1, mAsegTrain, mPathTrain, mAseg,
                     atlaspt[0], atlaspt[1], atlaspt[2]);
    }

    ivox->ComputeLikelihoodOffPath();
    ivox->ComputeLikelihoodOnPath(*phi, *theta);
    if (ivox->IsFZero()) {
      mRejectF = true;
      return false;
    }
    if (ivox->IsThetaZero()) {
      mAcceptTheta = true;
      return false;
    }
    ivox->ComputePriorOffPath();
    ivox->ComputePriorOnPath();

    if (mDebug & 0)
      mLog << ipt[0] << " " << ipt[1] << " " << ipt[2] << " "
           << ivox->GetLikelihoodOffPath() << " "
           << ivox->GetPriorOffPath() << " "
           << ivox->GetPathPriorOffPath() << " "
           << ivox->GetLikelihoodOnPath() << " "
           << ivox->GetPriorOnPath() << " "
           << ivox->GetPathPriorOnPath() << endl;

    mLikelihoodOnPathNew += ivox->GetLikelihoodOnPath();
    mPriorOnPathNew += ivox->GetPriorOnPath();

    mLikelihoodOffPathNew += ivox->GetLikelihoodOffPath();
    mPriorOffPathNew += ivox->GetPriorOffPath();

    phi++;
    theta++;
  }

  mAnatomicalPriorNew = ComputeAnatomicalPrior(mPathPointsNew);

  mPosteriorOnPathNew  = mLikelihoodOnPathNew  + mPriorOnPathNew
                                               + mAnatomicalPriorNew;
  mPosteriorOffPathNew = mLikelihoodOffPathNew + mPriorOffPathNew;

  if (mDebug)
    mLog << "mLikelihoodOnPathNew=" << mLikelihoodOnPathNew << " "
         << "mPriorOnPathNew=" << mPriorOnPathNew << endl
         << "mAnatomicalPriorNew=" << mAnatomicalPriorNew << endl
         << "mLikelihoodOffPathNew=" << mLikelihoodOffPathNew << " "
         << "mPriorOffPathNew=" << mPriorOffPathNew << endl
         << "mLikelihoodOn-OffPathNew="
         << mLikelihoodOnPathNew-mLikelihoodOffPathNew << " "
         << "mPriorOn-OffPathNew=" << mPriorOnPathNew-mPriorOffPathNew << endl
         << "mPathLengthNew=" << mPathPointsNew.size()/3 << endl;

  mLikelihoodOnPath = 0;
  mPriorOnPath = 0;
  mLikelihoodOffPath = 0;
  mPriorOffPath = 0;

  // Compute posterior on current path
  phi = mPathPhi.begin();
  theta = mPathTheta.begin();
  for (ipt = mPathPoints.begin(); ipt < mPathPoints.end(); ipt += 3) {
    Bite *ivox = mDataMask[ipt[0] + ipt[1]*mNx + ipt[2]*mNxy];

    if ( (mPathPrior0 || !mSegPrior0.empty() || mAsegTrain)
         && !ivox->IsPriorSet() ) {
      vector<int> atlaspt(3);

      MapToAtlas(atlaspt, ipt);

      ivox->SetPrior(mPathPrior0, mPathPrior1,
                     mSegPrior0, mSegPrior1, mAsegTrain, mPathTrain, mAseg,
                     atlaspt[0], atlaspt[1], atlaspt[2]);
    }

    ivox->ComputeLikelihoodOffPath();
    ivox->ComputeLikelihoodOnPath(*phi, *theta);
    if (ivox->IsFZero()) {
      mAcceptF = true;
      return false;
    }
    if (ivox->IsThetaZero()) {
      mRejectTheta = true;
      return false;
    }
    ivox->ComputePriorOffPath();
    ivox->ComputePriorOnPath();

    mLikelihoodOnPath += ivox->GetLikelihoodOnPath();
    mPriorOnPath += ivox->GetPriorOnPath();

    mLikelihoodOffPath += ivox->GetLikelihoodOffPath();
    mPriorOffPath += ivox->GetPriorOffPath();

    phi++;
    theta++;
  }

  mPosteriorOnPath  = mLikelihoodOnPath  + mPriorOnPath + mAnatomicalPrior;
  mPosteriorOffPath = mLikelihoodOffPath + mPriorOffPath;

  if (mDebug)
    mLog << "mLikelihoodOnPath=" << mLikelihoodOnPath << " "
         << "mPriorOnPath=" << mPriorOnPath << endl
         << "mAnatomicalPrior=" << mAnatomicalPrior << endl
         << "mLikelihoodOffPath=" << mLikelihoodOffPath << " "
         << "mPriorOffPath=" << mPriorOffPath << endl
         << "mLikelihoodOff-OnPath="
         << mLikelihoodOffPath-mLikelihoodOnPath << " "
         << "mPriorOff-OnPath=" << mPriorOffPath-mPriorOnPath << endl
         << "mPathLength=" << mPathPoints.size()/3 << endl;

  neglogratio = (mPosteriorOnPathNew - mPosteriorOffPathNew)
                / (mPathPointsNew.size()/3) 
              + (mPosteriorOffPath   - mPosteriorOnPath)
                / (mPathPoints.size()/3);
//neglogratio = mLikelihoodOnPathNew / (mPathPointsNew.size()/3)
//            - mLikelihoodOnPath / (mPathPoints.size()/3);

  if (mDebug)
    mLog << "alpha=" << exp(-neglogratio) << endl;

  // Accept or reject based on ratio of posteriors
  if (drand48() < exp(-neglogratio))
    return true;

  mRejectPosterior = true;
  return false;
}

//
// Compute prior on path given anatomical segmentation labels around path
//
double Coffin::ComputeAnatomicalPrior(vector<int> &PathPoints) {
  const double darc = mNumArc / (double) (PathPoints.size()/3);
  double larc = 0, prior = 0;
  vector<unsigned int>::const_iterator imatch;
  vector< vector<unsigned int> >::const_iterator iidlocal = mIdsLocal.begin(),
                                                 iidnear  = mIdsNear.begin(),
                                                 iid;
  vector< vector<float> >::const_iterator iprlocal = mPriorLocal.begin(), 
                                          iprnear  = mPriorNear.begin(),
                                          ipr;

  if (mPriorLocal.empty() && mPriorNear.empty())
    return 0;

  for (vector<int>::const_iterator ipt = PathPoints.begin();
                                   ipt != PathPoints.end(); ipt += 3) {
    vector<int> atlaspt(3);
    MapToAtlas(atlaspt, ipt);
    const int ix0 = atlaspt[0], iy0 = atlaspt[1], iz0 = atlaspt[2];
    const float seg0 = MRIgetVoxVal(mAseg, ix0, iy0, iz0, 0);

    // Find prior given local neighbor labels
    iid = iidlocal;
    ipr = iprlocal;

    for (vector<int>::const_iterator idir = mDirLocal.begin();
                                     idir != mDirLocal.end(); idir += 3) {
      const int ix = ix0 + idir[0],
                iy = iy0 + idir[1],
                iz = iz0 + idir[2];

      imatch = find(iid->begin(), iid->end(),
                    (unsigned int) MRIgetVoxVal(mAseg,
                                   ((ix > -1 && ix < mNxAtlas) ? ix : ix0),
                                   ((iy > -1 && iy < mNyAtlas) ? iy : iy0),
                                   ((iz > -1 && iz < mNzAtlas) ? iz : iz0),
                                   0));

      if (imatch < iid->end())
        prior += ipr->at(imatch - iid->begin());
      else
        prior += *(ipr->end() - 1);

      iid += mNumArc;
      ipr += mNumArc;
    }

    // Find prior given nearest neighbor labels
    iid = iidnear;
    ipr = iprnear;

    for (vector<int>::const_iterator idir = mDirNear.begin();
                                     idir != mDirNear.end(); idir += 3) {
      int dist = 0, ix = ix0 + idir[0],
                    iy = iy0 + idir[1],
                    iz = iz0 + idir[2];
      float seg = seg0;

      while ((ix > -1) && (ix < mNxAtlas) &&
             (iy > -1) && (iy < mNyAtlas) &&
             (iz > -1) && (iz < mNzAtlas) && (seg == seg0)) {
        seg = MRIgetVoxVal(mAseg, ix, iy, iz, 0);
        dist++;

        ix += idir[0];
        iy += idir[1];
        iz += idir[2];
      }

      imatch = find(iid->begin(), iid->end(), (unsigned int) seg);
      //mAsegDistArc[inear].push_back(dist);

      if (imatch < iid->end())
        prior += ipr->at(imatch - iid->begin());
      else
        prior += *(ipr->end() - 1);

      iid += mNumArc;
      ipr += mNumArc;
    }

    larc += darc;

    if (larc > 1)  {	// Move to the next segment
      larc -= 1;
      iidlocal++;
      iprlocal++;
      iidnear++;
      iprnear++;
    }
  }

  return prior;
}

//
// Copy newly accepted path over current path
//
void Coffin::UpdatePath() {
  copy(mControlPointsNew.begin(), mControlPointsNew.end(), 
       mControlPoints.begin());

  mPathPoints.resize(mPathPointsNew.size());
  copy(mPathPointsNew.begin(), mPathPointsNew.end(), mPathPoints.begin());

  mPathPhi.resize(mPathPhiNew.size());
  copy(mPathPhiNew.begin(), mPathPhiNew.end(), mPathPhi.begin());

  mPathTheta.resize(mPathThetaNew.size());
  copy(mPathThetaNew.begin(), mPathThetaNew.end(), mPathTheta.begin());

  mLikelihoodOnPath  = mLikelihoodOnPathNew; 
  mLikelihoodOffPath = mLikelihoodOffPathNew; 
  mPriorOnPath       = mPriorOnPathNew; 
  mPriorOffPath      = mPriorOffPathNew; 
  mPosteriorOnPath   = mPosteriorOnPathNew;
  mPosteriorOffPath  = mPosteriorOffPathNew;
  mAnatomicalPrior   = mAnatomicalPriorNew; 
}

//
// Update control point acceptance rate
//
void Coffin::UpdateAcceptanceRate() {
  vector<float>::const_iterator jump = mControlPointJumps.begin();
  
  if (!(mAcceptF || mAcceptTheta))
    // Update length of accepted jumps for all control points
    for (vector<float>::iterator span = mAcceptSpan.begin();
                                 span < mAcceptSpan.end(); span++) {
      *span += *jump;
      jump++;
    }
}

//
// Update control point rejection rate
//
void Coffin::UpdateRejectionRate() {
  vector<float>::const_iterator jump = mControlPointJumps.begin();

  if (mDebug)
    if (mRejectPosterior)
      mLog << "Reject due to posterior" << endl;
    else if (mRejectSpline)
      mLog << "Reject due to spline" << endl;
    else
      mLog << "Reject due to control point" << endl;

  if (mRejectPosterior || mRejectSpline)
    // Update length of rejected jumps all control points
    for (vector<float>::iterator span = mRejectSpan.begin();
                                 span < mRejectSpan.end(); span++) {
      *span += *jump;
      jump++;
    }
  else {
    vector<float>::iterator span = mRejectSpan.begin();

    // Update length of rejected jumps for specific rejected control points
    for (vector<bool>::const_iterator isrej = mRejectControl.begin();
                                      isrej < mRejectControl.end(); isrej++)
      if (*isrej)
        for (int ii = 0; ii < 3; ii++) {
          *span += *jump;
          jump++;
          span++;
        }
      else {
        jump += 3;
        span += 3;
      }
  }
}

//
// Update control point acceptance and rejection rates (single point update)
//
void Coffin::UpdateAcceptRejectRate1() {
  vector<int>::iterator acount = mAcceptCount.begin();
  vector<int>::iterator rcount = mRejectCount.begin();
  vector<float>::const_iterator jump = mControlPointJumps.begin();
  vector<float>::iterator aspan = mAcceptSpan.begin();
  vector<float>::iterator rspan = mRejectSpan.begin();

  for (vector<bool>::const_iterator isrej = mRejectControl.begin();
                                    isrej < mRejectControl.end(); isrej++)
    if (*isrej) {	// Increment rejection count and jump length
      for (int ii = 0; ii < 3; ii++) {
        *rspan += *jump;
        jump++;
        aspan++;
        rspan++;
      }

      (*rcount)++;
      acount++;
      rcount++;
    }
    else {		// Increment acceptance count and jump length
      for (int ii = 0; ii < 3; ii++) {
        *aspan += *jump;
        jump++;
        aspan++;
        rspan++;
      }

      (*acount)++;
      acount++;
      rcount++;
    }

  if (mDebug) {
    mLog << "Normalized jumps: ";
    for (jump = mControlPointJumps.begin(); jump < mControlPointJumps.end();
                                            jump++)
      mLog << sqrt(*jump) << " ";
    mLog << endl;

    acount = mAcceptCount.begin();
    rcount = mRejectCount.begin();

    mLog << "Acceptance counts: ";
    for (vector<bool>::const_iterator isrej = mRejectControl.begin();
                                      isrej < mRejectControl.end(); isrej++) {
      mLog << *acount << "/" << *acount + *rcount << "="
           << *acount / float(*acount + *rcount) << " ";
      acount++;
      rcount++;
    }
    mLog << endl;

    aspan = mAcceptSpan.begin();
    rspan = mRejectSpan.begin();

    mLog << "Acceptance spans: ";
    for (vector<float>::const_iterator pstd = mProposalStd.begin();
                                       pstd < mProposalStd.end(); pstd++) {
      mLog << *aspan << "/" << *aspan + *rspan << "="
           << *aspan / (*aspan + *rspan) << " ";

      aspan++;
      rspan++;
    }
    mLog << endl;
  }
}

//
// Update control point proposal distribution
//
void Coffin::UpdateProposalStd() {
  vector<float>::const_iterator aspan = mAcceptSpan.begin();
  vector<float>::const_iterator rspan = mRejectSpan.begin();

  for (vector<float>::iterator pstd = mProposalStd.begin();
                               pstd < mProposalStd.end(); pstd++) {
    *pstd *= sqrt((*aspan + 1) / (*rspan + 1));

    aspan++;
    rspan++;
  }

  if (mDebug) {
    mLog << "Proposal STDs: ";
    for (vector<float>::const_iterator pstd = mProposalStd.begin();
                                       pstd < mProposalStd.end(); pstd++)
      mLog << *pstd << " ";
    mLog << endl;
  }

  fill(mAcceptCount.begin(), mAcceptCount.end(), 0);
  fill(mRejectCount.begin(), mRejectCount.end(), 0);

  fill(mAcceptSpan.begin(), mAcceptSpan.end(), 0.0);
  fill(mRejectSpan.begin(), mRejectSpan.end(), 0.0);
}

//
// Save current path as an MCMC sample
//
void Coffin::SavePath() {
  // Save current control points
  mControlPointSamples.insert(mControlPointSamples.end(),
                              mControlPoints.begin(), mControlPoints.end());

  // Save current path length
  mPathLengthSamples.push_back(mPathPoints.size()/3);

  // Save current path energy
  mLikelihoodOnPathSamples.push_back(mLikelihoodOnPath);
  mPriorOnPathSamples.push_back(mPriorOnPath + mAnatomicalPrior);
  mPosteriorOnPathSamples.push_back(mPosteriorOnPath);

  // Add current path to path posterior
  for (vector<int>::const_iterator ipt = mPathPoints.begin();
                                   ipt < mPathPoints.end(); ipt += 3) {
    const int ix = ipt[0], iy = ipt[1], iz = ipt[2];
      
    MRIsetVoxVal(mPathSamples, ix, iy, iz, 0, 
                 MRIgetVoxVal(mPathSamples, ix, iy, iz, 0) + 1);
  }

  // Keep track of MAP path
  if (mPosteriorOnPath / (mPathPoints.size()/3) < mPosteriorOnPathMap) {
    copy(mControlPoints.begin(), mControlPoints.end(), 
         mControlPointsMap.begin());
    mPathPointsMap.resize(mPathPoints.size());
    copy(mPathPoints.begin(), mPathPoints.end(), mPathPointsMap.begin());
    mPosteriorOnPathMap = mPosteriorOnPath / (mPathPoints.size()/3);
  }
}

//
// Check that a point is inside the mask
//
bool Coffin::IsInMask(vector<int>::const_iterator Point) {
  return (Point[0] > -1) && (Point[0] < mMask->width) &&
         (Point[1] > -1) && (Point[1] < mMask->height) &&
         (Point[2] > -1) && (Point[2] < mMask->depth) &&
         (MRIgetVoxVal(mMask, Point[0], Point[1], Point[2], 0) > 0);
}

//
// Check that a point is inside an ROI
//
bool Coffin::IsInRoi(vector<int>::const_iterator Point, MRI *Roi) {
  vector<int> outpoint(3);

  MapToAtlas(outpoint, Point);

  return (outpoint[0] > -1) && (outpoint[0] < Roi->width) &&
         (outpoint[1] > -1) && (outpoint[1] < Roi->height) &&
         (outpoint[2] > -1) && (outpoint[2] < Roi->depth) &&
         (MRIgetVoxVal(Roi, outpoint[0], outpoint[1], outpoint[2], 0) > 0);
}

//
// Map point coordinates from diffusion space to atlas space
//
void Coffin::MapToAtlas(vector<int> &OutPoint,
                        vector<int>::const_iterator InPoint) {
  if (!mAffineReg.IsEmpty()) {
    vector<float> point(InPoint, InPoint+3);

    mAffineReg.ApplyXfm(point, point.begin());
#ifndef NO_CVS_UP_IN_HERE
    if (!mNonlinReg.IsEmpty())
      mNonlinReg.ApplyXfm(point, point.begin());
#endif

    for (int k = 0; k < 3; k++)
      OutPoint[k] = (int) round(point[k]);
  }
  else
    for (int k = 0; k < 3; k++)
      OutPoint[k] = InPoint[k];
}

//
// Write output files
//
void Coffin::WriteOutputs() {
  char fname[PATH_MAX];
  MRI *out1 = MRIclone(mMask, NULL);
  MRI *out2 = MRIclone(mMask, NULL);

  cout << "Writing output files to " << mOutDir << endl;
  mLog << "Writing output files to " << mOutDir << endl;

  // Save volume of path samples
  sprintf(fname, "%s/Fsamples_1_1.nii.gz", mOutDir);
  MRIwrite(mPathSamples, fname);

  // Save volumes of end point samples
  for (vector<int>::const_iterator icpt = mControlPointSamples.begin();
                                 icpt < mControlPointSamples.end(); icpt += 3) {
    int ix, iy, iz;

    ix = icpt[0];
    iy = icpt[1];
    iz = icpt[2];
    MRIsetVoxVal(out1, ix, iy, iz, 0, MRIgetVoxVal(out1, ix, iy, iz, 0) + 1);

    icpt += 3 * (mNumControl-1);

    ix = icpt[0];
    iy = icpt[1];
    iz = icpt[2];
    MRIsetVoxVal(out2, ix, iy, iz, 0, MRIgetVoxVal(out2, ix, iy, iz, 0) + 1);
  }

  sprintf(fname, "%s/Lsamples_1_1.nii.gz", mOutDir);
  MRIwrite(out1, fname);

  sprintf(fname, "%s/Tsamples_1_1.nii.gz", mOutDir);
  MRIwrite(out2, fname);

  // Save control point samples
  sprintf(fname, "%s/SAMPLES_1_1.txt", mOutDir);
  ofstream sampfile(fname, ios::out);

  if (!sampfile) {
    cout << "ERROR: Could not open " << fname << " for writing" << endl;
    exit(1);
  }

  for (vector<int>::const_iterator icpt = mControlPointSamples.begin();
                                   icpt < mControlPointSamples.end(); icpt += 3)
    sampfile << icpt[0] << " " << icpt[1] << " " << icpt[2] << endl;

  // Save length of path samples
  sprintf(fname, "%s/LENGTH_1_1.txt", mOutDir);
  ofstream lenfile(fname, ios::out);

  if (!lenfile) {
    cout << "ERROR: Could not open " << fname << " for writing" << endl;
    exit(1);
  }

  for (vector<int>::const_iterator ilen = mPathLengthSamples.begin();
                                   ilen < mPathLengthSamples.end(); ilen++)
    lenfile << *ilen << endl;

  // Save likelihood and prior of path samples
  sprintf(fname, "%s/FOLLOW_LIKELIHOOD_1_1.txt", mOutDir);
  ofstream likefile(fname, ios::out);

  if (!likefile) {
    cout << "ERROR: Could not open " << fname << " for writing" << endl;
    exit(1);
  }

  vector<float>::const_iterator ilik = mLikelihoodOnPathSamples.begin();
  for (vector<float>::const_iterator ipri = mPriorOnPathSamples.begin();
                                     ipri < mPriorOnPathSamples.end(); ipri++) {
    likefile << *ilik << " " << *ipri << endl;
    ilik++;
  }

  // Save MAP control point sample
  sprintf(fname, "%s/CONTROLS_1_1.txt", mOutDir);
  ofstream mapfile(fname, ios::out);

  if (!mapfile) {
    cout << "ERROR: Could not open " << fname << " for writing" << endl;
    exit(1);
  }

  for (vector<int>::const_iterator icpt = mControlPointsMap.begin();
                                   icpt < mControlPointsMap.end(); icpt += 3)
    mapfile << icpt[0] << " " << icpt[1] << " " << icpt[2] << endl;

  // Save MAP path sample
  MRIclear(out1);
  for (vector<int>::const_iterator ipt = mPathPointsMap.begin();
                                   ipt < mPathPointsMap.end(); ipt += 3) {
    const int ix = ipt[0], iy = ipt[1], iz = ipt[2];
    MRIsetVoxVal(out1, ix, iy, iz, 0, MRIgetVoxVal(out1, ix, iy, iz, 0) + 1);
  }

  sprintf(fname, "%s/CONTROLS_1_1_spline.nii.gz", mOutDir);
  MRIwrite(out1, fname);

  MRIfree(&out1);
  MRIfree(&out2);
}

