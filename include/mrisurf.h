/* sorry about this - the includes have gotten circular, and I don't
   know how else to fix it without incurring way too much pain.
*/
#ifndef LABEL_INCLUDED
#define LABEL_INCLUDED
#include "label.h"
#endif

#ifndef STATS_INCLUDED
#define STATS_INCLUDED
#include "stats.h"
#endif


#ifndef MRISURF_H
#define MRISURF_H

#include "macros.h"
#include "mri.h"
#include "volume_io.h"
#include "image.h"
#include "matrix.h"
#include "transform.h"
#include "const.h"
#include "label.h"
#include "colortab.h"

#define TALAIRACH_COORDS     0
#define SPHERICAL_COORDS     1
#define ELLIPSOID_COORDS     2

#define VERTICES_PER_FACE    3
#define ANGLES_PER_TRIANGLE  3

#define INFLATED_NAME        "inflated"
#define SMOOTH_NAME          "smoothwm"
#define SPHERE_NAME          "sphere"
#define ORIG_NAME            "orig"
#define WHITE_MATTER_NAME    "white"
#define GRAY_MATTER_NAME     "gray"
#define LAYERIV_NAME         "graymid"
#define GRAYMID_NAME         LAYERIV_NAME

typedef struct _area_label
{
  char     name[STRLEN] ;     /* name of region */
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
  float  nx ;
  float  ny ;
  float  nz ;
  float  area ;
  float  orig_area ;
  float  angle[ANGLES_PER_TRIANGLE] ;
  float  orig_angle[ANGLES_PER_TRIANGLE]  ;
  char   ripflag;                        /* ripped face */
#if 0
  float logshear,shearx,sheary;  /* compute_shear */
#endif
} face_type, FACE ;

#ifndef uchar
#define uchar  unsigned char
#endif
typedef struct vertex_type_
{
  float x,y,z;            /* curr position */
  float nx,ny,nz;         /* curr normal */
  float dx, dy, dz ;     /* current change in position */
  float odx, ody, odz ;  /* last change of position (for momentum) */
  float tdx, tdy, tdz ;  /* temporary storage for averaging gradient */
  float  curv;            /* curr curvature */
  float  curvbak ;
  float  val;             /* scalar data value (file: rh.val, sig2-rh.w) */
  float  imag_val ;       /* imaginary part of complex data value */
  float  cx, cy, cz ;     /* coordinates in canonical coordinate system */
  float  tx, ty, tz ;     /* tmp coordinate storage */
  float  tx2, ty2, tz2 ;     /* tmp coordinate storage */
  float  origx, origy,
         origz ;          /* original coordinates */
  float  pialx, pialy, pialz ;  /* pial surface coordinates */
  float  whitex, whitey, whitez ;  /* pial surface coordinates */
  float  infx, infy, infz; /* inflated coordinates */
  float  fx, fy, fz ;      /* flattened coordinates */
  float e1x, e1y, e1z ;  /* 1st basis vector for the local tangent plane */
  float e2x, e2y, e2z ;  /* 2nd basis vector for the local tangent plane */
#if 0
  float dipx,dipy,dipz;  /* dipole position */
  float dipnx,dipny,dipnz; /* dipole orientation */
#endif
  float nc;              /* curr length normal comp */
  float val2;            /* complex comp data value (file: sig3-rh.w) */
  float valbak;          /* scalar data stack */
  float val2bak;         /* complex comp data stack */
  float stat;            /* statistic */
#if 1
  int undefval;          /* [previously dist=0] */
  int old_undefval;      /* for smooth_val_sparse */
  int fixedval;          /* [previously val=0] */
#endif
  float fieldsign;       /* fieldsign--final: -1,0,1 (file: rh.fs) */
  float fsmask;          /* significance mask (file: rh.fm) */
  uchar num;               /* number neighboring faces */
  int   *f;                /* array neighboring face numbers */
  uchar *n;                /* [0-3, num long] */
  uchar vnum;              /* number neighboring vertices */
  int   *v;                /* array neighboring vertex numbers, vnum long */
  uchar  v2num ;            /* number of 2-connected neighbors */
  uchar  v3num ;            /* number of 3-connected neighbors */
  short  vtotal ;        /* total # of neighbors, will be same as one of above*/
  float d ;              /* for distance calculations */
  uchar nsize ;            /* size of neighborhood (e.g. 1, 2, 3) */
#if 0
  float *tri_area ;      /* array of triangle areas - num long */
  float *orig_tri_area ; /* array of original triangle areas - num long */
  float dist;            /* dist from sampled point [defunct: or 1-cos(a)] */
  float  ox,oy,oz;        /* last position (for undoing time steps) */
  float mx,my,mz;         /* last movement */
  float onc;             /* last length normal comp */
  float oval;            /* last scalar data (for smooth_val) */
  float *fnx ;           /* face normal - x component */
  float *fny ;           /* face normal - y component */
  float *fnz ;           /* face normal - z component */
  float bnx,bny,obnx,obny;                       /* boundary normal */
  float *tri_angle ;     /* angles of each triangle this vertex belongs to */
  float *orig_tri_angle ;/* original values of above */
  float stress;          /* explosion */
  float logshear,shearx,sheary,oshearx,osheary;  /* for shear term */
  float ftmp ;          /* temporary floating pt. storage */
  float logarat,ologarat,sqrtarat; /* for area term */
  float smx,smy,smz,osmx,osmy,osmz;            /* smoothed curr,last move */
#endif
  int   annotation;     /* area label (defunct--now from label file name!) */
  char   oripflag,origripflag;  /* cuts flags */
#if 0
  float coords[3];
#endif
  float theta, phi ;     /* parameterization */
  short  marked;          /* for a variety of uses */
  char   ripflag ;
  char   border;          /* flag */
  float area,origarea ;
  float K ;             /* Gaussian curvature */
  float H ;             /* mean curvature */
  float k1, k2 ;        /* the principal curvatures */
  float *dist ;         /* original distance to neighboring vertices */
  float *dist_orig ;    /* original distance to neighboring vertices */
  char   neg ;           /* 1 if the normal vector is inverted */
  float mean ;
  float mean_imag ;    /* imaginary part of complex statistic */
  float std_error ;
	unsigned long flags ;
} vertex_type, VERTEX ;

#define VERTEX_SULCAL  0x00000001L

typedef struct 
{
  int nvertices;
  unsigned int *vertex_indices;
} STRIP;

typedef struct
{
  int          nvertices ;      /* # of vertices on surface */
  int          nfaces ;         /* # of faces on surface */
  int          nstrips;
  VERTEX       *vertices ;
  FACE         *faces ;
  STRIP        *strips;
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
  float        neg_orig_area ;   /* amount of original surface in folds */
  int          zeros ;
  int          hemisphere ;            /* which hemisphere */
  int          initialized ;
  General_transform transform ;   /* the next two are from this struct (MNI transform) */
  Transform         *linear_transform ;
  Transform         *inverse_linear_transform ;
  int               free_transform ;
  double       radius ;           /* radius (if status==MRIS_SPHERE) */
  float        a, b, c ;          /* ellipsoid parameters */
  char         fname[STRLEN] ;    /* file it was originally loaded from */
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
  float        alpha ;            /* rotation around z-axis */
  float        beta ;             /* rotation around y-axis */
  float        gamma ;            /* rotation around x-axis */
  float        da, db, dg ;       /* old deltas */
  int          type ;             /* what type of surface was this initially*/
  int          max_vertices ;     /* may be bigger than nvertices */
  int          max_faces ;        /* may be bigger than nfaces */
  char         subject_name[STRLEN] ;/* name of the subject */
  float        canon_area ;
  int          noscale ;          /* don't scale by surface area if true */
  float        *dx2 ;             /* an extra set of gradient (not always alloced) */
  float        *dy2 ;
  float        *dz2 ;
	COLOR_TABLE  *ct ;
  int          useRealRAS;        /* if 0, vertex position is a conformed volume RAS with c_(r,a,s)=0 */
                                  /* if 1, verteix position is a real RAS (volume stored RAS)         */
                                  /* The default is 0.                                                */
} MRI_SURFACE, MRIS ;


#define IPFLAG_HVARIABLE           0x0001   /* for parms->flags */
#define IPFLAG_NO_SELF_INT_TEST    0x0002
#define IPFLAG_QUICK               0x0004  /* sacrifice quality for speed */
#define IPFLAG_ADD_VERTICES        0x0008

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
#define MRIS_RIGID_BODY            6


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
  float   l_nlarea ;          /* coefficient of nonlinear area term */
  float   l_nldist ;          /* coefficient of nonlinear distance term */
  float   l_corr ;            /* coefficient of correlation term */
  float   l_pcorr ;           /* polar correlation for rigid body */
  float   l_curv ;            /* coefficient of curvature term */
  float   l_scurv ;           /* coefficient of curvature term */
  float   l_link ;            /* coefficient of link term to keep white and pial vertices approximately along normal */
  float   l_spring ;          /* coefficient of spring term */
  float   l_spring_norm ;     /* coefficient of normalize spring term */
  float   l_tspring ;         /* coefficient of tangential spring term */
  float   l_nspring ;         /* coefficient of normal spring term */
  float   l_repulse ;         /* repulsize force on tessellation */
  float   l_repulse_ratio ;   /* repulsize force on tessellation */
  float   l_boundary ;        /* coefficient of boundary term */
  float   l_dist ;            /* coefficient of distance term */
  float   l_neg ;
  float   l_intensity ;       /* for settling surface at a specified val */
  float   l_sphere ;          /* for expanding the surface to a sphere */
  float   l_expand ;          /* for uniformly expanding the surface */
  float   l_grad ;            /* gradient term */
  float   l_convex ;          /* convexity term */
  float   l_tsmooth ;         /* thickness smoothness term */
  float   l_surf_repulse ;    /* repulsive orig surface (for white->pial) */
  float   l_external ;        /* external (user-defined) coefficient */
  float   l_shrinkwrap ;      /* move in if MRI=0 and out otherwise */
  int     n_averages ;        /* # of averages */
  int     min_averages ;
  int     nbhd_size ;
  int     max_nbrs ;
  int     write_iterations ;  /* # of iterations between saving movies */
  char    base_name[STRLEN] ; /* base name of movie files */
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
  double  desired_rms_height; /* desired height above tangent plane */
  double  starting_sse ;
  double  ending_sse ;
  double  scale ;             /* scale current distances to mimic spring */
  MRI_SP  *mrisp ;            /* parameterization  of this surface */
  int     frame_no ;          /* current frame in template parameterization */
  MRI_SP  *mrisp_template ;   /* parameterization of canonical surface */
  float   sigma ;             /* blurring scale */
  MRI     *mri_brain ;        /* for settling surfaace to e.g. g/w border */
  MRI     *mri_smooth ;       /* smoothed version of mri_brain */
  void    *user_parms ;       /* arbitrary spot for user to put stuff */
} INTEGRATION_PARMS ;

extern double (*gMRISexternalGradient)(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
extern double (*gMRISexternalSSE)(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
extern double (*gMRISexternalRMS)(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
extern int (*gMRISexternalTimestep)(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
extern int (*gMRISexternalRipVertices)(MRI_SURFACE *mris, INTEGRATION_PARMS *parms);
extern int (*gMRISexternalClearSSEStatus)(MRI_SURFACE *mris) ;
extern int (*gMRISexternalReduceSSEIncreasedGradients)(MRI_SURFACE *mris, double pct) ;

#define IP_USE_CURVATURE      0x0001
#define IP_NO_RIGID_ALIGN     0x0002
#define IP_RETRY_INTEGRATION  0x0004

/* can't include this before structure, as stats.h includes this file. */
/*#include "stats.h"*/
#include "label.h"

int MRISfindClosestCanonicalVertex(MRI_SURFACE *mris, float x, float y, 
                                    float z) ;
int MRISfindClosestOriginalVertex(MRI_SURFACE *mris, float x, float y, 
                                    float z) ;
int MRISfindClosestVertex(MRI_SURFACE *mris, float x, float y, float z) ;
double MRIScomputeSSE(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
double MRIScomputeSSEExternal(MRI_SURFACE *mris, INTEGRATION_PARMS *parms,
                              double *ext_sse) ;
double       MRIScomputeCorrelationError(MRI_SURFACE *mris, 
                                         MRI_SP *mrisp_template, int fno) ;
int          MRISallocExtraGradients(MRI_SURFACE *mris) ;
MRI_SURFACE  *MRISread(char *fname) ;
MRI_SURFACE  *MRISreadOverAlloc(char *fname, double pct_over) ;
MRI_SURFACE  *MRISfastRead(char *fname) ;
int          MRISreadOriginalProperties(MRI_SURFACE *mris, char *sname) ;
int          MRISreadCanonicalCoordinates(MRI_SURFACE *mris, char *sname) ;
int          MRISreadInflatedCoordinates(MRI_SURFACE *mris, char *sname) ;
int          MRISreadFlattenedCoordinates(MRI_SURFACE *mris, char *sname) ;
int          MRIScomputeCanonicalCoordinates(MRI_SURFACE *mris) ;
int          MRIScanonicalToWorld(MRI_SURFACE *mris, Real phi, Real theta,
                                  Real *pxw, Real *pyw, Real *pzw) ;
int          MRISreadPatch(MRI_SURFACE *mris, char *pname) ;
int          MRISreadPatchNoRemove(MRI_SURFACE *mris, char *pname) ;
int          MRISreadTriangleProperties(MRI_SURFACE *mris, char *mris_fname) ;
int          MRISreadBinaryCurvature(MRI_SURFACE *mris, char *mris_fname) ;
int          MRISreadCurvatureFile(MRI_SURFACE *mris, char *fname) ;
float        *MRISreadNewCurvatureVector(MRI_SURFACE *mris, char *sname) ;
float        *MRISreadCurvatureVector(MRI_SURFACE *mris, char *sname) ;
int          MRISreadFloatFile(MRI_SURFACE *mris, char *fname) ;
#define MRISreadCurvature MRISreadCurvatureFile

MRI *MRISloadSurfVals(char *srcvalfile, char *typestring, MRI_SURFACE *Surf, 
          char *subject, char *hemi, char *subjectsdir);
int          MRISreadValues(MRI_SURFACE *mris, char *fname) ;
int          MRISreadAnnotation(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteAnnotation(MRI_SURFACE *mris, char *fname) ;
int          MRISreadValuesBak(MRI_SURFACE *mris, char *fname) ;
int          MRISreadImagValues(MRI_SURFACE *mris, char *fname) ;
int          MRIScopyValuesToImagValues(MRI_SURFACE *mris) ;

int          MRIScopyValToVal2(MRI_SURFACE *mris) ;
int          MRIScopyValToVal2Bak(MRI_SURFACE *mris) ;
int          MRIScopyValToValBak(MRI_SURFACE *mris) ;
int          MRISsqrtVal(MRI_SURFACE *mris) ;
int          MRISmulVal(MRI_SURFACE *mris, float mul) ;

int          MRISwrite(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteAscii(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteVTK(MRI_SURFACE *mris, char *fname);
int          MRISwriteGeo(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteICO(MRI_SURFACE *mris, char *fname) ;
int          MRISwritePatchAscii(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteDists(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteCurvature(MRI_SURFACE *mris, char *fname) ;
int          MRISreadNewCurvatureFile(MRI_SURFACE *mris, char *fname) ;
int          MRISrectifyCurvature(MRI_SURFACE *mris) ;
int          MRISnormalizeCurvature(MRI_SURFACE *mris) ;
int          MRISnormalizeCurvatureVariance(MRI_SURFACE *mris) ;
int          MRISzeroMeanCurvature(MRI_SURFACE *mris) ;
int          MRISnonmaxSuppress(MRI_SURFACE *mris) ;
int          MRISscaleCurvatures(MRI_SURFACE *mris, 
                                 float min_curv, float max_curv) ;
int          MRISwriteAreaError(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteAngleError(MRI_SURFACE *mris, char *fname) ;
int          MRISwritePatch(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteValues(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteCurvatureToWFile(MRI_SURFACE *mris, char *fname) ;
int          MRISwriteTriangleProperties(MRI_SURFACE *mris, char *mris_fname);
int          MRISaverageCurvatures(MRI_SURFACE *mris, int navgs) ;
int          MRISminFilterCurvatures(MRI_SURFACE *mris, int niter) ;
int          MRISmaxFilterCurvatures(MRI_SURFACE *mris, int niter) ;
int          MRISaverageMarkedCurvatures(MRI_SURFACE *mris, int navgs) ;
double       MRIScomputeAverageCurvature(MRI_SURFACE *mris, double *psigma) ;
int          MRISaverageVertexPositions(MRI_SURFACE *mris, int navgs) ;

MRI_SURFACE  *MRISoverAlloc(int max_vertices, int max_faces, 
                            int nvertices, int nfaces) ;
MRI_SURFACE  *MRISalloc(int nvertices, int nfaces) ;
int          MRISfreeDists(MRI_SURFACE *mris) ;
int          MRISfree(MRI_SURFACE **pmris) ;
int   MRISintegrate(MRI_SURFACE *mris, INTEGRATION_PARMS *parms, int n_avgs);
MRI_SURFACE  *MRISprojectOntoSphere(MRI_SURFACE *mris_src, 
                                       MRI_SURFACE *mris_dst, double r) ;
MRI_SURFACE  *MRISprojectOntoEllipsoid(MRI_SURFACE *mris_src, 
                                       MRI_SURFACE *mris_dst, 
                                       float a, float b, float c) ;
int          MRISsetNeighborhoodSize(MRI_SURFACE *mris, int nsize) ;
int          MRISresetNeighborhoodSize(MRI_SURFACE *mris, int nsize) ;
int          MRISsampleDistances(MRI_SURFACE *mris, int *nbr_count,int n_nbrs);
int          MRISsampleAtEachDistance(MRI_SURFACE *mris, int nbhd_size,
                                      int nbrs_per_distance) ;
int          MRISscaleDistances(MRI_SURFACE *mris, float scale) ;
MRI_SURFACE  *MRISradialProjectOntoEllipsoid(MRI_SURFACE *mris_src, 
                                             MRI_SURFACE *mris_dst, 
                                             float a, float b, float c);
MRI_SURFACE  *MRISclone(MRI_SURFACE *mris_src) ;
MRI_SURFACE  *MRIScenter(MRI_SURFACE *mris_src, MRI_SURFACE *mris_dst) ;
int MRISorigVertexToVoxel(VERTEX *v, MRI *mri,Real *pxv, Real *pyv, Real *pzv) ;
int          MRISvertexToVoxel(VERTEX *v, MRI *mri,Real *pxv, Real *pyv, 
                               Real *pzv) ;
int          MRISworldToTalairachVoxel(MRI_SURFACE *mris, MRI *mri, 
                                       Real xw, Real yw, Real zw,
                                       Real *pxv, Real *pyv, Real *pzv) ;
int          MRIStalairachToVertex(MRI_SURFACE *mris, 
                                       Real xt, Real yt, Real zt) ;
int           MRIScanonicalToVertex(MRI_SURFACE *mris, Real phi, Real theta) ;
MRI_SURFACE  *MRIStalairachTransform(MRI_SURFACE *mris_src, 
                                    MRI_SURFACE *mris_dst);
MRI_SURFACE  *MRISunfold(MRI_SURFACE *mris, INTEGRATION_PARMS *parms, 
                         int max_passes) ;
MRI_SURFACE  *MRISquickSphere(MRI_SURFACE *mris, INTEGRATION_PARMS *parms, 
                         int max_passes) ;
int          MRISregister(MRI_SURFACE *mris, MRI_SP *mrisp_template, 
													INTEGRATION_PARMS *parms, int max_passes, float min_degrees, float max_degrees, int nangles) ;
int          MRISrigidBodyAlignLocal(MRI_SURFACE *mris, 
                                     INTEGRATION_PARMS *parms) ;
int          MRISrigidBodyAlignGlobal(MRI_SURFACE *mris, 
                                      INTEGRATION_PARMS *parms,
                                      float min_degrees,
                                      float max_degrees,
                                      int nangles) ;
MRI_SURFACE  *MRISunfoldOnSphere(MRI_SURFACE *mris, INTEGRATION_PARMS *parms, 
                                 int max_passes);
MRI_SURFACE  *MRISflatten(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
MRI_SURFACE  *MRISremoveNegativeVertices(MRI_SURFACE *mris, 
                                         INTEGRATION_PARMS *parms,
                                         int min_neg, float min_neg_pct) ;
int          MRIScomputeFaceAreas(MRI_SURFACE *mris) ;
int          MRISupdateEllipsoidSurface(MRI_SURFACE *mris) ;
MRI_SURFACE  *MRISrotate(MRI_SURFACE *mris_src, MRI_SURFACE *mris_dst, 
                         float alpha, float beta, float gamma) ;

MRI          *MRISwriteIntoVolume(MRI_SURFACE *mris, MRI *mri) ;
MRI_SURFACE  *MRISreadFromVolume(MRI *mri, MRI_SURFACE *mris) ;



int          MRIScomputeTriangleProperties(MRI_SURFACE *mris) ;
int          MRISpaintVolume(MRI_SURFACE *mris, LTA *lta, MRI *mri) ;
int          MRISsampleStatVolume(MRI_SURFACE *mris, void *sv,int time,
                                  int use_talairach_xform);

int          MRISflattenPatch(MRI_SURFACE *mris) ;
int          MRISflattenPatchRandomly(MRI_SURFACE *mris) ;
int          MRIScomputeMeanCurvature(MRI_SURFACE *mris) ;

int          MRIScomputeEulerNumber(MRI_SURFACE *mris, int *pnvertices, 
                                    int *pnfaces, int *pnedges) ;
int          MRIStopologicalDefectIndex(MRI_SURFACE *mris) ;
int          MRISremoveTopologicalDefects(MRI_SURFACE *mris,float curv_thresh);

int          MRIScomputeSecondFundamentalFormAtVertex(MRI_SURFACE *mris,
                                                      int vno, int *vertices,
                                                      int vnum) ;
int          MRIScomputeSecondFundamentalForm(MRI_SURFACE *mris) ;
int          MRISuseCurvatureDifference(MRI_SURFACE *mris) ;
int          MRISuseCurvatureStretch(MRI_SURFACE *mris) ;
int          MRISuseCurvatureMax(MRI_SURFACE *mris) ;
int          MRISuseCurvatureMin(MRI_SURFACE *mris) ;
int          MRISuseNegCurvature(MRI_SURFACE *mris) ;
int          MRISuseAreaErrors(MRI_SURFACE *mris) ;
int          MRISuseGaussianCurvature(MRI_SURFACE *mris) ;
int          MRISclearCurvature(MRI_SURFACE *mris) ;
int          MRISclearDistances(MRI_SURFACE *mris) ;
int          MRISusePrincipalCurvature(MRI_SURFACE *mris) ;
int          MRISuseMeanCurvature(MRI_SURFACE *mris) ;
int          MRIScomputeCurvatureIndices(MRI_SURFACE *mris, 
                                         double *pici, double *pfi) ;
int          MRISuseCurvatureRatio(MRI_SURFACE *mris) ;
int          MRISuseCurvatureContrast(MRI_SURFACE *mris) ;

double MRIScomputeFolding(MRI_SURFACE *mris) ;
double MRISavgInterVetexDist(MRIS *Surf, double *StdDev);
double MRISavgVetexRadius(MRIS *Surf, double *StdDev);

int          MRISprojectOntoCylinder(MRI_SURFACE *mris, float radius) ;
double       MRISaverageRadius(MRI_SURFACE *mris) ;
double       MRISaverageCanonicalRadius(MRI_SURFACE *mris) ;
double       MRISmaxRadius(MRI_SURFACE *mris) ;
int          MRISinflateToSphere(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
int          MRISinflateBrain(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
double       MRISrmsTPHeight(MRI_SURFACE *mris) ;
double       MRIStotalVariation(MRI_SURFACE *mris) ;
double       MRIStotalVariationDifference(MRI_SURFACE *mris) ;
double       MRIScurvatureError(MRI_SURFACE *mris, double Kd) ;
MRI_SURFACE  *MRISscaleBrain(MRI_SURFACE *mris_src, MRI_SURFACE *mris_dst, 
                             float scale) ;
int          MRISstoreMetricProperties(MRI_SURFACE *mris) ;
int          MRISrestoreMetricProperties(MRI_SURFACE *mris) ;
double       MRIScomputeAnalyticDistanceError(MRI_SURFACE *mris, int which,
                                              FILE *fp);
int          MRISzeroNegativeAreas(MRI_SURFACE *mris) ;
int          MRIScountNegativeTriangles(MRI_SURFACE *mris) ;
int          MRISstoreMeanCurvature(MRI_SURFACE *mris) ;
int          MRISreadTetherFile(MRI_SURFACE *mris, char *fname, float radius) ;
int          MRISreadVertexPositions(MRI_SURFACE *mris, char *fname) ;
int          MRISspringTermWithGaussianCurvature(MRI_SURFACE *mris, double gaussian_norm, double l_spring) ;
double       MRISmomentumTimeStep(MRI_SURFACE *mris, float momentum, float dt, float tol, 
																	float n_averages) ;
int          MRISapplyGradient(MRI_SURFACE *mris, double dt) ;
int          MRIScomputeNormals(MRI_SURFACE *mris) ;
int          MRIScomputeMetricProperties(MRI_SURFACE *mris) ;
int          MRISrestoreOldPositions(MRI_SURFACE *mris) ;
int          MRISstoreCurrentPositions(MRI_SURFACE *mris) ;
int          MRISupdateSurface(MRI_SURFACE *mris) ;
double       MRISpercentDistanceError(MRI_SURFACE *mris) ;
int          MRISscaleBrainArea(MRI_SURFACE *mris) ;


MRI_SP       *MRISPcombine(MRI_SP *mrisp, MRI_SP *mrisp_template, int fno);
MRI_SP       *MRISPaccumulate(MRI_SP *mrisp, MRI_SP *mrisp_template, int fno);
int          MRISPcoordinate(MRI_SP *mrisp, float x, float y, float z,
                             int *pu, int *pv) ;
double       MRISPfunctionVal(MRI_SURFACE_PARAMETERIZATION *mrisp, 
                              MRI_SURFACE *mris,
                              float x, float y, float z, int fno) ;
MRI_SP       *MRIStoParameterization(MRI_SURFACE *mris, MRI_SP *mrisp, 
                                     float scale, int fno) ;
MRI_SURFACE  *MRISfromParameterization(MRI_SP *mrisp, MRI_SURFACE *mris,
                                       int fno) ;
MRI_SURFACE  *MRISnormalizeFromParameterization(MRI_SP *mrisp, 
                                                MRI_SURFACE *mris, int fno) ;
MRI_SP       *MRISgradientToParameterization(MRI_SURFACE *mris, MRI_SP *mrisp, 
                                     float scale) ;
MRI_SURFACE  *MRISgradientFromParameterization(MRI_SP*mrisp,MRI_SURFACE *mris);

MRI_SP       *MRIScoordsToParameterization(MRI_SURFACE *mris, MRI_SP *mrisp, 
                                     float scale) ;
MRI_SURFACE  *MRIScoordsFromParameterization(MRI_SP *mrisp, MRI_SURFACE *mris);


MRI_SP       *MRISPblur(MRI_SP *mrisp_src, MRI_SP *mrisp_dst, float sigma,
                        int fno) ;
MRI_SP       *MRISPconvolveGaussian(MRI_SP *mrisp_src, MRI_SP *mrisp_dst, 
                                    float sigma, float radius, int fno) ;
MRI_SP       *MRISPalign(MRI_SP *mrisp_orig, MRI_SP *mrisp_src, 
                         MRI_SP *mrisp_tmp, MRI_SP *mrisp_dst) ;
MRI_SP       *MRISPtranslate(MRI_SP *mrisp_src, MRI_SP *mrisp_dst, int du, 
                             int dv) ;
MRI_SP       *MRISPclone(MRI_SP *mrisp_src) ;
MRI_SP       *MRISPalloc(float scale, int nfuncs) ;
int          MRISPfree(MRI_SP **pmrisp) ;
MRI_SP       *MRISPread(char *fname) ;
int          MRISPwrite(MRI_SP *mrisp, char *fname) ;
int          MRISwriteArea(MRI_SURFACE *mris, char *mris_fname) ;

#include "label.h"
double       MRISParea(MRI_SP *mrisp) ;
MRI_SP  *MRISPorLabel(MRI_SP *mrisp, MRI_SURFACE *mris, LABEL *area) ;
MRI_SP  *MRISPandLabel(MRI_SP *mrisp, MRI_SURFACE *mris, LABEL *area) ;

#define ORIGINAL_VERTICES   0
#define ORIG_VERTICES       ORIGINAL_VERTICES
#define GOOD_VERTICES       1
#define TMP_VERTICES        2
#define CANONICAL_VERTICES  3
#define CURRENT_VERTICES    4
#define INFLATED_VERTICES   5
#define FLATTENED_VERTICES  6
#define PIAL_VERTICES       7
#define TMP2_VERTICES       8
#define WHITE_VERTICES      9


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
#define NO_HEMISPHERE           2

#if 0
#define DEFAULT_A  44.0f
#define DEFAULT_B  122.0f
#define DEFAULT_C  70.0f
#else
#define DEFAULT_A  122.0f
#define DEFAULT_B  122.0f
#define DEFAULT_C  122.0f
#endif
#define DEFAULT_RADIUS  100.0f

#define MAX_DIM    DEFAULT_B

#if 1
#define DT_INCREASE  1.1 /* 1.03*/
#define DT_DECREASE  0.5
#else
#define DT_INCREASE  1.0 /* 1.03*/
#define DT_DECREASE  1.0
#endif
#define DT_MIN       0.01
#ifdef ERROR_RATIO
#undef ERROR_RATIO
#endif
#define ERROR_RATIO  1.03  /* 1.01 then 1.03 */

#define NO_LABEL     -1


#define WHITE_SURFACE 0
#define GRAY_SURFACE  1
#define GRAY_MID      2

#define REVERSE_X     0
#define REVERSE_Y     1
#define REVERSE_Z     2

int   MRIStranslate(MRI_SURFACE *mris, float dx, float dy, float dz) ;
int   MRISpositionSurface(MRI_SURFACE *mris, MRI *mri_brain, 
                          MRI *mri_smooth, INTEGRATION_PARMS *parms);
int   MRISpositionSurfaces(MRI_SURFACE *mris, MRI **mri_flash, 
                           int nvolumes, INTEGRATION_PARMS *parms);
int   MRISmoveSurface(MRI_SURFACE *mris, MRI *mri_brain, 
                          MRI *mri_smooth, INTEGRATION_PARMS *parms);
int   MRISscaleVals(MRI_SURFACE *mris, float scale) ;
int   MRISsetVals(MRI_SURFACE *mris, float val) ;
int   MRISaverageVals(MRI_SURFACE *mris, int navgs) ;
int   MRISaverageVal2baks(MRI_SURFACE *mris, int navgs) ;
int   MRISaverageVal2s(MRI_SURFACE *mris, int navgs) ;
int   MRISaverageMarkedVals(MRI_SURFACE *mris, int navgs) ;
int   MRISaverageEveryOtherVertexPositions(MRI_SURFACE *mris, int navgs, 
                                           int which) ;
int   MRISsoapBubbleVertexPositions(MRI_SURFACE *mris, int navgs) ;
int   MRISsoapBubbleOrigVertexPositions(MRI_SURFACE *mris, int navgs) ;
MRI   *MRISwriteSurfaceIntoVolume(MRI_SURFACE *mris, MRI *mri_template,
                                  MRI *mri) ;
#if 0
int   MRISmeasureCorticalThickness(MRI_SURFACE *mris, MRI *mri_brain, 
                                   MRI *mri_wm, float nsigma) ;
#else
int   MRISmeasureCorticalThickness(MRI_SURFACE *mris, int nbhd_size, 
                                   float max_thickness) ;
#endif
int
MRISfindClosestOrigVertices(MRI_SURFACE *mris, int nbhd_size) ;

int   MRISmarkRandomVertices(MRI_SURFACE *mris, float prob_marked) ;
int   MRISmarkNegativeVertices(MRI_SURFACE *mris, int mark) ;
int   MRISripNegativeVertices(MRI_SURFACE *mris) ;
int   MRISclearGradient(MRI_SURFACE *mris) ;
int   MRISclearMarks(MRI_SURFACE *mris) ;
int   MRISclearFixedValFlags(MRI_SURFACE *mris) ;
int   MRIScopyFixedValFlagsToMarks(MRI_SURFACE *mris) ;
int   MRISclearAnnotations(MRI_SURFACE *mris) ;
int   MRISsetMarks(MRI_SURFACE *mris, int mark) ;
int   MRISsequentialAverageVertexPositions(MRI_SURFACE *mris, int navgs) ;
int   MRISreverse(MRI_SURFACE *mris, int which) ;
int   MRISdisturbOriginalDistances(MRI_SURFACE *mris, double max_pct) ;
double MRISstoreAnalyticDistances(MRI_SURFACE *mris, int which) ;
int   MRISnegateValues(MRI_SURFACE *mris) ;
int   MRIScopyMeansToValues(MRI_SURFACE *mris) ;
int   MRIScopyCurvatureToValues(MRI_SURFACE *mris) ;
int   MRIScopyCurvatureToImagValues(MRI_SURFACE *mris) ;
int   MRIScopyCurvatureFromValues(MRI_SURFACE *mris) ;
int   MRIScopyCurvatureFromImagValues(MRI_SURFACE *mris) ;
int   MRIScopyImaginaryMeansToValues(MRI_SURFACE *mris) ;
int   MRIScopyStandardErrorsToValues(MRI_SURFACE *mris) ;
int   MRIScopyImagValuesToCurvature(MRI_SURFACE *mris) ;
int   MRIScopyValuesToCurvature(MRI_SURFACE *mris) ;
int   MRISaccumulateMeansInVolume(MRI_SURFACE *mris, MRI *mri, int mris_dof, 
                                  int mri_dof, int coordinate_system,int sno);
int   MRISaccumulateStandardErrorsInVolume(MRI_SURFACE *mris, MRI *mri, 
                                           int mris_dof, int mri_dof, 
                                           int coordinate_system, int sno) ;
int   MRISaccumulateMeansOnSurface(MRI_SURFACE *mris, int total_dof,
                                   int new_dof);
int   MRISaccumulateImaginaryMeansOnSurface(MRI_SURFACE *mris, int total_dof,
                                   int new_dof);
int   MRISaccumulateStandardErrorsOnSurface(MRI_SURFACE *mris, 
                                            int total_dof,int new_dof);
int   MRIScomputeAverageCircularPhaseGradient(MRI_SURFACE *mris, LABEL *area,
                                            float *pdx,float *pdy,float *pdz);

#define GRAY_WHITE     1
#define GRAY_CSF       2
int   MRIScomputeBorderValues(MRI_SURFACE *mris,MRI *mri_brain,
                              MRI *mri_smooth, Real inside_hi, Real border_hi,
                              Real border_low, Real outside_low, Real outside_hi,
                              double sigma,
                              float max_dist, FILE *log_fp, int white);
int  MRIScomputeWhiteSurfaceValues(MRI_SURFACE *mris, MRI *mri_brain, 
                                   MRI *mri_smooth);
int  MRIScomputeGraySurfaceValues(MRI_SURFACE *mris, MRI *mri_brain, 
                                  MRI *mri_smooth, float gray_surface);
int  MRIScomputeDistanceErrors(MRI_SURFACE *mris, int nbhd_size,int max_nbrs);
int  MRISscaleCurvature(MRI_SURFACE *mris, float scale) ;
int  MRISwriteTriangularSurface(MRI_SURFACE *mris, char *fname) ;
int  MRISripFaces(MRI_SURFACE *mris) ;
int  MRISremoveRipped(MRI_SURFACE *mris) ;
int  MRISbuildFileName(MRI_SURFACE *mris, char *sname, char *fname) ;
int  MRISsmoothSurfaceNormals(MRI_SURFACE *mris, int niter) ;
int  MRISsoapBubbleVals(MRI_SURFACE *mris, int niter) ;
int  MRISmodeFilterVals(MRI_SURFACE *mris, int niter) ;
int  MRISmodeFilterAnnotations(MRI_SURFACE *mris, int niter) ;
int  MRISmodeFilterZeroVals(MRI_SURFACE *mris) ;
int  MRISreadBinaryAreas(MRI_SURFACE *mris, char *mris_fname) ;
int  MRISwriteAreaErrorToValFile(MRI_SURFACE *mris, char *name) ;
int  MRIStransform(MRI_SURFACE *mris, MRI *mri, LTA *lta, MRI *mri_dst) ;
int  MRISanisotropicScale(MRI_SURFACE *mris, float sx, float sy, float sz) ;
double MRIScomputeVertexSpacingStats(MRI_SURFACE *mris, double *psigma,
                                     double *pmin, double *pmax, int *pvno,
                                     int *pvno2);
double MRIScomputeFaceAreaStats(MRI_SURFACE *mris, double *psigma,
                                     double *pmin, double *pmax);
int MRISprintTessellationStats(MRI_SURFACE *mris, FILE *fp) ;
int MRISmergeIcosahedrons(MRI_SURFACE *mri_src, MRI_SURFACE *mri_dst) ;
int MRISinverseSphericalMap(MRI_SURFACE *mris, MRI_SURFACE *mris_ico) ;
typedef struct
{
	int     max_patches ;
	int     max_unchanged ;
	int     niters;  /* stop the genetic algorithm after n iterations */
	int     genetic; /* to use the genetic algorithm */ 
	double  l_mri ;
	double  l_curv ;
	double  l_unmri ;
} TOPOLOGY_PARMS ;

int MRIScenterSphere(MRI_SURFACE *mris);
int MRISmarkOrientationChanges(MRI_SURFACE *mris);

MRI_SURFACE *MRIScorrectTopology(MRI_SURFACE *mris, 
                                 MRI_SURFACE *mris_corrected, MRI *mri, MRI *mri_wm,
                                 int nsmooth,TOPOLOGY_PARMS *parms) ;
int MRISripDefectiveFaces(MRI_SURFACE *mris) ;
int MRISunrip(MRI_SURFACE *mris) ;
int MRISdivideLongEdges(MRI_SURFACE *mris, double thresh) ;
int MRISremoveTriangleLinks(MRI_SURFACE *mris) ;
int MRISsetOriginalFileName(char *orig_name) ;

int MRISextractCurvatureVector(MRI_SURFACE *mris, float *curvs) ;
int MRISextractCurvatureDoubleVector(MRI_SURFACE *mris, double *curvs) ;
#define MRISexportCurvatureVector  MRISextractCurvatureVector

int MRISimportCurvatureVector(MRI_SURFACE *mris, float *curvs) ;
int MRISimportValVector(MRI_SURFACE *mris, float *vals) ;
int MRISexportValVector(MRI_SURFACE *mris, float *vals) ;
int MRISmaskLabel(MRI_SURFACE *mris, LABEL *area) ;
int MRISmaskNotLabel(MRI_SURFACE *mris, LABEL *area) ;
int MRISripLabel(MRI_SURFACE *mris, LABEL *area) ;
int MRISripNotLabel(MRI_SURFACE *mris, LABEL *area) ;
int MRISsegmentMarked(MRI_SURFACE *mris, LABEL ***plabel_array, int *pnlabels,
                      float min_label_area) ;
int MRISsegmentAnnotated(MRI_SURFACE *mris, LABEL ***plabel_array,int *pnlabels,
                      float min_label_area) ;

/* multi-timepoint (or stc) files */
int  MRISwriteStc(char *fname, MATRIX *m_data, float epoch_begin_lat,
                  float sample_period, int *vertices) ;

#if 1
#include "mrishash.h"
float  MRISdistanceToSurface(MRI_SURFACE *mris, MHT *mht,
                             float x0, float y0, float z0,
                             float nx, float ny, float nz) ;
int    MRISexpandSurface(MRI_SURFACE *mris, float distance, INTEGRATION_PARMS *parms) ;

#endif
                             
/* cortical ribbon */
MRI   *MRISribbon(MRI_SURFACE *inner_mris,MRI_SURFACE *outer_mris,MRI *mri_src,MRI *mri_dst);
MRI   *MRISaccentuate(MRI *mri_src,MRI *mri_dst,int lo_thresh,int hi_thresh);
MRI   *MRISshell(MRI *mri_src,MRI_SURFACE *mris,MRI *mri_dst,int clearflag);
MRI   *MRISfloodoutside(MRI *mri_src,MRI *mri_dst); // src never used. dst is modified

/* high resolution cortical ribbon */
MRI   *MRISpartialribbon(MRI_SURFACE *inner_mris_lh,MRI_SURFACE *outer_mris_lh,MRI_SURFACE *inner_mris_rh,MRI_SURFACE *outer_mris_rh,MRI *mri_src,MRI *mri_dst,MRI *mri_mask);
MRI   *MRISpartialaccentuate(MRI *mri_src,MRI *mri_dst,int lo_thresh,int hi_thresh);
MRI   *MRISpartialshell(MRI *mri_src,MRI_SURFACE *mris,MRI *mri_dst,int clearflag);
MRI   *MRISpartialfloodoutside(MRI *mri_src,MRI *mri_dst); // src is used. dst must be initialized

#define MRIS_BINARY_QUADRANGLE_FILE    0    /* homegrown */
#define MRIS_ASCII_TRIANGLE_FILE       1    /* homegrown */
#define MRIS_ASCII_FILE MRIS_ASCII_TRIANGLE_FILE
#define MRIS_GEO_TRIANGLE_FILE         2    /* movie.byu format */
#define MRIS_ICO_SURFACE               3
#define MRIS_TRIANGULAR_SURFACE        MRIS_ICO_SURFACE
#define MRIS_ICO_FILE                  4
#define MRIS_VTK_FILE                  5


#define IS_QUADRANGULAR(mris) \
   (mris->type == MRIS_BINARY_QUADRANGLE_FILE)


/* actual constants are in mri.h */
#define RH_LABEL           MRI_RIGHT_HEMISPHERE
#define RH_LABEL2          MRI_RIGHT_HEMISPHERE2
#define LH_LABEL           MRI_LEFT_HEMISPHERE

#define MRISPvox(m,u,v)   (*IMAGEFpix(m->Ip,u,v))


typedef struct
{
  float   x ;
  float   y ;
  float   z ;
} SMALL_VERTEX ;

typedef struct
{
  int   nvertices ;
  SMALL_VERTEX *vertices ;
} SMALL_SURFACE ;

#define MRISSread  MRISreadVerticesOnly
SMALL_SURFACE *MRISreadVerticesOnly(char *fname) ;
int           MRISSfree(SMALL_SURFACE **pmriss) ;


int  MRISsubsampleDist(MRI_SURFACE *mris, float spacing) ;
int  MRISwriteDecimation(MRI_SURFACE *mris, char *fname) ;
int  MRISreadDecimation(MRI_SURFACE *mris, char *fname) ;


#define VERTEX_COORDS      0
#define VERTEX_VALS        1
#define VERTEX_VAL         VERTEX_VALS
#define VERTEX_AREA        2
#define VERTEX_CURV        3
#define VERTEX_CURVATURE   VERTEX_CURV
#define VERTEX_LABEL       4
#define VERTEX_ANNOTATION  5

int MRISclearOrigArea(MRI_SURFACE *mris) ;
int MRIScombine(MRI_SURFACE *mris_src, MRI_SURFACE *mris_total, 
                MRIS_HASH_TABLE *mht, int which) ;
int MRISsphericalCopy(MRI_SURFACE *mris_src, MRI_SURFACE *mris_total, 
                MRIS_HASH_TABLE *mht, int which) ;
int   MRISorigAreaToCurv(MRI_SURFACE *mris) ;
int   MRISareaToCurv(MRI_SURFACE *mris) ;
int   MRISclear(MRI_SURFACE *mris, int which) ;
int   MRISnormalize(MRI_SURFACE *mris, int dof, int which) ;

int  MRIScopyMRI(MRIS *Surf, MRI *Src, int Frame, char *Field);
MRI *MRIcopyMRIS(MRI *mri, MRIS *surf, int Frame, char *Field);
MRI *MRISsmoothMRI(MRIS *Surf, MRI *Src, int nSmoothSteps, MRI *Targ);
int  MRISclearFlags(MRI_SURFACE *mris, int flags) ;
int  MRISsetFlags(MRI_SURFACE *mris, int flags) ;

int MRISmedianFilterVals(MRI_SURFACE *mris, int nmedians) ;
int MRISmedianFilterVal2s(MRI_SURFACE *mris, int nmedians) ;
int MRISmedianFilterVal2baks(MRI_SURFACE *mris, int nmedians) ;
int MRISmedianFilterCurvature(MRI_SURFACE *mris, int nmedians);
int MRISfileNameType(char *fname) ;
unsigned long MRISeraseOutsideOfSurface(float h,MRI* mri_dst,MRIS *mris,unsigned char val) ;


/* Some utility functions to handle reading and writing annotation
   values. MRISRGBToAnnot stuffs an r,g,b tuple into an annotation
   value and MRISAnnotToRGB separates an annotation value into an
   r,g,b tuple. */
#define MRISAnnotToRGB(annot,r,g,b)  \
     r = annot & 0xff ;         \
     g = (annot >> 8) & 0xff ;  \
     b = (annot >> 16) & 0xff ;
#define MRISRGBToAnnot(r,g,b,annot) \
     annot = ((r) & 0xff) | (((g) & 0xff) << 8) | (((b) & 0xff) << 16);

#endif



int MRISextendedNeighbors(MRIS *SphSurf,int TargVtxNo, int CurVtxNo,
			  double DotProdThresh, int *XNbrVtxNo, 
			  double *XNbrDotProd, int *nXNbrs,
			  int nXNbrsMax);
MRI *MRISgaussianSmooth(MRIS *Surf, MRI *Src, double GStd, MRI *Targ,
			double TruncFactor);
MRI *MRISdistSphere(MRIS *surf, double dmax);
int MRISgaussianWeights(MRIS *surf, MRI *dist, double GStd);
MRI *MRISspatialFilter(MRI *Src, MRI *wdist, MRI *Targ);

MATRIX *surfaceRASToSurfaceRAS_(MRI *src, MRI *dst, LTA *lta);
