#ifndef MRISURF_H
#define MRISURF_H

#include "macros.h"
#include "mri.h"
#include "volume_io.h"
#include "image.h"
#include "matrix.h"

#define TALAIRACH_COORDS     0
#define SPHERICAL_COORDS     1
#define ELLIPSOID_COORDS     2

#define VERTICES_PER_FACE    4
#define TRIANGLES_PER_FACE   2
#define ANGLES_PER_TRIANGLE  3

#define INFLATED_NAME        "inflated"
#define SMOOTH_NAME          "smoothwm"
#define SPHERE_NAME          "sphere"

typedef struct _area_label
{
  char     name[100] ;        /* name of region */
  float    cx ;               /* centroid x */
  float    cy ;               /* centroid y */
  float    cz ;               /* centroid z */
  int      label ;            /* an identifier (used as an index) */
} MRIS_AREA_LABEL ;

/*
  the vertices in the face structure are arranged in 
  counter-clockwise fashion when viewed from the outside.
  */
typedef struct face_type_
{
  int    v[VERTICES_PER_FACE];           /* vertex numbers of this face */
  float  nx[TRIANGLES_PER_FACE] ;
  float  ny[TRIANGLES_PER_FACE] ;
  float  nz[TRIANGLES_PER_FACE] ;
  float  area[TRIANGLES_PER_FACE] ;
  float  orig_area[TRIANGLES_PER_FACE] ;
  float  angle[TRIANGLES_PER_FACE][ANGLES_PER_TRIANGLE] ;
  float  orig_angle[TRIANGLES_PER_FACE][ANGLES_PER_TRIANGLE]  ;
  int    ripflag;                        /* ripped face */
#if 0
  float logshear,shearx,sheary;  /* compute_shear */
#endif
} face_type, FACE ;

typedef struct vertex_type_
{
  float x,y,z;            /* curr position */
  float nx,ny,nz;         /* curr normal */
  double dx, dy, dz ;     /* current change in position */
  double odx, ody, odz ;  /* last change of position (for momentum) */
  double tdx, tdy, tdz ;  /* temporary storage for averaging gradient */
  float  ox,oy,oz;        /* last position (for undoing time steps) */
  float  curv;            /* curr curvature */
  float  val;             /* scalar data value (file: rh.val, sig2-rh.w) */
  float  cx, cy, cz ;     /* coordinates in canonical coordinate system */
  float  tx, ty, tz ;     /* tmp coordinate storage */
  float  origx, origy,
         origz ;          /* original coordinates */
  float e1x, e1y, e1z ;  /* 1st basis vector for the local tangent plane */
  float e2x, e2y, e2z ;  /* 2nd basis vector for the local tangent plane */
#if 0
  float mx,my,mz;        /* last movement */
  float dipx,dipy,dipz;  /* dipole position */
  float dipnx,dipny,dipnz; /* dipole orientation */
  float nc;              /* curr length normal comp */
  float onc;             /* last length normal comp */
  float val2;            /* complex comp data value (file: sig3-rh.w) */
  float valbak;          /* scalar data stack */
  float val2bak;         /* complex comp data stack */
  float oval;            /* last scalar data (for smooth_val) */
  float stat;            /* statistic */
  int undefval;          /* [previously dist=0] */
  int old_undefval;      /* for smooth_val_sparse */
  int fixedval;          /* [previously val=0] */
  float dist;            /* dist from sampled point [defunct: or 1-cos(a)] */
  float fieldsign;       /* fieldsign--final: -1,0,1 (file: rh.fs) */
  float fsmask;          /* significance mask (file: rh.fm) */
#endif
  int num;               /* number neighboring faces */
  int *f;                /* array neighboring face numbers */
  int *n;                /* [0-3, num long] */
  int vnum;              /* number neighboring vertices */
  int *v;                /* array neighboring vertex numbers, vnum long */
  int v2num ;            /* number of 2-connected neighbors */
  int v3num ;            /* number of 3-connected neighbors */
  int vtotal ;          /* total # of neighbors, will be same as one of above*/
  float d ;              /* for distance calculations */
  int nsize ;            /* size of neighborhood (e.g. 1, 2, 3) */
#if 0
  float bnx,bny,obnx,obny;                       /* boundary normal */
  float *fnx ;           /* face normal - x component */
  float *fny ;           /* face normal - y component */
  float *fnz ;           /* face normal - z component */
  float *tri_area ;      /* array of triangle areas - num long */
  float *orig_tri_area ;     /* array of original triangle areas - num long */
  float *tri_angle ;     /* angles of each triangle this vertex belongs to */
  float *orig_tri_angle ;/* original values of above */
  int   annotation;      /* area label (defunct--now from label file name!) */
  float stress;          /* explosion */
  float logarat,ologarat,sqrtarat; /* for area term */
  float logshear,shearx,sheary,oshearx,osheary;  /* for shear term */
  float smx,smy,smz,osmx,osmy,osmz;              /* smoothed curr,last move */
  int   oripflag,origripflag;  /* cuts flags */
  float coord[3];
  float ftmp ;          /* temporary floating pt. storage */
#endif
  float theta, phi ;     /* parameterization */
  int   marked;          /* for a variety of uses */
  int   ripflag ;
  int   border;          /* flag */
  float area,origarea ;
  int   tethered ;
  float K ;             /* Gaussian curvature */
  float H ;             /* mean curvature */
  float k1, k2 ;        /* the principal curvatures */
  int   label ;         /* is this vertex part of a labeled region? */
  float *dist ;         /* original distance to neighboring vertices */
  float *dist_orig ;    /* original distance to neighboring vertices */
  int   neg ;           /* 1 if the normal vector is inverted */
} vertex_type, VERTEX ;

typedef struct
{
  int          nvertices ;      /* # of vertices on surface */
  int          nfaces ;         /* # of faces on surface */
  VERTEX       *vertices ;
  FACE         *faces ;
  float        xctr ;
  float        yctr ;
  float        zctr ;
  float        xlo ;
  float        ylo ;
  float        zlo ;
  float        xhi ;
  float        yhi ;
  float        zhi ;
  VERTEX       *v_temporal_pole ;
  VERTEX       *v_frontal_pole ;
  VERTEX       *v_occipital_pole ;
  float        max_curv ;
  float        min_curv ;
  float        total_area ;
  float        orig_area ;
  float        neg_area ;
  int          zeros ;
  int          hemisphere ;            /* which hemisphere */
  int          initialized ;
  General_transform transform ;   /* the next two are from this struct */
  Transform         *linear_transform ;
  Transform         *inverse_linear_transform ;
  int               free_transform ;
  double       radius ;           /* radius (if status==MRIS_SPHERE) */
  float        a, b, c ;          /* ellipsoid parameters */
  char         fname[100] ;       /* file it was originally loaded from */
  float        Hmin ;             /* min mean curvature */
  float        Hmax ;             /* max mean curvature */
  float        Kmin ;             /* min Gaussian curvature */
  float        Kmax ;             /* max Gaussian curvature */
  double       Ktotal ;           /* total Gaussian curvature */
  int          status ;           /* type of surface (e.g. sphere, plane) */
  int          patch ;            /* if a patch of the surface */
  int          nlabels ;
  MRIS_AREA_LABEL *labels ;       /* nlabels of these (may be null) */
  int          nsize ;            /* size of neighborhoods */
  float        avg_nbrs ;         /* mean # of vertex neighbors */
  void         *vp ;              /* for misc. use */
} MRI_SURFACE, MRIS ;


#define IPFLAG_HVARIABLE           0x0001   /* for parms->flags */

#define INTEGRATE_LINE_MINIMIZE    0  /* use quadratic fit */
#define INTEGRATE_MOMENTUM         1
#define INTEGRATE_ADAPTIVE         2
#define INTEGRATE_LM_SEARCH        3  /* binary search for minimum */

#define MRIS_SURFACE               0
#define MRIS_PATCH                 1
#define MRIS_CUT                   MRIS_PATCH
#define MRIS_PLANE                 2
#define MRIS_ELLIPSOID             3
#define MRIS_SPHERE                4
#define MRIS_PARAMETERIZED_SPHERE  5


/*
  the following structure is built after the surface has been morphed
  into a parameterizable surface (such as a sphere or an ellipsoid). It
  contains relevant data (such as curvature or sulc) which represented
  in the plane of the parameterization. This then allows operations such
  as convolution to be carried out directly in the parameterization space
  */

/* 
   define it for a sphere, the first axis (x or u) goes from 0 to pi,
   the second axis (y or v) goes from 0 to 2pi.
*/
#define PHI_MAX                   M_PI
#define U_MAX                     PHI_MAX
#define X_DIM(mrisp)              (mrisp->Ip->cols)
#define U_DIM(mrisp)              X_DIM(mrisp)
#define PHI_DIM(mrisp)            X_DIM(mrisp)
#define PHI_MAX_INDEX(mrisp)      (X_DIM(mrisp)-1)
#define U_MAX_INDEX(mrisp)        (PHI_MAX_INDEX(mrisp))

#define THETA_MAX                 (2.0*M_PI)
#define Y_DIM(mrisp)              (mrisp->Ip->rows)
#define V_DIM(mrisp)              (Y_DIM(mrisp))
#define THETA_DIM(mrisp)          (Y_DIM(mrisp))
#define THETA_MAX_INDEX(mrisp)    (Y_DIM(mrisp)-1)
#define V_MAX_INDEX(mrisp)        (THETA_MAX_INDEX(mrisp))

typedef struct
{
  MRI_SURFACE  *mris ;        /* surface it came from (if any) */
  IMAGE        *Ip ;        
#if 0
  /* 2-d array of curvature, or sulc in parms */
  float        data[X_DIM][Y_DIM] ;
  float        distances[X_DIM][Y_DIM] ;
  int          vertices[X_DIM][Y_DIM] ;   /* vertex numbers */
#endif
  float        sigma ;                    /* blurring scale */
} MRI_SURFACE_PARAMETERIZATION, MRI_SP ;

#define L_ANGLE              0.25f /*was 0.01*/ /* coefficient of angle term */
#define L_AREA               1.0f    /* coefficient of angle term */
#define N_AVERAGES           4096
#define WRITE_ITERATIONS     10
#define NITERATIONS          1
#define NO_PROJECTION        0
#define PROJECT_ELLIPSOID    1
#define ELLIPSOID_PROJECTION PROJECT_ELLIPSOID
#define PROJECT_PLANE        2
#define PLANAR_PROJECTION    PROJECT_PLANE
#define PROJECT_SPHERE       3

#define MOMENTUM             0.8
#define EPSILON              0.25

#define TOL                  1e-6  /* minimum error tolerance for unfolding */
#define DELTA_T              0.1


typedef struct
{
  double  tol ;               /* tolerance for terminating a step */
  float   l_angle ;           /* coefficient of angle term */
  float   l_area ;            /* coefficient of (negative) area term */
  float   l_parea ;           /* coefficient of (all) area term */
  float   l_corr ;            /* coefficient of correlation term */
  float   l_curv ;            /* coefficient of curvature term */
  float   l_spring ;          /* coefficient of spring term */
  float   l_boundary ;        /* coefficient of boundary term */
  float   l_dist ;            /* coefficient of distance term */
  float   l_neg ;
  int     n_averages ;        /* # of averages */
  int     min_averages ;
  int     nbhd_size ;
  int     max_nbrs ;
  int     write_iterations ;  /* # of iterations between saving movies */
  char    base_name[100] ;    /* base name of movie files */
  int     projection ;        /* what kind of projection to do */
  int     niterations ;       /* max # of time steps */
  float   a ;
  float   b ;
  float   c ;                 /* ellipsoid parameters */
  int     start_t ;           /* starting time step */
  int     t ;                 /* current time */
  FILE    *fp ;               /* for logging results */
  float   Hdesired ;          /* desired (mean) curvature */
  int     integration_type ;  /* line minimation or momentum */
  double  momentum ;
  double  dt ;                /* current time step (for momentum only) */
  double  base_dt ;           /* base time step (for momentum only) */
  int     flags ;
  double  dt_increase ;       /* rate at which time step increases */
  double  dt_decrease ;       /* rate at which time step decreases */
  double  error_ratio ;       /* ratio at which to undo step */
  double  epsilon ;           /* constant in Sethian inflation */
  double  fi_desired ;        /* desired folding index */
  double  ici_desired ;       /* desired intrinsic curvature index */
  double  starting_sse ;
  double  ending_sse ;
  double  scale ;             /* scale current distances to mimic spring */
  MRI_SP  *mrisp ;            /* parameterization  of this surface */
  int     frame_no ;          /* current frame in template parameterization */
  MRI_SP  *mrisp_template ;   /* parameterization of canonical surface */
  float   sigma ;             /* blurring scale */
} INTEGRATION_PARMS ;


/* can't include this before structure, as stats.h includes this file. */
#include "stats.h"


MRI_SURFACE  *MRISread(char *fname) ;
MRI_SURFACE  *MRISfastRead(char *fname) ;
int          MRISreadOriginalProperties(MRI_SURFACE *mris, char *sname) ;
int          MRISreadCanonicalCoordinates(MRI_SURFACE *mris, char *sname) ;
int          MRISreadPatch(MRI_SURFACE *mris, char *pname) ;
int          MRISreadTriangleProperties(MRI_SURFACE *mris, char *mris_fname) ;
int          MRISreadBinaryCurvature(MRI_SURFACE *mris, char *mris_fname) ;
int          MRISreadCurvatureFile(MRI_SURFACE *mris, char *fname) ;
int          MRISreadValues(MRI_SURFACE *mris, char *fname) ;

int          MRISwrite(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteAscii(MRI_SURFACE *mris, char *fname) ;
int          MRISwritePatchAscii(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteCurvature(MRI_SURFACE *mris, char *fname) ;
int          MRISnormalizeCurvature(MRI_SURFACE *mris) ;
int          MRISnonmaxSuppress(MRI_SURFACE *mris) ;
int          MRISscaleCurvatures(MRI_SURFACE *mris, 
                                 float min_curv, float max_curv) ;
int          MRISwriteAreaError(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteAngleError(MRI_SURFACE *mris, char *fname) ;
int          MRISwritePatch(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteValues(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteTriangleProperties(MRI_SURFACE *mris, char *mris_fname);
int          MRISaverageCurvatures(MRI_SURFACE *mris, int navgs) ;

MRI_SURFACE  *MRISalloc(int nvertices, int nfaces) ;
int          MRISfree(MRI_SURFACE **pmris) ;
MRI_SURFACE  *MRISprojectOntoSphere(MRI_SURFACE *mris_src, 
                                       MRI_SURFACE *mris_dst, double r) ;
MRI_SURFACE  *MRISprojectOntoEllipsoid(MRI_SURFACE *mris_src, 
                                       MRI_SURFACE *mris_dst, 
                                       float a, float b, float c) ;
int          MRISsetNeighborhoodSize(MRI_SURFACE *mris, int nsize) ;
int          MRISresetNeighborhoodSize(MRI_SURFACE *mris, int nsize) ;
int          MRISsampleDistances(MRI_SURFACE *mris, int *nbr_count,int n_nbrs);
int          MRISscaleDistances(MRI_SURFACE *mris, float scale) ;
MRI_SURFACE  *MRISradialProjectOntoEllipsoid(MRI_SURFACE *mris_src, 
                                             MRI_SURFACE *mris_dst, 
                                             float a, float b, float c);
MRI_SURFACE  *MRISclone(MRI_SURFACE *mris_src) ;
MRI_SURFACE  *MRIScenter(MRI_SURFACE *mris_src, MRI_SURFACE *mris_dst) ;
MRI_SURFACE  *MRIStalairachTransform(MRI_SURFACE *mris_src, 
                                    MRI_SURFACE *mris_dst);
MRI_SURFACE  *MRISunfold(MRI_SURFACE *mris, INTEGRATION_PARMS *parms, 
                         int max_passes) ;
int          MRISregister(MRI_SURFACE *mris, MRI_SP *mrisp_template, 
                           INTEGRATION_PARMS *parms, int max_passes) ;
MRI_SURFACE  *MRISunfoldOnSphere(MRI_SURFACE *mris, INTEGRATION_PARMS *parms, 
                                 int max_passes);
MRI_SURFACE  *MRISflatten(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
MRI_SURFACE  *MRISremoveNegativeVertices(MRI_SURFACE *mris, 
                                         INTEGRATION_PARMS *parms,
                                         int min_neg, float min_neg_pct) ;
int          MRIScomputeFaceAreas(MRI_SURFACE *mris) ;
int          MRISupdateEllipsoidSurface(MRI_SURFACE *mris) ;
MRI_SURFACE  *MRISrotate(MRI_SURFACE *mris_src, MRI_SURFACE *mris_dst, 
                         float dphi, float dtheta) ;

MRI          *MRISwriteIntoVolume(MRI_SURFACE *mris, MRI *mri) ;
MRI_SURFACE  *MRISreadFromVolume(MRI *mri, MRI_SURFACE *mris) ;



int          MRIScomputeTriangleProperties(MRI_SURFACE *mris, int no_angles) ;
int          MRISsampleStatVolume(MRI_SURFACE *mris, STAT_VOLUME *sv,int time,
                                  int use_talairach_xform);

int          MRISflattenPatch(MRI_SURFACE *mris) ;
int          MRISflattenPatchRandomly(MRI_SURFACE *mris) ;
int          MRIScomputeMeanCurvature(MRI_SURFACE *mris) ;

int          MRIScomputeEulerNumber(MRI_SURFACE *mris, int *pnvertices, 
                                    int *pnfaces, int *pnedges) ;
int          MRIStopologicalDefectIndex(MRI_SURFACE *mris) ;
int          MRISremoveTopologicalDefects(MRI_SURFACE *mris,float curv_thresh);

int          MRIScomputeSecondFundamentalForm(MRI_SURFACE *mris) ;
int          MRISuseAreaErrors(MRI_SURFACE *mris) ;
int          MRISuseGaussianCurvature(MRI_SURFACE *mris) ;
int          MRISclearCurvature(MRI_SURFACE *mris) ;
int          MRISuseMeanCurvature(MRI_SURFACE *mris) ;
int          MRIScomputeCurvatureIndices(MRI_SURFACE *mris, 
                                         double *pici, double *pfi) ;

double       MRIScomputeFolding(MRI_SURFACE *mris) ;

int          MRISprojectOntoCylinder(MRI_SURFACE *mris, float radius) ;
double       MRISaverageRadius(MRI_SURFACE *mris) ;
int          MRISinflateBrain(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
double       MRIStotalVariation(MRI_SURFACE *mris) ;
double       MRIStotalVariationDifference(MRI_SURFACE *mris) ;
double       MRIScurvatureError(MRI_SURFACE *mris, double Kd) ;
MRI_SURFACE  *MRISscaleBrain(MRI_SURFACE *mris_src, MRI_SURFACE *mris_dst, 
                             float scale) ;
int          MRISstoreMetricProperties(MRI_SURFACE *mris) ;
int          MRISstoreMeanCurvature(MRI_SURFACE *mris) ;
int          MRISreadTetherFile(MRI_SURFACE *mris, char *fname, float radius) ;
int          MRISreadVertexPositions(MRI_SURFACE *mris, char *fname) ;
int          MRIScomputeMetricProperties(MRI_SURFACE *mris) ;
int          MRISrestoreOldPositions(MRI_SURFACE *mris) ;
int          MRISstoreCurrentPositions(MRI_SURFACE *mris) ;
int          MRISupdateSurface(MRI_SURFACE *mris) ;
double       MRISpercentDistanceError(MRI_SURFACE *mris) ;


MRI_SP       *MRISPcombine(MRI_SP *mrisp, MRI_SP *mrisp_template, int fno);
double       MRISPfunctionVal(MRI_SURFACE_PARAMETERIZATION *mrisp, 
                              MRI_SURFACE *mris,
                              float x, float y, float z, int fno) ;
MRI_SP       *MRIStoParameterization(MRI_SURFACE *mris, MRI_SP *mrisp, 
                                     float scale, int fno) ;
MRI_SURFACE  *MRISfromParameterization(MRI_SP *mrisp, MRI_SURFACE *mris,
                                       int fno) ;
MRI_SP       *MRISgradientToParameterization(MRI_SURFACE *mris, MRI_SP *mrisp, 
                                     float scale) ;
MRI_SURFACE  *MRISgradientFromParameterization(MRI_SP*mrisp,MRI_SURFACE *mris);
MRI_SP       *MRISPblur(MRI_SP *mrisp_src, MRI_SP *mrisp_dst, float sigma,
                        int fno) ;
MRI_SP       *MRISPalign(MRI_SP *mrisp_orig, MRI_SP *mrisp_src, 
                         MRI_SP *mrisp_tmp, MRI_SP *mrisp_dst) ;
MRI_SP       *MRISPtranslate(MRI_SP *mrisp_src, MRI_SP *mrisp_dst, int du, 
                             int dv) ;
MRI_SP       *MRISPclone(MRI_SP *mrisp_src) ;
MRI_SP       *MRISPalloc(float scale, int nfuncs) ;
int          MRISPfree(MRI_SP **pmrisp) ;
MRI_SP       *MRISPread(char *fname) ;
int          MRISPwrite(MRI_SP *mrisp, char *fname) ;


#define ORIGINAL_VERTICES   0
#define ORIG_VERTICES       ORIGINAL_VERTICES
#define GOOD_VERTICES       1
#define TMP_VERTICES        2
#define CANONICAL_VERTICES  3


int          MRISsaveVertexPositions(MRI_SURFACE *mris, int which) ;
int          MRISrestoreVertexPositions(MRI_SURFACE *mris, int which) ;

/* constants for vertex->tethered */
#define TETHERED_NONE           0
#define TETHERED_FRONTAL_POLE   1
#define TETHERED_OCCIPITAL_POLE 2
#define TETHERED_TEMPORAL_POLE  3

#define MAX_TALAIRACH_Y         100.0f

/* constants for mris->hemisphere */
#define LEFT_HEMISPHERE         0
#define RIGHT_HEMISPHERE        1

#if 0
#define DEFAULT_A  44.0f
#define DEFAULT_B  122.0f
#define DEFAULT_C  70.0f
#else
#define DEFAULT_A  122.0f
#define DEFAULT_B  122.0f
#define DEFAULT_C  122.0f
#endif
#define DEFAULT_RADIUS  128.0f

#define MAX_DIM    DEFAULT_B

#if 1
#define DT_INCREASE  1.1 /* 1.03*/
#define DT_DECREASE  0.5
#else
#define DT_INCREASE  1.0 /* 1.03*/
#define DT_DECREASE  1.0
#endif
#define DT_MIN       0.01
#define ERROR_RATIO  1.03  /* 1.01 then 1.03 */

#define NO_LABEL     -1
#endif
