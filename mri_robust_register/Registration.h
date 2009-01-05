//
// Registration is a class to compute a robust registration
//
// written by Martin Reuter
// Nov. 4th ,2008
//

#ifndef Registration_H
#define Registration_H

#ifdef __cplusplus
extern "C" {
#endif
#include "matrix.h"
#include "mri.h"
#ifdef __cplusplus
}
#endif

#include <utility>
#include <string>
#include <vector>

class Registration
{
  public:
    Registration():transonly(false),rigid(true),robust(true), sat(-1),iscale(false),rtype(1),subsamplesize(-1),
                  mri_source(NULL),mri_target(NULL), Minit(NULL),Mfinal(NULL),lastp(NULL), mri_indexing(NULL) {};
    Registration(MRI * s, MRI *t):transonly(false),rigid(true),robust(true), sat(-1),iscale(false),rtype(1),subsamplesize(-1),
                   mri_source(MRIcopy(s,NULL)),mri_target(MRIcopy(t,NULL)),Minit(NULL),Mfinal(NULL),lastp(NULL),mri_indexing(NULL) {};
  
    ~Registration()
    { // we cleanup our private variables
       if (mri_source) MRIfree(&mri_source);
       if (mri_target) MRIfree(&mri_target);
       if (Minit) MatrixFree(&Minit);
       if (Mfinal) MatrixFree(&Minit);
       if (lastp) MatrixFree(&lastp);
       if (mri_indexing) MRIfree(&mri_indexing);
    }
    
  
    // Set parameters:
    void setTransonly(bool r) {transonly = r;};
    void setRigid(bool r)  {rigid = r;};
    void setRobust(bool r) {robust = r;};
    void setSaturation(double d) {sat = d;};
    void setIscale(bool i) {iscale = i;};
    void setRtype(int r)   {rtype = r;};
    //void setAllParams(bool probust=true, bool ptrans=false, bool prigid=true, bool piscale=false, int prtype = 1){robust = probust; transonly = ptrans; rigid = prigid; iscale = piscale; rtype = prtype;};
    bool isIscale()        {return iscale;};
    void setMinit(MATRIX* m){Minit = MatrixCopy(m,Minit);};
    void setSource (MRI * s){mri_source = s;};
    void setTarget (MRI * t){mri_target = t;};
    void setSubsamplesize (int sss){subsamplesize = sss;};
  
    // compute registration
    std::pair <MATRIX*, double> computeIterativeRegistration( int n=10,MRI * mriS=NULL, MRI* mriT=NULL, MATRIX* Minit = NULL, double iscaleinit = 1.0);
    std::pair <MATRIX*, double> computeMultiresRegistration (MRI * mriS= NULL, MRI* mriT= NULL, MATRIX* Minit = NULL, double iscaleinit = 1.0);
 
    bool warpSource(const std::string & fname, MATRIX* M = NULL, double is = -1);
 
    // testing
    void testRobust(const std::string & fname, int testno);
  
  protected:

    //   returns weights:
    std::pair < MATRIX*, MRI* > computeRegistrationStepW(MRI * mriS = NULL, MRI* mriT=NULL);
    //   returns param vector:
    MATRIX* computeRegistrationStepP(MRI * mriS = NULL, MRI* mriT = NULL);
    //   returns 4x4 matrix and iscale:
    std::pair <MATRIX*, double> computeRegistrationStep(MRI * mriS = NULL, MRI* mriT = NULL);
  
  
  private:
  
  // construct Ab and R:
   MATRIX* constructR(MATRIX* p);
   std::pair < MATRIX*, VECTOR* > constructAb(MRI *mriS, MRI *mriT);
   std::pair < MATRIX*, VECTOR* > constructAb2(MRI *mriS, MRI *mriT);

   // my MRI operations (helpers for construct Ab):
   MRI * convolute(MRI * mri, MRI * filter, int dir);
   MRI * getPrefilter();
   MRI * getDerfilter();
   MRI * subSample(MRI * mri);
   MRI * getBlur(MRI* mriS);
   MRI * getPartial(MRI* mriS, int dir);
   bool  getPartials(MRI* mri, MRI* & outfx, MRI* & outfy, MRI* &outfz, MRI* &outblur);
   MRI * getBlur2(MRI* mri);
   bool  getPartials2(MRI* mri, MRI* & outfx, MRI* & outfy, MRI* &outfz, MRI* &outblur);
   
   MRI * MRIvalscale(MRI *mri_src, MRI *mri_dst, double s);
   MATRIX* MRIgetZslice(MRI * mri, int slice);
   
   // conversions
   MATRIX* getMatrix(std::vector < double > d, int r, int c=-1, MATRIX* m=NULL);
   MATRIX * rt2mat(MATRIX * r, MATRIX * t, MATRIX *outM); // uses global rtype flag
   MATRIX * p2mat(MATRIX * p6, MATRIX *outM);
   MATRIX * aff2mat(MATRIX * aff, MATRIX *outM);
   std::pair < MATRIX*, double > convertP2Md(MATRIX* p);
   MATRIX * getHalfRT (MATRIX * m, MATRIX * mhalf=NULL);
   MATRIX * MatrixSqrt(MATRIX * m, MATRIX * sqrtm=NULL);
   
   // gaussian pyramid:
   std::vector < MRI* > buildGaussianPyramid (MRI * mri_in, int n);
   void freeGaussianPyramid(std::vector< MRI* >& p);

   // initial registration using pca
   MATRIX * initialize_transform(MRI *mri_in, MRI *mri_ref);
   int init_scaling(MRI *mri_in, MRI *mri_ref, MATRIX *m_L);
   int order_eigenvectors(MATRIX *m_src_evectors, MATRIX *m_dst_evectors);
   MATRIX * compute_pca(MRI *mri_in, MRI *mri_ref);
   MATRIX * pca_matrix(MATRIX *m_in_evectors, double in_means[3],
   MATRIX *m_ref_evectors, double ref_means[3]);
    bool transonly;
    bool rigid;
    bool robust;
    double sat;
    bool iscale;
    int rtype;
    int subsamplesize;
    
    MRI * mri_source;
    MRI * mri_target;
    MATRIX * Minit;
    MATRIX * Mfinal;
    double iscalefinal;
    
    // help vars
    MATRIX* lastp;
    
    MRI * mri_indexing;
};


#endif
