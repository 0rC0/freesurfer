//
// fastmarching.h
// 
// original: written by Florent Segonne (Feb, 2005)
//
//
// Warning: Do not edit the following four lines.  CVS maintains them.
// Revision Author: $Author: tosa $
// Revision Date  : $Date: 2005/02/11 21:25:08 $
// Revision       : $Revision: 1.1 $

// include guard
#ifndef fastmarching_h
#define fastmarching_h

// The following is usable from C
#ifdef __cplusplus
extern "C" {
#endif

#include "mri.h"
MRI *MRIextractDistanceMap(MRI *mri_src, MRI *mri_dst,int label, float max_distance, int mode);

#ifdef __cplusplus
}
#endif

// C++ portion starts here
#ifdef __cplusplus

#include <queue>
#include <functional>
#include <climits>
#include <list>

extern "C" {
#include "mrisurf.h"
#include "error.h"
}

#define mapMRI_XYZ(mri,x,y,z) for(int z =0 ; z < mri->depth ; z++) \
                 for(int y = 0 ; y < mri->height ; y++) \
                 for(int x = 0 ; x < mri->width ; x++)
const float DELTA = 1.0f;
const float EPS = 1e-6f;
const float BIG = 1e6f;

class stCoord {
 public:
	int x,y,z;
	stCoord(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
};

typedef std::list<stCoord> CoordList;
typedef CoordList::iterator CoordListIterator;
typedef CoordList::const_iterator const_CoordListIterator;

typedef enum { eAlive=0, eTrial=1, eFar=2, eForbidden=3 } eState;

template <int sign = +1>
  class FastMarching {

  // type definition for the min-heap
  class HeapCompare : std::binary_function<stCoord, stCoord, bool> {
    protected:
    MRI *mri;
    public:
    HeapCompare(MRI *_mri) : mri(_mri) {}
    bool operator() (const stCoord &a, const stCoord &b) const {
      return (sign * MRIFvox(mri,a.x,a.y,a.z) > sign * MRIFvox(mri,b.x,b.y,b.z));
    }
  };
  typedef std::priority_queue<stCoord,std::vector<stCoord>,HeapCompare> CoordHeap;
  protected:
  //members
  MRI *mri;
  CoordHeap trial_heap;
  CoordList alive_list;
  MRI *status;
  float limit;
  int width,height,depth;
	
  public: 
	
  //constructor, destructor
  FastMarching(MRI *_mri): mri(_mri),trial_heap(HeapCompare(mri)) {
    status=MRIalloc(mri->width,mri->height,mri->depth,MRI_UCHAR);
    width=mri->width;
    height=mri->height;
    depth=mri->depth;
    Init();
  }
	
  ~FastMarching(){
    MRIfree(&status);
  }

  // Iterators on the list of alive points
  const_CoordListIterator begin() const { return alive_list.begin(); }
  const_CoordListIterator end() const { return alive_list.end(); }

  void Init(){
    while(!trial_heap.empty()) trial_heap.pop();
    alive_list.clear();
    mapMRI_XYZ(status,x,y,z) MRIvox(status,x,y,z)=eFar;
  }
	
  void SetLimit(float _limit){ limit=_limit;}

  void AddTrialPoint(const int x, const int y, const int z) {
    if (MRIvox(status,x,y,z) == eFar) {
      MRIvox(status,x,y,z) = eTrial;
      MRIFvox(mri,x,y,z)=_UpdateValue(x,y,z);			
      trial_heap.push(stCoord(x,y,z));
    }
  }
  void AddAlivePoint(const int x, const int y, const int z) {
    if ( MRIvox(status,x,y,z) == eFar) {
      MRIvox(status,x,y,z) = eAlive;
      alive_list.push_back(stCoord(x,y,z));
    }
  }
  void InitTrialFromAlive() {

    for (const_CoordListIterator i=begin();i!=end();i++) {
      const int x = i->x;
      const int y = i->y;
      const int z = i->z;
      if (x>0) AddTrialPoint(x-1,y,z);
      if (x<width-1) AddTrialPoint(x+1,y,z);
      if (y>0) AddTrialPoint(x,y-1,z);
      if (y<height-1) AddTrialPoint(x,y+1,z);
      if (z>0) AddTrialPoint(x,y,z-1);
      if (z<depth-1) AddTrialPoint(x,y,z+1);
    }
  }
	
  void Run(float _limit) {
    // On memorise l'etendue du fast marching
    limit = _limit;
		
    // Tant qu'il y a des points trial
    while(!trial_heap.empty()) {
			
      // On enleve de la pile le point trial de valeur la plus faible
      stCoord pt = trial_heap.top();
			
      // Si on a atteint la limite
      if (sign*MRIFvox(mri,pt.x,pt.y,pt.z) >= sign*limit) {
	// On vide la pile et on marque tous ses points comme far
	while (!trial_heap.empty()) {
	  pt = trial_heap.top();
	  MRIvox(status,pt.x,pt.y,pt.z) = eFar;
	  MRIFvox(mri,pt.x,pt.y,pt.z) = limit;
	  trial_heap.pop();
	} 
	return;
      }
      // On enleve le point de la pile et on l'ajoute a la liste des points alive
      trial_heap.pop();	
      MRIvox(status,pt.x,pt.y,pt.z) = eAlive;
      alive_list.push_back(pt);
			
      // On determine les voisins du point et on les met a jour
      const int x = pt.x;
      const int y = pt.y;
      const int z = pt.z;
      if (x>0) _UpdatePoint(x-1,y,z);
      if (x<width-1) _UpdatePoint(x+1,y,z);
      if (y>0) _UpdatePoint(x,y-1,z);
      if (y<height-1) _UpdatePoint(x,y+1,z);
      if (z>0) _UpdatePoint(x,y,z-1);
      if (z<depth-1) _UpdatePoint(x,y,z+1);
    }
  }
	
  private:
  void _UpdatePoint(int x, int y, int z) {
    const eState st = (eState)MRIvox(status,x,y,z);
    if (st == eFar) {
      MRIvox(status,x,y,z)= eTrial;
      MRIFvox(mri,x,y,z) = _UpdateValue(x,y,z);
      trial_heap.push(stCoord(x,y,z));
    }
    else if (st == eTrial) {
      MRIFvox(mri,x,y,z) = _UpdateValue(x,y,z);
    }
  }

  float _GetValue(const int x, const int y, const int z) const {
    if (MRIvox(status,x,y,z) == eFar) return limit;
    return MRIFvox(mri,x,y,z);
  }

  float _UpdateValue(const int x, const int y, const int z) const {
		
    // On prend le minimum de chaque paire de voisins
    float A = (x==0) ? sign*_GetValue(x+1,y,z) : (x==width-1) ? sign*_GetValue(x-1,y,z) : MIN( sign*_GetValue(x+1,y,z), sign*_GetValue(x-1,y,z) );
    float B = (y==0) ? sign*_GetValue(x,y+1,z) : (y==height-1) ? sign*_GetValue(x,y-1,z) : MIN( sign*_GetValue(x,y+1,z), sign*_GetValue(x,y-1,z) );
    float C = (z==0) ? sign*_GetValue(x,y,z+1) : (z==depth-1) ? sign*_GetValue(x,y,z-1) : MIN( sign*_GetValue(x,y,z+1), sign*_GetValue(x,y,z-1) );
    
    // On reordonne les valeurs pour avoir C>=B>=A
    if (A>B) { const float tmp = A; A = B; B = tmp; }
    if (B>C) { const float tmp = B; B = C; C = tmp; }
    if (A>B) { const float tmp = A; A = B; B = tmp; }
	
    // On suppose sol>=C : premier trinome
    float a = 3;
    float b = -(A+B+C);
    float c = A*A + B*B + C*C - 1;
    float delta = b*b - a*c;
    if (delta>=0) {
      const float sol = ( -b + ::sqrt(delta) ) / a;
      // On a bien sol>=C, on a gagne
      if (sol+EPS>=C) return sign*sol;
    }
		
    // On supppose B<=sol<C : deuxieme trinome
    a = 2;
    b = -(A+B);
    c = A*A + B*B - 1;
    delta = b*b - a*c;
    if (delta>=0) {
      const float sol = ( -b + ::sqrt(delta) ) / a;
      // On a bien sol>=B, on a gagne
      if (sol+EPS>=B) return sign*sol;
    }
		
    // On suppose A<=sol<B
    return sign*(A+1);
  }

  public:
  void InitFromMRI(MRI *_mri, int label){
	
    if(sign>0){
      mapMRI_XYZ(_mri,x,y,z){
	int val=MRIvox(_mri,x,y,z);
	if(val!=label) {
	  MRIFvox(mri,x,y,z)=limit;
	  continue;
	}
	MRIvox(status,x,y,z)=eForbidden;
      }
    }else{
      mapMRI_XYZ(_mri,x,y,z){
	int val=MRIvox(_mri,x,y,z);
	if(val==label) {
	  MRIFvox(mri,x,y,z)=limit;
	  continue;
	}
	MRIvox(status,x,y,z)=eForbidden;
      }
    }

    mapMRI_XYZ(_mri,x,y,z){
      int val1=MRIvox(_mri,x,y,z),val2;
      int px,py,pz;
      px = (x < width-1) ? x+1:x;
      py = (y < height-1) ? y+1:y;
      pz = (z < depth-1) ? z+1:z;
      bool add = false;
      val2=MRIvox(_mri,px,y,z);
      if(val1!=val2 && (val1==label || val2==label)) { add = true; _AddAlivePoint(px,y,z); }
      val2=MRIvox(_mri,x,py,z);
      if (val1!=val2 && (val1==label || val2==label)) { add = true; _AddAlivePoint(x,py,z); }
      val2=MRIvox(_mri,x,y,pz);
      if (val1!=val2 && (val1==label || val2==label)) { add = true; _AddAlivePoint(x,y,pz); }
      if (add) _AddAlivePoint(x,y,z);
    }
		
    InitTrialFromAlive();
 
  } 

  void _AddAlivePoint(int x, int y, int z) {
    const eState st = (eState)MRIvox(status,x,y,z);
    if (st==eForbidden) return;
    MRIFvox(mri,x,y,z)=sign*0.5;
    AddAlivePoint(x,y,z);
  }
	
};
#endif  // C++ portion ends here


#endif // ifndef fastmarching_h
