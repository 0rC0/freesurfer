#ifndef GCA_MORPH_H
#define GCA_MORPH_H

#include "mri.h"
#include "gca.h"
#include "transform.h"

#define GCAM_UNLABELED   0
#define GCAM_LABELED     1

#define EXP_K            20.0
#define GCAM_RAS         1
#define GCAM_VOX         2

typedef struct
{
  Real   origx ;      //  mri original src voxel position (using lta)
  Real   origy ;
  Real   origz ;
  Real   x ;          //  updated original src voxel position
  Real   y ;
  Real   z ;
  Real   xs ;         //  not saved
  Real   ys ;
  Real   zs ;
  int    xn ;         /* node coordinates */
  int    yn ;         //  prior voxel position
  int    zn ;
  int    label ;      
  int    n ;          /* index in gcan structure */
  float  prior ;
  GC1D   *gc ;
  float  log_p ;         /* current log probability of this sample */
  float  dx, dy, dz;     /* current gradient */
  float  odx, ody, odz ; /* previous gradient */
  float  area ;
  float  orig_area ;
  int    status ;       /* ignore likelihood term */
  char   invalid;       /* if invalid = 1, then don't use this structure */
	float  label_dist ;   /* for computing label dist */
	float  last_se ;
	float  predicted_val ; /* weighted average of all class means in a ball around this node */
} GCA_MORPH_NODE, GMN ;

typedef struct
{
  int  width, height ,depth ;
  GCA  *gca ;          // using a separate GCA data (not saved)
  GMN  ***nodes ;
  int  neg ;
  double exp_k ;
  int  spacing ;
  MRI  *mri_xind ;    /* MRI ->gcam transform */
  MRI  *mri_yind ;
  MRI  *mri_zind ;
  VOL_GEOM   src;            /* src for the transform       */
  VOL_GEOM   atlas ;            /* dst for the transform       */
	int        ninputs ;
	int     type ;  
	int     status ;
} GCA_MORPH, GCAM ;

typedef struct
{
	int x ;   /* node coords */
	int y ;
	int z ;
} NODE_BIN ;

typedef struct
{
	int      nnodes ;
	NODE_BIN *node_bins ;
} NODE_BUCKET ;

typedef struct
{
	NODE_BUCKET ***nodes ;
	int         width ;
	int         height ;
	int         depth ;   /* should match the input image */
} NODE_LOOKUP_TABLE, NLT ;

typedef struct
{
  int    write_iterations ;
  double dt ;
  float  momentum ;
  int    niterations ;
  char   base_name[STRLEN] ;
  double l_log_likelihood ;
  double l_likelihood ;
  double l_area ;
  double l_jacobian ;
  double l_smoothness ;
  double l_distance ;
  double l_label ;
	double l_binary ;
  double l_map ;
	double l_area_intensity;
  double tol ;
  int    levels ;
  FILE   *log_fp ;
  int    start_t ;
  MRI    *mri ;
  float  max_grad ;
  double exp_k ;
  double sigma ;
  int    navgs ;
  double label_dist ;
  int    noneg ;
  double ratio_thresh ;
  int    integration_type ;
  int    nsmall ;
  int    relabel ;    /* are we relabeling (i.e. using MAP label, or just max prior label) */
  int    relabel_avgs ; /* what level to start relabeling at */
  int    reset_avgs ;   /* what level to reset metric properties at */
	MRI    *mri_binary ;
	MRI    *mri_binary_smooth ;
	double start_rms ;
	double end_rms ;
	NODE_LOOKUP_TABLE *nlt ;
} GCA_MORPH_PARMS, GMP ;


GCA_MORPH *GCAMalloc(int width, int height, int depth) ;
int       GCAMinit(GCA_MORPH *gcam, MRI *mri, GCA *gca, TRANSFORM *transform, int relabel) ;
int       GCAMinitLookupTables(GCA_MORPH *gcam) ;
int       GCAMwrite(GCA_MORPH *gcam, char *fname) ;
GCA_MORPH *GCAMread(char *fname) ;
int       GCAMfree(GCA_MORPH **pgcam) ;
MRI       *GCAMmorphFromAtlas(MRI *mri_src, GCA_MORPH *gcam, MRI *mri_dst) ;
MRI       *GCAMmorphToAtlasWithDensityCorrection(MRI *mri_src, GCA_MORPH *gcam, MRI *mri_morphed, int frame) ;
MRI       *GCAMmorphToAtlas(MRI *mri_src, GCA_MORPH *gcam, MRI *mri_dst, int frame) ;
MRI       *GCAMmorphToAtlasType(MRI *mri_src, GCA_MORPH *gcam, MRI *mri_dst, int frame, int interp_type) ;
int       GCAMmarkNegativeNodesInvalid(GCA_MORPH *gcam) ;
int       GCAMregister(GCA_MORPH *gcam, MRI *mri, GCA_MORPH_PARMS *parms) ;
int       GCAMregisterLevel(GCA_MORPH *gcam, MRI *mri, MRI *mri_smooth, 
                            GCA_MORPH_PARMS *parms) ;
int       GCAMsampleMorph(GCA_MORPH *gcam, float x, float y, float z, 
                          float *pxd, float *pyd, float *pzd) ;
int       GCAMcomputeLabels(MRI *mri, GCA_MORPH *gcam) ;
MRI       *GCAMbuildMostLikelyVolume(GCA_MORPH *gcam, MRI *mri) ;
MRI       *GCAMbuildVolume(GCA_MORPH *gcam, MRI *mri) ;
int       GCAMinvert(GCA_MORPH *gcam, MRI *mri) ;
int       GCAMfreeInverse(GCA_MORPH *gcam) ;
int       GCAMcomputeMaxPriorLabels(GCA_MORPH *gcam) ;
int       GCAMcomputeOriginalProperties(GCA_MORPH *gcam) ;
int       GCAMstoreMetricProperties(GCA_MORPH *gcam) ;
int       GCAMcopyNodePositions(GCA_MORPH *gcam, int from, int to) ;
MRI       *GCAMextract_density_map(MRI *mri_seg, MRI *mri_intensity, GCA_MORPH *gcam, int label, MRI *mri_out) ;
int       GCAMsample(GCA_MORPH *gcam, float xv, float yv, float zv, float *px, float *py, float *pz) ;
int      GCAMinitLabels(GCA_MORPH *gcam, MRI *mri_labeled) ;
GCA_MORPH *GCAMlinearTransform(GCA_MORPH *gcam_src, MATRIX *m_vox2vox, GCA_MORPH *gcam_dst) ;
int GCAMresetLabelNodeStatus(GCA_MORPH *gcam)  ;

#define GCAM_USE_LIKELIHOOD        0x0000
#define GCAM_IGNORE_LIKELIHOOD     0x0001
#define GCAM_LABEL_NODE            0x0002
#define GCAM_BINARY_ZERO           0x0004
#define GCAM_NEVER_USE_LIKELIHOOD  0x0008

int GCAMresetLikelihoodStatus(GCA_MORPH *gcam) ;
int GCAMsetLabelStatus(GCA_MORPH *gcam, int label, int status) ;
int GCAMsetStatus(GCA_MORPH *gcam, int status) ;
int GCAMapplyTransform(GCA_MORPH *gcam, TRANSFORM *transform) ;
int GCAMinitVolGeom(GCAM *gcam, MRI *mri_src, MRI *mri_atlas) ;
MRI *GCAMmorphFieldFromAtlas(GCA_MORPH *gcam, MRI *mri, int fno, int which, int save_inversion) ;
double GCAMcomputeRMS(GCA_MORPH *gcam, MRI *mri, GCA_MORPH_PARMS *parms) ;
int GCAMexpand(GCA_MORPH *gcam, float distance) ;
GCA_MORPH *GCAMregrid(GCA_MORPH *gcam, MRI *mri_dst, int pad, 
											GCA_MORPH_PARMS *parms, MRI **pmri) ;
int GCAMvoxToRas(GCA_MORPH *gcam) ;
int GCAMrasToVox(GCA_MORPH *gcam, MRI *mri) ;
int GCAMaddStatus(GCA_MORPH *gcam, int status_bit) ;
int GCAMremoveStatus(GCA_MORPH *gcam, int status_bit) ;

typedef struct
{
	int nlabels ;
	int input_labels[1000] ;
	int output_labels[1000] ;
	double means[1000] ;
} GCAM_LABEL_TRANSLATION_TABLE ;
MRI  *GCAMinitDensities(GCA_MORPH *gcam, MRI *mri_lowres_seg, MRI *mri_intensities,
												GCAM_LABEL_TRANSLATION_TABLE *gcam_ltt) ;
#define ORIGINAL_POSITIONS  0
#define ORIG_POSITIONS      ORIGINAL_POSITIONS
#define SAVED_POSITIONS     1
#define CURRENT_POSITIONS   2

#define GCAM_INTEGRATE_OPTIMAL 0
#define GCAM_INTEGRATE_FIXED   1
#define GCAM_INTEGRATE_BOTH    2

#define GCAM_VALID             0
#define GCAM_AREA_INVALID      1
#define GCAM_POSITION_INVALID  2
#define GCAM_BORDER_NODE       4

#define GCAM_X_GRAD 0
#define GCAM_Y_GRAD 1
#define GCAM_Z_GRAD 2
#define GCAM_LABEL  3
#define GCAM_ORIGX  4
#define GCAM_ORIGY  5
#define GCAM_ORIGZ  6
#define GCAM_NODEX  7
#define GCAM_NODEY  8
#define GCAM_NODEZ  9

#endif
