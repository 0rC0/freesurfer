/* to do:
optimize functional overlay drawing by grabbing volume bounds when loaded and only looping through those bounds when drawing

better disctinction between error output and regular output. see LoadFunctionalVolume for example. use NOTE: ALERT: or ERROR: before all 'dlog box' outputs.
 */

/*============================================================================
 Copyright (c) 1996 Martin Sereno and Anders Dale
=============================================================================*/
#define TCL
#define TKMEDIT 
/*#if defined(Linux) || defined(sun) || defined(SunOS) */
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
/*#endif */
#include <fcntl.h>
#include "error.h"
#include "diag.h"
#include "utils.h"
#include "const.h"


/*------------------begin medit.c-------------------*/

/*============================================================================
 Copyright (c) 1996, 1997 Anders Dale and Martin Sereno
=============================================================================*/
#include <tcl.h>
#include <tk.h>
// #include <tix.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "MRIio.h"
#include "volume_io.h"
#include "rgb_image.h"
#include "fio.h"
#include "mrisurf.h"
#include "mri_conform.h"

#ifndef OPENGL
#define OPENGL
#endif

#define TCL


#  define PR   {if(promptflag){fputs("% ", stdout);} fflush(stdout);}
int            tk_NumMainWindows = 0;


#include "proto.h"
#include "macros.h"
#include "xwindow.h"
#include <X11/keysym.h>
#  define bgnpolygon()     glBegin(GL_QUADS)  /*one list OK;GL_POLYGON s
lower*/
#  define bgnline()        glBegin(GL_LINES)
#  define bgnpoint()       glBegin(GL_POINTS)
#  define v2f(X)           glVertex2fv(X)
#  define endpolygon()     glEnd()
#  define endline()        glEnd()
#  define endpoint()       glEnd()
#  define swapbuffers()    tkoSwapBuffers()
#  define clear()          glClear(GL_COLOR_BUFFER_BIT)
#  define linewidth(X)     glLineWidth((float)(X))
#  define getorigin(X,Y)   *(X) = w.x; *(Y) = 1024 - w.y - w.h /*WINDOW_REC w;*/
#  define getsize(X,Y)     *(X) = w.w; *(Y) = w.h
#  define Colorindex       unsigned short
#  define color(R)         glIndexs(X)
#  define mapcolor(I,R,G,B)  \
           tkoSetOneColor((int)I,(float)R/255.0,(float)G/255.0,(float)B/255.0)
#  define rectwrite(X0,Y0,X1,Y1,P) \
           glMatrixMode ( GL_PROJECTION ); \
           glLoadIdentity (); \
           glOrtho ( 0, xdim-1, 0,ydim-1, -1.0, 1.0 ); \
           glRasterPos2i(X0,Y0); \
           glDrawPixels((X1-X0)+1,(Y1-Y0)+1,GL_RGBA,GL_UNSIGNED_BYTE,P) 
/*
           glDrawPixels((X1-X0)+1,(Y1-Y0)+1,GL_LUMINANCE,GL_UNSIGNED_BYTE,P) 
           glDrawPixels((X1-X0)+1,(Y1-Y0)+1,GL_COLOR_INDEX,GL_UNSIGNED_SHORT,P) */ 
/* best Extreme glDrawPixels: GL_ABGR_EXT,GL_UNSIGNED_BYTE */
#  define wintitle(X) \
           XStringListToTextProperty(&X,1,&tp); \
           XSetWMName(xDisplay,w.wMain,&tp)
#  define ortho2(X0,X1,Y0,Y1) \
           glMatrixMode(GL_PROJECTION); \
           glLoadIdentity(); \
           glOrtho(X0,X1,Y0,Y1,-1.0,1.0);
#ifdef BLACK
#undef BLACK
#endif
#ifdef RED
#undef RED
#endif
#ifdef YELLOW
#undef YELLOW
#endif
#ifdef GREEN
#undef GREEN
#endif

#  define BLACK   0  /* TODO: use tkoSetOneColor, glIndexs */
#  define GREEN   1
#  define RED     2
#  define YELLOW  3

#define CURSOR_VAL   255

/* #define SQR(x)       ((x)*(x)) */
#define MATCH(A,B)   (!strcmp(A,B))
#define MATCH_STR(S) (!strcmp(str,S))

#define NUMVALS 256
#define MAXIM 256
#define MAXPTS 10000
#define MAXPARS 10
#define MAPOFFSET 0
#define CORONAL    0
#define HORIZONTAL 1
#define SAGITTAL   2
#define POSTANT   0
#define INFSUP    1
#define RIGHTLEFT 2  /* radiol */
#define NAME_LENGTH  STRLEN
#define MAX_DIR_DEPTH  30
#define TMP_DIR          "tmp"             /* relative to subjectsdir/pname */
#define TRANSFORM_DIR    "mri/transforms"  /* ditto */
#define TALAIRACH_FNAME  "talairach.xfm"   /* relative to TRANSFORM_DIR */
#ifndef WM_EDITED_OFF
#define WM_EDITED_OFF 1
#endif
#define WM_MIN_VAL    2  /* 1 is used for voxels that are edited to off */
#define TO_WHITE  0
#define TO_BLACK  1
#define DIR_FILE "ic1.tri"
#define MAXCOR 500
#define MAXLEN 100
#define kDefaultWindowLocationX 0
#define kDefaultWindowLocationY 0
#define kWindowBottomBorderHeight 32
#define CVIDBUF 25

// kt - default scale factor, makes window bigger
#define kDefaultScaleFactor 2

int selectedpixval = 0;
int secondpixval = 0 ;
int updatepixval = TRUE;
int plane = CORONAL;
int xnum=256,ynum=256;
int ptype;
float ps,st,xx0,xx1,yy0,yy1,zz0,zz1;
int zf, ozf;
float fsf;
static int xdim,ydim;
unsigned long bufsize;
unsigned char **im[MAXIM];
unsigned char **im_b[MAXIM];
unsigned char **im2[MAXIM];
unsigned char **fill[MAXIM];
unsigned char **dummy_im[MAXIM];
unsigned char **sim[6]; 
unsigned char **sim2[6]; 
int second_im_allocated = FALSE;
int dummy_im_allocated = FALSE;
int wmfilter_ims_allocated = FALSE;
int changed[MAXIM];
GLubyte *vidbuf; 
/* Colorindex *vidbuf;  */
Colorindex *cvidbuf;
unsigned char *buf;
unsigned char *binbuff;
int imnr0,imnr1,numimg;
int wx0=114,wy0=302;  /* (100,100), (117,90), (556,90) */
int ptsflag = FALSE;
int maxflag = FALSE;
int surfflag = FALSE;
int surfloaded = FALSE;
int editflag = TRUE;
int revfsflag = FALSE;
int fieldsignloaded = FALSE;   /* fscontour */
int fieldsignflag = FALSE; /* overrides curvflag */
int surflinewidth = 1;
int curvloaded = FALSE;
int curvflag = FALSE;
int editedimage = FALSE;
int drawsecondflag = FALSE;
int inplaneflag = TRUE;
int linearflag = FALSE;
int bwflag = FALSE;
int truncflag = FALSE;
int scrsaveflag = FALSE;  /* was TRUE - BRF */
int openglwindowflag = FALSE;
int second_im_full = FALSE;
int circleflag = TRUE;
int promptflag = FALSE;
int followglwinflag = TRUE;
int initpositiondoneflag = FALSE;
int all3flag = FALSE;
int changeplanelock = FALSE;
int npts = 0;
int prad = 0;
int pradlast = 0;
int ndip = 0;
int dip_spacing = 10; /* voxels */
float tm[4][4];
int jold[MAXPTS],iold[MAXPTS],imold[MAXPTS];
float ptx[MAXPTS],pty[MAXPTS],ptz[MAXPTS];

float par[MAXPARS],dpar[MAXPARS];

static MRI_SURFACE *mris ;
double fthresh = 0.35;
double fsquash = 12.0;
double fscale = 255;
double fsthresh = 0.3;
double xtalairach = 0.0;
double ytalairach = 0.0;
double ztalairach = 0.0;
int white_lolim = 80;
int white_hilim = 140;
int gray_hilim = 100;
int flossflag = TRUE;
int spackleflag = TRUE;
int lim3=170,lim2=145,lim1=95,lim0=75;
double ffrac3=1.0,ffrac2=1.0,ffrac1=1.0,ffrac0=1.0;

int imc=0,ic=0,jc=0;
int impt = -1,ipt = -1,jpt = -1;
float x_click, y_click, z_click ;


char *subjectsdir;   /* SUBJECTS_DIR */
char *srname;        /* sessiondir--from cwd */
char *pname;         /* name */
char *title_str ;    /* window title */
char *imtype;        /* e.g., T1 */
char *imtype2;        /* e.g., T1 */
char *surface;       /* e.g., rh.smooth */
char *mfname;        /* abs single image stem name */
char *sfname;        /* abs surface name */
char *tfname;        /* (dir!) subjectsdir/name/tmp/ */
char *dipfname;      /* curr: $session/bem/brain3d.dip */
char *decfname;      /* curr: $session/bem/brain3d.dec */
char *hpfname;       /* headpts */
char *htfname;       /* headtrans */
char *sgfname;       /* rgb */
char *fsfname;       /* $session/fs/$hemi.fs */
char *fmfname;       /* $session/fs/$hemi.fm */
char *cfname;        /* $home/surf/hemi.curv */
char *xffname;       /* $home/name/mri/transforms/TALAIRACHg4251_FNAME */
char *rfname;        /* script */

/* Talairach stuff */
General_transform talairach_transform ; /* the next two are from this struct */
Transform         *linear_transform = NULL ;
Transform         *inverse_linear_transform = NULL ;
int               transform_loaded = 0 ;

static int   control_points = 0 ;
static int   num_control_points = 0 ;


// ======================================================= kevin teich's stuff

#include "xDebug.h"
#include "tkmVoxel.h"
#include "tkmVoxelList.h"
#include "tkmVoxelSpace.h"
#include "VoxelValueList.h"
#include "tkmFunctionalDisplay.h"

                                       /* irix compiler doesn't like inlines */
#ifdef IRIX
#define inline
#endif


// ==================================================================== OUTPUT

/* for regular output. this could be remapped to display to a file or some 
   window widget like a status bar. that'd be cool. */
#define InitOutput
#define DeleteOutput
#define OutputPrint            fprintf ( stdout,
#define EndOutputPrint         );

// ===========================================================================

// ================================================================== MESSAGES

                                   /* put our massages here to make changing
              them easier. */
#define kMsg_InvalidClick "\nInvalid click - out of voxel bounds.\n"         \
      "Although the screen displays areas that are out of voxel bounds in "  \
      "black, you cannot click on them to select them. Please click inside " \
      "voxel bounds, closer to the non-black image.\n\n"


// ===========================================================================


// =========================================================== READING VOLUMES

                                   /* this function takes a path and passes
              it to MRIRead. it then takes the MRI
              struct and grabs all the information
              it needs from it. this is an alternative
              function to read_images and is 
              independent of the SUBJECTS_DIR env
              variable. */
void ReadVolumeWithMRIRead ( char * inFileOrPath );

// ===========================================================================

// ================================================================== GRAPHICS


#define kBytesPerPixel                    4         // assume 32 bit pixels
#define kPixelComponentLevel_Max          255       // assume 1 byte components

                                     /* pixel offsets in bits, used in drawing
                                        loops to composite pixels together */
#if defined ( IRIX ) || defined ( SunOS )
#define kPixelOffset_Alpha                0
#define kPixelOffset_Green                8
#define kPixelOffset_Blue                 16
#define kPixelOffset_Red                  24
#else
#define kPixelOffset_Alpha                24
#define kPixelOffset_Blue                 16
#define kPixelOffset_Green                8
#define kPixelOffset_Red                  0
#endif

                                       /* color values for 
                                          drawing functions */
           
#if defined ( IRIX ) || defined ( SunOS )
#define kRGBAColor_Red      0xff0000ff
#define kRGBAColor_Green    0x00ff00ff
#define kRGBAColor_Yellow   0xffff00ff
#else
#define kRGBAColor_Red      0xff0000ff
#define kRGBAColor_Green    0xff00ff00
#define kRGBAColor_Yellow   0xff00ffff
#endif 


#define kCtrlPtCrosshairRadius            2
#define kCursorCrosshairRadius            2

                                     /* for indexing arrays of points */
#define kPointArrayIndex_X                0
#define kPointArrayIndex_Y                1
#define kPointArrayIndex_Beginning        0

                                       /* draws a crosshair cursor into a 
                                          video buffer. */
void DrawCrosshairInBuffer ( char * inBuffer,       // the buffer
           int inX, int inY,      // location in buffer
           int inSize,            // radius of crosshair
           long inColor );        // color, should be a
                                                      // kRGBAColor_ value

                                   /* draws a crosshair centered on the
              voxel */
void DrawCenteredCrosshairInBuffer ( char * inBuffer,
             int inX, int inY,     
             int inSize,           
             long inColor );       
                                                     

                                       /* fills a box with a color.
                                          starts drawing with the input coords
                                          as the upper left and draws a box 
                                          the dimesnions of the size */
void FillBoxInBuffer ( char * inBuffer,       // the buffer
                       int inX, int inY,      // location in buffer
                       int inSize,            // width/height of pixel
                       long inColor );        // color, should be a
                                              // kRGBAColor_ value


                                       /* fast pixel blitting */
inline
void SetGrayPixelInBuffer ( char * inBuffer, int inIndex, int inCount,
                            unsigned char inGray );

inline
void SetColorPixelInBuffer ( char * inBuffer, int inIndex, int inPixelSize,
                             unsigned char inRed, unsigned char inGreen,
                             unsigned char inBlue );

inline
void SetCompositePixelInBuffer ( char * inBuffer, int inIndex, int inCount,
                                 long inPixel );

                                       /* general function that draws a list
            of vertices as something in opengl */
void DrawWithFloatVertexArray ( GLenum inMode,
        float *inVertices, int inNumVertices );


void DrawFunctionalData ( char * inBuffer, int inVoxelSize,
        int inPlane, int inPlaneNum );


// ===========================================================================

// ==================================================== COORDINATE CONVERSIONS

#include "mritransform.h"

                                        /* convert a click to screen coords. 
                                           only converts the two coords that 
                                           correspond to the x/y coords on
                                           the visible plane, leaving the
                                           third untouched, so the out
                                           coords should be set to the current
                                           cursor coords before calling. */
void ClickToScreen ( int inH, int inV, int inPlane,
                     int *ioScreenJ, int *ioScreenI, int *ioScreenIM );

                                       /* converts screen coords to 
                                          voxel coords and back */
void ScreenToVoxel ( int inPlane,               // what plane we're on
                     int j, int i, int im,      // incoming screen coords
                     int *x, int *y, int *z);   // outgoing voxel coords

                                       /* there are actually two screen 
                                          spaces, unzoomed and local zoomed, 
                                          and when conveting screen to voxel
                                          and back, the x/y screen coords get
                                          the local zoomer conversion and the
                                          z gets the unzoomed conversion, so we
                                          have seperate inline functions for
                                          doing everything. */
inline int ScreenXToVoxelX ( int j );
inline int LocalZoomXToVoxelX ( int lj );
inline int ScreenYToVoxelY ( int i );
inline int LocalZoomYToVoxelY ( int li );
inline int ScreenZToVoxelZ ( int im );
inline int LocalZoomZToVoxelZ ( int lim );

void VoxelToScreen ( int x, int y, int z,       // incoming voxel coords
                     int inPlane,               // what plane we're on
                     int *j, int *i, int *im ); // outgoing screen coords 

inline int VoxelXToScreenX ( int x );
inline int VoxelXToLocalZoomX ( int x );
inline int VoxelYToScreenY ( int y );
inline int VoxelYToLocalZoomY ( int y );
inline int VoxelZToScreenZ ( int z );
inline int VoxelZToLocalZoomZ ( int z );

                                   /* goes from a screen voxel to a 2d point
              on the screen. puts the proper coords
              in the h/v coords and scales if in all3
              mode. */
void ScreenToScreenXY ( int i, int j, int k,
      int plane, int *h, int *v );

                                       /* converts ras coords to
                                          voxel coords and back */
void RASToVoxel ( Real x, Real y, Real z,        // incoming ras coords
                  int *xi, int *yi, int *zi );   // outgoing voxel coords

void VoxelToRAS ( int xi, int yi, int zi,        // incoming voxel coords
                  Real *x, Real *y, Real *z );   // outgoing RAS coords

                                       /* convert ras coords to the two
                                          two screen coords that are relevant
            for this plane. this is specifically
                                          used when we are zoomed in and need
                                          to get screen coords that are more
                                          precise than int voxels. */
void RASToFloatScreenXY ( Real x, Real y, Real z,    // incoming ras coords
                          int inPlane,               // plane to convert on
                          float *j, float *i );      // out float screen

                                       /* convert ras coords to screen coords
            and back. note that this is a much
            finer resolution of screen coords
            than the voxel-screen conversions. */
void RASToScreen ( Real inX, Real inY, Real inZ,
       int inPlane, 
       int * outScreenX, int * outScreenY, int * outScreenZ );
void ScreenToRAS ( int inPlane, int inScreenX, int inScreenY, int inScreenZ,
       Real * outRASX, Real * outRASY, Real * outRASZ );


// ===========================================================================

// =========================================================== BOUNDS CHECKING

                                        /* various bounds checking. these
                                           three check basic bounds conditions.
                                           if they fail, something is 
                                           very wrong, and using those coords
                                           will probably cause a crash. */
inline char IsScreenPointInBounds ( int inPlane, int j, int i, int im );
inline char IsVoxelInBounds ( int x, int y, int z );
inline char IsRASPointInBounds ( Real x, Real y, Real z );

                                        /* these two check if screen points
                                           are _visible_ within the current
                                           window, not adjusting for local
                                           zooming. if they fail, it doesn't
                                           mean the screen point will cause
                                           a crash, it just means that it
                                           won't be visible on the current
                                           window. */
inline char IsScreenPtInWindow ( int j, int i, int im );
inline char IsTwoDScreenPtInWindow ( int j, int i );

// ===========================================================================

// ================================================== SELECTING CONTROL POINTS

void DrawControlPoints ( char * inBuffer, int inPlane, int inPlaneNum );

                                       /* draw control point. switches on 
                                          the current display style of the 
                                          point. */
void DrawCtrlPt ( char * inBuffer,  // video buffer to draw into
                  int inX, int inY, // x,y location in the buffer
                  long inColor );   // color to draw in

                                       /* handles the clicking of ctrl pts.
                                          takes a screen pt and looks for the
                                          closest ctrl pt in the same plane
                                          displayed on the screen. if it
                                          finds one, it adds or removes it
                                          from the selection, depending on
                                          the ctrt and shift key. */
void SelectCtrlPt ( int inScreenX, int inScreenY,     // screen pt clicked
                    int inScreenZ, int inPlane,       // plane to search in
        unsigned int inState );           // modifier keys

                                        /* remove the selected control points 
                                           from the control point space */
void DeleteSelectedCtrlPts ();

                                   /* make the input anatomical voxel a
              control point */
void NewCtrlPt ( VoxelRef inVoxel );

                                   /* convert the cursor to anatomical coords
              and make it a control point. */
void NewCtrlPtFromCursor ();

                                   /* deselect all control points */
void DeselectAllCtrlPts ();

                                       /* reads the control.dat file, 
                                          transforms all pts from RAS space 
                                          to voxel space, and adds them as 
                                          control pts */
void ProcessCtrlPtFile ( char * inDir );

                                       /* writes all control points to the
                                          control.dat file in RAS space */
void WriteCtrlPtFile ( char * inDir );

                                       /* tsia */
void ToggleCtrlPtDisplayStatus ();
void SetCtrlPtDisplayStatus ( char inDisplay );
void ToggleCtrlPtDisplayStyle ();
void SetCtrlPtDisplayStyle ( int inStyle );

                                       /* global storage for ctrl space. */
VoxelSpaceRef gCtrlPtList = NULL;

                                       /* global storage for selected ctrl 
                                          pts. */
VoxelListRef gSelectionList = NULL;

                                       /* flag for displaying ctrl pts. if 
                                          true, draws ctrl pts in draw loop. */
char gIsDisplayCtrlPts;

                                       /* style of control point to draw */
#define kCtrlPtStyle_FilledVoxel              1
#define kCtrlPtStyle_Crosshair                2
char gCtrlPtDrawStyle;

                                       /* whether or not we've added the 
                                          contents of the control.dat file to
                                          our control pt space. we only want
                                          to add it once. */
char gParsedCtrlPtFile;

// ===========================================================================

// ================================================================== SURFACES

                                      /* for different surface types */
#define kSurfaceType_Current              0
#define kSurfaceType_Original             1
#define kSurfaceType_Canonical            2

                                       /* stuffs a vertex into a voxel, using
                                          the plane to determine orientation
                                          to screen space, where x/y is j/i
                                          and z is the depth. each uses a
                                          different set of coords from the
                                          vertex. */
void CurrentSurfaceVertexToVoxel ( vertex_type * inVertex, int inPlane, 
                            VoxelRef outVoxel );
void OriginalSurfaceVertexToVoxel ( vertex_type * inVertex, int inPlane, 
                                    VoxelRef outVoxel );
void CanonicalSurfaceVertexToVoxel ( vertex_type * inVertex, int inPlane, 
                                     VoxelRef outVoxel );

                                       /* set the status of surface displays.
                                          first checks to see if the surface
                                          is loaded, then sets the display
                                          flag. */
void SetCurrentSurfaceDisplayStatus ( char inDisplay );
void SetOriginalSurfaceDisplayStatus ( char inDisplay );
void SetCanonicalSurfaceDisplayStatus ( char inDisplay );

                                       /* set the vertex display status
            on surfaces */
void SetSurfaceVertexDisplayStatus ( char inDisplay );
char IsSurfaceVertexDisplayOn ();

                                       /* set the vertex display type,
            average or real */
void SetAverageSurfaceVerticesStatus ( char inDisplay );
char IsAverageSurfaceVerticesOn ();

                                       /* sets a certain vertex to be hilighted
                                          on the next redraw. */
void HiliteSurfaceVertex ( vertex_type * inVertex, int inSurface );
char IsSurfaceVertexHilited ( vertex_type * inVertex, int inSurface );

                                       /* surface drawing functions */
char IsVoxelsCrossPlane ( float inPlaneZ, 
                          float inVoxZ1, float inVoxZ2 );
void CalcDrawPointsForVoxelsCrossingPlane ( float inPlaneZ, int inPlane,
                                            VoxelRef inVox1, VoxelRef inVox2,
                                            float *outX, float *outY );
void DrawSurface ( MRI_SURFACE * theSurface, int inPlane, int inSurface );

                                       /* flags for toggling surface
                                          display. */
char gIsDisplayCurrentSurface = FALSE;
char gIsDisplayOriginalSurface = FALSE;
char gIsDisplayCanonicalSurface = FALSE;

                                       /* flags for determining whether a
                                          surface is loaded. */
char gIsCurrentSurfaceLoaded = FALSE;
char gIsOriginalSurfaceLoaded = FALSE;
char gIsCanonicalSurfaceLoaded = FALSE;

                                       /* flag for drawing surface vertices */
char gIsDisplaySurfaceVertices = FALSE;
                                       /* whether to draw the average vertex
            positions */
char gIsAverageSurfaceVertices = TRUE;

                                       /* out hilited vertex and the surface
            we should hilite */
vertex_type * gHilitedVertex = NULL;
int gHilitedVertexSurface= -1;

// ===========================================================================

// ========================================================= SELECTING REGIONS

/* selecting regions works much like editing. the user chooses a brush shape and size and paints in a region. the tool can be toggled between selecting and unselecting. the selected pixels are kept in a voxel space for optimized retreival in the draw loop. there are the usual functions for adding and removing voxels as well as saving them out to a file. */

VoxelSpaceRef gSelectedVoxels;
char isDisplaySelectedVoxels;

void InitSelectionModule ();
void DeleteSelectionModule ();

/* grabs the list of voxels selected and draws them into the buffer. */
void DrawSelectedVoxels ( char * inBuffer, int inPlane, int inPlaneNum );

/* handles clicks. uses the current brush settings to paint or paint selected voxels. */
void AllowSelectionModuleToRespondToClick ( VoxelRef inScreenVoxel );

/* adds or removes voxels to selections. if a voxel that isn't in the selection is told to be removed, no errors occur. this is called from the brush function.*/
void AddVoxelToSelection ( VoxelRef inVoxel, void * inData );
void RemoveVoxelFromSelection ( VoxelRef inVoxel, void * inData );

/* clears the current selection */
void ClearSelection ();

/* are we currently displaying the selection? */
char IsDisplaySelectedVoxels ();

/* write to and read from label files */
void SaveSelectionToLabelFile ( char * inFileName );
void LoadSelectionFromLabelFile ( char * inFileName );

// ===========================================================================

// =============================================================== BRUSH UTILS


                                   /* to run a brush function, set its params
              and give it a starting point and a
              function to pass hit voxels to. it will
              calculate what voxels are hit and pass
              them to the passed in function. */
typedef enum {
  kShape_Square = 0,
  kShape_Circular
} Brush_Shape;

// inData gets passed to the brush function.
void PaintVoxels ( void(*inFunction)(VoxelRef,void*), void * inData,
       VoxelRef inStartingPoint );
void SetBrushRadius ( int inRadius );
void SetBrushShape ( Brush_Shape inShape );
void SetBrush3DStatus ( char inFlag );

int gBrushRadius = 0;
Brush_Shape gBrushShape = kShape_Square;
char gBrushIsIn3D = FALSE;

// ===========================================================================

// ============================================================ EDITING VOXELS

/* for changing voxel values. this called from the brush function. */
void EditVoxel ( VoxelRef inVoxel, void * inData );

// ===========================================================================

// ============================================================== CURSOR UTILS

                                   /* to hide and show the cursor */
char gIsCursorVisible = TRUE;
void HideCursor ();
void ShowCursor ();
char IsCursorVisible ();


                                       /* main functions for setting the 
                                          cursor (red crosshair) to a certain
                                          point in screen coords. the latter
                  calls the former. */
void SetCursorToScreenPt ( int inScreenX, int inScreenY, int inScreenZ );
void SetCursorToRASPt ( Real inRASX, Real inRASY, Real inRASZ );

                                       /* when the tcl script changes planes
                                          in zoomed mode, some coordinates 
                                          that were in zoomed space get
                                          changed due to differences in the
                                          screen->voxel conversion based on
                                          the new plane. these two functions
                                          save the cursor in voxel coords,
                                          which are the same in any plane,
                                          and then after the plane switch
                                          changes them back to screen coords.
                                          these are called from the tcl
                                          script. */
void SaveCursorLocation ();
void RestoreCursorLocation ();

                                       /* sets the plane, preserving cursor
                                          location properly. */
void SetPlane ( int inNewPlane );

                                       /* print info about a screen point */
void PrintScreenPointInformation ( int j, int i, int im );

                                       /* send a message to the tk window when
            we change our coords. this makes the
            window update its slider coords. */
void SendUpdateMessageToTKWindow ();

                                       /* for saving our cursor position in
                                          voxel coords when we swtich planes
                                          while zoomed in. */
Real gSavedCursorVoxelX, gSavedCursorVoxelY, gSavedCursorVoxelZ;

// ===========================================================================

// =================================================================== ZOOMING


                                       /* controls our local zooming, or 
                                          zooming around the center point. 
                                          gCenter is the voxel that is at the 
                                          center of the screen. this should 
                                          not be such that with the current 
                                          zoom level, an edge of the window 
                                          is out of bounds of the voxel space.
                                          i.e. the only center possible at 
                                          zoom level 1 is 128, 128. at zoom 
                                          level 2, anything from 64,64 to 
                                          196,196 is possible. gLocalZoom is 
                                          the zoom level _in addition_ to
                                          the normal zf scaling factor that 
                                          is set at startup. so with zf = 2
                                          and gLocalZoom = 1, each voxel is 
                                          2x2 pixels. at zf=2 gLocalZoom=2,
                                          each voxel is 4x4 pixels. at zf=1
                                          and gLocalZoom=2, each voxel is 2x2
                                          pixels. */
#define kMinLocalZoom                          1
#define kMaxLocalZoom                          16

                                       /* this function sets the view
                                          center to a new screen point. */
void RecenterViewToScreenPt ( int inScreenX, int inScreenY, int inScreenZ );
void RecenterViewToCursor ();

                                       /* do local zooming. ZoomViewIn() and 
                                          Out() zoom in and out by a factor
                                          of 2, while UnzoomView() resets the 
                                          zoom to 1x and recenters to the
                                          middle voxel. */
void ZoomViewIn ();
void ZoomViewOut ();
void UnzoomView ();

                                       /* for managing and checking the 
                                          center point */
void SetCenterVoxel ( int x, int y, int z );
void CheckCenterVoxel ();

                                       /* print info about the current
                                          zoom level */
void PrintZoomInformation ();

                                       /* for storing the current center voxel
            and zoom level. */
int gCenterX, gCenterY, gCenterZ, gLocalZoom;


                                   /* nice way to get the cursor */
void GetCursorInScreenCoords ( VoxelRef ioVoxel );
void GetCursorInVoxelCoords ( VoxelRef ioVoxel );

// ===========================================================================

// ====================================================== SETTING VOXEL VALUES

                                   /* these are mainly here for purposes of
              readability. they just access an array
              based on voxel values. */

inline void SetVoxelValue ( int x, int y, int z, unsigned char inValue );
inline unsigned char GetVoxelValue ( int x, int y, int z );

// ===========================================================================

// ============================================================== EDITING UNDO

#include "xUndoList.h"

                                   /* this is a pretty simple implementation
              of undo that only supports pixel 
              editing. when editing, the pixels that
              were changed are saved along with their
              previous values in a list, one list per
              editing click. */

void InitUndoList ();
void DeleteUndoList ();

                                   /* note that the list is cleared when the
              mouse button 2 or 3 is pressed down
              right from the event handling code. 
              this is a hack, but it's the best we
              can do until we pass events and not
              just coords to ProcessClick. */
void ClearUndoList ();

                                   /* when pixels are editied, they are added
              to the list. if the user hits undo, the
              entire list is drawn to the screen, 
              using the SetVoxelValue() function. at
              the same time, a new list is made
              to save the positions of all the restored
              voxel values. that list becomes the
              new undo list, so you can effectivly
              undo an undo. */
void AddVoxelAndValueToUndoList ( VoxelRef inVoxel, int inValue );
void RestoreUndoList ();

                                   /* we need a struct for the undo list. this
              is what we add to it and what we get
              back when the list is restored. */
typedef struct {
  VoxelRef mVoxel;
  unsigned char mValue;
} UndoEntry, *UndoEntryRef;

void NewUndoEntry           ( UndoEntryRef* outEntry, 
            VoxelRef inVoxel, unsigned char inValue );
void DeleteUndoEntry        ( UndoEntryRef* ioEntry );

                                   /* these are our callback functions for the
              undo list. the first deletes an entry
              and the second actually performs the
              undo action and hands back the undone
              voxel. */
void DeleteUndoEntryWrapper ( xUndL_tEntryPtr* inEntryToDelete );
void UndoActionWrapper      ( xUndL_tEntryPtr  inUndoneEntry, 
            xUndL_tEntryPtr* outNewEntry );
void PrintEntryWrapper      ( xUndL_tEntryPtr  inEntry );

xUndoListRef gUndoList = NULL;

// ==========================================================================

// ================================================================= INTERFACE

                                       /* main handling function for clicks.
                                          looks at modifier keys and other
                                          program state and calls the
                                          approritate functions for
                                          manipulation cursor, zoom level, and
                                          selecting. */
void ProcessClick ( int inClickX, int inClickY );
void HandleMouseUp ( XButtonEvent inEvent );
void HandleMouseDown ( XButtonEvent inEvent );
void HandleMouseMoved ( XMotionEvent inEvent );

/* we have three main modes: edit, ctrl pt, and selection. these are analogous to tools. what mode we're in determines what mouse events do. */
typedef enum {
  kMode_Edit = 0,
  kMode_CtrlPt,
  kMode_Select
} Interface_Mode;

Interface_Mode gMode;

void SetMode ( Interface_Mode inMode );
Interface_Mode GetMode ();
char IsInMode ( Interface_Mode inMode );

// ===========================================================================

// ====================================================================== MISC

                                   /* determines if a number is odd. */
#define isOdd(x) (x%2)

                                   /* set and get the tcl interp to send
            the msg to */
void SetTCLInterp ( Tcl_Interp * inInterp );
Tcl_Interp * GetTCLInterp ();

                                   /* send a tcl command */
void SendTCLCommand ( char * inCommand );

                                   /* the tcl interpreter */
Tcl_Interp * gTCLInterp = NULL;

#define set3fv(v,x,y,z) {v[0]=x; v[1]=y; v[2]=z;}

// ===========================================================================

// end_kt 


/*--------------------- prototypes ------------------------------*/
#ifdef Linux
extern void scale2x(int, int, unsigned char *);
#endif

void do_one_gl_event(Tcl_Interp *interp) ;
int Medit(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]);
void usecnap(int usec) ;
int read_second_images(char *imdir2) ;
void resize_window_intstep(int newzf) ;
void write_point(char *dir) ;
void goto_vertex(int vno) ;
// kt - added corresponding orig and canon funcs
void goto_orig_vertex(int vno) ;
void goto_canon_vertex(int vno) ;
void select_control_points(void) ;
void reset_control_points(void) ;
void mark_file_vertices(char *fname) ;
void unmark_vertices(void) ;
void goto_point_coords(int imc1,int ic1,int jc1) ;
void mri2pix(float xpt, float ypt, float zpt, int *jpt, int *ipt,int *impt);
void rotate_brain(float a,char c) ;
void translate_brain(float a,char c) ;
void optimize2(void) ;
void optimize(int maxiter) ;
float Error(int p,float dp) ;
int  imval(float px,float py,float pz) ;
void resize_buffers(int x, int y) ;
void pop_gl_window(void) ;
void redraw(void) ;
void set_scale(void) ;
void goto_point(char *dir) ;
void upslice(void) ;
void downslice(void) ;
void pix_to_rgb(char *fname) ;
void scrsave_to_rgb(char *fname) ;
void save_rgb(char *fname) ;
void edit_pixel(int action);
int open_window(char *name) ;
void draw_surface(void) ;
int read_binary_surface(char *fname) ;
int read_surface(char *fname) ;
void show_orig_surface(void) ;
void show_current_surface(void) ;
int read_orig_vertex_positions(char *name) ;
int read_canonical_vertex_positions(char *fname) ;
int read_binary_surf(char *fname) ;
int show_vertex(void) ;
// kt - added corresponding orig and canon funcs
int show_orig_vertex(void) ;
int show_canon_vertex(void) ;
int dump_vertex(int vno) ;
int write_images(char *fpref) ;
int read_images(char *fpref) ;
void draw_image(int imc, int ic, int jc, unsigned char *** inImageSpace) ;
void draw_second_image(int imc, int ic, int jc) ;
void drawpts(void) ;
void write_dipoles(char *fname) ;
void write_decimation(char *fname) ;
void read_hpts(char *fname) ;
void read_htrans(char *fname) ;
void write_htrans(char *fname) ;
void make_filenames(char *lsubjectsdir,char *lsrname, char *lpname, 
                    char *limdir, char *lsurface) ;
void mirror(void) ;
void read_fieldsign(char *fname) ;
void read_fsmask(char *fname) ;
void read_binary_curvature(char *fname) ;
void smooth_3d(int niter) ;
void flip_corview_xyz(char *newx, char *newy, char *newz) ;
void wmfilter_corslice(int imc) ;
void sagnorm_allslices(void) ;
void sagnorm_corslice(int imc) ;
void alloc_second_im(void) ;
void smooth_surface(int niter) ;

char *Progname ;
/*--------------------- end prototypes ------------------------------*/

/*--------------------- twitzels hacks ------------------------------*/

#ifdef USE_LICENSE

extern char *crypt(const char *, const char *) ;
/* Licensing */
void checkLicense(char* dirname)
{
  FILE* lfile;
  char* email;
  char* magic;
  char* key;
  char* gkey;
  char* lfilename;

  lfilename = (char*)malloc(512);
  email = (char*)malloc(512);
  magic = (char*)malloc(512);
  key = (char*)malloc(512);
  gkey = (char*)malloc(1024);

  sprintf(lfilename,"%s/.license",dirname);

  lfile = fopen(lfilename,"r");
  if(lfile) {
    fscanf(lfile,"%s\n",email);
    fscanf(lfile,"%s\n",magic);
    fscanf(lfile,"%s\n",key);
    
    sprintf(gkey,"%s.%s",email,magic);
    if (strcmp(key,crypt(gkey,"*C*O*R*T*E*C*H*S*0*1*2*3*"))!=0) {
      printf("No valid license key !\n");
      exit(-1);
    }
  }
  else {
    printf("License file not found !\n");
    exit(-1);
  }
  free(email);
  free(magic);
  free(key);
  free(gkey);
  free(lfilename);
  return;  
}
#endif

char hacked_map[256];

/*--------------------- functional ----------------------------------*/
#ifndef FUNCTIONAL_C
#define FUNCTIONAL_C

#include <errno.h>

/* global variables */
int fvwidth,fvheight,fvslices,fvframes;

float fux, fuy, fuz;
float fvx, fvy, fvz;
float fcx, fcy, fcz;

unsigned char* scaledFVolume;
float** rawFVData;
fMRI_REG* fvRegistration;
float mri2fmritm[4][4];

int statsloaded;
int hot;
int do_overlay = 0;
int do_interpolate = 0;
int overlay_frame = 0;

float slfvps;
float slfvst;

double fslope = 1.00;
double fmid = 0.0;
double f2thresh =0.0;

char* overlay_file = NULL;
char loaded_fvfile [128];

typedef union 
{
  long  l;
  float f;
  int i;
  char buf[4];
  short s[2];
} t_uni4;

void transformFV(float a, float b, float c, float* d, float* e, float* f);

void cleanFOV()
{
  int i;
  for(i=0; i<fvslices; i++) {
    free(rawFVData[i]);
  }
  
  free(rawFVData);
}

void setupFOVars()
{
}

void setupFVU(float tux, float tuy, float tuz)
{
  transformFV(tux,tuy,tuz,&fux,&fuy,&fuz);
  fux = fux-fcx;
  fuy = fuy-fcy;
  fuz = fuz-fcz;
}

void setupFVV(float tvx, float tvy, float tvz)
{
  transformFV(tvx,tvy,tvz,&fvx,&fvy,&fvz);
  fvx = fvx-fcx;
  fvy = fvy-fcy;
  fvz = fvz-fcz;
}

void setupFVC(float tcx, float tcy, float tcz)
{
  if(hot)
    printf("setup centerpoint to: %f %f %f\n",tcx,tcy,tcz);
  transformFV(tcx,tcy,tcz,&fcx,&fcy,&fcz);
  if(hot)
    printf("centerpoint is: %f %f %f\n",fcx,fcy,fcz);
}


int countFVSlices(const char* prefix)
{
  int slice_number = 0;
  char fname[STRLEN];
  FILE* fp;

  do
  {
    sprintf(fname, "%s_%3.3d.bfloat", prefix, slice_number) ;
    fp = fopen(fname, "r") ;
    if (fp)   /* this is a valid slice */
    {
      fclose(fp) ;
      slice_number++ ;
    } else
      break;
  } while (1) ;

  return slice_number;
}

/* TODO: fvframes ??? */
void readFVSliceHeader(const char* filename) 
{
  FILE* fp;
  int width, height, nframes;

  fp = fopen(filename, "r");
  
  if(!fp)
    printf("could not open %s !\n",filename);
  else {
    fscanf(fp, "%d %d %d", &width, &height, &nframes);
    fvwidth = width; fvheight = height; fvframes = nframes;
    fclose(fp);
  }
}

float swapFVFloat(float value)
{

  t_uni4 fliess;
  char tmp;
  short tmp2;

  fliess.f = value;
  tmp = fliess.buf[0];
  fliess.buf[0] = fliess.buf[1];
  fliess.buf[1] = tmp;

  tmp = fliess.buf[2];
  fliess.buf[2] = fliess.buf[3];
  fliess.buf[2] = tmp;

  tmp2 = fliess.s[0];
  fliess.s[0]= fliess.s[1];
  fliess.s[1]=tmp2;
 
  return fliess.f;
}

void transformFV(float x, float y, float z, float* x_1, float* y_1,float* z_1)
{

  *x_1 = x*mri2fmritm[0][0]+y*mri2fmritm[0][1]+z*mri2fmritm[0][2]+mri2fmritm[0][3];
  *y_1 = x*mri2fmritm[1][0]+y*mri2fmritm[1][1]+z*mri2fmritm[1][2]+mri2fmritm[1][3];
  *z_1 = x*mri2fmritm[2][0]+y*mri2fmritm[2][1]+z*mri2fmritm[2][2]+mri2fmritm[2][3];
  
  if(hot)
    printf("after mult: %f, %f, %f\n",*x_1,*y_1,*z_1);
  *x_1 = ( ( fvRegistration->in_plane_res * fvwidth ) / 2.0 - (*x_1) ) / 
    fvRegistration->in_plane_res;
  *z_1 = fvheight - 
    ( (fvRegistration->in_plane_res * fvheight) / 2.0 - (*z_1) ) / 
    fvRegistration->in_plane_res;
  *y_1 = ( *y_1 - ( -fvRegistration->slice_thickness * fvslices ) /2.0) / 
    fvRegistration->slice_thickness;
}

/* TODO: swaponly on SGI's */
void readFVVolume(const char* prefixname)
{
  int fvi;
  int fvj;
  int fvsize;
  int fvl;
  char fvfname[255];
  int fvfile;
  float *fvptr;

  hot = 0;

  fvsize = fvwidth*fvheight*fvframes*sizeof(float);
  rawFVData = (float**)malloc(fvslices*sizeof(float*));

  for(fvi=0; fvi<fvslices; fvi++) {
    fvptr = (float*)malloc(fvwidth*fvheight*fvframes*sizeof(float));
    sprintf(fvfname, "%s_%3.3d.bfloat", prefixname, fvi);
    fvfile = open(fvfname, 0);
    if(fvfile) {
      fvl=read(fvfile,fvptr,fvsize);
      printf("slice %d read %d bytes\n",fvi,fvl);
      if( fvl<fvsize ) {
  printf("Scheisse: %s\n",strerror(errno));
      }
      close(fvfile);
      rawFVData[fvi]=fvptr;
#ifdef Linux      
      for(fvj=0; fvj<fvwidth*fvheight*fvframes; fvj++) {
  fvptr[fvj]=swapFVFloat(fvptr[fvj]);
      }
#endif
      
    } else {
      printf("Ooops! could not open file %s\n",fvfname);
      break;
    }
  }
  slfvps = fvRegistration->in_plane_res;
  slfvst = fvRegistration->slice_thickness;
}

void copyFVMatrix()
{
  MATRIX* tmp;

  tmp = fvRegistration->mri2fmri;

  mri2fmritm[0][0] = tmp->data[0];
  mri2fmritm[0][1] = tmp->data[1];
  mri2fmritm[0][2] = tmp->data[2];
  mri2fmritm[0][3] = tmp->data[3];
  mri2fmritm[1][0] = tmp->data[4];
  mri2fmritm[1][1] = tmp->data[5];
  mri2fmritm[1][2] = tmp->data[6];
  mri2fmritm[1][3] = tmp->data[7];
  mri2fmritm[2][0] = tmp->data[8];
  mri2fmritm[2][1] = tmp->data[9];
  mri2fmritm[2][2] = tmp->data[10];
  mri2fmritm[2][3] = tmp->data[11];
  mri2fmritm[3][0] = tmp->data[12];
  mri2fmritm[3][1] = tmp->data[13];
  mri2fmritm[3][2] = tmp->data[14];
  mri2fmritm[3][3] = tmp->data[15];  
}

void loadFV()
{
  char slicename[255];
  char* prefixname;
  char thePathSection[20][64], thePath[256], theStem[64];
  char * theCurSection;
  int theIndex, theRebuildIndex;

  fvRegistration = NULL;
  
  if(overlay_file == NULL) {
    printf("No overlay filename given ! please set funname !\n");
    return;
  }
  prefixname = overlay_file;


  if ( getenv("USE_NEW_OVERLAY") ) {
  // watch kevin's code hijack the functional volume loading from
  // twitzel's hack!!!

                                   /* this gets called the first time with
              the correct prefix name and then again
              a few times with different prefixnames,
              so make sure we only load it once and
              then do nothing if we have already
              loaded. */
  if ( !FuncDis_IsDataLoaded () ) {

    theCurSection = strtok ( prefixname, "/" );
    strcpy ( thePathSection[0], theCurSection );
    theIndex = 1;
    while ( NULL != (theCurSection = strtok ( NULL, "/" )) ) {
      strcpy ( thePathSection[theIndex++], theCurSection );
    }
    
    // last part is the stem, the rest is the path.
    strcpy ( theStem, thePathSection[--theIndex] );
    
    // rebuild the path.
    strcpy ( thePath, "" );
    for ( theRebuildIndex = 0;
    theRebuildIndex < theIndex; 
    theRebuildIndex++ ) {
      sprintf ( thePath, "%s/%s", thePath, thePathSection[theRebuildIndex] );
    }
    
    DebugPrint "loadFV(): got path %s and stem %s\n",
      thePath, theStem EndDebugPrint;
    
    // set the functions it needs to interface with us.
    FuncDis_SetRedrawFunction ( &redraw );
    FuncDis_SetSendTCLCommandFunction ( &SendTCLCommand );

    // load and init.
    FuncDis_LoadData ( thePath, theStem );
    FuncDis_InitGraphWindow ( GetTCLInterp () );
  }    

  // reset all of twitzels stuff.
  do_overlay = 0;
  overlay_file = NULL;
  statsloaded = 0;

  } else {
  

  fvRegistration = StatReadRegistration("register.dat");
 
  if(!fvRegistration) {
    printf("Could not load registration file !\n");
    exit(-1);
  }   
  fvslices = countFVSlices(prefixname);
  
  printf("found %d slices\n", fvslices);

  sprintf(slicename, "%s_%3.3d.hdr", prefixname,0);
  readFVSliceHeader(slicename);
  
  printf("read format: %d, %d, %d\n",fvwidth,fvheight,fvframes);
  copyFVMatrix();

  readFVVolume(prefixname);

  statsloaded = 1;
  strcpy ( loaded_fvfile, overlay_file );
  
  }
}

/* sample Data interpolates trilinear */

#define V000(o) rawFVData[unten][vorn+links+o] 
#define V100(o) rawFVData[unten][vorn+rechts+o]
#define V010(o) rawFVData[unten][hinten+links+o]
#define V001(o) rawFVData[oben][vorn+links+o]
#define V101(o) rawFVData[oben][vorn+rechts+o]
#define V011(o) rawFVData[oben][hinten+links+o]
#define V110(o) rawFVData[unten][hinten+rechts+o]
#define V111(o) rawFVData[oben][hinten+rechts+o]

#define NO_VALUE  -30000.0f

float sampleData(float x, float y, float z,int frame)
{
  int ux,lx,uy,ly,uz,lz; /* Koordinaten der NachbarVoxel */
  int rechts, links, vorn, hinten, oben, unten;
  float v;
  int offset;

  ux = ceilf(x); lx = floorf(x);
  uy = ceilf(y); ly = floorf(y);
  uz = ceilf((fvheight-1-z)); lz = floorf(fvheight-1-z);

  /* printf("Hampf: %d %d\n", uz, lz); */

  x = x - lx;
  y = y - ly;
  z = (fvheight-1-z) - lz;
  
  rechts = ux;
  links = lx;
  oben = uy;
  unten = ly; 
  vorn =  fvwidth*lz;
  hinten = fvwidth*uz;

  offset = frame*fvwidth*fvheight;

  if(ux >= fvwidth || uz >= fvheight || uy >= fvslices || lx < 0 || ly < 0 || lz < 0) {
     
      return NO_VALUE;
  }
    
  v= V000(offset)*(1-x)*(1-z)*(1-y) +
    V100(offset)*x*(1-z)*(1-y)+
    V010(offset)*(1-x)*z*(1-y)+
    V001(offset)*(1-x)*(1-z)*y+
    V101(offset)*x*(1-z)*y+
    V011(offset)*x*z*(1-y)+
    V111(offset)*x*z*y;

  /*printf("returning: %f\n",v);*/
  return v;
}

float lookupInParametricSpace(float u, float v, int frame)
{
  float x,y,z;

  if(frame<fvframes) {
    x = u*fux + v*fvx + fcx;
    y = u*fuy + v*fvy + fcy;
    z = u*fuz + v*fvz + fcz;
    if(x >= fvwidth || y >= fvslices || z >= fvheight || x< 0 || y < 0 || z < 0)
      return NO_VALUE;
    
    if(do_interpolate==1)
      return sampleData(x,y,z,frame); 
    else
      return rawFVData[(int)floor(y)][(int)floor(fvheight-z)*fvwidth+(int)floor(x)+frame*fvwidth*fvheight];
      
  } else {
    return NO_VALUE;
  }
}


float lookupInVoxelSpace(float x, float y, float z, int frame)
{
  float x1,y1,z1;

  if(frame<fvframes) {
    if(hot)
      printf("lookup got: %f, %f, %f\n",x,y,z);
    transformFV(x,y,z,&x1,&y1,&z1);
    if(hot)
      printf("resulting functional coord is: %f %f %f\n",x1,y1,z1);
    
    if(x1 >= fvwidth || x1 < 0 || y1 >=fvslices || y1 < 0 || z1 >= fvheight || z1 <= 0)
      return NO_VALUE ;
    
    if(do_interpolate == 1)
      return sampleData(x1,y1,z1,overlay_frame);
    else
      return rawFVData[(int)floor(y1)][(int)floor(63-z1)*fvwidth+(int)floor(x1)+frame*fvwidth*fvheight];
  }
  return(NO_VALUE) ; 
}
  
void printOutFunctionalCoordinate(float x, float y, float z)
{
  
  float x2,y2,z2;
  float x1,y1,z1;
  /*printf("got coord: %f, %f, %f\n",x,y,z);*/
  x1 = x;
  y1 = y;
  z1 = z;
  if(overlay_frame < fvframes) {
    /*printf("pretransform: %f, %f, %f\n",x1,y1,z1);
      hot = 1;*/
    transformFV(x1,y1,z1,&x2,&y2,&z2);
    hot = 0;
    /*printf("resulting functional coordinate: %f, %f, %f\n",x2,y2,z2);
      printf("resulting voxel coordinate is: (%f, %f, %f)\n",y2,63-z2,x2);*/
    printf("p-value: %f\n",
     (x2 >= fvwidth || x2 < 0 || y2 >=fvslices || y2 < 0 
      || z2 >= fvheight || z2 <= 0)?NO_VALUE:rawFVData[(int)floor(y2)]
     [(int)floor(63-z2)*fvwidth+(int)floor(x2)+overlay_frame*fvwidth*fvheight]); 
  }
}
#endif 
/*--------------------- drawing hacks -------------------------------*/
#ifndef HACK
#define HACK

/* NOTE: In fischl space slices are coded in z, in the fspace in y, so you
   have to swap z & y before !!! transforming
*/

/* static variables *wuerg* */
#define COLOR_WHEEL         0   /* complexval */
#define HEAT_SCALE          1   /* stat,positive,"" */
#define CYAN_TO_RED         2   /* stat,positive,"" */
#define BLU_GRE_RED         3   /* stat,positive,"" */
#define TWOCOND_GREEN_RED   4   /* complexval */
#define JUST_GRAY           5   /* stat,positive,"" */
#define BLUE_TO_RED_SIGNED  6   /* signed */
#define GREEN_TO_RED_SIGNED 7   /* signed */
#define RYGB_WHEEL          8   /* complexval */
#define NOT_HERE_SIGNED     9   /* signed */

float ux, uy, uz;
float vx, vy, vz;
float cx, cy, cz;

unsigned char* dhcache;
float* fcache;

int colscale=1;

/* setup the span vectors */

void initCache()
{
  int i;
  dhcache=(unsigned char*)malloc(512*512);
  fcache=(float*)malloc(512*512*sizeof(float));

  for(i=0; i<512*512; i++)
    fcache[i]=NO_VALUE;
}

void setupSpans()
{
  ux = 128;
  uy = 0;
  uz = 0;

  vx = 0;
  vy = 128;
  vz = 0;

  cx = 128;
  cy = 128;
  cz = 128;  
}

/* some default setups */

void setupCoronal(int slice)
{
  ux = 128;
  uy = 0;
  uz = 0;

  vx = 0;
  vy = 128;
  vz = 0;

  cx = 128;
  cy = 128;
  cz = slice;  

  if(statsloaded) {
    
    setupFVC(128-cx,128-(255-cz),cy-128);
    setupFVU(128-(cx+ux),128-(255-(cz+uz)),(cy+uy)-128);
    setupFVV(128-(cx+vx),128-(255-(cz+vz)),(cy+vy)-128);
  }
}

void setupSagittal(int slice)
{
  ux = 0;
  uy = 0;
  uz = 128;

  vx = 0;
  vy = 128;
  vz = 0;

  cx = slice;
  cy = 128;
  cz = 128;
  
  if(statsloaded) {
    setupFVC(128-cx,128-(255-cz),cy-128);
    setupFVU(128-(cx+ux),128-(255-(cz+uz)),(cy+uy)-128);
    setupFVV(128-(cx+vx),128-(255-(cz+vz)),(cy+vy)-128);
  }
   
}

void setupHorizontal(int slice)
{
  ux = 128;
  uy = 0;
  uz = 0;

  vx = 0;
  vy = 0;
  vz = 128;

  cx = 128;
  cy = slice;
  cz = 128;

  if(statsloaded) {
    setupFVC(128-cx,128-(255-cz),cy-128);
    setupFVU(128-(cx+ux),128-(255-(cz+uz)),(cy+uy)-128);
    setupFVV(128-(cx+vx),128-(255-(cz+vz)),(cy+vy)-128);
  }
}

/* getPlane 
   TODO: 
   * change this into a midpoint algorithm using the spatial 
     coherency of a plane
   * avoid a new setup for every scanline
   * think over integrating texturescaling for sgis
*/

void getPlane(char* fbuffer, int zf, int xoff, int yoff)
{
  float x, y, z;
  int w,h;
  float u,v;
  float step;
  float ostep;
  int ozf;
  int myoff;

  register float sux, svx, aux, avx;
  register float suy, svy, auy, avy;
  register float suz, svz, auz, avz;

  ozf = zf;
  myoff = 512*yoff+xoff;

#ifdef Linux
  if(zf==2) 
    zf = 1;
#endif

  step = 1.0/(128.0*zf);
  ostep = 1.0/(128.0*ozf);

  sux = step*ux; suy = step*uy; suz = step*uz;
  svx = step*vx; svy = step*vy; svz = step*vz;

  aux = -ux; auy = -uy; auz = -uz;
  avx = -vx; avy = -vy; avz = -vz;
  x = aux + avx + cx;
  y = auy + avy + cy; 
  z = auz + avz + cz;

  for(h=0; h<256*zf; ++h) 
  {
    x = aux + avx + cx;
    y = auy + avy + cy; 
    z = auz + avz + cz;
    for(w=0; w<256*zf; ++w) 
    {
      if(z<0 || z > 255 || x <0 || x>255 || y<0 || y>255) {
        dhcache[myoff+w]=0;
        continue;
      }
      /* this sucks on a sgi !!!
         floating point calculation is to slow on R5000 and
         this cast during memory access kills the remaining speed
         */
      dhcache[myoff+w] = im[(int)z][(int)(256-1-y)][(int)x];
      x += sux;
      y += suy; 
      z += suz;
    }
    avx += svx;
    avy += svy;
    avz += svz;

    aux = -ux;
    auy = -uy;
    auz = -uz;
    myoff += 512;
  }

  myoff = 512*yoff+xoff;
  u = v = -1;

  if(statsloaded && do_overlay==1){ 
    for(h=0; h <256*ozf; ++h) {
      for(w=0; w<256*ozf; ++w) {
  
  fcache[myoff+w] = lookupInParametricSpace(u,v,overlay_frame); 
  u += ostep; 
      }
      u = -1;
      v += ostep;
      myoff += 512;
    }
    
  } else if (!statsloaded && do_overlay==1) {
    loadFV();
  }
  
#ifdef Linux
  if(ozf==2)
    scale2x(512,512,dhcache);
#endif
}
void getMaximumProjection(int zf, int xoff, int yoff, int dir)
{
  int w, h;
  
  if(!dir) {
    for(h=0; h<256; h++) {
      for(w=0; w<256; w++) {
  dhcache[(h+yoff)*512+w+xoff] = sim[2][255-h][w];
      }
    }
  } else {
    for(h=0; h<256; h++) {
      for(w=0; w<256; w++) {
  dhcache[(h+yoff)*512+w+xoff] = sim[2][255-h][255-w];
      }
    }
  }
}

void setStatColor(float f, unsigned char *rp, unsigned char *gp, unsigned char *bp,
      float tmpoffset)
{

  float r,g,b;
  float ftmp,c1,c2;

  if (fabs(f)>f2thresh && fabs(f)<fmid)
  {
    ftmp = fabs(f);
    c1 = 1.0/(fmid-f2thresh);
    c2 = 1.0;

    ftmp = c2*(ftmp-f2thresh)+f2thresh;
    f = (f<0)?-ftmp:ftmp;
  }

  if (colscale==HEAT_SCALE)
  {
    if (f>=0)
    {
      r = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<f2thresh)?0:(f<fmid)?(f-f2thresh)/(fmid-f2thresh):1);
      g = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<fmid)?0:(f<fmid+1.00/fslope)?1*(f-fmid)*fslope:1);
      b = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0);
    } else
    {
      f = -f;
      b = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<f2thresh)?0:(f<fmid)?(f-f2thresh)/(fmid-f2thresh):1);
      g = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<fmid)?0:(f<fmid+1.00/fslope)?1*(f-fmid)*fslope:1);
      r = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0);
    }
    r = r*255;
    g = g*255;
    b = b*255;
  }
  else if (colscale==BLU_GRE_RED)
  {  
    if (f>=0)
    {
      r = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<f2thresh)?0:(f<fmid)?(f-f2thresh)/(fmid-f2thresh):1);
      g = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<fmid)?0:(f<fmid+1.00/fslope)?1*(f-fmid)*fslope:1);
      b = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<fmid)?0:(f<fmid+1.00/fslope)?1*(f-fmid)*fslope:1);
    } else
    {
      f = -f;
      b = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<f2thresh)?0:(f<fmid)?(f-f2thresh)/(fmid-f2thresh):1);
      g = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<fmid)?0:(f<fmid+1.00/fslope)?1*(f-fmid)*fslope:1);
      r = tmpoffset*((f<f2thresh)?1:(f<fmid)?1-(f-f2thresh)/(fmid-f2thresh):0)+
          ((f<fmid)?0:(f<fmid+1.00/fslope)?1*(f-fmid)*fslope:1);
    }
    r = r*255;
    g = g*255;
    b = b*255;
  }
  else if (colscale==JUST_GRAY)
  {
    if (f<0) f = -f;
    r = g = b = f*255;
  }
  else
    r = g = b = f*255 ;   /* for compiler warning - don't know what to do */
  *rp = (unsigned char)r;
  *gp = (unsigned char)g;
  *bp = (unsigned char)b;
}

void compose(unsigned char* stbuffer, unsigned char* outbuffer, int dir)
{
  int w,h;
  int hax,hay;
  int i,j,k;
  int curs;

  int imnr;
  int ims;
  unsigned char red,green,blue;
  unsigned char ccolor1,ccolor2;

  k = 0 ;   /* could be uninitialized otherwise?? */

  if(all3flag) {
    ccolor1 = 255;
    ccolor2 = 0;
  } else {
    ccolor1 = 255;
    ccolor2 = 0;
  }
  
  hax = xdim/2;
  hay = ydim/2;
  
  if(all3flag)
    curs = 15;
  else 
    curs = 2;

  for(h=0; h<ydim; h++) {

    for(w=0; w<xdim; w++) {

      if(do_overlay == 1) { 

        if(fcache[h*512+w]!=NO_VALUE)

          setStatColor(fcache[h*512+w],&red,&green,&blue,
                       stbuffer[h*512+w]/255.0);
        else {

          red = green = blue = hacked_map[stbuffer[h*512+w]+MAPOFFSET];
          if (truncflag)
            if (stbuffer[h*512+w]<white_lolim+MAPOFFSET || 
                stbuffer[h*512+w]>white_hilim+MAPOFFSET)
              red = green = blue = hacked_map[MAPOFFSET];
        }

      } else {

        red = green = blue = hacked_map[stbuffer[h*512+w]+MAPOFFSET];
        if (truncflag)
          if (stbuffer[h*512+w]<white_lolim+MAPOFFSET || 
              stbuffer[h*512+w]>white_hilim+MAPOFFSET)
            red = green = blue = hacked_map[MAPOFFSET];
      }
      outbuffer[4*h*xdim+4*w]=red; 
      outbuffer[4*h*xdim+4*w+1]=green; 
      outbuffer[4*h*xdim+4*w+2]=blue;
      outbuffer[4*h*xdim+4*w+3]=255;
    }
  }
  
  if(all3flag || plane==SAGITTAL) 
  {
    for (i=ic-curs;i<=ic+curs;i++) 
      {
        if (all3flag) 
          k = 4*(xdim*hay + hax + i/2*xdim/2+imc/2 + i/2*hax);
        else if(plane==SAGITTAL)        
          k = 4*(i*xdim+imc);
        outbuffer[k] = ccolor1 ; outbuffer[k+1] = ccolor2;
        outbuffer[k+2] = ccolor2;
        outbuffer[k+3]=255;
      }
    for (imnr=imc-curs;imnr<=imc+curs;imnr++) {
      if (all3flag) k = 4*(xdim*hay + hax + ic/2*xdim/2+imnr/2 + ic/2*hax);
      else if(plane==SAGITTAL)
        k = 4*(ic*xdim+imnr);
      outbuffer[k] = ccolor1; 
      outbuffer[k+1] = ccolor2;
      outbuffer[k+2] = ccolor2;
      outbuffer[k+3]=255;
    }
  }
  if(all3flag || plane==CORONAL) 
  {
    for (i=ic-curs;i<=ic+curs;i++) {
      if (all3flag) k = 4*(xdim*hay + i/2*xdim/2+jc/2 + i/2*hax);
      else if(plane==CORONAL)      
        k = 4*(i*xdim+jc);
      vidbuf[k] = ccolor1; vidbuf[k+1] = vidbuf[k+2] = ccolor2; 
      vidbuf[k+3]= 255;
    }
    for (j=jc-curs;j<=jc+curs;j++) {
      if (all3flag) k = 4*(xdim*hay + ic/2*xdim/2+j/2 + ic/2*hax);
      else if(plane==CORONAL)
        k = 4*(ic*xdim+j);
      vidbuf[k] = ccolor1; vidbuf[k+1] = vidbuf[k+2] = ccolor2; 
      vidbuf[k+3]= 255;
    }
  }
  if(all3flag || plane==HORIZONTAL) {
    for (imnr=imc-curs;imnr<=imc+curs;imnr++) {
      if (all3flag) k = 4*(imnr/2*xdim/2+jc/2 + imnr/2*hax);
      else if(plane == HORIZONTAL)
        k = 4*(imnr*xdim+jc);
      vidbuf[k] = ccolor1; vidbuf[k+1] = vidbuf[k+2] = ccolor2;
      vidbuf[k+3]=255;
    }
    for (j=jc-curs;j<=jc+curs;j++) {
      if (all3flag) k = 4*(imc/2*xdim/2+j/2 + imc/2*hax);
      else if(plane == HORIZONTAL)
        k = 4*(imc*xdim+j);
      vidbuf[k] = ccolor1; vidbuf[k+1] = vidbuf[k+2] = ccolor2;
      vidbuf[k+3]=255;
    }
  }
  
  if(all3flag) {
    if(dir==0) {
      for (i=ic-curs;i<=ic+curs;i++) {
        k = 4*(hax + i/2*xdim/2+imc/2 + i/2*hax);
        vidbuf[k] = ccolor1; vidbuf[k+1] = vidbuf[k+2] = ccolor2;
        vidbuf[k+3]=255;
      } 
    } else {
      ims = 511-imc;
      for (i=ic-curs;i<=ic+curs;i++) {
        k = 4*(hax + i/2*xdim/2+ims/2 + i/2*hax);
        vidbuf[k] = ccolor1; vidbuf[k+1] = vidbuf[k+2] = ccolor2;
        vidbuf[k+3]=255;
      } 
    }
    if(dir==0) {
      for (imnr=imc-curs;imnr<=imc+curs;imnr++) {
        k = 4*(hax + ic/2*xdim/2+imnr/2 + ic/2*hax);
        vidbuf[k] = ccolor1; vidbuf[k+1] = vidbuf[k+2] = ccolor2;
        vidbuf[k+3]=255;
      }
    } else {
      ims = 511-imc;
      for (imnr=ims-curs;imnr<=ims+curs;imnr++) {
        k = 4*(hax + ic/2*xdim/2+imnr/2 + ic/2*hax);
        vidbuf[k] = ccolor1; vidbuf[k+1] = vidbuf[k+2] = ccolor2;
        vidbuf[k+3]=255;
      }
    }
  }  
}

void draw_image_hacked(int imc, int ic, int jc)
{
  memset(fcache,0,4*512*512);

  memset(dhcache,128,512*512);

  if(!all3flag) {
    switch(plane) {
    case CORONAL:
      setupCoronal(imc/zf);
      break;
    case SAGITTAL:
      setupSagittal(jc/zf);
      break;
    case HORIZONTAL:
      setupHorizontal(ic/zf);
      break;
    }
    getPlane(NULL,2,0,0);
    compose(dhcache,vidbuf,0);
  } else if(all3flag) {
    setupCoronal(imc/zf);
    getPlane(NULL,1,0,256);
    setupSagittal(jc/zf);
    getPlane(NULL,1,256,256);
    setupHorizontal(ic/zf);
    getPlane(NULL,1,0,0);
    if(jc>255) {
      getMaximumProjection(1,256,0,0);
      compose(dhcache,vidbuf,0);
    } else { 
      getMaximumProjection(1,256,0,1);
      compose(dhcache,vidbuf,1);
    }
  }
  rectwrite(0,0,xdim-1,ydim-1,vidbuf);
  
  /* rectwrite(0,0,xdim-1,ydim-1,dhcache);*/
  
}

#endif

/*--------------------- twitzels hack end ---------------------------*/

int Medit(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    int i,j;
    int tclscriptflag=FALSE;
    char fname[NAME_LENGTH];
    char *lsubjectsdir;
    FILE *fp;
    char lpname[NAME_LENGTH], limdir[NAME_LENGTH], lsrname[NAME_LENGTH];
    char *getenv(), lsurface[NAME_LENGTH];
    /*%%%Sean %%%%*/
    /* char cwd[NAME_LENGTH]; */
    char *cwd; 
    char *word;
    char path[MAX_DIR_DEPTH][NAME_LENGTH];
    int theScaleFactor;
    char isUsingMRIRead;
    char theMRIReadPath [NAME_LENGTH];
    char isLoadingFunctionalData;
    char theFunctionalPath[256], theFunctionalStem[16];

    isUsingMRIRead = FALSE;
    isLoadingFunctionalData = FALSE;
    
    if (argc<2) {
printf("\n");
printf("Usage:\n");
printf("  Using SUBJECTS_DIR environment variable and subject name as a volume source:\n" );
printf("       %s name imagedir [relative_surf_name] [-] [-o functional_path stem] [-tcl script] [-s scale_factor]\n",argv[0]);
 printf("  Options:\n" );     
printf("           name  subjectdir name              (relative to $SUBJECTS_DIR)\n");
printf("       imagedir  orig,T1,brain,wm,filled      (relative to $subject/mri)\n");
printf("       surfname  ?h.orig,?h.smoothwm,?h.plump (relative to $subject/surf)\n");
 printf("functional_path  full path to functional data\n");
 printf("           stem  stem of functional data image files\n");

printf("\n  Specifying a specific dir or file as a volume source:\n");
printf("       %s -f file_or_path [full_surf_path] [-] [-o functinal_path stem] [-tcl script] [-s scale_factor]\n",argv[0]);
 printf("  Options:\n" );     
printf("-f file_or_path  look at file_or_path and guess format to read in\n");
printf(" full_surf_path  the full path including file name to a surface file\n");

 printf("\n  Using volume data in the current directory:\n" );
printf("       %s local [-] [-o functional_path stem] [-tcl script] [-s scale_factor]\n",argv[0]);

 printf("\n  General options:\n" );
 printf("   -tcl script  run in script mode with no visual output\n");
printf("      lone dash  disable editing\n");
printf("-s scale_factor  set window scale factor. 2 is default for 512x512.\n");
printf("\n                                         [vers: 272134--OpenGL]\n");
printf("\n");
exit(1);
    }

                                   /* -tcl and -z switches should always be 
              at the end. the rest of the args don't
              use switches, so we check for these
              switches first and then decrement arcg
              to disguise the fact that they were
              there. */
    // kt - look for scale factor switch
    if ( argc > 2 && MATCH ( argv[argc-2], "-s" ) ) {

      // read in their scale factor.
      theScaleFactor = atoi ( argv[argc-1] );

      // check bounds
      if ( theScaleFactor < 1 ) {
        printf ( "medit: scale factor to small, using 1\n" );
        theScaleFactor = 1;
      }
        
      printf ( "medit: setting scale factor to %d\n", theScaleFactor );

      // we're done with this arg. (this is not the best way to do this)
      argc -= 2;

    } else {

      // no switch, use default.
      theScaleFactor = kDefaultScaleFactor;
    }

    // set the zoom factor
    zf = ozf = theScaleFactor;

    // end_kt

    fsf = (float)zf;
    surflinewidth = zf;

    // look for -tcl switch
    if (MATCH(argv[argc-2],"-tcl")) {
      argc -= 2;   /* strip so like before */
      tclscriptflag = TRUE;
    }
    statsloaded = 0;

    if ( argc > 3 && MATCH ( argv[argc-3], "-o" ) ) {

      isLoadingFunctionalData = TRUE;
      strcpy ( theFunctionalPath, argv[argc-2] );
      strcpy ( theFunctionalStem, argv[argc-1] );
      argc -= 3;
    }

                                   /* at this point, our remaining argv is:
              0       1     2            3         4
              tkmedit name  imagedir     [surface] [-]
              tkmedit local [-]
              tkmedit -f    file_or_path [surface] [-]
              we now look at argv[1] and use it to 
              determine the method to read our volume.
              we also check for a surface. */
    
    // look at the first arg.
    strcpy ( lpname, argv[1] );

    // new style, use MRIread to load volume data. grab arg2 as the path.
    isUsingMRIRead = FALSE;
    if ( MATCH(lpname, "-f") ) {
      isUsingMRIRead = TRUE;
      strcpy ( theMRIReadPath, argv[2] );
      strcpy ( limdir, "" );

      // gave 'local' or '.' switch, so use local.
    } else if ( MATCH(lpname,"local") || MATCH(lpname,".") ) {
      strcpy ( limdir, "local" );

      // enough args to give us the subject name and volume type.
    } else if ( argc > 2 ) {
      strcpy ( limdir,argv[2] );

      // error.
    } else {

      printf("medit: ### imagedir missing\n"); exit(1); 
    }

    // check for teh subjects dir env variable. only need it if we're not
    // using MRIread.
    lsubjectsdir = getenv("SUBJECTS_DIR");
    if ( lsubjectsdir == NULL && !isUsingMRIRead ) {
      printf("medit: env var SUBJECTS_DIR undefined (use setenv)\n");
      printf("       to specify a file or path, use the form:\n");
      printf("       tkmedit -f file_or_path [surface] [-] [-tcl tcl_script] [-s scale_factor]\n" );
      exit(1);
    }

    // make the path that we should find the data in.
    sprintf ( fname, "%s/%s", lsubjectsdir, lpname );
    
    // if we're not using local dir and not using MRIRead...
    if ( !MATCH ( lpname, "local" ) &&
   !isUsingMRIRead ) {

      // try to open the dir.
      if ((fp=fopen (fname,"r"))==NULL) {
        printf("medit: ### can't find subject %s\n",fname); exit(1);}
      else fclose(fp);
    }

    // check for diable edit flag. it's always last.
    if (argv[argc-1][0]=='-') {

      editflag = FALSE;
      printf("medit: ### editing disabled\n");

      // set mode to selection
      SetMode ( kMode_Select );

    } else {

      // else default into editing mode.
      SetMode ( kMode_Edit );
    }

    // check for surface name. note that this will never be true if we're
    // in local mode. it also works for both 'name imagedir' and 
    // '-f file_or_path' methods.
    if ( (editflag && argc>3) || (!editflag && argc>4)) {

      // found one, read it in. set surfflag to load and show it.
      surfflag = TRUE;
      strcpy(lsurface,argv[3]);

    } else {

      // use default surface name.
      strcpy(lsurface,"rh.orig");
    }

    // check for bad flags.
    if (argc>2 && MATCH(limdir,"local") && editflag)
      printf("medit: ### ignored bad arg after \"local\": %s\n",argv[2]);

                                   /* at this point, we're done
              processing switches. */

    /* parse cwd to set session root (guess path to bem) */
    /* %%%%Sean %%%% */
    /* getwd(cwd); */
#ifdef Linux
    cwd = getcwd(NULL,0);
#else
    cwd = getenv("PWD");
#endif

    word = strtok(cwd,"/");
    strcpy(path[0],word);
    i = 1;
    while ((word = strtok(NULL,"/")) != NULL) {  /* save,count */
      strcpy(path[i],word);
      i++;
    }
    if (MATCH(path[i-1],"scripts") && 
        MATCH(path[i-2],lpname)) {
      printf("medit: in subjects \"scripts\" dir\n");
      j = i-1;
    } else if (MATCH(path[i-1],"scripts") &&
               MATCH(path[i-2],"eegmeg") &&
               MATCH(path[i-3],lpname)) {
      printf("medit: in subjects \"eegmeg/scripts\" dir\n");
      j = i-1;
    } else if (MATCH(path[i-1],"scripts")) {
      printf("medit: in \"scripts\" dir (not subjects,eegmeg)\n");
      j = i-1;
    } else if (MATCH(limdir,"local")) {  /* local even if in mri */
      printf("medit: local data set => session root is cwd\n");
      j = i;
    } else if (MATCH(path[i-2],"mri")) {
      printf("medit: in subdir of \"mri\" dir\n");
      j = i-1;
    } else {
      printf(
  "medit: not in \"scripts\" dir or \"mri\" subdir => session root is cwd\n");
      j = i;
    }
    sprintf(lsrname,"/%s",path[0]);  /* reassemble absolute */
    for(i=1;i<j;i++)
      sprintf(lsrname,"%s/%s",lsrname,path[i]);
    printf("medit: session root data dir ($session) set to:\n");
    printf("medit:     %s\n",lsrname);

    // make filenames.
    make_filenames(lsubjectsdir,lsrname,lpname,limdir,lsurface);

    // if we want to load a surface, do so. however... if we are using MRIRead,
    // they specified a full surface name, so use that (lsurface). else
    // use the partial surface name concatenated with the SUBJECTS_DIR (sfname)
    if ( surfflag ) {

      if ( isUsingMRIRead ) 
  read_surface( lsurface );
      else
  read_surface( sfname );
    }

    // read in the volume using the selected method. sets many of our volume
    // related variables as well as allocates and fills out the volume array.
    if ( isUsingMRIRead ) {
      ReadVolumeWithMRIRead ( theMRIReadPath );
    } else {
      read_images(mfname);
    }

    // now we can set up the bounds on our transforms.
    trans_SetBounds ( xx0, xx1, yy0, yy1, zz0, zz1 );
    trans_SetResolution ( ps, ps, st );

    // if we have functional data...
    if ( isLoadingFunctionalData ) {

      // load it.
      FuncDis_LoadData ( theFunctionalPath, theFunctionalStem );

      // go to selection mode.
      SetMode ( kMode_Select );
    }

    // allocate graphics buffers.
    vidbuf = (GLubyte*)        lcalloc ( (size_t)xdim*ydim*4,
           (size_t)sizeof(GLubyte)); 
    cvidbuf = (Colorindex *)   lcalloc ( (size_t)CVIDBUF,
           (size_t)sizeof(Colorindex));
    binbuff = (unsigned char *)lcalloc ( (size_t)3*xdim*ydim,
           (size_t)sizeof(char));

    // set initial cursor coords.
    imc = zf*imnr1/2;
    ic = ydim/2;
    jc = xdim/2;
   
    /* for twitzels stuff */
    statsloaded = 0;
    initCache();
    setupFOVars();

    /*loadFV("970121NH_02986_00003_00009_00001_001");*/
    /*loadFV("twsyn");*/
    /*overlay_file = "Sel_COND_6_0_diff";
      loadFV();*/
    /*loadFV("Sel_KP");*/
    /*loadFV("Sel_nomask_4_1_diff");*/
   
    /* end twitzels stuff */

    if (tclscriptflag) {
      /* called from tkmedit.c; do nothing (don't even open gl window) */
      /* wait for tcl interp to start; tkanalyse calls tcl script */
    }
    else {   /* open window for medit or non-script tkmedit */
      if (open_window(pname) < 0) {
  exit(1);
      }
      redraw();
    }

    return(0);
}

void
do_one_gl_event(Tcl_Interp *interp)   /* tcl */
{

  XEvent current, ahead;
  char buf[1000];
  char command[NAME_LENGTH];
  KeySym ks;
  short sx,sy; /* Screencoord sx,sy; */
  XWindowAttributes wat;
  Window junkwin;
  int rx, ry;

  
  if (!openglwindowflag) return;
  if (updatepixval) {
    Tcl_Eval(interp,"pixvaltitle 1 1 1");
    /*    Tcl_Eval(interp,"set selectedpixval $selectedpixval"); *//*touch for trace*/
    updatepixval = FALSE;
  }
  if (XPending(xDisplay)) {  /* do one if queue test */
  
    XNextEvent(xDisplay, &current);   /* blocks here if no event */

    switch (current.type) {

      case ConfigureNotify:
        XGetWindowAttributes(xDisplay, w.wMain, &wat);
        XTranslateCoordinates(xDisplay, w.wMain, wat.root,
                              -wat.border_width, -wat.border_width,
                              &rx, &ry, &junkwin);
        w.x = rx;  /* left orig         (current.xconfigure.x = 0 relative!) */
        w.y = ry;  /* top orig: +y down (current.xconfigure.y = 0 relative!) */
        w.w = current.xconfigure.width;
        w.h = current.xconfigure.height;
        resize_window_intstep(0);
        /* Tcl_Eval(interp,"set zf $zf"); */ /* touch for trace */
        if (followglwinflag && zf != 4) {
          sprintf(command,"wm geometry . +%d+%d",
                      w.x, w.y + w.h + kWindowBottomBorderHeight );
          Tcl_Eval(interp,command);
          /*Tcl_Eval(interp,"raise .");*/
        }
        break;

      case Expose:
        if (XPending(xDisplay)) {
          XPeekEvent(xDisplay, &ahead);
          if (ahead.type==Expose) break;  /* skip extras */
        }
        redraw();
        break;

    case ButtonPress:
    case ButtonRelease:

  // hack the click location.
        sx = current.xbutton.x;
        sy = current.xbutton.y;
        sx += w.x;   /* convert back to screen pos (ugh) */
        sy = 1024 - w.y - sy;
  current.xbutton.x = sx;
  current.xbutton.y = sy;
  
  // pass it to the handler
  if ( ButtonPress == current.type ) {
    HandleMouseDown ( current.xbutton );
  } else {

    HandleMouseUp ( current.xbutton );

    // redraw if going up.
    redraw ();
  }

  // tell the tk window to update its coords.
        SendUpdateMessageToTKWindow ();

        break;

      case MotionNotify:

  // only do it if we have a button down.
  if ( current.xmotion.state & Button1Mask ||
       current.xmotion.state & Button2Mask ||
       current.xmotion.state & Button3Mask ) {

    // hack the click location.
    sx = current.xmotion.x;
    sy = current.xmotion.y;
    sx += w.x;   /* convert back to screen pos (ugh) */
    sy = 1024 - w.y - sy;
    current.xmotion.x = sx;
    current.xmotion.y = sy;
    
    // pass it to the handler.
    HandleMouseMoved ( current.xmotion );
  }

        if (XPending(xDisplay)) {
          XPeekEvent(xDisplay, &ahead);
          if      (ahead.type==ButtonPress)   changeplanelock = TRUE;
          else if (ahead.type==ButtonRelease) changeplanelock = TRUE;
          else                                changeplanelock = FALSE;
        }
        break;

      case KeyPress:
        XLookupString(&current.xkey, buf, sizeof(buf), &ks, 0);
        switch (ks) {

        case XK_c: 
        case XK_0: 
        case XK_apostrophe:
          if ( current.xkey.state & Mod1Mask ) Tcl_Eval(interp,
                ".mri.main.right.cmp.aCOMPARE.bu invoke");
          break;

        case XK_M:
        case XK_m:
          if ( current.xkey.state & Mod1Mask ) 
            Tcl_Eval(interp,
                     ".mri.main.left.view.butt.left.amaxflag.ck invoke");
          break;

        case XK_d:

          if ( current.xkey.state & Mod1Mask ) 

            Tcl_Eval(interp, 
                     ".mri.main.left.view.butt.left.asurface.ck invoke");

          // kt
          else if (  current.xkey.state & ControlMask ) {

            DeleteSelectedCtrlPts ();
            redraw ();
          }
          // end_kt

          break;

        case XK_b:
          if ( current.xkey.state & Mod1Mask ) Tcl_Eval(interp,
                                      "set prad 0");
          break;

        case XK_v:

          if (  current.xkey.state & Mod1Mask  && ! current.xkey.state & ControlMask ) {
      Tcl_Eval(interp,
                 "set pradtmp $prad; set prad $pradlast; set pradlast $pradtmp");
    } else if (  current.xkey.state & ControlMask ) {

      // kt - ctrl+v toggles surface vertex display.
      SetSurfaceVertexDisplayStatus ( !IsSurfaceVertexDisplayOn () );
      redraw ();
    }
      

            break;
            
        case XK_B:
          if ( current.xkey.state & Mod1Mask ) Tcl_Eval(interp,
                  "set prad [expr ($prad+1)]");
            break;

        case XK_asterisk:
          if ( current.xkey.state & Mod1Mask ) Tcl_Eval(interp,
                  "set fsquash [expr $fsquash * 1.1]; set_scale");
            break;

        case XK_slash:
          if ( current.xkey.state & Mod1Mask ) Tcl_Eval(interp,
                  "set fsquash [expr $fsquash / 1.1]; set_scale");
          break;

        case XK_plus:
          if ( current.xkey.state & Mod1Mask ) Tcl_Eval(interp,
                   "set fthresh [expr $fthresh + 0.05]; set_scale");
            break;

        case XK_minus:
          if ( current.xkey.state & Mod1Mask ) Tcl_Eval(interp,
                   "set fthresh [expr $fthresh - 0.05]; set_scale");
            break;

        case XK_w:
          if ( current.xkey.state & Mod1Mask ) Tcl_Eval(interp,
                  ".mri.main.left.head.save.aSAVEIMG.bu invoke");
          break;

        case XK_x: 
        case XK_X: 
          // kt - changed to call the setplane function to properly
          // restore cursor coords.
          SetPlane ( SAGITTAL );
          redraw();
          break;
        
        case XK_y: 
        case XK_Y: 

          // kt
          if (  current.xkey.state & ControlMask ) {

            ToggleCtrlPtDisplayStyle ();
            redraw ();

          } else {
            // end_kt

            // kt - changed to call the setplane function to properly
            // restore cursor coords.
            SetPlane ( HORIZONTAL );
            redraw();
          }

          break;

        case XK_z: 
        case XK_Z: 

    // kt - ctrl-z undo
    if ( current.xkey.state & ControlMask &&
         IsInMode ( kMode_Edit ) ) {

      RestoreUndoList ();
      redraw ();

    } else {
      
      // kt - changed to call the setplane function to properly
      // restore cursor coords.
      SetPlane ( CORONAL );
      redraw();

    }
          break;

        case XK_r:
          if ( current.xkey.state & Mod1Mask ) {
            /*Tcl_Eval(interp,"raise .");*/
            goto_point(tfname);
            redraw();
          }
          break;

        case XK_f:
          if ( current.xkey.state & Mod1Mask ) write_point(tfname);
          break;

        case XK_I:
          if ( current.xkey.state & Mod1Mask ) { mirror(); redraw();}
          break;

        case XK_R:
          if ( current.xkey.state & Mod1Mask ) {
            read_hpts(hpfname);
            read_htrans(htfname);
            redraw();
          }
          break;

        case XK_W:
          if ( current.xkey.state & Mod1Mask ) 
            write_htrans(htfname);
          break;
          
          // kt
        case XK_h:

          if (  current.xkey.state & ControlMask ) {
           
            // toggle the display status of the ctrl pts
            ToggleCtrlPtDisplayStatus ();
            redraw ();
          }

          break;

        case XK_s:

          if (  current.xkey.state & ControlMask ) {

            // write the ctrl pt file.
            WriteCtrlPtFile ( tfname );

          }

          break;

        case XK_n:

          if (  current.xkey.state & ControlMask ) {

            // clear all selected points.
            VList_ClearList ( gSelectionList );

            redraw ();
          }

          break;
          // end_kt

  case XK_a:
    
    if (  current.xkey.state & ControlMask ) {

      // toggle the average vertex display status.
      SetAverageSurfaceVerticesStatus 
        ( !IsAverageSurfaceVerticesOn() );
      redraw ();
    }
    break;

        case XK_Up:
        case XK_Right:
    upslice ();
    redraw ();
          break;
          
        case XK_Down:
        case XK_Left:
    downslice ();
    redraw ();
          break;

          /* TODO: install X versions of rotate headpoints bindings */
#if 0
          /* modifiers */
        case XK_Shift_L:   
        case XK_Shift_R:
          shiftkeypressed=TRUE; 
          break;

        case XK_Control_L: 
        case XK_Control_R: 
          ctrlkeypressed=TRUE; 
          break;

        case XK_Alt_L:     
        case XK_Alt_R:
          altkeypressed=TRUE;   
          break;
#endif
        }
        break;
#if 0
      case KeyRelease:   /* added this mask to xwindow.c */
  
  XLookupString(&current.xkey, buf, sizeof(buf), &ks, 0);
        switch (ks) {
          case XK_Shift_L:   case XK_Shift_R:   shiftkeypressed=FALSE; break;
          case XK_Control_L: case XK_Control_R: ctrlkeypressed=FALSE;  break;
          case XK_Alt_L:     case XK_Alt_R:     altkeypressed=FALSE;   break;
        }
        break;
#endif
    }
    /* return GL_FALSE; */
  }

}

int
open_window(char *name)

{

  XSizeHints hin;

  if (openglwindowflag) {
    printf("medit: ### GL window already open: can't open second\n");
    PR return(0);
  }

  /* TKO_DEPTH because all OpenGL 4096 visuals have depth buffer!! */
  /* tkoInitDisplayMode(TKO_DOUBLE | TKO_INDEX | TKO_DEPTH);  */
  /*  tkoInitDisplayMode(TKO_DOUBLE | TKO_INDEX );  */
   tkoInitDisplayMode(TKO_RGB | TKO_SINGLE);   

  if (!initpositiondoneflag)
    tkoInitPosition ( kDefaultWindowLocationX,
          kDefaultWindowLocationY,
          xdim, ydim );
  if (!tkoInitWindow(name)) {
    printf("medit: ### tkoInitWindow(name) failed\n");exit(1);}
  hin.max_width = hin.max_height = 4*xnum + xnum/2;  /* maxsize */
  hin.min_aspect.x = hin.max_aspect.x = xdim;        /* keepaspect */
  hin.min_aspect.y = hin.max_aspect.y = ydim;
  hin.flags = PMaxSize|PAspect;
  XSetWMNormalHints(xDisplay, w.wMain, &hin);

  /* TODO: bitmap drawing is slower */
  /* (did: glDisable's,4bit,RasPos:+0.5,glOrtho-1,z=0,wintitle,clrdepth) */
  /* (did: glPixelStorei:GL_UNPACK_ALIGNMENT,glOrtho not -1) */
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glOrtho(0.0, (double)(xnum-1), 0.0, (double)(ynum-1), -1.0, 1.0);
  /*  0.0 to x,y-1.0:  COR,SAG surf up2; HOR surf *correct* */
  /*  0.0 to x,y:      COR,SAG surf up/left; HOR surf down/left */
  /* -0.5 to x,y-0.5:  COR,SAG surf up/left; HOR surf down/left */
  /*  0.5 to x,y+0.5:  fails (??!!) */
  /*  0.0 to x-1,y:    COR,SAG surf up; HOR down */
  /*  0.0 to x,y-1:    COR,SAG surf up2/left; HOR left */
  /*  0.0 to x-1,y+1:  COR,SAG surf *correct*; HOR down */
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  openglwindowflag = TRUE;
  return(0);
}

void
resize_window_intstep(int newzf) {

  int tzf;

  if (newzf==0)
    tzf = rint((float)w.w/(float)xnum); /* int constrain interactive resize */
  else
    tzf = newzf;
  tzf = (tzf<1)?1:(tzf>4)?4:tzf;
  if (w.w%xnum || w.h%ynum || newzf>0) {
    ozf = zf;
    zf = tzf;
    fsf = (float)zf;
    xdim = zf*xnum;
    ydim = zf*ynum;
    XResizeWindow(xDisplay, w.wMain, xdim, ydim);
    if (TKO_HAS_OVERLAY(w.type))
      XResizeWindow(xDisplay, w.wOverlay, xdim, ydim);
    glViewport(0, 0, xdim, ydim); 
    resize_buffers(xdim, ydim);
    w.w = xdim;
    w.h = ydim;

    surflinewidth=zf;
    imc = (zf*imc)/ozf;
    ic = (zf*ic)/ozf;
    jc = (zf*jc)/ozf;
  }
}

void 
move_window( int x, int y) {

  if (openglwindowflag) {
    XMoveWindow(xDisplay, w.wMain, x, y);
    w.x = x;
    w.y = y;
  }
  else if (!initpositiondoneflag) {
    tkoInitPosition(x,y,xdim,ydim);
    initpositiondoneflag = TRUE;
  }
  else ;

}

void
edit_pixel(int action)
{
  int theXRadius, theYRadius, theZRadius;
  int theX, theY, theZ;
  unsigned char thePixelValue, theNewPixelValue;
  int theVoxX, theVoxY, theVoxZ;
  VoxelRef theVoxel;

  Voxel_New ( &theVoxel );

  // set all radiuses to the default prad.
  theXRadius = theYRadius = theZRadius = prad;

  // if our non3d flag is on, limit the radius of the plane we're in.
  if ( inplaneflag ) { 
    if ( plane==CORONAL )    theZRadius = 0;
    if ( plane==HORIZONTAL ) theYRadius = 0;
    if ( plane==SAGITTAL )   theXRadius = 0; 
  }
  
  // convert cursor to voxel coords to get our center voxel
  ScreenToVoxel ( plane, jc, ic, imc,
                  &theVoxX, &theVoxY, &theVoxZ );

  for ( theZ = theVoxZ - theZRadius; theZ <= theVoxZ + theZRadius; theZ++ )
    for ( theY = theVoxY - theYRadius; theY <= theVoxY + theYRadius; theY++ )
      for ( theX = theVoxX - theXRadius; theX <= theVoxX + theXRadius; theX++){

        // if this voxel is valid..
        DisableDebuggingOutput;
        if ( IsVoxelInBounds ( theX, theY, theZ ) ) {
          
          EnableDebuggingOutput;
        
          // if we're drawing in a circle and this point is outside
          // our radius squared, skip this voxel.
          if ( circleflag && 
               (theVoxX-theX)*(theVoxX-theX) + 
               (theVoxY-theY)*(theVoxY-theY) + 
               (theVoxZ-theZ)*(theVoxZ-theZ) > prad*prad ) {
            continue;
          }

          // get the pixel value at this voxel.
          thePixelValue = GetVoxelValue ( theX, theY, theZ );

          // choose a new color.
          if ( TO_WHITE == action && 
               thePixelValue < WM_MIN_VAL ) {

            theNewPixelValue = 255;

          } else if ( TO_BLACK == action ) {
            
            theNewPixelValue = WM_EDITED_OFF;

          } else {
      
      theNewPixelValue = thePixelValue;
    }
  

    // if this pixel is different from the new value..
    if ( theNewPixelValue != thePixelValue ) {
      
      // save this value in the undo list.
      Voxel_Set ( theVoxel, theX, theY, theZ );
      AddVoxelAndValueToUndoList ( theVoxel, thePixelValue );
    }

    // set the pixel value.
    SetVoxelValue ( theX, theY, theZ, theNewPixelValue );
        }

        EnableDebuggingOutput;
      }
  
  // mark the slice files that we changed.
  for ( theZ = -prad; theZ <= prad; theZ++ ) {
    if ( theVoxZ + theZ >= 0 && theVoxZ + theZ < numimg ) {
      changed [ theVoxZ + theZ ] = TRUE;
      editedimage = imnr0 + theVoxZ + theZ;
    }
  }

  Voxel_Delete ( &theVoxel );
}

void
save_rgb(char *fname)
{
  if (!openglwindowflag) {
    printf("medit: ### save_rgb failed: no gl window open\n");PR return; }

  if (scrsaveflag) { scrsave_to_rgb(fname); }
  else             { pix_to_rgb(fname);     }
}

void
scrsave_to_rgb(char *fname)  /* about 2X faster than pix_to_rgb */
{
  char command[2*NAME_LENGTH];
  FILE *fp;
  long xorig,xsize,yorig,ysize;
  int x0,y0,x1,y1;

  getorigin(&xorig,&yorig);
  getsize(&xsize,&ysize);

  x0 = (int)xorig;  x1 = (int)(xorig+xsize-1);
  y0 = (int)yorig;  y1 = (int)(yorig+ysize-1);
  fp = fopen(fname,"w");
  if (fp==NULL) {printf("medit: ### can't create file %s\n",fname);PR return;}
  fclose(fp);
  if (bwflag) sprintf(command,"scrsave %s %d %d %d %d -b\n",fname,x0,x1,y0,y1);
  else        sprintf(command,"scrsave %s %d %d %d %d\n",fname,x0,x1,y0,y1);
  system(command);
  printf("medit: file %s written\n",fname);PR
}

void
pix_to_rgb(char *fname)
{
  GLint swapbytes, lsbfirst, rowlength;
  GLint skiprows, skippixels, alignment;
  RGB_IMAGE *image;
  int y,width,height,size;
  unsigned short *r,*g,*b;
  unsigned short  *red, *green, *blue;
  FILE *fp;

  if (bwflag) {printf("medit: ### bw pix_to_rgb failed--TODO\n");PR return;}

  width = (int)xdim;
  height = (int)ydim;
  size = width*height;

  red = (unsigned short *)calloc(size, sizeof(unsigned short));
  green = (unsigned short *)calloc(size, sizeof(unsigned short));
  blue = (unsigned short *)calloc(size, sizeof(unsigned short));

  glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
  glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
  glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
  glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

  glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glReadPixels(0, 0, width, height, GL_RED,GL_UNSIGNED_SHORT, (GLvoid *)red);
  glReadPixels(0, 0, width, height, GL_GREEN,GL_UNSIGNED_SHORT,(GLvoid *)green);  glReadPixels(0, 0, width, height, GL_BLUE,GL_UNSIGNED_SHORT, (GLvoid *)blue);

  glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
  glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
  glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

  fp = fopen(fname,"w");
  if (fp==NULL){printf("medit: ### can't create file %s\n",fname);PR return;}
  fclose(fp);
  image = iopen(fname,"w",UNCOMPRESSED(1), 3, width, height, 3);
  for(y = 0 ; y < height; y++) {
    r = red + y * width;
    g = green + y * width;
    b = blue + y * width;
    putrow(image, r, y, 0);
    putrow(image, g, y, 1);
    putrow(image, b, y, 2);
  }
  iclose(image);
  free(red); free(green); free(blue);

  printf("medit: file %s written\n",fname);PR
}

void
downslice () {

  int theVoxX, theVoxY, theVoxZ;
  int theScreenX, theScreenY, theScreenZ;

  // get the screen pt as a voxel
  ScreenToVoxel ( plane, jc, ic, imc, 
                  &theVoxX, &theVoxY, &theVoxZ );

  // switch on the current plane...
  switch ( plane ) {

  case CORONAL:
    theVoxZ --;
    break;

  case HORIZONTAL:
    theVoxY --;
    break;

  case SAGITTAL:
    theVoxX --;
    break;
  }

  // set the voxel back to the cursor
  VoxelToScreen ( theVoxX, theVoxY, theVoxZ,
                  plane, &theScreenX, &theScreenY, &theScreenZ );
  SetCursorToScreenPt ( theScreenX, theScreenY, theScreenZ );

  /*
  if (plane==CORONAL)
    imc = (imc<zf)?imnr1*zf-zf+imc:imc-zf;
  else if (plane==HORIZONTAL)
    ic = (ic<zf)?ydim-zf+ic:ic-zf;
  else if (plane==SAGITTAL)
    jc = (jc<zf)?xdim-zf+jc:jc-zf;
  */
}

void
upslice () {

  int theVoxX, theVoxY, theVoxZ;
  int theScreenX, theScreenY, theScreenZ;

  // get the screen pt as a voxel
  ScreenToVoxel ( plane, jc, ic, imc, 
                  &theVoxX, &theVoxY, &theVoxZ );

  // switch on the current plane...
  switch ( plane ) {

  case CORONAL:
    theVoxZ ++;
    break;

  case HORIZONTAL:
    theVoxY ++;
    break;

  case SAGITTAL:
    theVoxX ++;
    break;
  }

  // set the voxel back to the cursor
  VoxelToScreen ( theVoxX, theVoxY, theVoxZ,
                  plane, &theScreenX, &theScreenY, &theScreenZ );
  SetCursorToScreenPt ( theScreenX, theScreenY, theScreenZ );

  /*
  if (plane==CORONAL)
    imc = (imc>=imnr1*zf-zf)?imc+zf-imnr1*zf:imc+zf;
  else if (plane==HORIZONTAL)
    ic = (ic>=ydim-zf)?ic+zf-ydim:ic+zf;
  else if (plane==SAGITTAL)
    jc = (jc>=xdim-zf)?jc+zf-xdim:jc+zf;
  */
}

void
goto_point(char *dir)
{
  char fname[NAME_LENGTH];
  FILE *fp;
  float xpt,ypt,zpt;

  sprintf(fname,"%s/edit.dat",dir);
  fp=fopen(fname,"r");
  if (fp==NULL) {printf("medit: ### File %s not found\n",fname);PR return;}
  fscanf(fp,"%f %f %f",&xpt,&ypt,&zpt);
  fclose(fp);
  SetCursorToRASPt ( (Real)xpt, (Real)ypt, (Real)zpt );
}
void
unmark_vertices(void)
{
  int vno ;

  if (!mris)
  {
    fprintf(stderr, "no surface loaded.\n") ;
    return ;
  }
  for (vno = 0 ; vno < mris->nvertices ; vno++)
    mris->vertices[vno].marked = 0 ;
  redraw() ;
}
void
mark_file_vertices(char *fname)
{
  FILE  *fp ;
  char  line[200], *cp ;
  int   vno, nvertices, nargs ;
  float area ;

  if (!mris)
  {
    fprintf(stderr, "no surface loaded.\n") ;
    return ;
  }
  fp = fopen(fname, "r") ;
  if (!fp)
  {
    fprintf(stderr, "could not open file %s.\n", fname) ;
    return ;
  }

  fgetl(line, 150, fp) ;
  nargs = sscanf(line, "%d %f", &nvertices, &area) ;
  if (nargs == 2)
    fprintf(stderr, "marking %d vertices, %2.3f mm^2 surface area\n",
            nvertices, area) ;
  else if (nargs == 1)
    fprintf(stderr, "marking %d vertices\n", nvertices) ;

  while  ((cp = fgetl(line, 150, fp)) != NULL)
  {
    sscanf(cp, "%d", &vno) ;
    if (vno >= 0 && vno < mris->nvertices)
    {
      mris->vertices[vno].marked = 1 ;
    }
  }
  goto_vertex(vno) ;
  fclose(fp) ;
}
void
select_control_points(void)
{

  OutputPrint "NOTE: select_control_points is being deprecated. "
    "From now one, please use CtrlPtMode or ControlPointMode.\n" 
    EndOutputPrint;

  SetMode ( kMode_CtrlPt );

}

void
reset_control_points(void) {

  OutputPrint "NOTE: reset_control_points is being deprecated. "
    "From now one, please use one of the mode changing commands to change "
    "modes, such as EditMode or SelectionMode\n."
    EndOutputPrint;

  SetMode ( kMode_Edit );
}

void
goto_vertex(int vno) {

  VERTEX *v ;

  if (!mris) {
    OutputPrint "Cannont goto vertex: surface is not loaded.\n" EndOutputPrint;
    return ;
  }

  v = &mris->vertices[vno] ;

  OutputPrint "Found vertex %d at RAS pt (%2.2f, %2.2f, %2.2f)\n",
    vno, v->x, v->y, v->z EndOutputPrint;

  // hilite this vertex.
  HiliteSurfaceVertex ( v, kSurfaceType_Current );

  // set the cursor and go there.
  SetCursorToRASPt ( (Real)(v->x), (Real)(v->y), (Real)(v->z) );
  RecenterViewToScreenPt ( jc, ic, imc );

  redraw() ;
}

void
goto_orig_vertex(int vno) {

  VERTEX *v ;

  if (!mris) {
    OutputPrint "Cannont goto vertex: surface is not loaded.\n" EndOutputPrint;
    return ;
  }
  v = &mris->vertices[vno] ;

  OutputPrint "Found vertex %d at RAS pt (%2.2f, %2.2f, %2.2f)\n",
    vno, v->origx, v->origy, v->origz EndOutputPrint;

  // hilite this vertex.
  HiliteSurfaceVertex ( v, kSurfaceType_Original );

  // set the cursor and go there.
  SetCursorToRASPt ( (Real)(v->origx), (Real)(v->origy), (Real)(v->origz) );
  RecenterViewToScreenPt ( jc, ic, imc );

  redraw() ;
}

void
goto_canon_vertex(int vno) {

  VERTEX *v ;

  if (!mris) {
    OutputPrint "Cannont goto vertex: surface is not loaded.\n" EndOutputPrint;
    return ;
  }
  v = &mris->vertices[vno] ;

  OutputPrint "Found vertex %d at RAS pt (%2.2f, %2.2f, %2.2f)\n",
    vno, v->cx, v->cy, v->cz EndOutputPrint;

  // hilite this vertex.
  HiliteSurfaceVertex ( v, kSurfaceType_Canonical );

  // set the cursor and go there.
  SetCursorToRASPt ( (Real)(v->cx), (Real)(v->cy), (Real)(v->cz) );
  RecenterViewToScreenPt ( jc, ic, imc );

  redraw() ;
}


void
goto_point_coords(int imc1, int ic1,int jc1)
{
  float xpt,ypt,zpt;

  xpt = xx1-ps*jc1/fsf;
  ypt = yy0+st*imc1/fsf;
  zpt = zz1-ps*(255.0-ic1/fsf);
  SetCursorToRASPt ( (Real)xpt, (Real)ypt, (Real)zpt );
}

void
write_point(char *dir)
{
  char fname[NAME_LENGTH];
  FILE *fp;
  Real theTalX, theTalY, theTalZ;
  int theVoxX, theVoxY, theVoxZ;
  Real theRASX, theRASY, theRASZ;
  VoxelRef theVoxel;

  // make a new voxel
  Voxel_New ( &theVoxel );

  if (control_points) {

    sprintf(fname,"%s/control.dat",dir);
    if (control_points > 0 || num_control_points++ > 0) {

      // kt -changed from "a" to "a+" to create file if it doesn't exist
      fp=fopen(fname,"a+");

    } else {

      OutputPrint "opening control point file %s...\n", fname EndOutputPrint;
      fp=fopen(fname,"w");
    }

  } else {

    sprintf(fname,"%s/edit.dat",dir);
    fp=fopen(fname,"w");
  }


  // if we're in control point mode, add this point to the control point
  // space.
  if ( IsInMode ( kMode_CtrlPt ) ) {

    NewCtrlPtFromCursor ();
  }

  if (fp==NULL) {
    printf("medit: ### can't create file %s\n",fname); PR 
    return;
  }

  // screen to ras
  ScreenToVoxel ( plane, jc, ic, imc, &theVoxX, &theVoxY, &theVoxZ );
  VoxelToRAS ( theVoxX, theVoxY, theVoxZ, &theRASX, &theRASY, &theRASZ );
  
  // write RAS space pt to file
  fprintf ( fp,"%f %f %f\n", theRASX, theRASY, theRASZ );
  DebugPrint "writing RAS point to %s...\n", fname EndDebugPrint;
  
  // if we have a tal transform for this volume...
  if ( transform_loaded && IsInMode ( kMode_Edit ) ) {
    
    // ras to tal
    transform_point ( linear_transform, theRASX, theRASY, theRASZ,
                      &theTalX, &theTalY, &theTalZ );
    
    
    // write tal space point to file
    fprintf(fp, "%f %f %f\n", theTalX, theTalY, theTalZ );
    DebugPrint "writing Tal point to %s...\n", fname EndDebugPrint;
    
    // these are global variables, used elsewhere.
    xtalairach = theTalX;
    ytalairach = theTalY;
    ztalairach = theTalZ; 
  }

  /*else { fprintf(stderr, "NOT writing transformed point to file...\n") ; }*/
  fclose(fp);

  // free the voxel
  Voxel_Delete ( &theVoxel );
}

void coords_to_talairach(void)
{
  float xpt,ypt,zpt;
  Real  x, y, z, x_tal, y_tal, z_tal ;

  xpt = xx1-ps*jc/fsf;
  ypt = yy0+st*imc/fsf;
  zpt = zz1-ps*(255.0-ic/fsf);
  x = (Real)xpt ; y = (Real)ypt ; z = (Real)zpt ;
  if (transform_loaded) {
    transform_point(linear_transform, x, y, z, &x_tal, &y_tal, &z_tal) ;
    xtalairach = x_tal;  ytalairach = y_tal;  ztalairach = z_tal; 
  }
}

void talairach_to_coords(void)
{
  int nimc,nic,njc;
  float xpt,ypt,zpt;
  double dzf;
  Real  x, y, z, x_tal, y_tal, z_tal ;

  if (transform_loaded) {
    x_tal = (Real)xtalairach; y_tal = (Real)ytalairach; z_tal =(Real)ztalairach;
    transform_point(inverse_linear_transform, x_tal, y_tal, z_tal, &x, &y, &z) ;
    xpt = (float)x; ypt = (float)y; zpt = (float)z;
    if (ptype==0) /* Horizontal */
    {
      jpt = (int)((xx1-xpt)*zf/ps+0.5);
      ipt = (int)((ypt-yy0)*zf/ps+0.5);
      impt = (int)((zpt-zz0)*zf/st+0.5);
    } else if (ptype==2) /* Coronal */
    {
      jpt = (int)((xx1-xpt)*zf/ps+0.5);
      ipt = (int)((255.0-(zz1-zpt)/ps)*zf+0.5);
      impt = (int)((ypt-yy0)*zf/st+0.5);
    } else if (ptype==1) /* Sagittal */
    {
      jpt = (int)((xx1-xpt)*zf/ps+0.5);
      ipt = (int)((ypt-yy0)*zf/ps+0.5);
      impt = (int)((zpt-zz0)*zf/st+0.5);
    }
    dzf = (double)zf;
    nimc = zf * (int)(rint((double)impt/dzf));  /* round to slice */
    nic =  zf * (int)(rint((double)ipt/dzf));
    njc =  zf * (int)(rint((double)jpt/dzf));
    if (njc/zf>=0 && njc/zf<xnum && nimc/zf>=0 && nimc/zf<=numimg &&
        (ydim-1-nic)/zf>=0 && (ydim-1-nic)/zf<ynum) {
      jc = njc;
      imc = nimc;
      ic = nic;
    }
    jpt=ipt=impt = -1;
  }
}

// this is the old set_cursor function. i'm keeping it around for reference.
// it has been replaced with SetCursorToRASPt().
#if 0
void set_cursor(float xpt, float ypt, float zpt)
{
  double dzf;
  Real   x, y, z, x_tal, y_tal, z_tal ;
  int    xi, yi, zi ;

  // we get xpt,ypt,zpt in ras space

  // ras to screen
  x = y = z = 0.0;
  if (ptype==0) /* Horizontal */
  {
    jpt = (int)((xx1-xpt)*zf/ps+0.5);
    ipt = (int)((ypt-yy0)*zf/ps+0.5);
    impt = (int)((zpt-zz0)*zf/st+0.5);
  } else if (ptype==2) /* Coronal */
  {
    jpt = (int)((xx1-xpt)*zf/ps+0.5);
    ipt = (int)((255.0-(zz1-zpt)/ps)*zf+0.5);
    impt = (int)((ypt-yy0)*zf/st+0.5);
  } else if (ptype==1) /* Sagittal */
  {
    jpt = (int)((xx1-xpt)*zf/ps+0.5);
    ipt = (int)((ypt-yy0)*zf/ps+0.5);
    impt = (int)((zpt-zz0)*zf/st+0.5);
  }

  dzf = (double)zf;
  imc = zf * (int)(rint((double)impt/dzf));  /* round to slice */
  ic =  zf * (int)(rint((double)ipt/dzf));
  jc =  zf * (int)(rint((double)jpt/dzf));
  /*jc = jpt;*/
  /*ic = ipt;*/
  /*imc = impt;*/
  jpt=ipt=impt = -1;  /* crosshair off */

  if (imc/zf>=0 && imc/zf<imnr1) 
  {

    // convert screen coordinates to voxel coordinates
    xi = (int)(jc/zf); 
    yi = (int)((ydim-1-ic)/zf); 
    zi = (int)(imc/zf) ;

    selectedpixval = im[zi][yi][xi];
    printf("%s=%d ",imtype,selectedpixval);
    if (second_im_allocated) 
    {
      selectedpixval = secondpixval = im2[zi][yi][xi];
      printf("(%s=%d ",imtype2, secondpixval);
    }
  }
  else
    xi = yi = zi = 0 ; /* ???? something should be done here */

  if (ptype==0) /* Horizontal */
  {
    printf(
    "imnr(P/A)=%d, i(I/S)=%df, j(R/L)=%d (x=%2.1f y=%2.1f z=%2.1f)\n",
       zi,yi,xi,xx1-ps*jc/fsf,yy0+ps*ic/fsf,zz0+st*imc/fsf);
    x = (Real)(xx1-ps*jc/fsf) ;
    y = (Real)(yy0+ps*ic/fsf) ;
    z = (Real)(zz0+st*imc/fsf) ;
  } else if (ptype==2) /* Coronal */
  {
    printf(
    "imnr(P/A)=%d, i(I/S)=%d, j(R/L)=%d (x=%2.1f y=%2.1f z=%2.1f)\n",
            zi,yi,xi,xx1-ps*jc/fsf,yy0+st*imc/fsf,zz1-ps*(255.0-ic/fsf));

    // screen to ras
    x = (Real)(xx1-ps*jc/fsf) ;
    y = (Real)(yy0+st*imc/fsf) ;
    z = (Real)(zz1-ps*(255.0-ic/fsf)) ;
  } else if (ptype==1) /* Sagittal */
  {
    printf(
    "imnr(P/A)=%d, i(I/S)=%d, j(R/L)=%d (x=%2.1f y=%2.1f z=%2.1f)\n",
         zi,yi,xi,xx1-ps*jc/fsf,yy0+ps*ic/fsf,zz0+st*imc/fsf);
    x = (Real)(xx1-ps*jc/fsf) ;
    y = (Real)(yy0+ps*ic/fsf) ;
    z = (Real)(zz0+st*imc/fsf) ;
  }
  if (transform_loaded)
  {
    transform_point(linear_transform, x, y, z, &x_tal, &y_tal, &z_tal) ;
    printf("TALAIRACH: (%2.1f, %2.1f, %2.1f)\n", x_tal, y_tal, z_tal) ;
    xtalairach = x_tal;  ytalairach = y_tal;  ztalairach = z_tal; 
  }

  /* twitzels stuff */
  if(statsloaded)
    printOutFunctionalCoordinate(x,y,z);
  updatepixval = TRUE;
  PR
}
#endif

void initmap_hacked(void)
{
  int i;
  for(i=0; i < 256; i++)
    hacked_map[i]=i;
}

void mapcolor_hacked(unsigned char idx, unsigned char v)
{
  hacked_map[idx]=v;
}
 
void set_scale(void)
{
  Colorindex i;
  float f;
  short v;

  for (i=0;i<NUMVALS;i++)
  {
/*
    f = i/fscale+fthresh;
    f = ((f<0.0)?0.0:((f>1.0)?1.0:f));
    f = pow(f,fsquash);
    v = f*fscale+0.5;
*/
    if (linearflag)
      v = i;
    else {
      f = i/fscale;
      f = 1.0/(1.0+exp(-fsquash*(f-fthresh)));
      v = f*fscale+0.5;
    }
    mapcolor_hacked(i+MAPOFFSET,v);
    mapcolor(i+MAPOFFSET,v,v,v);
  }
  mapcolor(NUMVALS+MAPOFFSET,v,0.0,0.0);
}

#define FIRST_TITLE "editable image: name of dir containing COR images"
#define SECOND_TITLE  "Second Buffer"


void redraw(void) {

  XTextProperty tp;
  static int lastplane = CORONAL;

  if (!openglwindowflag) {
    printf("medit: ### redraw failed: no gl window open\n");PR return; }

  if (changeplanelock && plane!=lastplane) plane = lastplane;
  lastplane = plane;

  set_scale();
  color(BLACK);
  /* clear(); */

 
  // draw the correct image. if we are drawing the second...
  if (drawsecondflag && second_im_allocated) {

    // kt - call draw_image with im2 instead of draw_second_image
    draw_image ( imc, ic, jc, im2 );

  } else {

    // else just one image.
    if (do_overlay)
      draw_image_hacked(imc,ic,jc);
    else
      draw_image ( imc, ic, jc, im ); 

  }

  // update the window title
  if ( second_im_allocated ) {
    
    // if we have two images, print both values.
    sprintf(title_str, "%s:%s (%s=%d, %s=%d)", pname, imtype2, imtype, 
            selectedpixval, imtype2, secondpixval) ;
  } else {

    // otherwise just the first.
    sprintf(title_str, "%s:%s (%s=%d)", pname, imtype,imtype,selectedpixval);
  }

  // print the string to the window title.
  wintitle(title_str);

  if (ptsflag && !all3flag) drawpts();

  if (surfflag && surfloaded) {

    // if we are not in all 3 mode..
    if ( !all3flag ) {
      
      // just draw our current surface.
      if ( gIsDisplayCurrentSurface )
        DrawSurface ( mris, plane, kSurfaceType_Current );
      if ( gIsDisplayOriginalSurface )
        DrawSurface ( mris, plane, kSurfaceType_Original );
      if ( gIsDisplayCanonicalSurface )
        DrawSurface ( mris, plane, kSurfaceType_Canonical );

    } else {

      // else draw all our surfaces.
      if ( gIsDisplayCurrentSurface ) {
        DrawSurface ( mris, CORONAL, kSurfaceType_Current );
        DrawSurface ( mris, SAGITTAL, kSurfaceType_Current );
        DrawSurface ( mris, HORIZONTAL, kSurfaceType_Current );
      }
      if ( gIsDisplayOriginalSurface ) {
        DrawSurface ( mris, CORONAL, kSurfaceType_Original );
        DrawSurface ( mris, SAGITTAL, kSurfaceType_Original );
        DrawSurface ( mris, HORIZONTAL, kSurfaceType_Original );
      }
      if ( gIsDisplayCanonicalSurface ) {
        DrawSurface ( mris, CORONAL, kSurfaceType_Canonical );
        DrawSurface ( mris, SAGITTAL, kSurfaceType_Canonical );
        DrawSurface ( mris, HORIZONTAL, kSurfaceType_Canonical );
      }

    }
  }

  if (surfflag && !surfloaded) {
    printf("medit: ### no surface read\n");PR
    surfflag=FALSE;
  }
  /*  swapbuffers(); */
}

void pop_gl_window(void)
{
  XRaiseWindow(xDisplay, w.wMain);
}

void resize_buffers(int x,int y)
{
  free(vidbuf);
  free(cvidbuf);
  free(binbuff);
  vidbuf = (GLubyte*)lcalloc((size_t)xdim*ydim*4,(size_t)sizeof(GLubyte));  

  /*  vidbuf = (Colorindex *)lcalloc((size_t)x*y,(size_t)sizeof(Colorindex));  */
  cvidbuf = (Colorindex *)lcalloc((size_t)CVIDBUF,(size_t)sizeof(Colorindex));
  binbuff = (unsigned char *)lcalloc((size_t)3*x*y,(size_t)sizeof(char));
}

void
mri2pix(float xpt, float ypt, float zpt, int *jpt, int *ipt,int *impt)
{
  if (ptype==0) /* Horizontal */
  {
    *jpt = (int)((xx1-xpt)/ps+0.5);
    *ipt = (int)((ypt-yy0)/ps+0.5);
    *impt = (int)((zpt-zz0)/st+0.5);
  } else if (ptype==2) /* Coronal */
  {
    *jpt = (int)((xx1-xpt)/ps+0.5);
    *ipt = (int)((255.0-(zz1-zpt)/ps)+0.5);
    *impt = (int)((ypt-yy0)/st+0.5);
  } else if (ptype==1) /* Sagittal */
  {
    *jpt = (int)((xx1-xpt)/ps+0.5);
    *ipt = (int)((ypt-yy0)/ps+0.5);
    *impt = (int)((zpt-zz0)/st+0.5);
  }
}

int 
imval(float px,float py,float pz)
{
  float x,y,z;
  int j,i,imn;

  x = px*tm[0][0]+py*tm[0][1]+pz*tm[0][2]+tm[0][3];
  y = px*tm[1][0]+py*tm[1][1]+pz*tm[1][2]+tm[1][3];
  z = px*tm[2][0]+py*tm[2][1]+pz*tm[2][2]+tm[2][3];
  mri2pix(x,y,z,&j,&i,&imn);
  if (imn>=0&&imn<numimg&&i>=0&&i<ynum&&j>=0&&j<xnum)
    return(im[imn][ynum-1-i][j]);
  else return 0;
}

float
Error(int p,float dp)
{
  int i,num;
  float mu,error,sum;
  float mu1,mu2,sum1,sum2;

  if (p>=0)
    par[p] += dp;
  mu = mu1 = mu2 = 0;
  num = 0;
  for (i=0;i<npts;i++)
  {
    mu += imval(ptx[i],pty[i],ptz[i]);
    mu1 += imval(ptx[i]*0.9,pty[i]*0.9,ptz[i]*0.9);
    mu2 += imval(ptx[i]*1.05,pty[i]*1.05,ptz[i]*1.05);
    num ++;
  }
  mu /= num;
  mu1 /= num;
  mu2 /= num;
  sum = sum1 = sum2 = 0;
  num = 0;
  for (i=0;i<npts;i++)
  {
    error = imval(ptx[i],pty[i],ptz[i])-mu;
    sum += error*error;
    error = imval(ptx[i]*0.9,pty[i]*0.9,ptz[i]*0.9)-mu1;
    sum1 += error*error;
    error = imval(ptx[i]*1.05,pty[i]*1.05,ptz[i]*1.05)-mu2;
    sum2 += error*error;
    num ++;
  }
  sum = sqrt((sum+sum2)/num);
  if (p>=0)
    par[p] -= dp;
  return sum;
}

void
optimize(int maxiter)
{
  float lambda = 0.03;
  float epsilon = 0.1;
  float momentum = 0.8;
  int iter,p;
  float dE[3];
  float error;

  for (iter=0;iter<maxiter;iter++)
  {
    error = Error(-1,0);
    printf("%d: %5.2f %5.2f %5.2f %7.3f\n",
           iter,par[0],par[1],par[2],error);PR
    for (p=0;p<3;p++)
    {
      dE[p] = tanh((Error(p,epsilon/2)-Error(p,-epsilon/2))/epsilon);
    }
    for (p=0;p<3;p++)
    {
      par[p] += (dpar[p] = momentum*dpar[p] - lambda*dE[p]);
    }
  }
  error = Error(-1,0);
  printf("%d: %5.2f %5.2f %5.2f %7.3f\n",
         iter,par[0],par[1],par[2],error);PR
}
void
optimize2(void)
{
  float epsilon = 0.5;
  float p0,p1,p2,p0min,p1min,p2min;
  float error,minerror;

  p0min = p1min = p2min = 0.0f;
  error = Error(-1,0);
  minerror = error;
  printf("%5.2f %5.2f %5.2f %7.3f\n",
         par[0],par[1],par[2],error);PR
  for (p0 = -10;p0 <= 10;p0 += epsilon)
  for (p1 = -10;p1 <= 10;p1 += epsilon)
  for (p2 = -10;p2 <= 10;p2 += epsilon)
  {
    par[0] = p0;
    par[1] = p1;
    par[2] = p2;
    error = Error(-1,0);
    if (error<minerror)
    {
      printf("%5.2f %5.2f %5.2f %7.3f\n",
             par[0],par[1],par[2],error);PR
      minerror = error;
      p0min = p0;
      p1min = p1;
      p2min = p2;
    }
  }
  par[0] = p0min;
  par[1] = p1min;
  par[2] = p2min;
  error = Error(-1,0);
  printf("%5.2f %5.2f %5.2f %7.3f\n",
         par[0],par[1],par[2],error);PR
}

void
translate_brain(float a,char c)
{
  int i,j,k;
  float m1[4][4],m2[4][4];

  for (i=0;i<4;i++)
  for (j=0;j<4;j++)
    m1[i][j] = (i==j)?1.0:0.0;
  if (c=='y')
    m1[1][3] = a;
  else if (c=='x')
    m1[0][3] = a;
  else if (c=='z')
    m1[2][3] = a;
  else printf("Illegal axis %c\n",c);
  for (i=0;i<4;i++)
  for (j=0;j<4;j++)
  {
    m2[i][j] = 0;
    for (k=0;k<4;k++)
      m2[i][j] += tm[i][k]*m1[k][j];
  }
  for (i=0;i<4;i++)
  for (j=0;j<4;j++)
    tm[i][j] = m2[i][j];
}

void
rotate_brain(float a,char c)
{
  int i,j,k;
  float m1[4][4],m2[4][4];
  float sa,ca;

  for (i=0;i<4;i++)
  for (j=0;j<4;j++)
    m1[i][j] = (i==j)?1.0:0.0;
  a = a*M_PI/1800;
  sa = sin(a);
  ca = cos(a);
  if (c=='y')
  {
    m1[0][0] = m1[2][2] = ca;
    m1[2][0] = -(m1[0][2] = sa);
  } else
  if (c=='x')
  {
    m1[1][1] = m1[2][2] = ca;
    m1[1][2] = -(m1[2][1] = sa);
  } else
  if (c=='z')
  {
    m1[0][0] = m1[1][1] = ca;
    m1[0][1] = -(m1[1][0] = sa);
  } else printf("Illegal axis %c\n",c);
  for (i=0;i<4;i++)
  for (j=0;j<4;j++)
  {
    m2[i][j] = 0;
    for (k=0;k<4;k++)
      m2[i][j] += tm[i][k]*m1[k][j];
  }
  for (i=0;i<4;i++)
  for (j=0;j<4;j++)
    tm[i][j] = m2[i][j];
}

int
read_images(char *fpref)
{
  int i,j,k;
  FILE *fptr, *xfptr;
  char fname[STRLEN], char_buf[STRLEN];

  sprintf(fname,"%s.info",fpref);
  fptr = fopen(fname,"r");
  if (fptr==NULL) {
    strcpy(fpref,"COR-");
    sprintf(fname,"%s.info",fpref);
    fptr = fopen(fname,"r");
    if (fptr==NULL) {
      printf("medit: ### File %s not found (tried local dir, too)\n",fname);
      exit(1);
    }
  } 

  fscanf(fptr,"%*s %d",&imnr0);
  fscanf(fptr,"%*s %d",&imnr1);
  fscanf(fptr,"%*s %d",&ptype);
  fscanf(fptr,"%*s %d",&xnum);
  fscanf(fptr,"%*s %d",&ynum);
  fscanf(fptr,"%*s %*f");
  fscanf(fptr,"%*s %f",&ps);
  fscanf(fptr,"%*s %f",&st);
  fscanf(fptr,"%*s %*f");
  fscanf(fptr,"%*s %f",&xx0); /* strtx */
  fscanf(fptr,"%*s %f",&xx1); /* endx */
  fscanf(fptr,"%*s %f",&yy0); /* strty */
  fscanf(fptr,"%*s %f",&yy1); /* endy */
  fscanf(fptr,"%*s %f",&zz0); /* strtz */
  fscanf(fptr,"%*s %f",&zz1); /* endz */
  fscanf(fptr, "%*s %*f") ;   /* tr */
  fscanf(fptr, "%*s %*f") ;   /* te */
  fscanf(fptr, "%*s %*f") ;   /* ti */
  fscanf(fptr, "%*s %s",char_buf);

  /* marty: ignore abs paths in COR-.info */
  xfptr = fopen(xffname,"r");
  if (xfptr==NULL)
    printf("medit: Talairach xform file not found (ignored)\n");
  else {
    fclose(xfptr);
    if (input_transform_file(xffname, &talairach_transform) != OK)
      printf("medit: ### File %s load failed\n",xffname);
    else {
      printf("medit: Talairach transform file loaded\n");
      printf("medit: %s\n",xffname);
      linear_transform = get_linear_transform_ptr(&talairach_transform) ;
      inverse_linear_transform =
                      get_inverse_linear_transform_ptr(&talairach_transform) ;
      transform_loaded = TRUE;
    }
  }

  ps *= 1000;
  st *= 1000;
  xx0 *= 1000;
  xx1 *= 1000;
  yy0 *= 1000;
  yy1 *= 1000;
  zz0 *= 1000;
  zz1 *= 1000;
  fclose(fptr);
  numimg = imnr1-imnr0+1;
  xdim=xnum*zf;
  ydim=ynum*zf;

  bufsize = ((unsigned long)xnum)*ynum;
  buf = (unsigned char *)lcalloc(bufsize,sizeof(char));
  for (k=0;k<numimg;k++)
  {
    im[k] = (unsigned char **)lcalloc(ynum,sizeof(char *));
    for (i=0;i<ynum;i++)
    {
      im[k][i] = (unsigned char *)lcalloc(xnum,sizeof(char));
    }
  }
  for (k=0;k<6;k++)
  {
    sim[k] = (unsigned char **)lcalloc(ynum,sizeof(char *));
    for (i=0;i<ynum;i++)
    {
      sim[k][i] = (unsigned char *)lcalloc(xnum,sizeof(char));
    }
  }

  for (k=0;k<numimg;k++)
  {
    changed[k] = FALSE;
    file_name(fpref,fname,k+imnr0,"%03d");
    fptr = fopen(fname,"rb");
    if(fptr==NULL){printf("medit: ### File %s not found.\n",fname);exit(1);}
    fread(buf,sizeof(char),bufsize,fptr);
    buffer_to_image(buf,im[k],xnum,ynum);
    fclose(fptr);
  }

  for (k=0;k<numimg;k++)
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
  {
    if (im[k][i][j]/2>sim[3][i][j]) sim[3][i][j] = im[k][i][j]/2;
    if (im[k][i][j]/2>sim[4][k][j]) sim[4][k][j] = im[k][i][j]/2;
    if (im[k][i][j]/2>sim[5][i][k]) sim[5][i][k] = im[k][i][j]/2;
  }
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
  for (k=0;k<3;k++)
    sim[k][i][j] = sim[k+3][i][j];
  return(0);
}


int
read_second_images(char *imdir2)
{
  int i,j,k,n;
  FILE *fptr;
  char fname[NAME_LENGTH], cmd[NAME_LENGTH], fpref[NAME_LENGTH];
  char fnamefirst[NAME_LENGTH], notilde[NAME_LENGTH];

  strcpy(imtype2, imdir2) ;


  /* TODO: replace w/setfile (add mri/<subdir>); tk.c: omit arg like others */
  if (imdir2[0] == '/')
    sprintf(fpref,"%s/COR-",imdir2);
  else if (imdir2[0] == '~') {
    for(n=1;n<=strlen(imdir2);n++)  notilde[n-1] = imdir2[n];
    sprintf(fpref,"%s/%s/%s/COR-",subjectsdir,pname,notilde);
  }
  else if (MATCH(pname,"local"))
    sprintf(fpref,"%s/%s/COR-",srname,imdir2);
  else
    sprintf(fpref,"%s/%s/mri/%s/COR-",subjectsdir,pname,imdir2);

  sprintf(fnamefirst,"%s.info",mfname);
  sprintf(fname,"%s.info",fpref);
  sprintf(cmd,"diff %s %s",fnamefirst,fname);
  if (system(cmd)!=0)
    printf("medit: ### second COR-.info file doesn't match first--ignored\n");

  if (!second_im_allocated)
    alloc_second_im();
 
  for (k=0;k<numimg;k++) {
    file_name(fpref,fname,k+imnr0,"%03d");
    fptr = fopen(fname,"rb");
    if(fptr==NULL){printf("medit: ### File %s not found.\n",fname);return(0);}
    fread(buf,sizeof(char),bufsize,fptr);
    buffer_to_image(buf,im2[k],xnum,ynum);
    fclose(fptr);
  }

  for (k=0;k<numimg;k++)
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++) {
    if (im2[k][i][j]/2>sim2[3][i][j]) sim2[3][i][j] = im2[k][i][j]/2;
    if (im2[k][i][j]/2>sim2[4][k][j]) sim2[4][k][j] = im2[k][i][j]/2;
    if (im2[k][i][j]/2>sim2[5][i][k]) sim2[5][i][k] = im2[k][i][j]/2;
  }
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
  for (k=0;k<3;k++)
    sim2[k][i][j] = sim2[k+3][i][j];

  drawsecondflag = TRUE;
  second_im_full = TRUE;
  redraw();
  return(0);
}

int
write_images(char *fpref)
{
  int k;
  FILE *fptr;
  char fname[STRLEN];

  if (!editflag) { 
    printf(
      "medit: ### can't write images: (set editflag TRUE)\n");return(0);PR }
  for (k=0;k<numimg;k++) {
    if (changed[k]) {
      changed[k] = FALSE;
      file_name(fpref,fname,k+imnr0,"%03d");
      fptr = fopen(fname,"wb");
      image_to_buffer(im[k],buf,xnum,ynum);
      fwrite(buf,sizeof(char),bufsize,fptr);
      fclose(fptr);
      printf("medit: file %s written\n",fname);PR
    }
  }
  editedimage = FALSE;
  return(0);
}
int
dump_vertex(int vno)
{
  VERTEX *v, *vn ;
  int    n ;
  float  dx, dy, dz, dist ;

  if (!mris)
    ErrorReturn(ERROR_BADPARM, 
                (ERROR_BADPARM, "%s: surface must be loaded\n", Progname)) ;

  v = &mris->vertices[vno] ;
  fprintf(stderr, 
          "v %d: x = (%2.2f, %2.2f, %2.2f), n = (%2.2f, %2.2f, %2.2f)\n",
          vno, v->x, v->y, v->z, v->nx, v->ny, v->nz) ;
  if (curvloaded)
    fprintf(stderr, "curv=%2.2f\n", v->curv) ;
  fprintf(stderr, "nbrs:\n") ;
  for (n = 0 ; n < v->vnum ; n++)
  {
    vn = &mris->vertices[v->v[n]] ;
    dx = vn->x - v->x ; dy = vn->y - v->y ; dz = vn->z - v->z ;
    dist = sqrt(dx*dx + dy*dy + dz*dz) ;
    fprintf(stderr, "\tvn %d, dist = %2.2f\n", v->v[n], dist) ;
  }
  return(NO_ERROR) ;
}

int
show_vertex ( void ) {

  VERTEX   *v ;
  int      vno, min_vno ;
  float    dx, dy, dz, dist = 0.0, min_dist ;

  if (!mris)
    ErrorReturn ( ERROR_BADPARM, 
                  (ERROR_BADPARM, "%s: surface must be loaded\n", Progname)) ;

  DebugPrint "show_vertex(): looking from RAS (%2.5f, %2.5f, %2.5f)\n",
    x_click, y_click, z_click EndDebugPrint;

  min_vno = -1 ; min_dist = 1e9 ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    dx = v->x - x_click ; dy = v->y - y_click ; dz = v->z - z_click ; 
    dist = dx*dx + dy*dy + dz*dz ;
    if (dist < min_dist)
    {
      min_dist = dist ;
      min_vno = vno ;
    }
  }
  v = &mris->vertices[min_vno] ;
  fprintf(stderr, "vno %d @ (%2.1f, %2.1f, %2.1f), %2.1f mm away\n",
          min_vno, v->x, v->y, v->z, sqrt(min_dist)) ;

  // hilite this vertex.
  HiliteSurfaceVertex ( v, kSurfaceType_Current );

  // set the cursor and go there.
  // set_cursor ( v->x, v->y, v->z );
  // RecenterViewToScreenPt ( jc, ic, imc );

  // kt - redraw hilighted vertex.
  redraw ();

  return(NO_ERROR) ;
}

int
show_orig_vertex ( void ) {

  VERTEX   *v ;
  int      vno, min_vno ;
  float    dx, dy, dz, dist = 0.0, min_dist ;

  if (!mris)
    ErrorReturn ( ERROR_BADPARM, 
                  (ERROR_BADPARM, "%s: surface must be loaded\n", Progname)) ;

  DebugPrint "show_orig_vertex(): looking from RAS (%2.5f, %2.5f, %2.5f)\n",
    x_click, y_click, z_click EndDebugPrint;

  min_vno = -1 ; min_dist = 1e9 ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    dx = v->origx - x_click ; 
    dy = v->origy - y_click ;
    dz = v->origz - z_click ; 
    dist = dx*dx + dy*dy + dz*dz ;
    if (dist < min_dist)
    {
      min_dist = dist ;
      min_vno = vno ;
    }
  }

  v = &mris->vertices[min_vno] ;

  // hilite this vertex.
  HiliteSurfaceVertex ( v, kSurfaceType_Original );

  // set the cursor and go there.
  // set_cursor ( v->x, v->y, v->z );
  // RecenterViewToScreenPt ( jc, ic, imc );

  fprintf(stderr, "vno %d @ (%2.1f, %2.1f, %2.1f), %2.1f mm away\n",
          min_vno, v->origx, v->origy, v->origz, sqrt(min_dist)) ;

   // kt - redraw hilighted vertex.
  redraw ();

 return(NO_ERROR) ;
}

int
show_canon_vertex ( void ) {

  VERTEX   *v ;
  int      vno, min_vno ;
  float    dx, dy, dz, dist = 0.0, min_dist ;

  if (!mris)
    ErrorReturn ( ERROR_BADPARM, 
                  (ERROR_BADPARM, "%s: surface must be loaded\n", Progname)) ;

  DebugPrint "show_canon_vertex(): looking from RAS (%2.5f, %2.5f, %2.5f)\n",
    x_click, y_click, z_click EndDebugPrint;

  min_vno = -1 ; min_dist = 1e9 ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    dx = v->cx - x_click ; 
    dy = v->cy - y_click ;
    dz = v->cz - z_click ; 
    dist = dx*dx + dy*dy + dz*dz ;
    if (dist < min_dist)
    {
      min_dist = dist ;
      min_vno = vno ;
    }
  }
  v = &mris->vertices[min_vno] ;

  // hilite this vertex.
  HiliteSurfaceVertex ( v, kSurfaceType_Canonical );

  // set the cursor and go there.
  // set_cursor ( v->x, v->y, v->z );
  //  RecenterViewToScreenPt ( jc, ic, imc );

  fprintf(stderr, "vno %d @ (%2.1f, %2.1f, %2.1f), %2.1f mm away\n",
          min_vno, v->cx, v->cy, v->cz, sqrt(min_dist)) ;

   // kt - redraw hilighted vertex.
  redraw ();

  return(NO_ERROR) ;
}

void HandleMouseUp ( XButtonEvent inEvent ) {

  int x, y, z;
  int theClickedX, theClickedY, theClickedZ;
  VoxelRef theScreenVoxel, theAnatomicalVoxel;

  Voxel_New ( &theScreenVoxel );
  Voxel_New ( &theAnatomicalVoxel );

                                   /* first set our click points to the
              current cursor pts. this is  done
              because our ClickToScreen func leaves
              the coord that represents that z coords
              on the screen untouched, which won't be
              chaned by clicking in the x/y plane. */
  // get the clicked point in screen coords.
  theClickedX = jc;
  theClickedY = ic;
  theClickedZ = imc;

  // covert the click to the screen pt.
  ClickToScreen ( inEvent.x, inEvent.y, plane, 
      &theClickedX, &theClickedY, &theClickedZ );
  Voxel_Set ( theScreenVoxel, theClickedX, theClickedY, theClickedZ );

  // check bounds. if no good, print an error message and bail.
  if ( !IsScreenPointInBounds ( plane, EXPAND_VOXEL_INT(theScreenVoxel) ) ) {
    OutputPrint kMsg_InvalidClick EndOutputPrint;
    Voxel_Delete ( &theScreenVoxel );
    Voxel_Delete ( &theAnatomicalVoxel );  
    return;
  }

  // convert to voxel.
  ScreenToVoxel ( plane, EXPAND_VOXEL_INT(theScreenVoxel), &x, &y, &z );
  Voxel_Set ( theAnatomicalVoxel, x, y, z );

                                   /* on mouse up, always set the cursor to
              the clicked point unless we are in ctrl
              pt mode and used button 2, in which case
              we pass it to the ctrl pt selection
              function. */
  if ( 2 == inEvent.button && IsInMode ( kMode_CtrlPt ) ) {
    
    SelectCtrlPt ( EXPAND_VOXEL_INT(theScreenVoxel), plane, inEvent.state );
    
  } else {
    
    SetCursorToScreenPt ( EXPAND_VOXEL_INT(theScreenVoxel) );
    PrintScreenPointInformation ( EXPAND_VOXEL_INT(theScreenVoxel) );
    
    // when setting cursor, allow functional module to respond to click.
    if ( FuncDis_IsDataLoaded () ) 
      FuncDis_HandleClickedVoxel ( theAnatomicalVoxel );
  }

                                   /* if control key was held down, check for
              zooming. ctrl-button1 is zoom in,
              ctrl-button2 is zoom out. when zooming, 
              we center on the point first and then
              zoom in or out. */
  // if ctrl was down...
  if ( inEvent.state & ControlMask ) {
  
    // always zoom in on button 1.
    if ( 1 == inEvent.button ) {
      RecenterViewToScreenPt ( EXPAND_VOXEL_INT(theScreenVoxel) );
      ZoomViewIn ();
    }      
 
    // always zoom out on button 3.
    if ( 3 == inEvent.button ) {
      RecenterViewToScreenPt ( EXPAND_VOXEL_INT(theScreenVoxel) );
      ZoomViewOut ();
    }     
  }

  Voxel_Delete ( &theScreenVoxel );
  Voxel_Delete ( &theAnatomicalVoxel );  
}

void HandleMouseDown ( XButtonEvent inEvent ) {

  int x, y, z;
  int theClickedX, theClickedY, theClickedZ;
  VoxelRef theScreenVoxel, theAnatomicalVoxel;
  int theEditAction;

  // nothing happens on button 1.
  if ( 1 == inEvent.button )
    return;

  // if control key and button 1 or 3 is down, they really want to
  // zoom, and we will handle that on mouse up, not here.
  if ( (inEvent.state & ControlMask) &&
       (1 == inEvent.button || 3 == inEvent.button) )
    return;

  Voxel_New ( &theScreenVoxel );
  Voxel_New ( &theAnatomicalVoxel );

  // get the clicked point in screen coords.
  theClickedX = jc;
  theClickedY = ic;
  theClickedZ = imc;
  ClickToScreen ( inEvent.x, inEvent.y, plane, 
      &theClickedX, &theClickedY, &theClickedZ );
  Voxel_Set ( theScreenVoxel, theClickedX, theClickedY, theClickedZ );

  // check bounds. if no good, print an error message and bail.
  if ( !IsScreenPointInBounds ( plane, EXPAND_VOXEL_INT(theScreenVoxel) ) ) {
    OutputPrint kMsg_InvalidClick EndOutputPrint;
    Voxel_Delete ( &theScreenVoxel );
    Voxel_Delete ( &theAnatomicalVoxel );  
    return;
  }

  // convert to voxel.
  ScreenToVoxel ( plane, EXPAND_VOXEL_INT(theScreenVoxel), &x, &y, &z );
  Voxel_Set ( theAnatomicalVoxel, x, y, z );

                                   /* on mouse down, we only do something if
              we're beginning to paint. this happens
              when we're in select mode or edit mode.
              we just pass the point to the
              approriate paint function. additionally,
              if we're in editing mode and pressing
              button2 or 3, this is a good time to
              clear the undo list since we're
              starting a new edit. */
  // what mode?
  switch ( GetMode() ) {

    // editing...
  case kMode_Edit: 
    
    // clear the undo list.
    ClearUndoList ();
    
    // if button 2, paint voxels to white.
    if ( 2 == inEvent.button ) {
      theEditAction = TO_WHITE;
    } else {
      // if button 3, paint voxels to black.
      theEditAction = TO_BLACK;
    }
    
    PaintVoxels ( EditVoxel, &theEditAction, theAnatomicalVoxel );
    break;

  // selecting...
  case kMode_Select:

    // if button 2, select voxels.
    if ( 2 == inEvent.button ) {
      PaintVoxels ( AddVoxelToSelection, NULL, theAnatomicalVoxel );
    } else {
      // if button 3, unselect voxels.
      PaintVoxels ( RemoveVoxelFromSelection, NULL, theAnatomicalVoxel );
    }
    break;

  case kMode_CtrlPt:
    break;
  }
    
}

void HandleMouseMoved ( XMotionEvent inEvent ) {

  int x, y, z;
  int theClickedX, theClickedY, theClickedZ;
  VoxelRef theScreenVoxel, theAnatomicalVoxel;
  int theEditAction;

  // nothing happens on button 1.
  if ( inEvent.state & Button1Mask )
    return;

  // if control key and button 1 or 3 is down, they really want to
  // zoom, and we will handle that on mouse up, not here.
  if ( ( inEvent.state & ControlMask ) &&
       ( inEvent.state & Button1Mask || inEvent.state & Button3Mask ) )
    return;

  Voxel_New ( &theScreenVoxel );
  Voxel_New ( &theAnatomicalVoxel );

  // get the clicked point in screen coords.
  theClickedX = jc;
  theClickedY = ic;
  theClickedZ = imc;
  ClickToScreen ( inEvent.x, inEvent.y, plane, 
      &theClickedX, &theClickedY, &theClickedZ );
  Voxel_Set ( theScreenVoxel, theClickedX, theClickedY, theClickedZ );

  // check bounds. if no good, print an error message and bail.
  if ( !IsScreenPointInBounds ( plane, EXPAND_VOXEL_INT(theScreenVoxel) ) ) {
    OutputPrint kMsg_InvalidClick EndOutputPrint;
    Voxel_Delete ( &theScreenVoxel );
    Voxel_Delete ( &theAnatomicalVoxel );  
    return;
  }

  // convert to voxel.
  ScreenToVoxel ( plane, EXPAND_VOXEL_INT(theScreenVoxel), &x, &y, &z );
  Voxel_Set ( theAnatomicalVoxel, x, y, z );

                                   /* on mouse moved, we only do something if
              we're beginning to paint. this happens
              when we're in select mode or edit mode.
              we just pass the point to the
              approriate paint function. */
  // what mode?
  switch ( GetMode() ) {

    // editing...
  case kMode_Edit: 
    
    // if button 2, paint voxels to white.
    if ( inEvent.state & Button2Mask ) {
      theEditAction = TO_WHITE;
    } else {
      // if button 3, paint voxels to black.
      theEditAction = TO_BLACK;
    }
    
    PaintVoxels ( EditVoxel, &theEditAction, theAnatomicalVoxel );
    break;

  // selecting...
  case kMode_Select:

    // if button 2, select voxels.
    if ( inEvent.state & Button2Mask ) {
      PaintVoxels ( AddVoxelToSelection, NULL, theAnatomicalVoxel );
    } else {
      // if button 3, unselect voxels.
      PaintVoxels ( RemoveVoxelFromSelection, NULL, theAnatomicalVoxel );
    }
    break;

  case kMode_CtrlPt:
    break;
  }
}   


#if 0
void ProcessClick ( int inClickX, int inClickY ) {

  int theClickedJ, theClickedI, theClickedIM;
  int theVoxelX, theVoxelY, theVoxelZ;
  VoxelRef theVoxel;

  // first set our click points to the current cursor pts. this is 
  // done because our ClickToScreen func leaves the
  // coord that represents that z coords on the screen untouched, which won't
  // be chaned by clicking in the x/y plane.
  theClickedJ = jc;
  theClickedI = ic;
  theClickedIM = imc;

  // covert the click to the screen pt.
  ClickToScreen ( inClickX, inClickY, plane,
                  &theClickedJ, &theClickedI, &theClickedIM );

  // see if they are in bounds.
  if ( !IsScreenPointInBounds ( plane, 
        theClickedJ, theClickedI, theClickedIM ) ) {

    OutputPrint kMsg_InvalidClick EndOutputPrint;
    return;
  }

  // convert to voxel coords.
  ScreenToVoxel ( plane, theClickedJ, theClickedI, theClickedIM,
                  &theVoxelX, &theVoxelY, &theVoxelZ );

  // now look at what button is pressed.
  if ( button1pressed ) {

    // set the cursor to the clicked point and print
    // out a bunch of information on it.
    SetCursorToScreenPt ( theClickedJ, theClickedI, theClickedIM );
    
    printf ( "\nSelected voxel information:\n" );
    PrintScreenPointInformation ( jc, ic, imc );

    // if ctrl is down and we're not in all3 view...
    if ( ctrlkeypressed && !all3flag ) {
      
      // recenter and zoom in.
      RecenterViewToScreenPt ( theClickedJ, theClickedI, theClickedIM );
      ZoomViewIn ();

      // print the new config.
      PrintZoomInformation ();
    }
      
  } else if ( button2pressed ) {

    // if selecting..
    if ( IsInMode ( kMode_CtrlPt ) ) {

      // select a ctrl point.
      SelectCtrlPt ( theClickedJ, theClickedI, theClickedIM, plane );

      // if in editing mode..
    } else {

      // select the pixel and color it white.
      SetCursorToScreenPt ( theClickedJ, theClickedI, theClickedIM );
      edit_pixel ( TO_WHITE );
    }

  } else if ( button3pressed ) {

    // set the cursor to the clicked voxel
    SetCursorToScreenPt ( theClickedJ, theClickedI, theClickedIM );

    // if ctrl is down and we're not in all3 mode..
    if ( ctrlkeypressed && !all3flag ) {
      
      // zoom out
       RecenterViewToScreenPt ( theClickedJ, theClickedI, theClickedIM );
     ZoomViewOut ();

      // print info about the new configurtion.
      PrintZoomInformation ();
      
      // else, if we're in editing mode..
    } else if ( IsInMode ( kMode_Edit ) ) {

      // color it black.
      edit_pixel ( TO_BLACK );
    }
  }

  // allow the functional overlay module to respond to this click.
  if ( IsFunctionalDataLoaded () ) {

    Voxel_New ( &theVoxel );
    Voxel_Set ( theVoxel, theVoxelX, theVoxelY, theVoxelZ );
    AllowFunctionalModuleToRespondToClick ( theVoxel );
    Voxel_Delete ( &theVoxel );
  }

  // because there were weird dragging effects if you jerked the 
  // mouse right after clicking. seems the event loop misses a lot 
  // of key and button releases, so we force them here.
  if ( IsInMode ( kMode_CtrlPt ) ) {

    button1pressed = FALSE;
    button2pressed = FALSE;
    button3pressed = FALSE;
  }
  
  // not sure whether or not to do these...
  /*
  ctrlkeypressed = FALSE;
  altkeypressed = FALSE;
  shiftkeypressed = FALSE;
  */


  // twitzel's stuff, i don't know what it does.
  /*
  hot=1;
  lookupInVoxelSpace(x,y,z,0);
  setupCoronal(imc);
  hot=0;*/
  updatepixval = TRUE;

}
#endif

void HideCursor () {

  gIsCursorVisible = FALSE;
  redraw ();
}

void ShowCursor () {

  gIsCursorVisible = TRUE;
  redraw ();
}

char IsCursorVisible () {
  
  return gIsCursorVisible;
}


void SetCursorToScreenPt ( int inScreenX, int inScreenY, int inScreenZ ) {
  
  Real theRASX, theRASY, theRASZ;

  // do some bounds checking
  if ( ! IsScreenPointInBounds ( plane, inScreenX, inScreenY, inScreenZ ) ) {

    DebugPrint "!!! SetCursorToScreenPt(): Screen pt out of bounds: (%d, %d, %d)\n", inScreenX, inScreenY, inScreenZ    EndDebugPrint;
    return;
  }
    
  // copy screen points into global screen variables
  jc = inScreenX;
  ic = inScreenY;
  imc = inScreenZ;

  // we have to set these variables because they are used in other functions
  // show_vertex in particular. and they want ras coords too.
  ScreenToRAS ( plane, inScreenX, inScreenY, inScreenZ,
    &theRASX, &theRASY, &theRASZ );

  x_click = (float) theRASX;
  y_click = (float) theRASY;
  z_click = (float) theRASZ;

  // tell the tk window to update its coords
  SendUpdateMessageToTKWindow ();
}

void SetCursorToRASPt ( Real inX, Real inY, Real inZ ) {

  int theScreenX, theScreenY, theScreenZ;

  // get a screen pt from the ras pt.
  RASToScreen ( inX, inY, inZ, plane, 
    &theScreenX, &theScreenY, &theScreenZ );

  // set the screen point.
  SetCursorToScreenPt ( theScreenX, theScreenY, theScreenZ );
}

void SaveCursorLocation () {

  // save screen cursor coords to RAS coords.
  ScreenToRAS ( plane, jc, ic, imc, 
    &gSavedCursorVoxelX, &gSavedCursorVoxelY, &gSavedCursorVoxelZ );
  
}

void RestoreCursorLocation () {
  
  // restore screen cursor from saved RAS coords.
  RASToScreen ( gSavedCursorVoxelX, gSavedCursorVoxelY, gSavedCursorVoxelZ,
    plane, &jc, &ic, &imc );

}

void SetPlane ( int inNewPlane ) {

  // don't change if we don't have to.
  if ( inNewPlane == plane ) {
    return;
  }

  // save the cursor location.
  SaveCursorLocation ();
  
  // set the plane value.
  plane = inNewPlane;

  // restore the cursor.
  RestoreCursorLocation ();
}

void RecenterViewToScreenPt ( int inScreenX, int inScreenY, int inScreenZ ) {
  
  int theNewCenterX, theNewCenterY, theNewCenterZ;
  //  Real theTestCursorX, theTestCursorY, theTestCursorZ;

  // first get the cursor in RAS form to remember it
  SaveCursorLocation ();
  
  // get the voxel to center around.
  ScreenToVoxel ( plane, inScreenX, inScreenY, inScreenZ,
                  &theNewCenterX, &theNewCenterY, &theNewCenterZ );
  
  // now center us where they had clicked before.
  SetCenterVoxel ( theNewCenterX, theNewCenterY, theNewCenterZ );
  
  // check our center now...
  CheckCenterVoxel ();
  
  // reset the cursor in the new screen position
  RestoreCursorLocation ();

  /*
  DebugCode {
    
    ScreenToRAS ( plane, jc, ic, imc,
      &theTestCursorX, &theTestCursorY, &theTestCursorZ );
    
    if ( theTestCursorX != theSavedCursorX ||
         theTestCursorY != theSavedCursorY ||
         theTestCursorZ != theSavedCursorZ )
      DebugPrint "Cursor voxels don't match: was (%2.1f, %2.1f, %2.1f), now (%2.1f, %2.1f, %2.1f)\n", 
        theSavedCursorX, theSavedCursorY, theSavedCursorZ,
        theTestCursorX, theTestCursorY, theTestCursorZ
        EndDebugPrint;
    
  } EndDebugCode;
  */
}

void RecenterViewToCursor () {

  RecenterViewToScreenPt ( jc, ic, imc );
}


void ZoomViewIn () {

  // don't zoom in all3 mode.
  if ( all3flag ) return;

  // remember the cursor
  SaveCursorLocation ();

  // double our local zoom factor and check its bounds.
  gLocalZoom *= 2;
  if ( gLocalZoom > kMaxLocalZoom )
    gLocalZoom = kMaxLocalZoom;

  // reset the cursor in the new screen position
  RestoreCursorLocation ();
}

void ZoomViewOut () {

  // don't zoom in all3 mode.
  if ( all3flag ) return;

  // remember the cursor
  SaveCursorLocation ();

  // half our local zoom factor and check its bounds.
  gLocalZoom /= 2;
  if ( gLocalZoom < kMinLocalZoom )
    gLocalZoom = kMinLocalZoom;

  // if we are at min zoom..
  if ( kMinLocalZoom == gLocalZoom ) {

    // set the center to the middle of the voxel space.
    SetCenterVoxel ( xnum/2, ynum/2, xnum/2 );
  }

  // check the center, because it may be invalid now that we zoomed out.
  CheckCenterVoxel ();

  // reset the cursor in the new screen position
  RestoreCursorLocation ();
}

void UnzoomView () {

  int theSavedCursorX, theSavedCursorY, theSavedCursorZ;

  // first get the cursor in voxel form to remember it
  ScreenToVoxel ( plane, jc, ic, imc,
                  &theSavedCursorX, &theSavedCursorY, &theSavedCursorZ );

  // set zoom to the minimum..
  gLocalZoom = kMinLocalZoom;

  // set the center to the middle of the voxel space.
  SetCenterVoxel ( xnum/2, ynum/2, xnum/2 );

  // reset the cursor in the new screen position
  VoxelToScreen ( theSavedCursorX, theSavedCursorY, theSavedCursorZ,
                  plane, &jc, &ic, &imc );
}
  

void draw_image( int imc, int ic, int jc,
                 unsigned char *** inImageSpace )
{
  int i,j,k;
  int theDestIndex;
  int hax,hay,curs;
  int theVoxX, theVoxY, theVoxZ;
  int theScreenX, theScreenY, theScreenZ;
  VoxelRef theVoxel;
  int theVoxelSize;
  GLubyte v;

  hax = xdim/2;
  hay = ydim/2;

  if (all3flag) curs = 15;
  else          curs = 2;

  // create a new voxel
  Voxel_New ( &theVoxel );

  
  // calc voxel size, or the number of pixels that represent a single voxel.
  if ( !all3flag ) {
    theVoxelSize = zf * gLocalZoom;
  } else {
    theVoxelSize = zf/2;
  }

  if ( maxflag && !all3flag ) {

    k = 0;
    switch ( plane ) {

    case CORONAL:
      for ( i = ydim-1; i >= 0; i-- )   // i from bottom to top
        for ( j = 0; j < xdim; j++ )    // j from left to right
          vidbuf[k++] = sim[0][i/zf][j/zf]+MAPOFFSET;
      break;

    case HORIZONTAL:
      for ( i = 0; i < ydim; i++ )      // i from top to bottom
        for ( j = 0; j < xdim; j++ )    // j from left to right
          vidbuf[k++] = sim[1][i/zf][j/zf]+MAPOFFSET;
      break;

    case SAGITTAL:
      for ( i = ydim-1; i >= 0; i-- )   // i from bottom to top
        for ( j = 0; j < xdim; j++ )    // j from left to right
          vidbuf[k++] = sim[2][i/zf][j/zf]+MAPOFFSET;
      break;
    }

  } else {

    if ( CORONAL == plane || all3flag ) {

      // if we're in full screen...
      if ( !all3flag ) {

        // start at the beginning
        theDestIndex = 0;

      } else {

        // else we're in all3 view, and start halfway down.
        theDestIndex = kBytesPerPixel * xdim * hay;
      }
      
      // this is our z axis in this view.
      theScreenZ = imc;
      
      if ( theScreenZ >= 0 && theScreenZ < imnr1*zf ) {

  // start drawing image
        for ( theScreenY = 0; theScreenY < ydim; theScreenY += theVoxelSize ) {

          // skip odd lines if we're in all3 view.
          if ( all3flag && isOdd(theScreenY ) )
            continue;

          for ( theScreenX = 0; theScreenX < xdim; theScreenX += theVoxelSize  ) {

            // ?? don't know what this does.
            if ( 128 == theScreenX && 128 == theScreenY )
              DiagBreak();

            // if drawing all 3, skip odd cols
            if ( all3flag && isOdd ( theScreenX ) )
              continue;

            // because our voxels could be out of bounds. we check for it,
            // so we don't need debugging output.
            DisableDebuggingOutput;

            // get the voxel coords
            ScreenToVoxel ( CORONAL, theScreenX, theScreenY, theScreenZ,
                            &theVoxX, &theVoxY, &theVoxZ );

            // if they are good..
            if ( IsVoxelInBounds ( theVoxX, theVoxY, theVoxZ )) {

              // get the index into hacked_map from im
              v = inImageSpace[theVoxZ][theVoxY][theVoxX] + MAPOFFSET;
              
              // if we're truncating values and the index is below the min
              // or above the max, set it to MAPOFFSET
              if ( truncflag )
                if ( v < white_lolim + MAPOFFSET || 
                     v > white_hilim + MAPOFFSET )
                  v = MAPOFFSET;
              
            } else {

              // out of voxel bounds, set pixel to black.
              v = 0;
            }

            // turn debugging back on.
            EnableDebuggingOutput;
            
            // draw the value as a gray pixel.
            SetGrayPixelInBuffer ( vidbuf, theDestIndex, theVoxelSize,
           hacked_map[v] );

            // next pixel. it's actually voxel size places away,
      // becasue we just draw voxel size pixels.
            theDestIndex += kBytesPerPixel * theVoxelSize;
            
            // if we're in all3 view..
            if ( all3flag )
              
              // everytime we complete a line, drop k down to the beginning
              // of the next line by skipping hax pixels. check xdim-2
              // because we skip xdim-1; it's odd and we skip odd pixels
              if ( xdim - 2 == theScreenX )
                theDestIndex += kBytesPerPixel * hax;
          } 

    // step down a row. we're already at the end of one row, so we step
    // down theVoxelSize-1 rows.
    theDestIndex += kBytesPerPixel * (theVoxelSize-1) * xdim;
        }

        // end drawing image

        // draw control points.
        if ( gIsDisplayCtrlPts ) {
    DrawControlPoints ( vidbuf, CORONAL, imc/zf );
        }

  // if we have functional data to draw...
  if ( FuncDis_IsDataLoaded () ) {
    DrawFunctionalData ( vidbuf, theVoxelSize, CORONAL, imc/zf );
  }

  // draw our selected voxels.
  if ( IsDisplaySelectedVoxels () ) {
    DrawSelectedVoxels ( vidbuf, CORONAL, imc/zf );
  }

        // if our cursor is in view, draw it.
  if ( IsCursorVisible () ) {
    if ( IsScreenPtInWindow ( jc, ic, imc ) ) {
      
      if ( !all3flag ) {
        
        DrawCrosshairInBuffer ( vidbuf, jc, ic, 
              kCursorCrosshairRadius, 
              kRGBAColor_Red );
        
      } else {
        
        // adjust coords for all3 view.
        DrawCrosshairInBuffer ( vidbuf, jc/2, hay + ic/2, 
              kCursorCrosshairRadius, 
              kRGBAColor_Red );
        
      }
    }
  }

        // end_kt

      } else { 

        // fill the screen with black.
       for ( theScreenY = 0; theScreenY < ydim; theScreenY++ ) {
          if ( all3flag && isOdd(theScreenY ) )
            continue;
          for ( theScreenX = 0; theScreenX < xdim; theScreenX++ ) {
            if ( all3flag && isOdd(theScreenX) )
              continue;
            vidbuf [ theDestIndex++ ] = MAPOFFSET;
          }
        }
      }
    }

    // This section is not commented except where it differs from the
    // above section. it is essentially the same except for determining
    // the x/y/z coords in various spaces and determining the drawing
    // location of the all3 view panels.
      
    if ( HORIZONTAL == plane || all3flag ) {

      // start at position 0 regardless of all3flag status, because the
      // horizontal all3 panel is in the lower left hand corner.
      theDestIndex = 0;
      
      theScreenY = ( ydim-1 - ic );

      if ( theScreenY >= 0 && theScreenY < ynum*zf ) {

        for ( theScreenZ = 0; theScreenZ < ydim; theScreenZ += theVoxelSize ) {
          
          if ( all3flag && isOdd(theScreenZ) )
            continue;
          
          for ( theScreenX = 0; theScreenX < xdim; theScreenX += theVoxelSize ) {

            if ( all3flag && isOdd(theScreenX) )
              continue;
 
            DisableDebuggingOutput;

            // different mapping from screen space to voxel space
            // here we use ic instead of theScreenY becayse we already 
            // flipped theScreenY and we would just be flipping it again
            // here.
            ScreenToVoxel ( HORIZONTAL, theScreenX, ic, theScreenZ,
                            &theVoxX, &theVoxY, &theVoxZ );

            if ( IsVoxelInBounds ( theVoxX, theVoxY, theVoxZ ) ) {

              v = inImageSpace[theVoxZ][theVoxY][theVoxX] + MAPOFFSET;
              
              if ( truncflag )
                if ( v < white_lolim + MAPOFFSET || 
                     v > white_hilim + MAPOFFSET )
                  v = MAPOFFSET;

            } else {

              v = 0;

            }
            
      EnableDebuggingOutput;

            // ?? don't know what this does.
            if ( ( imc == theScreenY && abs(theScreenX-jc) <= curs) ||
                 ( jc == theScreenY && abs(theScreenY-imc) <= curs) ) 
              v=0 /*NUMVALS+MAPOFFSET*/;
            
            SetGrayPixelInBuffer ( vidbuf, theDestIndex, theVoxelSize, 
           hacked_map[v] );
            
            theDestIndex += kBytesPerPixel * theVoxelSize;

            if ( all3flag )
              if ( xdim - 2 == theScreenX )
                theDestIndex += kBytesPerPixel * hax;
          } 
    theDestIndex += kBytesPerPixel * (theVoxelSize-1) * xdim;
        }
        
        if ( gIsDisplayCtrlPts ) {
    DrawControlPoints ( vidbuf, HORIZONTAL, (ydim-1-ic)/zf );
        }
        
  if ( FuncDis_IsDataLoaded () ) {
    DrawFunctionalData ( vidbuf, theVoxelSize, CORONAL, imc/zf );
  }

  if ( IsDisplaySelectedVoxels () ) {
    DrawSelectedVoxels ( vidbuf, HORIZONTAL,  (ydim-1-ic)/zf );
  }

  if ( IsCursorVisible () ) {
    if ( IsScreenPtInWindow ( jc, ic, imc ) ) {
      if ( !all3flag ) {
        
        DrawCrosshairInBuffer ( vidbuf, jc, imc, 
              kCursorCrosshairRadius, 
              kRGBAColor_Red );
        
      } else {
        
        DrawCrosshairInBuffer ( vidbuf, jc/2, imc/2, 
              kCursorCrosshairRadius, 
              kRGBAColor_Red );
        
      }
    }
  }
        
      } else {
        for ( theScreenY = 0; theScreenY < ydim; theScreenY++ ) {
          if ( all3flag && isOdd(theScreenY) )
            continue;
          for ( theScreenX = 0; theScreenX < xdim; theScreenX++ ) {
            if ( all3flag && isOdd(theScreenX) )
              continue;
            vidbuf [ theDestIndex ] = MAPOFFSET;
          }
        }
      }
    }

    if ( SAGITTAL == plane || all3flag ) {

      if ( !all3flag )
        theDestIndex = 0;
      else
        // start in lower right quad
        theDestIndex = kBytesPerPixel * ( xdim*hay + hax );

      theScreenX = jc;

      if ( theScreenX >= 0 && theScreenX < xnum*zf ) {

        for ( theScreenY = 0; theScreenY < ydim; theScreenY += theVoxelSize ) {

          if ( all3flag && isOdd(theScreenY) )
            continue;
          
          for ( theScreenZ = 0; theScreenZ < xdim; theScreenZ += theVoxelSize ) {

            if ( all3flag && isOdd(theScreenZ) )
              continue;

            DisableDebuggingOutput;

            ScreenToVoxel ( SAGITTAL, theScreenX, theScreenY, theScreenZ,
                            &theVoxX, &theVoxY, &theVoxZ );

            if ( IsVoxelInBounds ( theVoxX, theVoxY, theVoxZ ) ) {
              
              v = inImageSpace[theVoxZ][theVoxY][theVoxX] + MAPOFFSET;
              
              if ( truncflag )
                if ( v < white_lolim + MAPOFFSET ||
                     v > white_hilim + MAPOFFSET )
                  v = MAPOFFSET;

            } else {

              v = 0;
            }

      EnableDebuggingOutput;

            SetGrayPixelInBuffer ( vidbuf, theDestIndex, theVoxelSize,
           hacked_map[v] );

            theDestIndex += kBytesPerPixel * theVoxelSize;

            if ( all3flag )
              if ( xdim - 2 == theScreenZ )
                theDestIndex += kBytesPerPixel * hax;

          }
    theDestIndex += kBytesPerPixel * (theVoxelSize-1) * xdim;
        }
        
        if ( gIsDisplayCtrlPts ) {
    DrawControlPoints ( vidbuf, SAGITTAL, jc/zf );
        }

  if ( FuncDis_IsDataLoaded () ) {
    DrawFunctionalData ( vidbuf, theVoxelSize, CORONAL, imc/zf );
  }

  if ( IsDisplaySelectedVoxels () ) {
    DrawSelectedVoxels ( vidbuf, SAGITTAL, jc/zf );
  }

  if ( IsCursorVisible () ) {
    if ( IsScreenPtInWindow ( jc, ic, imc ) ) {
      if ( !all3flag ) {
        
        DrawCrosshairInBuffer ( vidbuf, imc, ic, 
              kCursorCrosshairRadius, 
              kRGBAColor_Red );
        
      } else {
        
        // draw into lower right hand quad
        DrawCrosshairInBuffer ( vidbuf, hax + imc/2, hay + ic/2, 
              kCursorCrosshairRadius, 
              kRGBAColor_Red );
        
      }
    }
  }

      } else {
        for ( theScreenY = 0; theScreenY < ydim; theScreenY++ ) {
          if ( all3flag && isOdd(theScreenY) )
            continue;
          for ( theScreenX = 0; theScreenX < xdim; theScreenX++ ) {
            if ( all3flag && isOdd(theScreenX) )
              continue;
            vidbuf [ theDestIndex ] = MAPOFFSET;
          }
        }
      }
    }
    
    // copy a bunch of stuff from sim. this is the lower right hand corner
    // of the all3 view.
    if ( all3flag ) {

      theDestIndex = kBytesPerPixel * hax;

      // drawing every other line..
      for ( theScreenY = 0; theScreenY < ydim; theScreenY += 2 )
        for ( theScreenX = 0; theScreenX < xdim; theScreenX += 2 ) {

          v = sim[2][255-theScreenY/zf][theScreenX/zf] + MAPOFFSET;
          SetGrayPixelInBuffer ( vidbuf, theDestIndex, 1, v );
          theDestIndex += kBytesPerPixel;

          if ( theScreenX == xdim - 2 )
            theDestIndex += kBytesPerPixel * hax;
        }
    }

    rectwrite(0,0,xdim-1,ydim-1,vidbuf);
  }

  Voxel_Delete ( &theVoxel );
}

void
DrawControlPoints ( char * inBuffer, int inPlane, int inPlaneNum ) {

  VoxelRef theVoxel;
  VoxelListRef theList;
  int theListCount, theListIndex;
  char theListErr;
  char isSelected;
  long theColor;
  int theScreenX, theScreenY, theScreenZ, theScreenH, theScreenV;

  Voxel_New ( &theVoxel );

  switch ( inPlane ) {
  case CORONAL:
    theListErr = 
      VSpace_GetVoxelsInZPlane ( gCtrlPtList, inPlaneNum, &theList );
    break;
  case SAGITTAL:
    theListErr = 
      VSpace_GetVoxelsInXPlane ( gCtrlPtList, inPlaneNum, &theList );
    break;
  case HORIZONTAL:
    theListErr = 
      VSpace_GetVoxelsInYPlane ( gCtrlPtList, inPlaneNum, &theList );
    break;
  }

  if ( theListErr != kVSpaceErr_NoErr ) {
    DebugPrint "DrawControlPoints(): Error in VSpace_GetVoxelsInPlane: %s\n", 
      VSpace_GetErrorString ( theListErr ) EndDebugPrint;
    theList = NULL;
  }
  
  // if we have a list..
  if ( theList != NULL ) {
    
    // get its count
    theListErr = VList_GetCount ( theList, &theListCount );
    if ( theListErr != kVListErr_NoErr ) {
      DebugPrint "DrawControlPoints(): Error in VList_GetCount: %s\n",
  VList_GetErrorString ( theListErr ) EndDebugPrint;
      theListCount = 0;
    }

    // get each voxel...
    for ( theListIndex = 0; 
    theListIndex < theListCount; 
    theListIndex++ ) {
      
      theListErr = VList_GetNthVoxel ( theList, theListIndex, theVoxel );
      if ( theListErr != kVListErr_NoErr ) {
  DebugPrint "DrawControlPoints(): Error in VList_GetNthVoxel: %s\n",
    VList_GetErrorString ( theListErr ) EndDebugPrint;
  continue;
      }

      // see if it's selected. 
      theListErr = VList_IsInList ( gSelectionList, theVoxel, &isSelected );
      if ( theListErr != kVListErr_NoErr ) {
  DebugPrint "DrawControlPoints(): Error in VList_IsInList: %s\n",
    VList_GetErrorString(theListErr) EndDebugPrint
    isSelected = FALSE;
      }
              
      // if it's selected, draw in one color, else use another color
      if ( isSelected )
  theColor = kRGBAColor_Yellow;
      else
  theColor = kRGBAColor_Green;
      
      // go to screen points
      VoxelToScreen ( Voxel_GetX(theVoxel), Voxel_GetY(theVoxel),
          Voxel_GetZ(theVoxel), 
          inPlane, &theScreenX, &theScreenY, &theScreenZ );
      
      // if it's in the window...
      if ( IsScreenPtInWindow ( theScreenX, theScreenY, theScreenZ ) ) {
  
  // draw the point.
  ScreenToScreenXY ( theScreenX, theScreenY, theScreenZ,
         inPlane, &theScreenH, &theScreenV );
    
  DrawCtrlPt ( inBuffer, theScreenH, theScreenV, theColor );
      }
    }
  }
}

void DrawFunctionalData ( char * inBuffer, int inVoxelSize,
        int inPlane, int inPlaneNum ) {

  VoxelRef theVoxel;
  int x, y;
  float theValue;
  int theScreenX, theScreenY, theScreenZ, theScreenH, theScreenV;
  char theDestRed, theDestGreen, theDestBlue;
  long theDestColor;
  FuncDis_ErrorCode theErr;

  // if we're not visible, don't draw.
  if ( !FuncDis_IsOverlayVisible() ) {
    return;
  }

  Voxel_New ( &theVoxel );

  DisableDebuggingOutput;
  
  for ( y = 0; y < ynum; y++ ) {
    for ( x = 0; x < xnum; x++ ) {

      switch ( inPlane ) {
      case CORONAL:
  Voxel_Set ( theVoxel, x, y, inPlaneNum  );
  break;
      case SAGITTAL:
  Voxel_Set ( theVoxel, inPlaneNum, x, y  );
  break;
      case HORIZONTAL:
  Voxel_Set ( theVoxel, x, inPlaneNum, y );
  break;
      }

                                   /* if we get an error here, it just means
              that there is no value for this
              anatomical voxel, and we can safely
              ignore it. */
      // get the value.
      theErr = FuncDis_GetValueAtAnatomicalVoxel ( theVoxel, &theValue );

      if ( theErr == kFuncDisErr_NoError ) {

  VoxelToScreen ( EXPAND_VOXEL_INT(theVoxel),
      inPlane, &theScreenX, &theScreenY, &theScreenZ );
  
  if ( IsScreenPtInWindow ( theScreenX, theScreenY, theScreenZ ) ) {
    
    DebugPrint "-- (%d, %d, %d): %2.5f\n",
      EXPAND_VOXEL_INT(theVoxel), theValue EndDebugPrint;

    ScreenToScreenXY ( theScreenX, theScreenY, theScreenZ,
           inPlane, &theScreenH, &theScreenV );
    
    FuncDis_CalcColorForValue ( theValue, 0.25,
        &theDestRed, &theDestGreen, &theDestBlue );
    
    // only draw if we're not all 0s.
    if ( theDestRed != 0 || theDestGreen != 0 || theDestBlue != 0 ) {
      
      theDestColor = 
        ( (unsigned char)kPixelComponentLevel_Max << kPixelOffset_Alpha |
    (unsigned char)theDestBlue << kPixelOffset_Blue |
    (unsigned char)theDestGreen << kPixelOffset_Green |
    (unsigned char)theDestRed << kPixelOffset_Red );
      
      FillBoxInBuffer ( inBuffer, theScreenH, theScreenV, inVoxelSize,
            theDestColor );
    }
  }
  
      } else if ( theErr != kFuncDisErr_InvalidAnatomicalVoxel ) {

  DebugPrint "Error in FuncDis_GetValueAtAnatomcalValue: %d, %s\n",
    theErr, FuncDis_GetErrorString(theErr) EndDebugPrint;
      }

    }
  }

  EnableDebuggingOutput;

  Voxel_Delete ( &theVoxel );
}
 

/*
void DrawFunctionalData ( char * inBuffer, int inPlane, int inPlaneNum ) {

  VoxelValueListRef theList;
  VoxelValueList_ErrorCode theListErr;
  VoxelRef theVoxel;
  float theValue;
  int theScreenX, theScreenY, theScreenZ, theScreenH, theScreenV;

  Voxel_New ( &theVoxel );

  VoxelValueList_New ( &theList );
  GetFunctionalValuesInPlane ( inPlane, inPlaneNum, theList );

  theListErr = kVoxelValueListErr_NoErr;
  while ( kVoxelValueListErr_NoErr == theListErr ) {
    
    theListErr =
      VoxelValueList_GetNextVoxelAndValue ( theList,
              theVoxel, &theValue );
    
    DisableDebuggingOutput;

    VoxelToScreen ( Voxel_GetX(theVoxel), Voxel_GetY(theVoxel),
        Voxel_GetZ(theVoxel), 
        inPlane, &theScreenX, &theScreenY, &theScreenZ );
    
    if ( IsScreenPtInWindow ( theScreenX, theScreenY, theScreenZ ) ) {

      EnableDebuggingOutput;
      DebugPrint "-- (%d, %d, %d): %2.5f\n",
  Voxel_GetX(theVoxel), Voxel_GetY(theVoxel), 
  Voxel_GetZ(theVoxel), theValue EndDebugPrint;

      ScreenToScreenXY ( theScreenX, theScreenY, theScreenZ,
       inPlane, &theScreenH, &theScreenV );

      FillBoxInBuffer ( inBuffer, theScreenH, theScreenV, 5,
      kRGBAColor_Red );
    }

    EnableDebuggingOutput;
  }

  Voxel_Delete ( &theVoxel );
}
*/

void
draw_second_image(int imc, int ic, int jc)
{
  int i,j,imnr,k;
  int hax,hay,curs;
  int idx_buf;
  GLubyte v;

  hax = xdim/2;
  hay = ydim/2;
  if (all3flag) curs = 15;
  else          curs = 2;
  
  if (maxflag && !all3flag)
  {
    k = 0;
    for (i=0;i<ydim;i++)
      for (j=0;j<xdim;j++)
      {
        if (plane==CORONAL)
          vidbuf[k] = sim2[0][255-i/zf][j/zf]+MAPOFFSET;
        else if (plane==HORIZONTAL)
          vidbuf[k] = sim2[1][i/zf][j/zf]+MAPOFFSET;
        else if (plane==SAGITTAL)
          vidbuf[k] = sim2[2][255-i/zf][j/zf]+MAPOFFSET;
        k++;
      }
  }
  else /*No max flag */        
  {
    if (plane==CORONAL || all3flag)
    {
      k = 0;
      for (i=0;i<ydim;i++)
        for (j=0;j<xdim;j++)
        {
          if (i == 128 && j == 128)
            DiagBreak() ;
          if (all3flag && (i%2 || j%2)) continue;
          if (imc/zf>=0&&imc/zf<imnr1)
      v = im2[imc/zf][(ydim-1-i)/zf][j/zf]+MAPOFFSET;  
    else 
      v=MAPOFFSET;
          if (truncflag)
            if (v<white_lolim+MAPOFFSET || v>white_hilim+MAPOFFSET)
              v=MAPOFFSET;
          if (i==ipt||j==jpt) v=255-(v-MAPOFFSET)+MAPOFFSET;
          if ((i==ic&&abs(j-jc)<=curs)||
              (j==jc&&abs(i-ic)<=curs))
      {
        if (all3flag) {
    idx_buf = 4*((xdim*hay) + k + ((i/2)*hax)) ;
    vidbuf[idx_buf] = 255;
    vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = 0;
    k++;
        } else {
    vidbuf[k]=255;
    vidbuf[k+1] = vidbuf[k+2]= 0;
    k+=4;
        }
      }
          else if (all3flag) 
      {
        idx_buf = 4*((xdim*hay) + k + ((i/2)*hax)) ;
        
        vidbuf[idx_buf] = vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = hacked_map[v];
        /*
    #else
    vidbuf[idx_buf] = vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = v;
    #endif
    vidbuf[idx_buf + 3]=255;
        */
        k++;
      }
          else
      {
        
        vidbuf[k] = vidbuf[k+1] = vidbuf[k+2]= hacked_map[v]; 
        /*
    #else 
    vidbuf[k] = vidbuf[k+1] = vidbuf[k+2]= v; 
    #endif
        */
        vidbuf[k+3]=255;
        k+=4;
      }
        }
    } 
    if (plane==HORIZONTAL || all3flag)
      {
      k = 0;
      for (imnr=0;imnr<ydim;imnr++)
        for (j=0;j<xdim;j++)
        {
          if (all3flag && (imnr%2 || j%2)) continue;
          if (imnr/zf>=0&&imnr/zf<imnr1)
            v = im2[imnr/zf][(ydim-1-ic)/zf][j/zf]+MAPOFFSET;
          else v=MAPOFFSET;
          if (truncflag)
            if (v<white_lolim+MAPOFFSET || v>white_hilim+MAPOFFSET)
              v=MAPOFFSET;
          if (imnr==impt||j==jpt) v=255-(v-MAPOFFSET)+MAPOFFSET;
          if ((imnr==imc&&abs(j-jc)<=curs)||
              (j==jc&&abs(imnr-imc)<=curs))
      {
        if (all3flag) {
    /*idx_buf = 4*((xdim*hay) + hax + k + ((imnr/2)*hax)) ;*/
    idx_buf = 4*(k + ((imnr/2)*hax));
    vidbuf[idx_buf] = 255;
    vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = 0;
    k++;
        } else {
    vidbuf[k]=255;
    vidbuf[k+1] = vidbuf[k+2]= 0;
    k+=4;
        }
      }
    else if (all3flag)
      {
        /* idx_buf = 4*((xdim*hay) + hax + k + ((imnr/2)*hax)) ; */
        idx_buf = 4*(k + ((imnr/2)*hax));
        vidbuf[idx_buf] = vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = hacked_map[v];
        /*
    #else
    vidbuf[idx_buf] = vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = v;
    #endif
        */
        vidbuf[idx_buf + 3]=255;
        k++;
      }
          else
      {
        
        vidbuf[k] = vidbuf[k+1] = vidbuf[k+2]= hacked_map[v];
        /*
    #else
    vidbuf[k] = vidbuf[k+1] = vidbuf[k+2]= v;
    #endif
        */
        vidbuf[k+3]=255;
        k+=4;
      }
        }
    } /*else*/
    if (plane==SAGITTAL || all3flag)
    {
      k = 0;
      for (i=0;i<ydim;i++)
        for (imnr=0;imnr<xdim;imnr++)
    {
      if (all3flag && (i%2 || imnr%2)) continue;
      if (imnr/zf>=0&&imnr/zf<imnr1)
        v = im2[imnr/zf][(ydim-1-i)/zf][jc/zf]+MAPOFFSET;  
      else v=MAPOFFSET;
      if (truncflag)
        if (v<white_lolim+MAPOFFSET || v>white_hilim+MAPOFFSET)
    v=MAPOFFSET;
      if (imnr==impt||i==ipt) v=255-(v-MAPOFFSET)+MAPOFFSET;
      if ((imnr==imc&&abs(i-ic)<=curs)||
    (i==ic&&abs(imnr-imc)<=curs)) {
        if (all3flag) {
    /*idx_buf = 4*(k + ((i/2)*hax));*/
    idx_buf = 4*((xdim*hay) + hax + k + ((i/2)*hax)) ;
    vidbuf[idx_buf] = 255;
    vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = 0;
    k++;
        } else {
    vidbuf[k]=255;
    vidbuf[k+1] = vidbuf[k+2]= 0;
    k+=4;
        }
      }
      else if (all3flag) 
        {
    /* idx_buf = 4*(k + ((i/2)*hax)); */
    idx_buf = 4*((xdim*hay) + hax + k + ((i/2)*hax)) ;
    vidbuf[idx_buf] = vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = hacked_map[v]; 
    /*
      #else
      vidbuf[idx_buf] = vidbuf[idx_buf+1] = vidbuf[idx_buf + 2] = v;
      #endif
    */
    vidbuf[idx_buf + 3]=255;
    k++;
        }
      else
        {
    
    vidbuf[k] = vidbuf[k+1] = vidbuf[k+2]= hacked_map[v];   
    /*
      #else
      vidbuf[k] = vidbuf[k+1] = vidbuf[k+2]= v; 
      #endif
    */
    vidbuf[k+3]=255;
    k+=4;
        }
    }
    }
    if (all3flag) 
    {
      k = 0;
      for (i=0;i<ydim;i++)
        for (j=0;j<xdim;j++) 
        {
          if (i%2 || j%2) continue;
          {
            idx_buf = 4*(hax + k + i/2*hax);
            vidbuf[idx_buf] = vidbuf[idx_buf+1] = vidbuf[idx_buf+2]= 
              sim[2][255-i/zf][j/zf]+MAPOFFSET;
            vidbuf[idx_buf+3] = 255;
            k++;
          }
        }
    }
    rectwrite(0,0,xdim-1,ydim-1,vidbuf);
  }
}

void
show_orig_surface(void) {

  // kt - old code commented out
  // MRISrestoreVertexPositions(mris, ORIG_VERTICES) ;

  // kt - call new function to toggle display status
  SetOriginalSurfaceDisplayStatus ( !gIsDisplayOriginalSurface );

  redraw() ;
}
void
show_canonical_surface(void) {

  // kt - old code commented out
  // MRISrestoreVertexPositions(mris, CANONICAL_VERTICES) ;

  // kt - call new function to toggle display status
  SetCanonicalSurfaceDisplayStatus ( !gIsDisplayCanonicalSurface );

  redraw() ;
}
void
smooth_surface(int niter)
{
  if (mris)
    MRISaverageVertexPositions(mris, niter) ;
  redraw() ;
}
void
show_current_surface(void) {

  // kt - old code commented out
  // MRISrestoreVertexPositions(mris, TMP_VERTICES) ;

  // kt - call new function to toggle display status
  SetCurrentSurfaceDisplayStatus ( !gIsDisplayCurrentSurface );

  redraw() ;
}

int 
read_canonical_vertex_positions(char *fname)
{
  if (!mris)
    return(NO_ERROR) ;

  // kt - save current vertices to temp
  MRISsaveVertexPositions ( mris, TMP_VERTICES );

  // read vertices into current
  if (MRISreadVertexPositions(mris, fname) != NO_ERROR) {

    DebugPrint "read_canonical_vertex_positions ( %s ):\n\tcould not read canonical vertex positions\n", fname EndDebugPrint;
    OutputPrint "Couldn't read canonical vertices from %s.\n", 
      fname EndOutputPrint;
    return(Gerror) ;
  }

  // save current to canonical
  MRISsaveVertexPositions(mris, CANONICAL_VERTICES) ;

  // kt - restore current from temp
  MRISrestoreVertexPositions ( mris, TMP_VERTICES );

  // kt - surface is loaded.
  gIsCanonicalSurfaceLoaded = TRUE;

  // kt - print a status msg
  OutputPrint "Canonical surface loaded. To toggle display, type \"canon\"\n."
    EndOutputPrint;

  return(NO_ERROR) ;
}
int
read_orig_vertex_positions(char *name)
{
  if (!mris)
    return(NO_ERROR) ;

  // kt - save current vertices to temp
  MRISsaveVertexPositions ( mris, TMP_VERTICES );

  // read vertices into current
  if (MRISreadVertexPositions(mris, name) != NO_ERROR) {

    DebugPrint "read_orig_vertex_positions ( %s ):\n\tcould not read original vertex positions\n", name EndDebugPrint;
    OutputPrint "Couldn't read original vertices from %s.\n", 
      name EndOutputPrint;
    return(Gerror) ;
  }

  // save current to canonical
  MRISsaveVertexPositions(mris, ORIG_VERTICES) ;

  // kt - restore current from temp
  MRISrestoreVertexPositions ( mris, TMP_VERTICES );

  // kt - surface is loaded.
  gIsOriginalSurfaceLoaded = TRUE;

  // kt - print a status msg
  OutputPrint "Original surface loaded. To toggle display, type \"orig\"\n."
    EndOutputPrint;

  return(NO_ERROR) ;
}

int
read_surface(char *name)
{
  char fname[STRLEN], *cp ;
  int theStatus;

  cp = strchr(name, '/') ;
  if (!cp)  /* no path specified - put the path into it */
  {
    strcpy(surface, name) ;
    sprintf(sfname,"%s/%s/surf/%s",subjectsdir,pname,name);
    strcpy(fname, sfname) ;
  }
  else
    strcpy(fname, name) ;

  OutputPrint "Reading surface file %s\n", fname EndOutputPrint;

  theStatus = read_binary_surface(fname);

  // kt - if the return is 0, it seems the call was successful.
  if ( theStatus == 0 ) {

    // mark the surface as loaded.
    gIsCurrentSurfaceLoaded = TRUE;
    
    // print a status msg.
    OutputPrint "Surface loaded. To toggle display, type \"current\"\n." 
      EndOutputPrint;

  } else {

    DebugPrint "read_surface( %s ):\n\tread_binary_surface( %s ) failed, returned %d\n", name, fname, theStatus EndDebugPrint;
    OutputPrint "Surface failed to load.\n" EndOutputPrint;
  }
  
  return theStatus;
}

int
read_binary_surface(char *fname)
{
#if 1
  
  if (mris)
    MRISfree(&mris) ;
  mris = MRISread(fname) ;

  if (!mris)
  {
    surfflag = FALSE;
    surfloaded = FALSE;
    curvflag = FALSE;
    curvloaded = FALSE;
    fieldsignflag = FALSE;
    fieldsignloaded = FALSE;
    gIsDisplayCurrentSurface = FALSE;
    return(ERROR_NOFILE) ;
  }
#else

  int k,n;                   /* loop counters */
  int ix,iy,iz;
  int version;
  int first,surfchanged=FALSE;
  int overtex_index, oface_index;
  FILE *fp;

  fp = fopen(fname,"r");
  if (fp==NULL) { printf("medit: ### File %s not found\n",fname);PR return(-1);}

  omris->nvertices = mris->nvertices;
  omris->nfaces = mris->nfaces;
  fread3(&first,fp);
  if (first == 16777215) {
    version = -1;
    printf("medit: new surface file format\n");
  }
  else {
    rewind(fp);
    version = 0;
    printf("medit: old surface file format\n");
  }
  fread3(&mris->nvertices,fp);
  fread3(&mris->nfaces,fp);

  printf("medit: vertices=%d, faces=%d\n",mris->nvertices,mris->nfaces);
  if (surfloaded) {
    if (omris->nvertices!=mris->nvertices || omris->nfaces!=mris->nfaces) {
      for (k=0;k<omris->nvertices;k++)
        free((int *)mris->vertices[k].f);
      free((vertex_type *)vertex);
      free((face_type *)face);
      surfchanged = TRUE;
    }
  }
  if (!surfloaded || surfchanged) {
    vertex = (vertex_type *)lcalloc(mris->nvertices,sizeof(vertex_type));
    face = (face_type *)lcalloc(mris->nfaces,sizeof(face_type));
  }

  for (k=0;k<mris->nvertices;k++)
  {
    fread2(&ix,fp);
    fread2(&iy,fp);
    fread2(&iz,fp);
    mris->vertices[k].x = ix/100.0;
    mris->vertices[k].y = iy/100.0;
    mris->vertices[k].z = iz/100.0;
    if (version==0)
    {
      fread1(&mris->vertices[k].num,fp);
      if (!surfloaded || surfchanged)
        mris->vertices[k].f = (int *)lcalloc(mris->vertices[k].num,sizeof(int));
      for (n=0;n<mris->vertices[k].num;n++)
        fread3(&mris->vertices[k].f[n],fp);
    } else mris->vertices[k].num = 0;
  }
  for (k=0;k<mris->nfaces;k++)
  {
    for (n=0;n<4;n++)
    {
      fread3(&mris->faces[k].v[n],fp);
      if (version<0)
        mris->vertices[mris->faces[k].v[n]].num++;
    }
  }
  fclose(fp);
  if (version<0)
  {
    for (k=0;k<mris->nvertices;k++)
    {
      if (!surfloaded || surfchanged)
        mris->vertices[k].f = (int *)lcalloc(mris->vertices[k].num,sizeof(int));
      mris->vertices[k].num = 0;
    }
    for (k=0;k<mris->nfaces;k++)
    {
      for (n=0;n<4;n++)
      {
        mris->vertices[mris->faces[k].v[n]].f[mris->vertices[mris->faces[k].v[n]].num++] = k;
      }
    }
  }
  for (k=0;k<mris->nvertices;k++) {   /* fscontour */
    mris->vertices[k].fieldsign = 0;
    mris->vertices[k].fsmask = 1;
    mris->vertices[k].curv = 0;
  }
#endif
  surfflag = TRUE;
  surfloaded = TRUE;
  curvflag = FALSE;
  curvloaded = FALSE;
  fieldsignflag = FALSE;
  fieldsignloaded = FALSE;
  gIsDisplayCurrentSurface = TRUE;
  gIsCurrentSurfaceLoaded = TRUE;
  MRISsaveVertexPositions(mris, TMP_VERTICES) ;
  return(0);
}

void SetCurrentSurfaceDisplayStatus ( char inDisplay ) {

  // check to see if surface is loaded. if not, display an error msg.
  if ( !gIsCurrentSurfaceLoaded ) {
    printf ( "Current surface is not loaded. To load this surface, type \"read_surface\" followed by a surface.\n" );
    return;
  }

  // toggle the display status.
  gIsDisplayCurrentSurface = inDisplay;

  // print a status msg.
  if ( gIsDisplayCurrentSurface ) {
    printf ( "Current surface display is ON, shown in yellow\n" );
    if ( IsSurfaceVertexDisplayOn() ) {
      printf ( ", with blue vertices .\n" );
    } else {
      printf ( ".\n" );
    }
  } else {
    printf ( "Current surface display is OFF.\n" );
  }    
}

void SetOriginalSurfaceDisplayStatus ( char inDisplay ) {
  
  // check to see if surface is loaded. if not, display an error msg.
  if ( !gIsOriginalSurfaceLoaded ) {
    printf ( "Original surface is not loaded. To load this surface, type \"read_orig\" followed by a surface name.\n" );
    return;
  }

  // toggle the display status.
  gIsDisplayOriginalSurface = inDisplay;

  // print a status msg.
  if ( gIsDisplayOriginalSurface ) {
    printf ( "Original surface display is ON, shown in green" );
    if ( IsSurfaceVertexDisplayOn() ) {
      printf ( ", with magenta vertices .\n" );
    } else {
      printf ( ".\n" );
    }
  } else {
    printf ( "Original surface display is OFF.\n" );
  }    
}

void SetCanonicalSurfaceDisplayStatus ( char inDisplay ) {

  // check to see if surface is loaded. if not, display an error msg.
  if ( !gIsCanonicalSurfaceLoaded ) {
    printf ( "Canonical surface is not loaded. To load this surface, type \"read_canon\" followed by a surface name.\n" );
    return;
  }

  // set the display status.
  gIsDisplayCanonicalSurface = inDisplay;

  // print a status msg.
  if ( gIsDisplayCanonicalSurface ) {
    printf ( "Canonical surface display is ON, shown in red\n" );
    if ( IsSurfaceVertexDisplayOn() ) {
      printf ( ", with cyan vertices .\n" );
    } else {
      printf ( ".\n" );
    }
  } else {
    printf ( "Canonical surface display is OFF.\n" );
  }    
}

void SetSurfaceVertexDisplayStatus ( char inDisplay ) {

  // set display status.
  gIsDisplaySurfaceVertices = inDisplay;

  // print a status msg.
  if ( IsSurfaceVertexDisplayOn() ) {
    printf ( "Surface vertex display is ON.\n" );
  } else {
    printf ( "Surface vertex display is OFF.\n" );
  }    
}

char IsSurfaceVertexDisplayOn () {

  return gIsDisplaySurfaceVertices;
}

void SetAverageSurfaceVerticesStatus ( char inDisplay ) {

  // set display status.
  gIsAverageSurfaceVertices = inDisplay;

  // print a status msg.
  if ( IsAverageSurfaceVerticesOn() ) {
    printf ( "Displaying averaged surface vertices.\n" );
  } else {
    printf ( "Displaying true surface vertices.\n" );
  }    
}

char IsAverageSurfaceVerticesOn () {

  return gIsAverageSurfaceVertices;
}

void HiliteSurfaceVertex ( vertex_type * inVertex, int inSurface ) {

  gHilitedVertex = inVertex;
  gHilitedVertexSurface = inSurface;
}

char IsSurfaceVertexHilited ( vertex_type * inVertex, int inSurface ) {

  if ( inVertex == gHilitedVertex &&
       inSurface == gHilitedVertexSurface ) {

    return TRUE;

  } else {

    return FALSE;
  }
}

void CurrentSurfaceVertexToVoxel ( vertex_type * inVertex, int inPlane,
           VoxelRef inVoxel ) {

  // use the plane to determine the orientation of the voxel.
  switch ( inPlane ) {
  case CORONAL:
    Voxel_SetFloat ( inVoxel, inVertex->x, inVertex->z, inVertex->y );
    break;
  case SAGITTAL:
    Voxel_SetFloat ( inVoxel, inVertex->y, inVertex->z, inVertex->x );
    break;
  case HORIZONTAL:
    Voxel_SetFloat ( inVoxel, inVertex->x, inVertex->y, inVertex->z );
    break;
  }
}

void OriginalSurfaceVertexToVoxel ( vertex_type * inVertex, int inPlane,
            VoxelRef inVoxel ) {

  // use the plane to determine the orientation of the voxel.
  switch ( inPlane ) {
  case CORONAL:
    Voxel_SetFloat(inVoxel, inVertex->origx, inVertex->origz, inVertex->origy);
    break;
  case SAGITTAL:
    Voxel_SetFloat(inVoxel, inVertex->origy, inVertex->origz, inVertex->origx);
    break;
  case HORIZONTAL:
    Voxel_SetFloat(inVoxel, inVertex->origx, inVertex->origy, inVertex->origz);
    break;
  }
}

void CanonicalSurfaceVertexToVoxel ( vertex_type * inVertex, int inPlane,
             VoxelRef inVoxel ) {

  // use the plane to determine the orientation of the voxel.
  switch ( inPlane ) {
  case CORONAL:
    Voxel_SetFloat ( inVoxel, inVertex->cx, inVertex->cz, inVertex->cy );
    break;
  case SAGITTAL:
    Voxel_SetFloat ( inVoxel, inVertex->cy, inVertex->cz, inVertex->cx );
    break;
  case HORIZONTAL:
    Voxel_SetFloat ( inVoxel, inVertex->cx, inVertex->cy, inVertex->cz );
    break;
  }
}


// determine if voxels cross a plane
char IsVoxelsCrossPlane ( float inPlaneZ, 
                          float inVoxZ1, float inVoxZ2 ) {

  float theZDist1, theZDist2;

  // get the distance of each voxel from the plane
  theZDist1 = inVoxZ1 - inPlaneZ;
  theZDist2 = inVoxZ2 - inPlaneZ;

  // if they are on opposite sides of the plane or one or both are on
  // the plane...
  if ( theZDist1 * theZDist2 <= 0.0 ) {

    return TRUE;

  } else {

    return FALSE;
  }
}
                          

// space is relative to face
void CalcDrawPointsForVoxelsCrossingPlane ( float inPlaneZ, int inPlane,
                                            VoxelRef inVox1, VoxelRef inVox2,
                                            float *outX, float *outY ) {
  
  float theHeight, theRASX, theRASY;

  // averaging?
  if ( IsAverageSurfaceVerticesOn() ) {

    // find a height. if they arn't both on the plane...
    if ( Voxel_GetFloatZ(inVox2) - Voxel_GetFloatZ(inVox1) != 0 ) {
      
      // height is one difference over the other, ends up positive.
      theHeight = ( inPlaneZ - Voxel_GetFloatZ(inVox1) ) / 
  ( Voxel_GetFloatZ(inVox2) - Voxel_GetFloatZ(inVox1) );
      
    } else {
      
      // else height is just one.
      theHeight = 1.0;
    }
    
    // interperolate using the height to find the draw point.
    theRASX = Voxel_GetFloatX(inVox1) + 
      theHeight * ( Voxel_GetFloatX(inVox2) - Voxel_GetFloatX(inVox1) );
    
    theRASY = Voxel_GetFloatY(inVox1) + 
      theHeight * ( Voxel_GetFloatY(inVox2) - Voxel_GetFloatY(inVox1) );

  } else {
    
    // this is the real location of the vertex. vox2 is the actual vertex,
    // vox1 is the neighbor.
    theRASX = Voxel_GetFloatX ( inVox2 );
    theRASY = Voxel_GetFloatY ( inVox2 );
  }

  // convert from ras to screen
  switch ( inPlane ) {
  case CORONAL:
    RASToFloatScreenXY ( theRASX, 0, theRASY, inPlane, outX, outY );
    break;
  case SAGITTAL:
    RASToFloatScreenXY ( 0, theRASX, theRASY, inPlane, outX, outY );
    break;
  case HORIZONTAL:
    RASToFloatScreenXY ( theRASX, theRASY, 0, inPlane, outX, outY );
    break;
  }
}


void FindRASScreenBounds ( Real *outMinX, Real *outMinY,
         Real *outMaxX, Real *outMaxY ) {

  Real theDummy, theMinX, theMinY, theMaxX, theMaxY;

  switch ( plane ) {
  case CORONAL:
    ScreenToRAS ( plane, 1, 1, 1, &theMinX, &theDummy, &theMinY );
    ScreenToRAS ( plane, xdim-2, ydim-2, xdim-2, &theMaxX, &theDummy, &theMaxY );
    break;
  case HORIZONTAL:
    break;
  case SAGITTAL:
    break;
  }

  DebugPrint "FindRASScreenBounds(): Got minx = %2.1f, miny = %2.1f, maxx = %2.1f, maxy = %2.1f\n", theMinX, theMinY, theMaxX, theMaxY EndDebugPrint;

  *outMinX = theMinX;
  *outMinY = theMinY;
  *outMaxX = theMaxX;
  *outMaxY = theMaxY;
}



void DrawSurface ( MRI_SURFACE * theSurface, int inPlane, int inSurface ) {

  face_type * theFace;
  vertex_type * theNeighborVertex, * theVertex;
  int theNeighborVertexIndex, theVertexIndex, theFaceIndex, theNumFaces;
  VoxelRef theVox1, theVox2;
  float theDrawPoint[VERTICES_PER_FACE][2], // an array of 2d points
    theHilitedVertexDrawPoint[20][2];
  float theDrawPointX, theDrawPointY;
  int theNumDrawPoints, theNumHilitedPoints;
  int thePlaneZ;
  float theLineColor[3], theVertexColor[3];
  float theLineWidth, thePointSize;
  char isHilitedVertexOnScreen;

  Voxel_New ( &theVox1 );
  Voxel_New ( &theVox2 );

  // set our drawing plane correctly
  ortho2 ( 0.0, xdim-1, 0.0, ydim-1 );

  // switch on our plane and set the ortho space and get the relative plane z.
  switch ( inPlane ) {
  case CORONAL:
    thePlaneZ = yy0+st*imc/fsf;
    break;
  case SAGITTAL:
    thePlaneZ = xx1-ps*jc/fsf;
    break;
  case HORIZONTAL:
    thePlaneZ = zz1-ps*(255.0-ic/fsf);
    break;
  default:
    thePlaneZ = 0;
  }

  // for each face in this surface...
  theNumFaces = theSurface->nfaces;
  for ( theFaceIndex = 0; theFaceIndex < theNumFaces; theFaceIndex++ ) {

    // get the face.
    theFace = &(theSurface->faces[theFaceIndex]);

    // get the last neighboring vertex. 
    theNeighborVertexIndex = VERTICES_PER_FACE - 1;
    theNeighborVertex = 
      &(theSurface->vertices[theFace->v[theNeighborVertexIndex]]);

    // no points to draw yet.
    theNumDrawPoints = 0;
    theNumHilitedPoints = 0;
    isHilitedVertexOnScreen = FALSE;

    // for every vertex in this face...
    for ( theVertexIndex = 0; 
          theVertexIndex < VERTICES_PER_FACE; 
          theVertexIndex++ ) {

      // get the vertex.
      theVertex = &(theSurface->vertices[theFace->v[theVertexIndex]]);

      // make voxels out of the points we want to test.
      switch ( inSurface ) {
      case kSurfaceType_Current:
        CurrentSurfaceVertexToVoxel ( theNeighborVertex, inPlane, theVox1 );
        CurrentSurfaceVertexToVoxel ( theVertex, inPlane, theVox2 );
        break;
      case kSurfaceType_Original:
        OriginalSurfaceVertexToVoxel ( theNeighborVertex, inPlane, theVox1 );
        OriginalSurfaceVertexToVoxel ( theVertex, inPlane, theVox2 );
        break;
      case kSurfaceType_Canonical:
        CanonicalSurfaceVertexToVoxel ( theNeighborVertex, inPlane, theVox1 );
        CanonicalSurfaceVertexToVoxel ( theVertex, inPlane, theVox2 );
        break;
      }

      // see if they cross the plane
      if ( IsVoxelsCrossPlane ( thePlaneZ,
                                Voxel_GetFloatZ(theVox1), 
                                Voxel_GetFloatZ(theVox2) )) {

        // calculate the point to draw.
        CalcDrawPointsForVoxelsCrossingPlane ( thePlaneZ, inPlane,
                                               theVox1, theVox2,
                                               &theDrawPointX,&theDrawPointY );

        // if this point is on screen...
        if ( IsTwoDScreenPtInWindow ( theDrawPointX, theDrawPointY ) ) {
          
          // if we're in all3 view, half the coords according to what plane
          // we're in.
          if ( all3flag ) {
            switch ( inPlane ) {
            case CORONAL:
              theDrawPointX = (theDrawPointX)/2.0;
              theDrawPointY = ((float)ydim/2.0) + (theDrawPointY)/2.0;
              break;
            case SAGITTAL:
              theDrawPointX = ((float)xdim/2.0) + (theDrawPointX)/2.0;
              theDrawPointY = ((float)ydim/2.0) + (theDrawPointY)/2.0;
              break;
            case HORIZONTAL:
              theDrawPointX = (theDrawPointX)/2.0;
              theDrawPointY = (theDrawPointY)/2.0;                        
              break;
            }
          }
          
          // copy the point into our array of points to draw and inc the
          // count.
          theDrawPoint[theNumDrawPoints][kPointArrayIndex_X] = theDrawPointX;
          theDrawPoint[theNumDrawPoints][kPointArrayIndex_Y] = theDrawPointY;
          theNumDrawPoints++;

          // chose a color.
          if ( curvflag && curvloaded ) {
            if ( theVertex->curv > 0.0 ) {
        set3fv ( theLineColor, 1.0, 0.0, 0.0 );
        set3fv ( theVertexColor, 0.0, 1.0, 1.0 );
            } else {
        set3fv ( theLineColor, 0.0, 1.0, 0.0 );
        set3fv ( theVertexColor, 1.0, 0.0, 1.0 );
      }
            
          } else {
            
            // switch on surface types
            switch ( inSurface ) {

              // yellow lines and blue vertices for default
            case kSurfaceType_Current:
        set3fv ( theLineColor, 1.0, 1.0, 0.0 );
        set3fv ( theVertexColor, 0.0, 0.0, 1.0 );
              break;

              // green lines and purple vertices for original
            case kSurfaceType_Original:
        set3fv ( theLineColor, 0.0, 1.0, 0.0 );
        set3fv ( theVertexColor, 1.0, 0.0, 1.0 );
              break;
              
              // red lines and vertices for canonical
            case kSurfaceType_Canonical:
        set3fv ( theLineColor, 1.0, 0.0, 0.0 );
        set3fv ( theVertexColor, 0.0, 1.0, 1.0 );
              break;
            }
    }
    
    // is this the hilited vertex?
    if ( IsSurfaceVertexHilited ( theVertex, inSurface ) ) {
      
      // mark it to be drawn later.
      theHilitedVertexDrawPoint
        [theNumHilitedPoints][kPointArrayIndex_X] = theDrawPointX;
      theHilitedVertexDrawPoint
        [theNumHilitedPoints][kPointArrayIndex_Y] = theDrawPointY;
      theNumHilitedPoints ++;
      isHilitedVertexOnScreen = TRUE;
    }
  }
      }
      
      // use this vertex as the neighboring vertex now.
      theNeighborVertexIndex = theVertexIndex;
      theNeighborVertex = theVertex;
    }
      
    // we have points to draw..
    if ( theNumDrawPoints > 0 ) {

      // set our line width and color.
      theLineWidth  = surflinewidth;
      if ( all3flag ) {
  theLineWidth /= 2.0;
      }
      glLineWidth ( theLineWidth );
      glColor3fv ( theLineColor );

      // draw a line with these vertices
      DrawWithFloatVertexArray ( GL_LINES, 
         (float*)theDrawPoint, theNumDrawPoints );

      // set point size and color.
      thePointSize = theLineWidth + 1;
      if ( thePointSize > gLocalZoom ) {
  thePointSize = 1;
      }

      // if we're drawing vertices...
      if ( IsSurfaceVertexDisplayOn () ) {

  glPointSize ( thePointSize );
  glColor3fv ( theVertexColor );
  
  // draw points with these vertices.
  DrawWithFloatVertexArray ( GL_POINTS, 
           (float*)theDrawPoint, theNumDrawPoints );
      }

      // draw the hilited vertex if there is one.
      if ( isHilitedVertexOnScreen ) {
  
  // big points size and purple color.
  glPointSize ( thePointSize * 3 );
  glColor3f ( 1.0, 0.0, 1.0 );
  
  // draw these vertices as points.
  DrawWithFloatVertexArray ( GL_POINTS, 
           (float*)theHilitedVertexDrawPoint,
           theNumHilitedPoints );
      }
    }
  }

  glFlush ();
}

void DrawWithFloatVertexArray ( GLenum inMode,
         float *inVertices, int inNumVertices ) {

  int theVertexIndex;

  // start drawing whatever it is we're drawing.
  glBegin ( inMode );

  // for each vertex...
  for ( theVertexIndex = 0; theVertexIndex < inNumVertices; theVertexIndex++ ) {

    // this is a screen point. if it's onscreen...
    if ( IsTwoDScreenPtInWindow ( 
      inVertices[2*theVertexIndex + kPointArrayIndex_X],
      inVertices[2*theVertexIndex + kPointArrayIndex_Y] ) ) {

      /*
      DebugPrint "(%2.1f, %2.1f) ", inVertices[theVertexIndex][kPointArrayIndex_X], inVertices[theVertexIndex][kPointArrayIndex_Y] EndDebugPrint;
      */
      
      // draw it.
      glVertex2fv ( &(inVertices[2*theVertexIndex + kPointArrayIndex_Beginning]) );
    }
  }

  // stop drawing.
  glEnd ();
}


// old draw_surface function, replaced by DrawSurface().
void
draw_surface(void)
{
  int k,n,ln,num;
  face_type *f;
  vertex_type *v,*lv;
  float h,xc,yc,zc,x,y,z,tx,ty,tz,lx,ly,lz,d,ld,vc[10][2];
  float theScreenX, theScreenY;
  float fsf = zf;

  if (all3flag) linewidth(surflinewidth/2.0);
  else          linewidth(surflinewidth);

  mapcolor(NUMVALS+MAPOFFSET+1,  64,  64,  64);  /* undef gray */
  mapcolor(NUMVALS+MAPOFFSET+2,   0,   0, 255);  /* fs blue */
  mapcolor(NUMVALS+MAPOFFSET+3, 220, 220,   0);  /* fs yellow */

  mapcolor(NUMVALS+MAPOFFSET+4, 255,   0,   0);  /* curv red */
  mapcolor(NUMVALS+MAPOFFSET+5,   0, 255,   0);  /* curv green */

  if (plane==CORONAL || all3flag) {

    // kt - changed ortho, we're just drawing onto a window the size
    // of our window, which gives us the same coordinate system as in
    // draw_surface().
    ortho2 ( 0.0, xdim-1, 0.0, ydim-1 );

    // get the ras coord of the slice we're on
    yc = yy0+st*imc/fsf;

    // for each face...
    for (k=0;k<mris->nfaces;k++) {
      
      // get the face
      f = &mris->faces[k];

      // get the last neighboring vertex's y distance from our slice
      ln = VERTICES_PER_FACE-1;
      ld = mris->vertices[f->v[ln]].y - yc;

      // for every vertex...
      num = 0;
      for ( n = 0; n < VERTICES_PER_FACE; n++ ) { 

        // get the face's vertex.
        v = &mris->vertices[f->v[n]];

        // get the vertex y distance from this slice
        y = v->y;
        d = y - yc;

        // if they are on opposite sides of the slice (or one is on
        // the slice) ...
        if ( d * ld <= 0 ) {

          // get the last neighboring vertex
          lv = &mris->vertices[f->v[ln]];

          // get the coords of the two vertices
          x = v->x;
          z = v->z;
          lx = lv->x;
          ly = lv->y;
          lz = lv->z;

          // if their y (height) distance is not 0, then set height
          // to distance from the slice to the last vertex y over the
          // distance from the vetex y to the last vertex y
          h = ( y - ly != 0 ) ? ( yc - ly ) / ( y - ly ) : 1.0;
          
          // get our point to draw in ras space
          tx = lx + h * ( x - lx );
          tz = lz + h * ( z - lz );

          // kt
          // convert to precise screen coords
          RASToFloatScreenXY ( tx, 0, tz, CORONAL, 
                               &theScreenX, &theScreenY );
          
          // if this is in our current screen
          if ( IsTwoDScreenPtInWindow ( theScreenX, theScreenY ) ) {

            // set our coords to draw
            if ( !all3flag ) {

              vc[num][0] = theScreenX;
              vc[num][1] = theScreenY;
            
            } else {
            
              // in in all3 view, do the necessary conversion (see 
              // draw_image() notes)
              vc[num][0] = theScreenX/2.0;
              vc[num][1] = ((float)ydim/2.0) + theScreenY/2.0;
            }

            DebugPrint "Found draw point %2.1f, %2.1f\n",
              vc[num][0], vc[num][1] EndDebugPrint;
            
            // end_kt          
            
            // choose a color...
            if (fieldsignflag && fieldsignloaded) {  /* fscontour */
              if (v->fsmask<fsthresh)     color(NUMVALS+MAPOFFSET+1);
              else if (v->fieldsign>0.0)  color(NUMVALS+MAPOFFSET+2);
              else if (v->fieldsign<0.0)  color(NUMVALS+MAPOFFSET+3);
              else                        color(NUMVALS+MAPOFFSET+1); /*nondef*/
            }
            else if (curvflag && curvloaded) {
#if 0
              if (v->curv>0.0)  color(NUMVALS+MAPOFFSET+4); /* TODO:colormap */
              else              color(NUMVALS+MAPOFFSET+5);
#else
              /* should do red-green interpolation here! */
              if (v->curv>0.0)  
                glColor3f(1.0, 0.0, 0.0);
              else              
                glColor3f(0.0, 1.0, 0.0);
#endif
            }
            
            else /* color(YELLOW); */ /* just surface */
              glColor3f(1.0,1.0,0.0);
            
            // go to next vc point
            num++;
          }
        }

        // save the last distance and vertex index
        ld = d;
        ln = n;
      }

      // if we have points to draw...
      if ( num > 0 ) {
        
        // draw the points in a line
        bgnline();
        for ( n = 0; n < num; n++ ) {

          v2f ( &vc[n][0] );
        }
        endline();
      }
    }

    for (k=0;k<npts;k++) {
      x = ptx[k]*tm[0][0]+pty[k]*tm[0][1]+ptz[k]*tm[0][2]+tm[0][3];
      y = ptx[k]*tm[1][0]+pty[k]*tm[1][1]+ptz[k]*tm[1][2]+tm[1][3];
      z = ptx[k]*tm[2][0]+pty[k]*tm[2][1]+ptz[k]*tm[2][2]+tm[2][3];
      if (fabs(y-yc)<st) {
        vc[0][0] = (xx1-x)/ps;
        vc[0][1] = ynum-((zz1-z)/ps);
        bgnpoint();
        color(GREEN);
        v2f(&vc[0][0]);
        endpoint();
      }
    }
  }

  if (plane==SAGITTAL || all3flag)
  {
    ortho2 ( 0.0, xdim-1, 0.0, ydim-1 );
    xc = xx1-ps*jc/fsf;
    for (k=0;k<mris->nfaces;k++)
    {
      f = &mris->faces[k];
      ln = VERTICES_PER_FACE-1 ;
      ld = mris->vertices[f->v[ln]].x-xc;
      num = 0;
      for (n=0;n<VERTICES_PER_FACE;n++)
      {
        v = &mris->vertices[f->v[n]];
        x = v->x;
        d = x-xc;
        if (d*ld<=0)
        {
          lv = &mris->vertices[f->v[ln]];
          y = v->y;
          z = v->z;
          lx = lv->x;
          ly = lv->y;
          lz = lv->z;
          h = (x-lx!=0)?(xc-lx)/(x-lx):1.0;
          ty = ly+h*(y-ly);
          tz = lz+h*(z-lz);

          RASToFloatScreenXY ( 0, ty, tz, SAGITTAL,
                               &theScreenX, &theScreenY );

          if ( IsTwoDScreenPtInWindow ( theScreenX, theScreenY ) ) {

            if ( !all3flag ) {
              vc[num][0] = theScreenX;
              vc[num][1] = theScreenY;
            } else {
              vc[num][0] = ((float)xdim)/2.0 + theScreenX/2.0;
              vc[num][1] = ((float)ydim)/2.0 + theScreenY/2.0;
            }
          
            if (fieldsignflag && fieldsignloaded) {  /* fscontour */
              if (v->fsmask<fsthresh)     color(NUMVALS+MAPOFFSET+1);
              else if (v->fieldsign>0.0)  color(NUMVALS+MAPOFFSET+2);
              else if (v->fieldsign<0.0)  color(NUMVALS+MAPOFFSET+3);
              else                        color(NUMVALS+MAPOFFSET+1);
            }
            else if (curvflag && curvloaded) {
              if (v->curv>0.0)  color(NUMVALS+MAPOFFSET+4);
              else              color(NUMVALS+MAPOFFSET+5);
            }
            else color(YELLOW);
            num++;
          }
        }
        ld = d;
        ln = n;
      }
      if (num>0)
      {
        bgnline();
        for (n=0;n<num;n++)
        {
          v2f(&vc[n][0]);
        }
        endline();
      }
    }
    for (k=0;k<npts;k++)
    {
      x = ptx[k]*tm[0][0]+pty[k]*tm[0][1]+ptz[k]*tm[0][2]+tm[0][3];
      y = ptx[k]*tm[1][0]+pty[k]*tm[1][1]+ptz[k]*tm[1][2]+tm[1][3];
      z = ptx[k]*tm[2][0]+pty[k]*tm[2][1]+ptz[k]*tm[2][2]+tm[2][3];
      if (fabs(x-xc)<ps)
      {
        vc[0][0] = (y-yy0)/st;
        vc[0][1] = ynum-((zz1-z)/ps);
        bgnpoint();
        color(GREEN);
        v2f(&vc[0][0]);
        endpoint();
      }
    }
  }
  if (plane==HORIZONTAL || all3flag)
  {
    ortho2 ( 0.0, xdim-1, 0.0, ydim-1 );
    zc = zz1-ps*(255.0-ic/fsf);
    for (k=0;k<mris->nfaces;k++)
    {
      f = &mris->faces[k];
      ln = VERTICES_PER_FACE-1 ;
      ld = mris->vertices[f->v[ln]].z-zc;
      num = 0;
      for (n=0;n<VERTICES_PER_FACE;n++)
      {
        v = &mris->vertices[f->v[n]];
        z = v->z;
        d = z-zc;
        if (d*ld<=0)
        {
          lv = &mris->vertices[f->v[ln]];
          x = v->x;
          y = v->y;
          lx = lv->x;
          ly = lv->y;
          lz = lv->z;
          h = (z-lz!=0)?(zc-lz)/(z-lz):1.0;
          tx = lx+h*(x-lx);
          ty = ly+h*(y-ly);

          RASToFloatScreenXY ( tx, ty, 0, HORIZONTAL,
                               &theScreenX, &theScreenY );
          if ( IsTwoDScreenPtInWindow ( theScreenX, theScreenY ) ) {
            if ( !all3flag ) {
              vc[num][0] = theScreenX;
              vc[num][1] = theScreenY;
            } else {
              vc[num][0] = theScreenX/2.0;
              vc[num][1] = theScreenY/2.0;
            }            
            
            if (fieldsignflag && fieldsignloaded) {  /* fscontour */
              if (v->fsmask<fsthresh)     color(NUMVALS+MAPOFFSET+1);
              else if (v->fieldsign>0.0)  color(NUMVALS+MAPOFFSET+2);
              else if (v->fieldsign<0.0)  color(NUMVALS+MAPOFFSET+3);
              else                        color(NUMVALS+MAPOFFSET+1);
            } else if (curvflag && curvloaded) {
              if (v->curv>0.0)  color(NUMVALS+MAPOFFSET+4);
              else              color(NUMVALS+MAPOFFSET+5);
            }
            else color(YELLOW);
            num++;
          }
        }
        ld = d;
        ln = n;
      }
      if (num>0)
      {
        bgnline();
        for (n=0;n<num;n++)
        {
          v2f(&vc[n][0]);
        }
        endline();
      }
    }
    for (k=0;k<npts;k++)
    {
      x = ptx[k]*tm[0][0]+pty[k]*tm[0][1]+ptz[k]*tm[0][2]+tm[0][3];
      y = ptx[k]*tm[1][0]+pty[k]*tm[1][1]+ptz[k]*tm[1][2]+tm[1][3];
      z = ptx[k]*tm[2][0]+pty[k]*tm[2][1]+ptz[k]*tm[2][2]+tm[2][3];
      if (fabs(z-zc)<ps)
      {
        vc[0][0] = (xx1-x)/ps;
        vc[0][1] = ((y-yy0)/st);
        bgnpoint();
        color(GREEN);
        v2f(&vc[0][0]);
        endpoint();
      }
    }
  }

  /* TODO: cursor back on top */
  if (plane==CORONAL || all3flag) {
#ifndef OPENGL  /* OPENGL: cvidbuf pixels mangled?? jc,ic / zk?? */
    k = 0;
    for (i=ic-2;i<=ic+2;i++)
    for (j=jc-2;j<=jc+2;j++) {
      if ((i==ic&&abs(j-jc)<=2)||(j==jc&&abs(i-ic)<=2))
        cvidbuf[k] = NUMVALS+MAPOFFSET;
      else
        cvidbuf[k] = vidbuf[i*xdim+j];
      /*printf("k=%d cvidbuf[k]=%d\n",k,cvidbuf[k]);*/
      k++;
    }
    rectwrite((short)(jc-2),(short)(ic-2),
              (short)(jc+2),(short)(ic+2),cvidbuf);
#endif
#if 0  /* almost */
    color(NUMVALS+MAPOFFSET);
    linewidth(1);
    fjc = jc*255.0/256.0;
    fic = ic*257.0/255.0;
    bgnline();
      vc[0][0] = (fjc-2.0)/(float)zf;
      vc[0][1] = fic/(float)zf;
      v2f(vc[0]);
      vc[1][0] = (fjc+2.0)/(float)zf;
      vc[1][1] = fic/(float)zf;
      v2f(vc[1]);
    endline();
    bgnline();
      vc[0][0] = fjc/(float)zf;
      vc[0][1] = (fic-2.0)/(float)zf;
      v2f(vc[0]);
      vc[1][0] = fjc/(float)zf;
      vc[1][1] = (fic+2.0)/(float)zf;
      v2f(vc[1]);
    endline();
#endif
    ;
  }
  if (plane==SAGITTAL) {
    ;
  }
  if (plane==HORIZONTAL) {
    ;
  }

}


void
drawpts(void)
{
  int k;
  float xc,yc,zc,x,y,z;
  float v1[2],v2[2],v3[2],v4[3];
  float ptsize2=1;

  if (plane==CORONAL)
  {
    ortho2(0.0,xnum-1,0.0,ynum+1);
    yc = yy0+st*imc/fsf;
    for (k=npts-1;k>=0;k--)
    {
      x = ptx[k]*tm[0][0] + pty[k]*tm[0][1] + ptz[k]*tm[0][2] + tm[0][3];
      y = ptx[k]*tm[1][0] + pty[k]*tm[1][1] + ptz[k]*tm[1][2] + tm[1][3];
      z = ptx[k]*tm[2][0] + pty[k]*tm[2][1] + ptz[k]*tm[2][2] + tm[2][3];
      if (maxflag || fabs(y-yc)<st) {
        v1[0] = (xx1-x)/ps-ptsize2/ps;
        v1[1] = ynum-((zz1-z)/ps)-ptsize2/ps;
        v2[0] = (xx1-x)/ps+ptsize2/ps;
        v2[1] = ynum-((zz1-z)/ps)-ptsize2/ps;
        v3[0] = (xx1-x)/ps+ptsize2/ps;
        v3[1] = ynum-((zz1-z)/ps)+ptsize2/ps;
        v4[0] = (xx1-x)/ps-ptsize2/ps;
        v4[1] = ynum-((zz1-z)/ps)+ptsize2/ps;
        bgnpolygon();
        if (k<3) color(RED);
        else     color(GREEN);
        v2f(v1);
        v2f(v2);
        v2f(v3);
        v2f(v4);
        endpolygon();
      }
    }
  }
  if (plane==SAGITTAL)
  {
    ortho2(0.0,xnum-1,0.0,ynum+1);
    xc = xx1-ps*jc/fsf;
    for (k=npts-1;k>=0;k--)
    {
      x = ptx[k]*tm[0][0]+pty[k]*tm[0][1]+ptz[k]*tm[0][2]+tm[0][3];
      y = ptx[k]*tm[1][0]+pty[k]*tm[1][1]+ptz[k]*tm[1][2]+tm[1][3];
      z = ptx[k]*tm[2][0]+pty[k]*tm[2][1]+ptz[k]*tm[2][2]+tm[2][3];
      if (maxflag || fabs(x-xc)<ps)
      {
        v1[0] = (y-yy0)/st-ptsize2/ps;
        v1[1] = ynum-((zz1-z)/ps)-ptsize2/ps;
        v2[0] = (y-yy0)/st+ptsize2/ps;
        v2[1] = ynum-((zz1-z)/ps)-ptsize2/ps;
        v3[0] = (y-yy0)/st+ptsize2/ps;
        v3[1] = ynum-((zz1-z)/ps)+ptsize2/ps;
        v4[0] = (y-yy0)/st-ptsize2/ps;
        v4[1] = ynum-((zz1-z)/ps)+ptsize2/ps;
        bgnpolygon();
        if (k<3)
          color(RED);
        else
          color(GREEN);
        v2f(v1);
        v2f(v2);
        v2f(v3);
        v2f(v4);
        endpolygon();
      }
    }
  }
  if (plane==HORIZONTAL)
  {
    ortho2(0.0,xnum-1,0.0,ynum-1);
    zc = zz1-ps*(255.0-ic/fsf);
    for (k=npts-1;k>=0;k--)
    {
      x = ptx[k]*tm[0][0]+pty[k]*tm[0][1]+ptz[k]*tm[0][2]+tm[0][3];
      y = ptx[k]*tm[1][0]+pty[k]*tm[1][1]+ptz[k]*tm[1][2]+tm[1][3];
      z = ptx[k]*tm[2][0]+pty[k]*tm[2][1]+ptz[k]*tm[2][2]+tm[2][3];
      if (maxflag || fabs(z-zc)<ps)
      {
        v1[0] = (xx1-x)/ps-ptsize2/ps;
        v1[1] = ((y-yy0)/st)-ptsize2/ps;
        v2[0] = (xx1-x)/ps+ptsize2/ps;
        v2[1] = ((y-yy0)/st)-ptsize2/ps;
        v3[0] = (xx1-x)/ps+ptsize2/ps;
        v3[1] = ((y-yy0)/st)+ptsize2/ps;
        v4[0] = (xx1-x)/ps-ptsize2/ps;
        v4[1] = ((y-yy0)/st)+ptsize2/ps;
        bgnpolygon();
        if (k<3)
          color(RED);
        else
          color(GREEN);
        v2f(v1);
        v2f(v2);
        v2f(v3);
        v2f(v4);
        endpolygon();
      }
    }
  }
}

void
write_dipoles(char *fname)
{
  int i,j,k,di,dj,dk;
  float x,y,z;
  FILE *fptr;
  int ***neighindex,index,nneighbors;

  ndip = 0;
/*
  ndip=2;
*/
  neighindex=(int ***)malloc((numimg/dip_spacing+1)*sizeof(int **));
  for (k=0;k<numimg;k+=dip_spacing)
  { 
    neighindex[k/dip_spacing]=(int **)malloc((ynum/dip_spacing+1)*sizeof(int *));
    for (i=0;i<ynum;i+=dip_spacing)
      neighindex[k/dip_spacing][i/dip_spacing]=(int *)malloc((xnum/dip_spacing+1)*
                                                             sizeof(int));
  }

  for (k=0;k<numimg;k+=dip_spacing)
  for (i=0;i<ynum;i+=dip_spacing)
  for (j=0;j<xnum;j+=dip_spacing)

  if (im[k][i][j]!=0)

/*
  if (sqrt(0.0+SQR(k-numimg/2)+SQR(i-ynum/2)+SQR(j-xnum/2))<=100)
*/
  {
    neighindex[k/dip_spacing][i/dip_spacing][j/dip_spacing] = ndip;
    ndip++;
  } else
   neighindex[k/dip_spacing][i/dip_spacing][j/dip_spacing] = -1;

  fptr = fopen(fname,"w");
  if (fptr==NULL) {printf("medit: ### can't create file %s\n",fname);PR return;}
  fprintf(fptr,"#!ascii\n");
  fprintf(fptr,"%d\n",ndip);
/*
  fprintf(fptr,"%6.2f %6.2f %6.2f %6.2f %6.2f %6.2f\n",
          30.0,-75.0,40.0,0.0,0.0,0.0);
  fprintf(fptr,"%6.2f %6.2f %6.2f %6.2f %6.2f %6.2f\n",
          45.0,10.0,70.0,0.0,0.0,0.0);
*/
  for (k=0;k<numimg;k+=dip_spacing)
  for (i=0;i<ynum;i+=dip_spacing)
  for (j=0;j<xnum;j+=dip_spacing)
  if (neighindex[(k)/dip_spacing][(i)/dip_spacing][(j)/dip_spacing]>=0)
  {
    x = xx1-ps*j;
    y = yy0+st*k;
    z = zz1-ps*i;
    fprintf(fptr,"%6.2f %6.2f %6.2f %6.2f %6.2f %6.2f\n",x,y,z,0.0,0.0,0.0);
  }
/*
  fprintf(fptr,"%d \n",0);
  fprintf(fptr,"%d \n",0);
*/
  for (k=0;k<numimg;k+=dip_spacing)
  for (i=0;i<ynum;i+=dip_spacing)
  for (j=0;j<xnum;j+=dip_spacing)
  if (neighindex[(k)/dip_spacing][(i)/dip_spacing][(j)/dip_spacing]>=0)
  {
    nneighbors = 0;
    for (dk= -dip_spacing;dk<=dip_spacing;dk+=2*dip_spacing)
      if (dk>0)
      if (neighindex[(k+dk)/dip_spacing][(i)/dip_spacing][(j)/dip_spacing]>=0)
        nneighbors++;
    for (di= -dip_spacing;di<=dip_spacing;di+=2*dip_spacing)
      if (di>0)
      if (neighindex[(k)/dip_spacing][(i+di)/dip_spacing][(j)/dip_spacing]>=0)
        nneighbors++;
    for (dj= -dip_spacing;dj<=dip_spacing;dj+=2*dip_spacing)
      if (dj>0)
      if (neighindex[(k)/dip_spacing][(i)/dip_spacing][(j+dj)/dip_spacing]>=0)
        nneighbors++;
    fprintf(fptr,"%d ",nneighbors);
    for (dk= -dip_spacing;dk<=dip_spacing;dk+=2*dip_spacing)
      if (dk>0)
      if ((index=neighindex[(k+dk)/dip_spacing][(i)/dip_spacing][(j)/dip_spacing])>=0)
        fprintf(fptr,"%d ",index);
    for (di= -dip_spacing;di<=dip_spacing;di+=2*dip_spacing)
      if (di>0)
      if ((index=neighindex[(k)/dip_spacing][(i+di)/dip_spacing][(j)/dip_spacing])>=0)
        fprintf(fptr,"%d ",index);
    for (dj= -dip_spacing;dj<=dip_spacing;dj+=2*dip_spacing)
      if (dj>0)
      if ((index=neighindex[(k)/dip_spacing][(i)/dip_spacing][(j+dj)/dip_spacing])>=0)
        fprintf(fptr,"%d ",index);
    fprintf(fptr,"\n");
  }
  fclose(fptr);
  printf("medit: dipole file %s written\n",fname);PR
}

void
write_decimation(char *fname)
{
  int k;
  FILE *fptr;

  fptr = fopen(fname,"w");
  if (fptr==NULL) {printf("medit: ### can't create file %s\n",fname);PR return;}
  fprintf(fptr,"#!ascii\n");
  fprintf(fptr,"%d\n",ndip);
  for (k=0;k<ndip;k++)
  {
    fprintf(fptr,"1\n");
  }
  fclose(fptr);
  printf("medit: decimation file %s written\n",fname);PR
}

void
read_hpts(char *fname)      /* sprintf(fname,"%s","../bem/head2mri.hpts"); */
{
  FILE *fp;
  char line[NAME_LENGTH];
  int i;


  fp = fopen(fname,"r");
  if (fp==NULL) {printf("medit: ### File %s not found\n",fname);PR return;}

  ptsflag = TRUE;
  fgets(line,NAME_LENGTH,fp);
  i = 0;
  while (!feof(fp))
  {
    jold[i] = iold[i] = imold[i] = 0;
    if (line[0]=='2') /* .show format? */
      sscanf(line,"%*s%*s%*s%*s%*s%*s%*s%f%*s%f%*s%f",
             &ptx[i],&pty[i],&ptz[i]);
    else /* .apos format */
    {
      sscanf(line,"%*s%*s%f%f%f",&ptx[i],&pty[i],&ptz[i]);
      ptx[i] *= 1000; /* convert from m to mm */
      pty[i] *= 1000;
      ptz[i] *= 1000;
    }
    fgets(line,NAME_LENGTH,fp);
    i++;
  }
  printf("head points file %s read\n",fname);
  npts = i;
  fclose(fp);
}

void
read_htrans(char *fname)   /* sprintf(fname,"%s","../bem/head2mri.trans"); */
{
  int i,j,k;
  FILE *fp;
  fp = fopen(fname,"r");
  if (fp==NULL) {
    printf("medit: no existing htrans file: making default\n");PR
    for (i=0;i<4;i++)
    for (j=0;j<4;j++)
      tm[i][j] = (i==j);
  }
  else {
    for (i=0;i<4;i++)
    for (j=0;j<4;j++)
      fscanf(fp,"%f",&tm[i][j]);
    printf("transformation file %s read\n",fname);PR
  }
  for (k=0;k<MAXPARS;k++)
    par[k] = dpar[k] = 0;
}

void
write_htrans(char *fname)   /* sprintf(fname,"%s","../bem/head2mri.trans"); */
{
  int i,j;
  FILE *fptr;

  fptr = fopen(fname,"w");
  if (fptr==NULL) {printf("medit: can't create file %s\n",fname);return;}
  for (i=0;i<4;i++) {
    for (j=0;j<4;j++)
      fprintf(fptr,"%13.6e ",tm[i][j]);
    fprintf(fptr,"\n");
  }
  fclose(fptr);
  printf("transformation file %s written\n",fname);
}

void
make_filenames(char *lsubjectsdir,char *lsrname, char *lpname, char *limdir,
               char *lsurface)
{
  char tmpsurface[NAME_LENGTH], surfhemi[NAME_LENGTH];
    
  subjectsdir = (char *)malloc(NAME_LENGTH*sizeof(char)); /* malloc for tcl */
  srname = (char *)malloc(NAME_LENGTH*sizeof(char));
  pname = (char *)malloc(NAME_LENGTH*sizeof(char));
  title_str = (char *)malloc(NAME_LENGTH*sizeof(char));
  imtype = (char *)malloc(NAME_LENGTH*sizeof(char));
  imtype2 = (char *)malloc(NAME_LENGTH*sizeof(char));
  surface = (char *)malloc(NAME_LENGTH*sizeof(char));
  mfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  sfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  tfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  dipfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  decfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  hpfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  htfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  sgfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  fsfname = (char *)malloc(NAME_LENGTH*sizeof(char)); /* fscontour */
  fmfname = (char *)malloc(NAME_LENGTH*sizeof(char)); /* fscontour */
  cfname = (char *)malloc(NAME_LENGTH*sizeof(char));
  xffname = (char *)malloc(NAME_LENGTH*sizeof(char));
  rfname = (char *)malloc(NAME_LENGTH*sizeof(char));

  strcpy(subjectsdir,lsubjectsdir);
  strcpy(srname,lsrname);
  strcpy(pname,lpname);
  strcpy(imtype,limdir);
  strcpy(surface,lsurface);
  sprintf(mfname,"%s/%s/mri/%s/COR-",subjectsdir,pname,imtype);
  sprintf(sfname,"%s/%s/surf/%s",subjectsdir,pname,surface);
  sprintf(tfname,"%s/%s/%s",subjectsdir,pname,TMP_DIR);
  /* next relative to session */
  sprintf(dipfname,"%s/bem/brain3d%d.dip",srname,dip_spacing);
  sprintf(decfname,"%s/bem/brain3d%d.dec",srname,dip_spacing);
  sprintf(hpfname,"%s/bem/head2mri.hpts",srname);
  sprintf(htfname,"%s/bem/head2mri.trans",srname);
  sprintf(sgfname,"/tmp/tkmedit.rgb");
  strcpy(tmpsurface,surface);  /* strtok writes in arg it parses! */
  strcpy(surfhemi,strtok(tmpsurface,"."));
  sprintf(fsfname,"%s/fs/%s.fs",srname,surfhemi);
  sprintf(fmfname,"%s/fs/%s.fm",srname,surfhemi);
  sprintf(cfname,"%s/%s/surf/%s.curv",subjectsdir,pname,surfhemi);
  sprintf(xffname,"%s/%s/%s/%s",
                     subjectsdir,pname,TRANSFORM_DIR,TALAIRACH_FNAME);
}

void
mirror(void)
{
  flip_corview_xyz("-x","y","z");
}

void
read_fieldsign(char *fname)  /* fscontour */
{
  int k,vnum;
  float f;
  FILE *fp;

  if (!surfloaded) {printf("medit: ### surface %s not loaded\n",fname);PR return;}

  fp = fopen(fname,"r");
  if (fp==NULL) {printf("medit: ### File %s not found\n",fname);PR return;}
  fread(&vnum,1,sizeof(int),fp);
  printf("medit: mris->nvertices = %d, vnum = %d\n",mris->nvertices,vnum);
  if (vnum!=mris->nvertices) {
    printf("medit: ### incompatible vertex number in file %s\n",fname);PR
    return; }
  for (k=0;k<vnum;k++)
  {
    fread(&f,1,sizeof(float),fp);
    mris->vertices[k].fieldsign = f;
  }
  fclose(fp);
  fieldsignloaded = TRUE;
  fieldsignflag = TRUE;
  surflinewidth = 3;
  PR
}

void
read_fsmask(char *fname)  /* fscontour */
{
  int k,vnum;
  float f;
  FILE *fp;

  if (!surfloaded) {printf("medit: ### surface %s not loaded\n",fname);PR return;}

  printf("medit: read_fsmask(%s)\n",fname);
  fp = fopen(fname,"r");
  if (fp==NULL) {printf("medit: ### File %s not found\n",fname);PR return;}
  fread(&vnum,1,sizeof(int),fp);
  if (vnum!=mris->nvertices) {
    printf("medit: ### incompatible vertex number in file %s\n",fname);PR
    return; }
  for (k=0;k<vnum;k++)
  {
    fread(&f,1,sizeof(float),fp);
    mris->vertices[k].fsmask = f;
  }
  fclose(fp);
  PR
}


void
read_binary_curvature(char *fname)
{
  float curvmin, curvmax, curv;
  int   k;

  MRISreadCurvatureFile(mris, fname) ;

  curvmin= 1000000.0f ; curvmax = -curvmin;
  for (k=0;k<mris->nvertices;k++)
  {
    curv = mris->vertices[k].curv;
    if (curv>curvmax) curvmax=curv;
    if (curv<curvmin) curvmin=curv;
  }
  printf("medit: curvature read: min=%f max=%f\n",curvmin,curvmax);PR
  curvloaded = TRUE;
  curvflag = TRUE;
}

void
smooth_3d(int niter)
{
  int iter,i,j,k;
  int i2,j2,k2;
  int n;
  float sum;

  if (!dummy_im_allocated) {
    for (k=0;k<numimg;k++) {
      dummy_im[k] = (unsigned char **)lcalloc(ynum,sizeof(char *));
      for (i=0;i<ynum;i++) {
        dummy_im[k][i] = (unsigned char *)lcalloc(xnum,sizeof(char));
      }
    }
    dummy_im_allocated = TRUE;
  }

  for (iter=0;iter<niter;iter++) {  /* smooth */
    for (j=1;j<xnum-1;j++) {
      printf(".");
      for (k=1;k<numimg-1;k++)
      for (i=1;i<ynum-1;i++) {
        sum = 0;
        n = 0;
        for(k2=k-1;k2<k+2;k2++)  /* 27 */
        for(i2=i-1;i2<i+2;i2++)
        for(j2=j-1;j2<j+2;j2++) {
          sum += im[k2][i2][j2];
          n++;
        }
        /*dummy_im[k][i][j] = (sum+im[k][i][j])/(float)(n+1);*/
        dummy_im[k][i][j] = (sum + 27*im[k][i][j])/(float)(n+27);
      }
      fflush(stdout);
    }
  }

  /* update view */
  for (k=0;k<numimg;k++)
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
    im[k][i][j] = dummy_im[k][i][j];
  for (k=0;k<numimg;k++)
    changed[k] = TRUE;
  editedimage = TRUE;

  for (k=0;k<6;k++)
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
    sim[k][i][j] = 0;
  for (k=0;k<numimg;k++)
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++) {
    if (im[k][i][j]/2>sim[3][i][j]) sim[3][i][j] = im[k][i][j]/2;
    if (im[k][i][j]/2>sim[4][k][j]) sim[4][k][j] = im[k][i][j]/2;
    if (im[k][i][j]/2>sim[5][i][k]) sim[5][i][k] = im[k][i][j]/2;
  }
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
  for (k=0;k<3;k++)
    sim[k][i][j] = sim[k+3][i][j];

  printf("\nmedit: finished %d smooth_3d steps\n",niter);PR
}

  /* args: [-]{x,y,z} [-]{x,y,z} [-]{x,y,z} */
void
flip_corview_xyz(char *newx, char *newy, char *newz)
{
  int fx,fy,fz;
  int lx,ly,lz;
  int xx,xy,xz,yx,yy,yz,zx,zy,zz;
  int i,j,k;
  int ni,nj,nk;

  ni = nj = nk = 0;
  fx=(newx[0]=='-')?1:0;
  fy=(newy[0]=='-')?1:0;
  fz=(newz[0]=='-')?1:0;
  lx = strlen(newx)-1;
  ly = strlen(newy)-1;
  lz = strlen(newz)-1;
  if(newx[lx]==newy[ly] || newx[lx]==newz[lz] || newy[ly]==newz[lz]) {
    printf("medit: ### degenerate flip\n");PR return; }
  xx=(newx[lx]=='x')?1:0; xy=(newx[lx]=='y')?1:0; xz=(newx[lx]=='z')?1:0;
  yx=(newy[ly]=='x')?1:0; yy=(newy[ly]=='y')?1:0; yz=(newy[ly]=='z')?1:0;
  zx=(newz[lz]=='x')?1:0; zy=(newz[lz]=='y')?1:0; zz=(newz[lz]=='z')?1:0;

  if (!dummy_im_allocated) {
    for (k=0;k<numimg;k++) {
      dummy_im[k] = (unsigned char **)lcalloc(ynum,sizeof(char *));
      for (i=0;i<ynum;i++) {
        dummy_im[k][i] = (unsigned char *)lcalloc(xnum,sizeof(char));
      }
    }
    dummy_im_allocated = TRUE;
  }

  for (k=0;k<numimg;k++) {
    printf(".");
    for (i=0;i<ynum;i++)
    for (j=0;j<xnum;j++) {
      if(xx) nj=j; if(xy) nj=i; if(xz) nj=k;
      if(yx) ni=j; if(yy) ni=i; if(yz) ni=k;
      if(zx) nk=j; if(zy) nk=i; if(zz) nk=k;
      if(fx) nj=255-nj;
      if(fy) ni=255-ni;
      if(fz) nk=255-nk;
      dummy_im[k][i][j] = im[nk][ni][nj];
    }
    fflush(stdout);
  }
  printf("\n");

  /* update view */
  for (k=0;k<numimg;k++)
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
    im[k][i][j] = dummy_im[k][i][j];
  for (k=0;k<numimg;k++)
    changed[k] = TRUE;
  editedimage = TRUE;

  for (k=0;k<6;k++)
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
    sim[k][i][j] = 0;
  for (k=0;k<numimg;k++)
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++) {
    if (im[k][i][j]/2>sim[3][i][j]) sim[3][i][j] = im[k][i][j]/2;
    if (im[k][i][j]/2>sim[4][k][j]) sim[4][k][j] = im[k][i][j]/2;
    if (im[k][i][j]/2>sim[5][i][k]) sim[5][i][k] = im[k][i][j]/2;
  }
  for (i=0;i<ynum;i++)
  for (j=0;j<xnum;j++)
  for (k=0;k<3;k++)
    sim[k][i][j] = sim[k+3][i][j];

  printf("medit: imageflip done--to overwrite current COR: write_images\n");PR
  redraw();
}

void
wmfilter_corslice(int imc)
{
  char *getenv(),*mri_dir,fname[NAME_LENGTH];
  int nver,ncor;
  int i,j,k,k2,di,dj,dk,m,n,u,ws2=2,maxi,mini;
  float xcor[MAXCOR],ycor[MAXCOR],zcor[MAXCOR];
  float fwhite_hilim=140,fwhite_lolim=80,fgray_hilim=100;
  float numvox,numnz,numz;
  float f,f2,a,b,c,x,y,z,s;
  float cfracz = 0.6;
  float cfracnz = 0.6;
  double sum2,sum,var,avg,tvar,maxvar,minvar;
  double sum2v[MAXLEN],sumv[MAXLEN],avgv[MAXLEN],varv[MAXLEN],nv[MAXLEN];
  FILE *fptr;

  k2 = imc/zf;
  if (plane!=CORONAL) {
    printf("medit: ### can only wmfilter CORONAL slice\n");PR return; }
  if(k2<ws2 || k2>numimg-ws2) {
    printf("medit: ### slice too close to edge\n");PR return; }
  mri_dir = getenv("MRI_DIR");
  if (mri_dir==NULL) {
    printf("medit: ### env var MRI_DIR undefined (setenv, restart)\n");return;}
  if (!flossflag && !spackleflag) { 
    printf("medit: ### no spackle or floss  ...skipping wmfilter\n"); return; }

  sprintf(fname,"%s/lib/bem/%s",mri_dir,DIR_FILE);
  fptr = fopen(fname,"r");
  if (fptr==NULL) {printf("medit: ### File %s not found\n",fname);PR return;}
  fscanf(fptr,"%d",&nver);
  for (i=0,ncor=0;i<nver;i++) {
    fscanf(fptr,"%*d %f %f %f",&x,&y,&z);
    for (j=0;j<ncor;j++)
      if ((x==-xcor[j]) && (y==-ycor[j]) && (z==-zcor[j])) goto L1;
    xcor[ncor] = x;
    ycor[ncor] = y;
    zcor[ncor] = z;
    ncor++;
  L1:;
  }
  fwhite_hilim = (float)white_hilim;
  fwhite_lolim = (float)white_lolim;
  fgray_hilim = (float)gray_hilim;

  if (!wmfilter_ims_allocated) {
    for (k=0;k<numimg;k++) {
      fill[k] = (unsigned char **)lcalloc(ynum,sizeof(char *));
      im_b[k] = (unsigned char **)lcalloc(ynum,sizeof(char *));
      for (i=0;i<ynum;i++) {
        fill[k][i] = (unsigned char *)lcalloc(xnum,sizeof(char));
        im_b[k][i] = (unsigned char *)lcalloc(xnum,sizeof(char));
      }
    }
    wmfilter_ims_allocated = TRUE;
  }

  if (!second_im_allocated)
    alloc_second_im();

  /*for (k=0;k<numimg-1;k++)*/
  for (k=k2-ws2;k<=k2+ws2;k++)
  for (i=0;i<ynum-1;i++)
  for (j=0;j<xnum-1;j++) {
    im_b[k][i][j] = im[k][i][j];
    im2[k][i][j] = im[k][i][j];
    if (im_b[k][i][j]>fwhite_hilim || im_b[k][i][j]<fwhite_lolim)
      im_b[k][i][j] = 0;
    fill[k][i][j] = im_b[k][i][j];
  }

  k = k2;
  printf("medit: %d unique orientations\n",ncor);PR
  printf("medit: white_lolim = %d   white_hilim = %d   gray_hilim = %d\n",
                 white_lolim,white_hilim,gray_hilim);PR
  printf("medit: wmfiltering coronal slice %d\n",k2);
  if (flossflag)   printf("medit:   floss => '.'\n");
  if (spackleflag) printf("medit:   spackle => '#'\n");
  printf("medit:    ...\n"); 

  for (i=ws2;i<ynum-1-ws2;i++)
  for (j=ws2;j<xnum-1-ws2;j++) {
    numvox = numnz = numz = 0;
    for (dk = -ws2;dk<=ws2;dk++)
    for (di = -ws2;di<=ws2;di++)
    for (dj = -ws2;dj<=ws2;dj++) {
      f = im_b[k+dk][i+di][j+dj];
      s = dk*c+di*b+dj*a;
      numvox++;
      if (f!=0) numnz++;
      else      numz++;
    }
    if ((im_b[k][i][j]==0 && numnz>=cfracnz*SQR(2*ws2+1)) ||
        (im_b[k][i][j]!=0 && numz>=cfracz*SQR(2*ws2+1))) {
      maxvar = -1000000;
      minvar = 1000000;
      maxi = mini = -1;
      for (m=0;m<ncor;m++) {
        a = xcor[m];
        b = ycor[m];
        c = zcor[m];
        sum = sum2 = n = 0;
        for (u=0;u<2*ws2+1;u++)
          sumv[u] = sum2v[u] = nv[u] = 0;
        for (dk = -ws2;dk<=ws2;dk++)
        for (di = -ws2;di<=ws2;di++)
        for (dj = -ws2;dj<=ws2;dj++) {
          u = ws2+floor(dk*c+di*b+dj*a+0.5);
          u = (u<0)?0:(u>2*ws2+1-1)?2*ws2+1-1:u;
          f = im_b[k+dk][i+di][j+dj];
          sum2v[u] += f*f;
          sumv[u] += f;
          nv[u] += 1;
          sum2 += f*f;
          sum += f;
          n += 1;
        }
        avg = sum/n;
        var = sum2/n-avg*avg;
        tvar = 0;
        for (u=0;u<2*ws2+1;u++) {
          avgv[u] = sumv[u]/nv[u];
          varv[u] = sum2v[u]/nv[u]-avgv[u]*avgv[u];
          tvar += varv[u];
        }
        tvar /= (2*ws2+1);
        if (tvar>maxvar) {maxvar=tvar;maxi=m;}
        if (tvar<minvar) {minvar=tvar;mini=m;}
      }
      a = xcor[mini];
      b = ycor[mini];
      c = zcor[mini];

      numvox = numnz = numz = sum = sum2 = 0;
      for (dk = -ws2;dk<=ws2;dk++)
      for (di = -ws2;di<=ws2;di++)
      for (dj = -ws2;dj<=ws2;dj++) {
        f = im_b[k+dk][i+di][j+dj];
        f2 = im2[k+dk][i+di][j+dj];
        s = dk*c+di*b+dj*a;
        if (fabs(s)<=0.5) {
          numvox++;
          sum2 += f2;
          if (f!=0) { numnz++; sum += f; }
          else      { numz++; }
        }
      }
      if (numnz!=0) sum /= numnz;
      if (numvox!=0) sum2 /= numvox;
      f = im_b[k][i][j];
      f2 = im2[k][i][j];
      if (flossflag && f!=0 && numz/numvox>cfracz && f<=fgray_hilim) {
        f=0; printf("."); }
      else if (spackleflag && f==0 && numnz/numvox>cfracnz) {
        f=sum; printf("#"); }
      fill[k][i][j] = f;
    }
  }
  printf("\n");PR
  /*for (k=0;k<numimg-1;k++)*/
  k = k2;
  for (i=0;i<ynum-1;i++)
  for (j=0;j<xnum-1;j++) {
    im_b[k][i][j] = fill[k][i][j];
    if (im_b[k][i][j]>fwhite_hilim || im_b[k][i][j]<fwhite_lolim)
      im_b[k][i][j]=0;
  }

  /*for (k=0;k<numimg-1;k++)*/
  k = k2;
  for (k=k2-ws2;k<=k2+ws2;k++)
  for (i=0;i<ynum-1;i++)
  for (j=0;j<xnum-1;j++) {
    if (k==k2) im2[k][i][j] = im_b[k][i][j];
    else       im2[k][i][j] = 0;
  }
  printf("medit: wmfiltered slice put in 2nd image set (can't be saved)\n");PR
  drawsecondflag = TRUE;
  redraw();
}

void
norm_allslices(int normdir)
{
  int i,j,k;
  int x,y;
  float imf[256][256];
  float flim0,flim1,flim2,flim3;

  x = y = 0;
  if ((plane==CORONAL && normdir==POSTANT) ||
      (plane==SAGITTAL && normdir==RIGHTLEFT) ||
      (plane==HORIZONTAL && normdir==INFSUP)) {
    printf("medit: ### N.B.: norm gradient not visible in this view\n");
  }
  printf("medit: normalizing all slices...\n");
  flim0 = (float)lim0;
  flim1 = (float)lim1;
  flim2 = (float)lim2;
  flim3 = (float)lim3;
  for (k=0;k<numimg;k++) {
    printf(".");
    for (i=0;i<256;i++)
    for (j=0;j<256;j++) {
      if (normdir==POSTANT)   {x=i; y=255-k;}  /* SAG:x */
      if (normdir==INFSUP)    {x=j;     y=i;}  /* COR:y */
      if (normdir==RIGHTLEFT) {x=k; y=255-j;}  /* HOR:x */
      imf[y][x] = im[k][i][j];
      if ((255-y)<=flim0)
        imf[y][x]*=ffrac0;
      else if ((255-y)<=flim1)
        imf[y][x]*=(ffrac0+((255-y)-flim0)*(ffrac1-ffrac0)/(flim1-flim0));
      else if ((255-y)<=flim2)
        imf[y][x]*=(ffrac1+((255-y)-flim1)*(ffrac2-ffrac1)/(flim2-flim1));
      else if ((255-y)<=flim3)
        imf[y][x]*=(ffrac2+((255-y)-flim2)*(ffrac3-ffrac2)/(flim3-flim2));
      else
        imf[y][x]*=ffrac3;
      if (imf[y][x]>255) imf[y][x]=255;
      im[k][i][j] = floor(imf[y][x]+0.5);
    }
    fflush(stdout);
  }
  printf("\n");
  printf("medit: done (to undo: quit w/o SAVEIMG)\n");PR
  for (k=0;k<numimg;k++)
    changed[k] = TRUE;
  editedimage = TRUE;
  redraw();
}

void norm_slice(int imc, int ic,int jc, int normdir)
{
  int k,i,j;
  int x,y;
  int k0,k1,i0,i1,j0,j1;
  int imc0,ic0,jc0;
  float imf[256][256];
  float flim0,flim1,flim2,flim3;

  x = y = 0;
  k0 = k1 = i0 = i1 = j0 = j1 = 0;
  if ((plane==CORONAL && normdir==POSTANT) ||
      (plane==SAGITTAL && normdir==RIGHTLEFT) ||
      (plane==HORIZONTAL && normdir==INFSUP)) {
    printf("medit: ### N.B.: norm gradient not visible in this view\n");
  }

  if (!second_im_allocated)
    alloc_second_im();

  flim0 = (float)lim0;
  flim1 = (float)lim1;
  flim2 = (float)lim2;
  flim3 = (float)lim3;
  imc0 = imc/zf;
  ic0 = (ydim-1-ic)/zf;
  jc0 = jc/zf;
  if (plane==CORONAL)   {k0=imc0; k1=imc0+1; i0=0;   i1=256;   j0=0;  j1=256;}
  if (plane==SAGITTAL)  {k0=0;    k1=numimg; i0=0;   i1=256;   j0=jc0;j1=jc0+1;}
  if (plane==HORIZONTAL){k0=0;    k1=numimg; i0=ic0; i1=ic0+1; j0=0;  j1=256;}
  /*printf("k0=%d k1=%d i0=%d i1=%d j0=%d j1=%d\n",k0,k1,i0,i1,j0,j1);*/
  for (k=k0;k<k1;k++)
  for (i=i0;i<i1;i++)
  for (j=j0;j<j1;j++) {
    if (normdir==POSTANT)   {x=i; y=255-k;}  /* SAG:x */
    if (normdir==INFSUP)    {x=j;     y=i;}  /* COR:y */
    if (normdir==RIGHTLEFT) {x=k; y=255-j;}  /* HOR:x */
    imf[y][x] = im[k][i][j];
    if ((255-y)<=flim0)
      imf[y][x]*=ffrac0;
    else if ((255-y)<=flim1)
      imf[y][x]*=(ffrac0+((255-y)-flim0)*(ffrac1-ffrac0)/(flim1-flim0));
    else if ((255-y)<=flim2)
      imf[y][x]*=(ffrac1+((255-y)-flim1)*(ffrac2-ffrac1)/(flim2-flim1));
    else if ((255-y)<=flim3)
      imf[y][x]*=(ffrac2+((255-y)-flim2)*(ffrac3-ffrac2)/(flim3-flim2));
    else
      imf[y][x]*=ffrac3;
    if (imf[y][x]>255) imf[y][x]=255;
    im2[k][i][j] = floor(imf[y][x]+0.5);
  }
  printf("medit: normalized slice put in 2nd image set (can't be saved)\n");PR
  drawsecondflag = TRUE;
  redraw();
}

void
alloc_second_im(void)
{
  int i,k;

  for (k=0;k<numimg;k++) {
    im2[k] = (unsigned char **)lcalloc(ynum,sizeof(char *));
    for (i=0;i<ynum;i++) {
      im2[k][i] = (unsigned char *)lcalloc(xnum,sizeof(char));
    }
  }
  for (k=0;k<6;k++) {
    sim2[k] = (unsigned char **)lcalloc(ynum,sizeof(char *));
    for (i=0;i<ynum;i++) {
      sim2[k][i] = (unsigned char *)lcalloc(xnum,sizeof(char));
    }
  }
  second_im_allocated = TRUE;
}




/*----------------end medit.c -------------------------------*/

/* %%%%Sean %%% */


/* boilerplate wrap function defines for easier viewing */
#define WBEGIN (ClientData clientData,Tcl_Interp *interp,int argc,char *argv[]){
#define ERR(N,S)  if(argc!=N){interp->result=S;return TCL_ERROR;}
#define WEND   return TCL_OK;}
#define REND  (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL

/*=======================================================================*/
/* function wrappers and errors */
int                  W_open_window  WBEGIN
  ERR(1,"Wrong # args: open_window")
                       open_window(pname);  WEND

int                  W_resize_window_intstep  WBEGIN
  ERR(2,"Wrong # args: resize_window_intstep <mag=1,2,3>")
                       resize_window_intstep(atoi(argv[1]));  WEND

int                  W_move_window  WBEGIN
  ERR(3,"Wrong # args: move_window <x> <y>")
                       move_window(atoi(argv[1]),atoi(argv[2]));  WEND

int                  W_pop_gl_window WBEGIN
  ERR(1,"Wrong # args: pop_gl_window")
                       pop_gl_window();  WEND

int                  W_redraw  WBEGIN 
  ERR(1,"Wrong # args: redraw")
                       redraw();  WEND

int                  W_upslice WBEGIN 
  ERR(1,"Wrong # args: upslice")
                       upslice();  WEND

int                  W_downslice WBEGIN 
  ERR(1,"Wrong # args: downslice")
                       downslice();  WEND

int                  W_rotate_brain_x  WBEGIN
  ERR(2,"Wrong # args: rotate_brain_x <deg>")
                       rotate_brain(atof(argv[1]),'x'); WEND

int                  W_rotate_brain_y  WBEGIN
  ERR(2,"Wrong # args: rotate_brain_y <deg>")
                       rotate_brain(atof(argv[1]),'y'); WEND

int                  W_rotate_brain_z  WBEGIN
  ERR(2,"Wrong # args: rotate_brain_z <deg>")
                       rotate_brain(atof(argv[1]),'z'); WEND

int                  W_translate_brain_x  WBEGIN
  ERR(2,"Wrong # args: translate_brain_x <mm>")
                       translate_brain(atof(argv[1]),'x');  WEND

int                  W_translate_brain_y  WBEGIN
  ERR(2,"Wrong # args: translate_brain_y <mm>")
                       translate_brain(atof(argv[1]),'y');  WEND

int                  W_translate_brain_z  WBEGIN
  ERR(2,"Wrong # args: translate_brain_z <mm>")
                       translate_brain(atof(argv[1]),'z');  WEND

int                  W_write_images  WBEGIN
  ERR(1,"Wrong # args: write_images")
                       write_images(mfname); WEND

int                  W_write_dipoles  WBEGIN
  ERR(1,"Wrong # args: write_dipoles")
                       write_dipoles(dipfname); WEND

int                  W_write_decimation  WBEGIN
  ERR(1,"Wrong # args: write_decimation")
                       write_decimation(decfname); WEND

int                  W_goto_vertex  WBEGIN 
  ERR(2,"Wrong # args: goto_vertex")
                       goto_vertex(atoi(argv[1]));  WEND

                       // kt - added corresponding orig and canon funcs
int                  W_goto_orig_vertex  WBEGIN 
  ERR(2,"Wrong # args: goto_orig_vertex")
                       goto_orig_vertex(atoi(argv[1]));  WEND

int                  W_goto_canon_vertex  WBEGIN 
  ERR(2,"Wrong # args: goto_canon_vertex")
                       goto_canon_vertex(atoi(argv[1]));  WEND

int                  W_select_control_points  WBEGIN 
  ERR(1,"Wrong # args: select_control_points")
                       select_control_points();  WEND

int                  W_reset_control_points  WBEGIN 
  ERR(1,"Wrong # args: reset_control_points")
                       reset_control_points();  WEND

int                  W_mark_file_vertices  WBEGIN 
  ERR(2,"Wrong # args: mark_file_vertices")
                       mark_file_vertices(argv[1]);  WEND

int                  W_unmark_vertices  WBEGIN 
  ERR(1,"Wrong # args: unmark_vertices")
                       unmark_vertices();  WEND

int                  W_goto_point  WBEGIN
  ERR(1,"Wrong # args: goto_point")
                       goto_point(tfname); WEND

int                  W_goto_point_coords  WBEGIN
  ERR(4,"Wrong # args: goto_point_coords <inmr*zf> <i*zf> <j*zf>")
                       goto_point_coords((int)atof(argv[1]),(int)atof(argv[2]),
                                         (int)atof(argv[3])); WEND
int                  W_write_point  WBEGIN
  ERR(1,"Wrong # args: write_point")
                       write_point(tfname); WEND

int                  W_coords_to_talairach  WBEGIN
  ERR(1,"Wrong # args: coords_to_talairach")
                       coords_to_talairach(); WEND

int                  W_talairach_to_coords  WBEGIN
  ERR(1,"Wrong # args: talairach_to_coords")
                       talairach_to_coords(); WEND

int                  W_read_hpts  WBEGIN
  ERR(1,"Wrong # args: read_hpts")
                       read_hpts(hpfname); WEND

int                  W_read_htrans  WBEGIN
  ERR(1,"Wrong # args: read_htrans")
                       read_htrans(htfname); WEND

int                  W_write_htrans  WBEGIN
  ERR(1,"Wrong # args: write_htrans")
                       write_htrans(htfname); WEND

int                  W_save_rgb  WBEGIN
  ERR(1,"Wrong # args: save_rgb")
                       save_rgb(sgfname);  WEND

int                  W_set_scale  WBEGIN
  ERR(1,"Wrong # args: set_scale")
                       set_scale();  WEND

int                  W_mirror  WBEGIN
  ERR(1,"Wrong # args: mirror")
                       mirror();  WEND

int                  W_read_fieldsign  WBEGIN
  ERR(1,"Wrong # args: read_fieldsign")
                       read_fieldsign(fsfname);  WEND

int                  W_read_fsmask WBEGIN
  ERR(1,"Wrong # args: read_fsmask")
                       read_fsmask(fmfname);  WEND

int                  W_read_binary_curv  WBEGIN 
  ERR(1,"Wrong # args: read_binary_curv")
                       read_binary_curvature(cfname);  WEND

int                  W_read_curv  WBEGIN 
  ERR(2,"Wrong # args: read_curv")
                       read_binary_curvature(argv[1]);  WEND

int                  W_smooth_3d  WBEGIN
  ERR(2,"Wrong # args: smooth_3d <steps>")
                       smooth_3d(atoi(argv[1]));  WEND

int                  W_flip_corview_xyz  WBEGIN
  ERR(4,"Wrong # args: flip_corview_xyz <[-]{x,y,z}> <[-]{x,y,z}> <[-]{x,y,z}>")
                       flip_corview_xyz(argv[1],argv[2],argv[3]);  WEND

int                  W_read_second_images  WBEGIN
  ERR(2,"Wrong # args: read_second_images <imtype2>     [T1,wm,filled,other]")
                       read_second_images(argv[1]);  WEND

int                  W_read_binary_surf  WBEGIN
  ERR(1,"Wrong # args: read_binary_surf")
                       read_binary_surface(sfname); WEND

int                  W_show_vertex  WBEGIN
  ERR(1,"Wrong # args: show_vertex")
                       show_vertex(); WEND

                       // kt - added corresponding orig and canon funcs
int                  W_show_orig_vertex  WBEGIN
  ERR(1,"Wrong # args: show_orig_vertex")
                       show_orig_vertex(); WEND

int                  W_show_canon_vertex  WBEGIN
  ERR(1,"Wrong # args: show_canon_vertex")
                       show_canon_vertex(); WEND

int                  W_dump_vertex  WBEGIN
  ERR(2,"Wrong # args: dump_vertex")
                       dump_vertex(atoi(argv[1])); WEND

int                  W_read_surface  WBEGIN
  ERR(2,"Wrong # args: read_surface <surface file>")
                       read_surface(argv[1]); WEND

int                  W_read_orig_vertex_positions  WBEGIN
  ERR(2,"Wrong # args: read_orig_vertex_positions <surface file>")
                       read_orig_vertex_positions(argv[1]); WEND

int                  W_read_canonical_vertex_positions  WBEGIN
  ERR(2,"Wrong # args: read_canonical_vertex_positions <surface file>")
                       read_canonical_vertex_positions(argv[1]); WEND

int                  W_show_current_surface  WBEGIN
  ERR(1,"Wrong # args: show_current_surface ")
                       show_current_surface(); WEND

int                  W_smooth_surface  WBEGIN
  ERR(2,"Wrong # args: smooth_surface ")
                       smooth_surface(atoi(argv[1])); WEND

int                  W_show_canonical_surface  WBEGIN
  ERR(1,"Wrong # args: show_canonical_surface ")
                       show_canonical_surface(); WEND

int                  W_show_orig_surface  WBEGIN
  ERR(1,"Wrong # args: show_orig_surface ")
                       show_orig_surface(); WEND

int                  W_wmfilter_corslice  WBEGIN
  ERR(1,"Wrong # args: wmfilter_corslice")
                       wmfilter_corslice(imc); WEND

int                  W_norm_slice  WBEGIN
  ERR(2,"Wrong # args: norm_slice <dir: 0=PostAnt,1=InfSup,2=LeftRight>")
                       norm_slice(imc,ic,jc,atoi(argv[1])); WEND

int                  W_norm_allslices  WBEGIN
  ERR(2,"Wrong # args: norm_allslices <dir: 0=PostAnt,1=InfSup,2=LeftRight>")
                       norm_allslices(atoi(argv[1])); WEND

                       // kt
int TclScreenToVoxel ( ClientData inClientData, Tcl_Interp * inInterp,
                       int argc, char ** argv );
int TclVoxelToScreen ( ClientData inClientData, Tcl_Interp * inInterp,
                       int argc, char ** argv );

int TclHideCursor WBEGIN
     ERR ( 1, "Wrong # args: HideCursor" )
     HideCursor ();
     WEND

int TclShowCursor WBEGIN
     ERR ( 1, "Wrong # args: ShowCursor" )
     ShowCursor ();
     WEND

int TclSaveCursorLocation WBEGIN
     ERR ( 1, "Wrong # args: SaveCursorLocation" )
     SaveCursorLocation ();
     WEND

int TclRestoreCursorLocation WBEGIN
     ERR ( 1, "Wrong # args: RestoreCursorLocation" )
     RestoreCursorLocation ();
     WEND

     // zooming
int W_ZoomViewIn WBEGIN
     ERR ( 1, "Wrong # args: ZoomViewIn" )
     ZoomViewIn ();
     redraw ();
     WEND

int W_ZoomViewOut WBEGIN
     ERR ( 1, "Wrong # args: ZoomViewOut" )
     ZoomViewOut ();
     redraw ();
     WEND

int W_UnzoomView WBEGIN
     ERR ( 1, "Wrong # args: UnzoomView" )
     UnzoomView ();
     redraw ();
     WEND

     // for changing modes
int W_CtrlPtMode WBEGIN
     ERR ( 1, "Wrong # args: CtrlPtMode" )
     SetMode ( kMode_CtrlPt );
     WEND

int W_EditMode WBEGIN
     ERR ( 1, "Wrong # args: EditMode" )
     SetMode ( kMode_Edit );
     WEND

int W_SelectMode WBEGIN
     ERR ( 1, "Wrong # args: SelectMode" )
     SetMode ( kMode_Select );
     WEND

     // for setting brush info
int W_SetBrushRadius WBEGIN
     ERR ( 2, "Wrong # args: SetBrushRadius radius" )
     SetBrushRadius ( atoi(argv[1]) );
     WEND

int W_SetBrushShape WBEGIN
     ERR ( 2, "Wrong # args: SetBrushShape 0=square,1=circular" )
     SetBrushShape ( (Brush_Shape)(atoi(argv[1])) );
     WEND

int W_SetBrush3DStatus WBEGIN
     ERR ( 2, "Wrong # args: SetBrush3DStatus status" )
     SetBrush3DStatus ( atoi(argv[1]) );
     WEND

     // control points
int W_HideControlPoints WBEGIN
     ERR ( 1, "Wrong # args: HideControlPoints" )
     SetCtrlPtDisplayStatus ( FALSE );
     redraw ();
     WEND

int W_ShowControlPoints WBEGIN
     ERR ( 1, "Wrong # args: ShowControlPoints" )
     SetCtrlPtDisplayStatus ( TRUE );
     redraw ();
     WEND

int W_SetControlPointsStyle WBEGIN
     ERR ( 2, "Wrong # args: SetControlPointsStyle 1=filled|2=crosshair" )
     SetCtrlPtDisplayStyle ( atoi(argv[1]) );
     redraw ();
     WEND

int W_DeselectAllControlPoints WBEGIN
     ERR ( 1, "Wrong # args: DeselectAllControlPoints" )
     DeselectAllCtrlPts ();
     redraw ();
     WEND

int W_NewControlPoint WBEGIN
     ERR ( 1, "Wrong # args: NewControlPoint" )
     NewCtrlPtFromCursor ();
     redraw ();
     WEND

int W_DeleteSelectedControlPoints WBEGIN
     ERR ( 1, "Wrong # args: DeleteSelectedControlPoints" )
     DeleteSelectedCtrlPts ();
     redraw ();
     WEND

int W_ProcessContrlPointFile WBEGIN
     ERR ( 1, "Wrong # args: ProcessControlPointFile" )
     ProcessCtrlPtFile ( tfname );
     WEND

int W_WriteControlPointFile WBEGIN
     ERR ( 1, "Wrong # args: WriteControlPointFile" )
     WriteCtrlPtFile ( tfname );
     WEND
     
     // editing
int W_UndoLastEdit WBEGIN 
     ERR ( 1, "Wrong # args: UndoLastEdit" )
     RestoreUndoList ();
     redraw ();
     WEND

     // saving and loading labels
int W_SaveSelectionToLabelFile WBEGIN
     ERR ( 2, "Wrong # args: SaveSelectionToLabelFile label_name" )
     SaveSelectionToLabelFile ( argv[1] );
     WEND

int W_LoadSelectionFromLabelFile WBEGIN
     ERR ( 2, "Wrong # args: LoadSelectionFromLabelFile label_name" )
     LoadSelectionFromLabelFile ( argv[1] );
     redraw ();
     WEND

// surface
int W_ShowCurrentSurface WBEGIN
     ERR ( 1, "Wrong # args: ShowCurrentSurface" )
     SetCurrentSurfaceDisplayStatus ( TRUE );
     redraw ();
     WEND

int W_HideCurrentSurface WBEGIN
     ERR ( 1, "Wrong # args: HideCurrentSurface" )
     SetCurrentSurfaceDisplayStatus ( FALSE );
     redraw ();
     WEND

int W_ShowCanonicalSurface WBEGIN
     ERR ( 1, "Wrong # args: ShowCanonicalSurface" )
     SetCanonicalSurfaceDisplayStatus ( TRUE );
     redraw ();
     WEND

int W_HideCanonicalSurface WBEGIN
     ERR ( 1, "Wrong # args: HideCanonicalSurface" )
     SetCanonicalSurfaceDisplayStatus ( FALSE );
     redraw ();
     WEND

int W_ShowOriginalSurface WBEGIN
     ERR ( 1, "Wrong # args: ShowOriginalSurface" )
     SetOriginalSurfaceDisplayStatus ( TRUE );
     redraw ();
     WEND

int W_HideOriginalSurface WBEGIN
     ERR ( 1, "Wrong # args: HideOriginalSurface" )
     SetOriginalSurfaceDisplayStatus ( FALSE );
     redraw ();
     WEND

int W_ShowSurfaceVertices WBEGIN
     ERR ( 1, "Wrong # args: ShowSurfaceVertices" )
     SetSurfaceVertexDisplayStatus ( TRUE );
     redraw ();
     WEND

int W_HideSurfaceVertices WBEGIN
     ERR ( 1, "Wrong # args: HideSurfaceVertices" )
     SetSurfaceVertexDisplayStatus ( FALSE );
     redraw ();
     WEND

int W_SetSurfaceVertexStyle WBEGIN
     ERR ( 2, "Wrong # args: SetSurfaceVertexStyle 0=not_interp|1=interp" )
     SetAverageSurfaceVerticesStatus ( atoi(argv[1]) );
     redraw ();
     WEND


  // end_kt

/*=======================================================================*/

/* for tcl/tk */
static void StdinProc _ANSI_ARGS_((ClientData clientData, int mask));
static void Prompt _ANSI_ARGS_((Tcl_Interp *interp, int partial));
static Tk_Window mainWindow;
static Tcl_Interp *interp;
static Tcl_DString command;
static int tty;

// static Tk_Cursor theCrossCursor;



int main(argc, argv)   /* new main */
int argc;
char **argv;
{
  int code;
  int scriptok=FALSE;
  int cur_arg;
  static char *display = NULL;
  char tkmedit_tcl[NAME_LENGTH];
  char script_tcl[NAME_LENGTH];
  char *envptr;
  FILE *fp ;
  char theErr;

  // kt

  // init our debugging macro code, if any.
  InitDebugging;
  EnableDebuggingOutput;

  DebugPrint "Debugging output is on.\n" EndDebugPrint;

  // init the selection list 
  theErr = VList_New ( &gSelectionList );
  if ( theErr ) {
    DebugPrint "Error in VList_Init: %s\n", 
      VList_GetErrorString(theErr) EndDebugPrint;
    exit (1);
  }

  // init our control pt list
  theErr = VSpace_New ( &gCtrlPtList );
  if ( theErr != kVSpaceErr_NoErr ) {
    DebugPrint "Error in VSpace_Init: %s\n",
      VSpace_GetErrorString ( theErr ) EndDebugPrint;
    exit (1);
  }

  // init the undo list.
  InitUndoList ();

  // and the selection module.
  InitSelectionModule ();

  // start off not displaying control pts.
  gIsDisplayCtrlPts = FALSE;

  // start off in crosshair mode
  gCtrlPtDrawStyle = kCtrlPtStyle_Crosshair;

  // haven't parsed control.dat yet.
  gParsedCtrlPtFile = FALSE;

  // start out in the center, min zoom.
  gCenterX = gCenterY = gCenterZ = 128;
  gLocalZoom = kMinLocalZoom;

  // start out displaying no surfaces.
  gIsDisplayCurrentSurface = FALSE;
  gIsDisplayOriginalSurface = FALSE;
  gIsDisplayCanonicalSurface = FALSE;

  // no surfaces are loaded.
  gIsCurrentSurfaceLoaded = FALSE;
  gIsOriginalSurfaceLoaded = FALSE;
  gIsCanonicalSurfaceLoaded = FALSE;


  // end_kt

  initmap_hacked();
  /* get tkmedit tcl startup script location from environment */
  envptr = getenv("MRI_DIR");
  if (envptr==NULL) {
    printf("tkmedit: env var MRI_DIR undefined (use setenv)\n");
    printf("    [dir containing mri distribution]\n");
    exit(1);
  }
#ifdef USE_LICENSE
  checkLicense(envptr);
#endif

  // kt - check for env flag to use local script.
  if ( getenv ( "USE_LOCAL_TKMEDIT_TCL" ) ) {
    sprintf(tkmedit_tcl,"%s","tkmedit.tcl");  // for testing local script file
  } else {
    sprintf(tkmedit_tcl,"%s/lib/tcl/%s",envptr,"tkmedit.tcl"); 
  }

  if ((fp=fopen(tkmedit_tcl,"r"))==NULL) {
    printf("tkmedit: script %s not found\n",tkmedit_tcl);
    exit(1);
  }
  else fclose(fp);

  /* look for script: (1) cwd, (2) MRI_DIR/lib/tcl */
  strcpy(script_tcl,"newscript.tcl");   /* default if no script */
  for (cur_arg=1; cur_arg<argc; cur_arg++) {  /* just look for -tcl arg */
    if (!MATCH(argv[cur_arg],"-tcl"))
      continue;
    cur_arg++;
    if (cur_arg==argc) {
      printf("tkmedit: ### no script name given\n");
      exit(1);
    }
    if (argv[cur_arg][0]=='-') {
      printf("tkmedit: ### option is bad script name: %s\n",argv[cur_arg]);
      exit(1);
    }
    strcpy(script_tcl,argv[cur_arg]);
    fp = fopen(script_tcl,"r");  /* first look in cwd */
    if (fp==NULL) {
      sprintf(script_tcl,"%s/lib/tcl/%s",envptr,argv[cur_arg]);/* then libdir */      fp = fopen(script_tcl,"r");
      if (fp==NULL) {
        printf(
          "tkmedit: (1) File ./%s not found\n",argv[cur_arg]);
        printf(
          "         (2) File %s/lib/tcl/%s not found\n",envptr,argv[cur_arg]);
        exit(1);
      }
    }
    scriptok = TRUE;
  }

  /* start program, now as function; gl window not opened yet */
  Medit((ClientData) NULL, interp, argc, argv); /* event loop commented out */

  /* start tcl/tk; first make interpreter */
  interp = Tcl_CreateInterp();

  // kt - set global interp
  SetTCLInterp ( interp );

  /* make main window (not displayed until event loop starts) */
  mainWindow = Tk_CreateMainWindow(interp, display, argv[0], "Tk");
  if (mainWindow == NULL) {
    fprintf(stderr, "%s\n", interp->result);
    exit(1); }

  //  theCrossCursor = Tk_GetCursor ( interp, mainWindow, "cross" );
  //  Tk_DefineCursor ( mainWindow, theCrossCursor );


  /* set the "tcl_interactive" variable */
  tty = isatty(0);
  Tcl_SetVar(interp, "tcl_interactive", (tty) ? "1" : "0", TCL_GLOBAL_ONLY);
  if (tty) promptflag = TRUE;

  /* read tcl/tk internal startup scripts */
  if (Tcl_Init(interp) == TCL_ERROR) {
    fprintf(stderr, "Tcl_Init failed: %s\n", interp->result); }
  if (Tk_Init(interp)== TCL_ERROR) {
    fprintf(stderr, "Tk_Init failed: %s\n", interp->result); }
  //  if (Tix_Init(interp) == TCL_ERROR ) {
  //    fprintf(stderr, "Tix_Init failed: %s\n", interp->result); }

  /*=======================================================================*/
  /* register wrapped surfer functions with interpreter */
  Tcl_CreateCommand(interp, "open_window",        W_open_window,        REND);
 Tcl_CreateCommand(interp,"resize_window_intstep",W_resize_window_intstep,REND);
  Tcl_CreateCommand(interp, "move_window",        W_move_window,        REND);
  Tcl_CreateCommand(interp, "pop_gl_window",      W_pop_gl_window,      REND);
  Tcl_CreateCommand(interp, "redraw",             W_redraw,             REND);
  Tcl_CreateCommand(interp, "upslice",            W_upslice,            REND);
  Tcl_CreateCommand(interp, "downslice",          W_downslice,          REND);
  Tcl_CreateCommand(interp, "rotate_brain_x",     W_rotate_brain_x,     REND);
  Tcl_CreateCommand(interp, "rotate_brain_y",     W_rotate_brain_y,     REND);
  Tcl_CreateCommand(interp, "rotate_brain_z",     W_rotate_brain_z,     REND);
  Tcl_CreateCommand(interp, "translate_brain_x",  W_translate_brain_x,  REND);
  Tcl_CreateCommand(interp, "translate_brain_y",  W_translate_brain_y,  REND);
  Tcl_CreateCommand(interp, "translate_brain_z",  W_translate_brain_z,  REND);
  Tcl_CreateCommand(interp, "write_images",       W_write_images,       REND);
  Tcl_CreateCommand(interp, "write_dipoles",      W_write_dipoles,      REND);
  Tcl_CreateCommand(interp, "write_decimation",   W_write_decimation,   REND);
  Tcl_CreateCommand(interp, "goto_point",         W_goto_point,         REND);
  Tcl_CreateCommand(interp, "goto_point_coords",  W_goto_point_coords,  REND);
  Tcl_CreateCommand(interp, "write_point",        W_write_point,        REND);
  Tcl_CreateCommand(interp, "coords_to_talairach",W_coords_to_talairach,REND);
  Tcl_CreateCommand(interp, "talairach_to_coords",W_talairach_to_coords,REND);
  Tcl_CreateCommand(interp, "read_hpts",          W_read_hpts,          REND);
  Tcl_CreateCommand(interp, "read_htrans",        W_read_htrans,        REND);
  Tcl_CreateCommand(interp, "write_htrans",       W_write_htrans,       REND);
  Tcl_CreateCommand(interp, "save_rgb",           W_save_rgb,           REND);
  Tcl_CreateCommand(interp, "set_scale",          W_set_scale,          REND);
  Tcl_CreateCommand(interp, "mirror",             W_mirror,             REND);
  Tcl_CreateCommand(interp, "read_fieldsign",     W_read_fieldsign,     REND);
  Tcl_CreateCommand(interp, "read_fsmask",        W_read_fsmask,        REND);
  Tcl_CreateCommand(interp, "read_binary_curv",   W_read_binary_curv,   REND);
  Tcl_CreateCommand(interp, "read_curv",   W_read_curv,                 REND);
  Tcl_CreateCommand(interp, "goto_vertex",        W_goto_vertex,        REND);
  // kt - added corresponding orig and canon funcs
  Tcl_CreateCommand(interp, "goto_orig_vertex",   W_goto_orig_vertex,   REND);
  Tcl_CreateCommand(interp, "goto_canon_vertex",  W_goto_canon_vertex,  REND);
  Tcl_CreateCommand(interp, "select_control_points",W_select_control_points,
                    REND);
  Tcl_CreateCommand(interp, "reset_control_points",W_reset_control_points,
                    REND);
  Tcl_CreateCommand(interp, "mark_file_vertices", W_mark_file_vertices, REND);
  Tcl_CreateCommand(interp, "unmark_vertices",    W_unmark_vertices,    REND);
  Tcl_CreateCommand(interp, "smooth_3d",          W_smooth_3d,          REND);
  Tcl_CreateCommand(interp, "flip_corview_xyz",   W_flip_corview_xyz,   REND);
  Tcl_CreateCommand(interp, "read_second_images", W_read_second_images, REND);
  Tcl_CreateCommand(interp, "show_vertex",        W_show_vertex,   REND);
  // kt - added corresponding orig and canon funcs
  Tcl_CreateCommand(interp, "show_orig_vertex",   W_show_orig_vertex,   REND);
  Tcl_CreateCommand(interp, "show_canon_vertex",  W_show_canon_vertex,   REND);
  Tcl_CreateCommand(interp, "dump_vertex",        W_dump_vertex,   REND);
  Tcl_CreateCommand(interp, "read_binary_surf",   W_read_binary_surf,   REND);
  Tcl_CreateCommand(interp, "read_surface",       W_read_surface,   REND);
  Tcl_CreateCommand(interp, "read_orig_vertex_positions",       
                    W_read_orig_vertex_positions,   REND);
  Tcl_CreateCommand(interp, "read_canonical_vertex_positions",       
                    W_read_canonical_vertex_positions,   REND);

  Tcl_CreateCommand(interp, "orig",      W_show_orig_surface,      REND);
  Tcl_CreateCommand(interp, "canonical", W_show_canonical_surface, REND);
  Tcl_CreateCommand(interp, "pial",      W_show_canonical_surface, REND);
  Tcl_CreateCommand(interp, "current",   W_show_current_surface,   REND);
  Tcl_CreateCommand(interp, "smooth",    W_smooth_surface,         REND);
  
  Tcl_CreateCommand(interp, "wmfilter_corslice",  W_wmfilter_corslice,  REND);
  Tcl_CreateCommand(interp, "norm_slice",         W_norm_slice,         REND);
  Tcl_CreateCommand(interp, "norm_allslices",     W_norm_allslices,     REND);

  // kt
  Tcl_CreateCommand ( interp, "ScreenToVoxel", TclScreenToVoxel,
                      (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL );
  Tcl_CreateCommand ( interp, "VoxelToScreen", TclVoxelToScreen,
                      (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL );
  Tcl_CreateCommand ( interp, "SaveCursorLocation", 
          TclSaveCursorLocation, REND);
  Tcl_CreateCommand ( interp, "RestoreCursorLocation", 
                      TclRestoreCursorLocation, REND);

  // cursor
  Tcl_CreateCommand ( interp, "HideCursor", TclHideCursor, REND );
  Tcl_CreateCommand ( interp, "hidecursor", TclHideCursor, REND );
  Tcl_CreateCommand ( interp, "ShowCursor", TclShowCursor, REND );
  Tcl_CreateCommand ( interp, "showcursor", TclShowCursor, REND );

  // zooming
  Tcl_CreateCommand ( interp, "ZoomViewIn",  W_ZoomViewIn,  REND );
  Tcl_CreateCommand ( interp, "ZoomViewOut", W_ZoomViewOut, REND );
  Tcl_CreateCommand ( interp, "UnzoomView",  W_UnzoomView,  REND );


  // for changing modes.
  Tcl_CreateCommand ( interp, "CtrlPtMode",       W_CtrlPtMode, REND );
  Tcl_CreateCommand ( interp, "ControlPointMode", W_CtrlPtMode, REND );
  Tcl_CreateCommand ( interp, "EditMode",         W_EditMode,   REND );
  Tcl_CreateCommand ( interp, "SelectMode",       W_SelectMode, REND );
  Tcl_CreateCommand ( interp, "ctrlptmode",       W_CtrlPtMode, REND );
  Tcl_CreateCommand ( interp, "controlptmode",    W_CtrlPtMode, REND );
  Tcl_CreateCommand ( interp, "editmode",         W_EditMode,   REND );
  Tcl_CreateCommand ( interp, "selectmode",       W_SelectMode, REND );

  // ctrl pts
  Tcl_CreateCommand ( interp, "HideControlPoints", 
          W_HideControlPoints,           REND );
  Tcl_CreateCommand ( interp, "ShowControlPoints", 
          W_ShowControlPoints,           REND );
  Tcl_CreateCommand ( interp, "SetControlPointsStyle",
          W_SetControlPointsStyle,       REND );
  Tcl_CreateCommand ( interp, "DeselectAllControlPoints",
          W_DeselectAllControlPoints,    REND );
  Tcl_CreateCommand ( interp, "NewControlPoint",   
          W_NewControlPoint,             REND );
  Tcl_CreateCommand ( interp, "DeleteSelectedControlPoints",
          W_DeleteSelectedControlPoints, REND );
  Tcl_CreateCommand ( interp, "ProcessControlPointFile", 
          W_ProcessContrlPointFile,      REND );
  Tcl_CreateCommand ( interp, "WriteControllPtFile",
          W_WriteControlPointFile,       REND );

  // edit
  Tcl_CreateCommand ( interp, "UndoLastEdit", W_UndoLastEdit, REND );

  // brush control
  Tcl_CreateCommand ( interp, "SetBrushRadius",   W_SetBrushRadius,   REND );
  Tcl_CreateCommand ( interp, "SetBrushShape",    W_SetBrushShape,    REND );
  Tcl_CreateCommand ( interp, "SetBrush3DStatus", W_SetBrush3DStatus, REND );

  // labels
  Tcl_CreateCommand ( interp, "savelabel", 
          W_SaveSelectionToLabelFile,   REND );
  Tcl_CreateCommand ( interp, "SaveLabel", 
          W_SaveSelectionToLabelFile,   REND );
  Tcl_CreateCommand ( interp, "loadlabel", 
          W_LoadSelectionFromLabelFile, REND );
  Tcl_CreateCommand ( interp, "LoadLabel", 
          W_LoadSelectionFromLabelFile, REND );

  // surfaces
  Tcl_CreateCommand ( interp, "LoadMainSurface",       
          W_read_surface,                    REND );
  Tcl_CreateCommand ( interp, "LoadOriginalSurface",       
          W_read_orig_vertex_positions,      REND );
  Tcl_CreateCommand ( interp, "LoadCanonicalSurface",       
          W_read_canonical_vertex_positions, REND );
  Tcl_CreateCommand ( interp, "GotoMainVertex",        
          W_goto_vertex,                     REND );
  Tcl_CreateCommand ( interp, "GotoOriginalVertex",   
          W_goto_orig_vertex,                REND );
  Tcl_CreateCommand ( interp, "GotoCanonicalVertex",  
          W_goto_canon_vertex,               REND );
  Tcl_CreateCommand ( interp, "ShowMainVertex",  
          W_show_vertex,                     REND );
  Tcl_CreateCommand ( interp, "ShowOriginalVertex",  
          W_show_orig_vertex,                REND );
  Tcl_CreateCommand ( interp, "ShowCanonicalVertex",  
          W_show_canon_vertex,               REND );
  Tcl_CreateCommand ( interp, "ShowMainSurface", 
          W_ShowCurrentSurface,              REND );
  Tcl_CreateCommand ( interp, "HideMainSurface", 
          W_HideCurrentSurface,              REND );
  Tcl_CreateCommand ( interp, "ShowOriginalSurface", 
          W_ShowOriginalSurface,             REND );
  Tcl_CreateCommand ( interp, "HideOriginalSurface", 
          W_HideOriginalSurface,             REND );
  Tcl_CreateCommand ( interp, "ShowCanonicalSurface", 
          W_ShowCanonicalSurface,            REND );
  Tcl_CreateCommand ( interp, "HideCanonicalSurface", 
          W_HideCanonicalSurface,            REND );
  Tcl_CreateCommand ( interp, "ShowSurfaceVertices",
          W_ShowSurfaceVertices,             REND );
  Tcl_CreateCommand ( interp, "HideSurfaceVertices",
          W_HideSurfaceVertices,             REND );
  Tcl_CreateCommand ( interp, "SetSurfaceVertexStyle",
          W_SetSurfaceVertexStyle,           REND );

  // end_kt

  /*=======================================================================*/

  /***** link global BOOLEAN variables to tcl equivalents */
  Tcl_LinkVar(interp,"num_control_points",
              (char *)&num_control_points,TCL_LINK_INT);
  Tcl_LinkVar(interp,"control_points",(char*)&control_points,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"maxflag",(char *)&maxflag, TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"surfflag",(char *)&surfflag, TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"editflag",(char *)&editflag, TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"fieldsignflag",(char *)&fieldsignflag, TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"curvflag",(char *)&curvflag, TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"drawsecondflag",(char *)&drawsecondflag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"inplaneflag",(char *)&inplaneflag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"linearflag",(char *)&linearflag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"bwflag",(char *)&bwflag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"truncflag",(char *)&truncflag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"flossflag",(char *)&flossflag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"spackleflag",(char *)&spackleflag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"circleflag",(char *)&circleflag, TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"promptflag",(char *)&promptflag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"followglwinflag",(char *)&followglwinflag, 
                                                        TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"all3flag",(char *)&all3flag,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"changeplanelock",(char *)&changeplanelock, 
                                                        TCL_LINK_BOOLEAN);

  /*=======================================================================*/
  /***** link global INT variables to tcl equivalents */
  Tcl_LinkVar(interp,"zf",(char *)&zf, TCL_LINK_INT);
  Tcl_LinkVar(interp,"xdim",(char *)&xdim, TCL_LINK_INT);
  Tcl_LinkVar(interp,"ydim",(char *)&ydim, TCL_LINK_INT);
  Tcl_LinkVar(interp,"imnr1",(char *)&imnr1, TCL_LINK_INT);
  Tcl_LinkVar(interp,"plane",(char *)&plane, TCL_LINK_INT);
  Tcl_LinkVar(interp,"imc",(char *)&imc, TCL_LINK_INT);
  Tcl_LinkVar(interp,"ic",(char *)&ic, TCL_LINK_INT);
  Tcl_LinkVar(interp,"jc",(char *)&jc, TCL_LINK_INT);
  Tcl_LinkVar(interp,"prad",(char *)&prad, TCL_LINK_INT);
  Tcl_LinkVar(interp,"pradlast",(char *)&pradlast, TCL_LINK_INT);
  Tcl_LinkVar(interp,"dip_spacing",(char *)&dip_spacing, TCL_LINK_INT);
  Tcl_LinkVar(interp,"editedimage",(char *)&editedimage, TCL_LINK_INT);
  Tcl_LinkVar(interp,"surflinewidth",(char *)&surflinewidth, TCL_LINK_INT);
  Tcl_LinkVar(interp,"white_lolim",(char *)&white_lolim, TCL_LINK_INT);
  Tcl_LinkVar(interp,"white_hilim",(char *)&white_hilim, TCL_LINK_INT);
  Tcl_LinkVar(interp,"gray_hilim",(char *)&gray_hilim, TCL_LINK_INT);
  Tcl_LinkVar(interp,"lim3",(char *)&lim3, TCL_LINK_INT);
  Tcl_LinkVar(interp,"lim2",(char *)&lim2, TCL_LINK_INT);
  Tcl_LinkVar(interp,"lim1",(char *)&lim1, TCL_LINK_INT);
  Tcl_LinkVar(interp,"lim0",(char *)&lim0, TCL_LINK_INT);
  Tcl_LinkVar(interp,"second_im_full",(char *)&second_im_full, TCL_LINK_INT);

  Tcl_LinkVar(interp, "gCenterX", (char*)&gCenterX, TCL_LINK_INT);
  Tcl_LinkVar(interp, "gCenterY", (char*)&gCenterY, TCL_LINK_INT);
  Tcl_LinkVar(interp, "gCenterZ", (char*)&gCenterZ, TCL_LINK_INT);
  Tcl_LinkVar(interp, "gLocalZoom", (char*)&gLocalZoom, TCL_LINK_INT);


  /*=======================================================================*/
  /***** link global DOUBLE variables to tcl equivalents (were float) */
  Tcl_LinkVar(interp,"fsquash",(char *)&fsquash, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"fthresh",(char *)&fthresh, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"fsthresh",(char *)&fsthresh, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"fscale",(char *)&fscale, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"ffrac3",(char *)&ffrac3, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"ffrac2",(char *)&ffrac2, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"ffrac1",(char *)&ffrac1, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"ffrac0",(char *)&ffrac0, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"xtalairach",(char *)&xtalairach, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"ytalairach",(char *)&ytalairach, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"ztalairach",(char *)&ztalairach, TCL_LINK_DOUBLE);
  
  /*=======================================================================*/
  /***** link global malloced STRING vars */
  Tcl_LinkVar(interp,"home",        (char *)&subjectsdir,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"session",     (char *)&srname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"subject",     (char *)&pname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"imtype",      (char *)&imtype,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"surface",     (char *)&surface,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"abs_imstem",  (char *)&mfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"sfname",      (char *)&sfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"subjtmpdir",  (char *)&tfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"dip",         (char *)&dipfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"dec",         (char *)&decfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"hpts",        (char *)&hpfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"htrans",      (char *)&htfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"rgb",         (char *)&sgfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"fs",          (char *)&fsfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"fm",          (char *)&fmfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"curv",        (char *)&cfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"insurf",      (char *)&sfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"transform",   (char *)&xffname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"script",      (char *)&rfname,TCL_LINK_STRING);
  Tcl_LinkVar(interp,"selectedpixval",(char *)&selectedpixval, TCL_LINK_INT);
  Tcl_LinkVar(interp,"secondpixval",(char *)&secondpixval, TCL_LINK_INT);
  /*=======================================================================*/
  /***** twitzels stuff ****/
  Tcl_LinkVar(interp,"f2thresh",(char *)&f2thresh, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"fslope",(char *)&fslope, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"fmid",(char *)&fmid, TCL_LINK_DOUBLE);
  Tcl_LinkVar(interp,"do_overlay",(char *)&do_overlay,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"do_interpolate",(char *)&do_interpolate,TCL_LINK_BOOLEAN);
  Tcl_LinkVar(interp,"overlay_frame",(char *)&overlay_frame, TCL_LINK_INT);
  Tcl_LinkVar(interp,"colscale",(char *)&colscale,TCL_LINK_INT);
  Tcl_LinkVar(interp,"floatstem",(char *)&overlay_file,TCL_LINK_STRING);

  strcpy(rfname,script_tcl);  /* save in global (malloc'ed in Program) */

  // kt - have the func display register its commands
  FuncDis_RegisterTclCommands ( interp );

  /* run tcl/tk startup script to set vars, make interface; no display yet */
  printf("tkmedit: interface: %s\n",tkmedit_tcl);
  code = Tcl_EvalFile(interp,tkmedit_tcl);
  if (*interp->result != 0)  printf(interp->result);

  /* if command line script exists, now run as batch job (possibly exiting) */
  if (scriptok) {    /* script may or may not open gl window */
    printf("tkmedit: run tcl script: %s\n",script_tcl);
    code = Tcl_EvalFile(interp,script_tcl);
    if (*interp->result != 0)  printf(interp->result);
  } else {
    ; /* tkmedit has already opened gl window if no script */
  }

  /* always start up command line shell too */
  Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) 0);
  if (tty)
    Prompt(interp, 0);
  fflush(stdout);
  Tcl_DStringInit(&command);
  Tcl_ResetResult(interp);

  /*Tk_MainLoop();*/  /* standard */

  // kt - if doing functional data, init graph window.
  if ( FuncDis_IsDataLoaded () ) {

    // set the functions it needs to interface with us.
    FuncDis_SetRedrawFunction ( &redraw );
    FuncDis_SetSendTCLCommandFunction ( &SendTCLCommand );

    // now init the graph window.
    FuncDis_InitGraphWindow ( interp );
  }

  // kt - new commands summary
  printf("\nNew commands, see inverse/Notes/tkmedit.txt for details.\n");
  printf("\nMagnification:\n");
  printf("\t-z: switch to change scale factor from command line\n");
  printf("\tCtrl-button 1: Zoom in\n");
  printf("\tCtrl-button 3: Zoom out\n");

  printf("\nSurface display:\n");
  printf("\t'read_surface <surf_name>': read a current surface\n");
  printf("\t'read_original_surface <surf_name>': read an original surface\n");
  printf("\t'read_canonical_surface <surf_name>': read a canonical surface\n");
  printf("\t'current': toggle current surface display\n");
  printf("\t'original': toggle original surface display\n");
  printf("\t'canonical': toggle canonical surface display\n");
  printf("\tCtrl-v: toggle surface vertex display\n");
  printf("\tCtrl-a: toggle vertex display type, average or real\n");

  printf("\nControl point mode:\n");
  printf("\tCtrl-h: toggle control point visibility\n");
  printf("\tCtrl-y: toggle control point display style\n");
  printf("\tButton 2: select nearest control point\n");
  printf("\tShift-button 2: add nearest control point to selection\n");
  printf("\tCtrl-button 2: remove nearest control point from selection\n");
  printf("\tCtrl-n: deselect all points\n");
  printf("\tCtrl-d: delete selected points\n");
  printf("\tCtrl-s: save control points to control.dat file\n");

  printf("\nEdit mode:\n");
  printf("\tCtrl-z: undos last edit stroke\n");

  printf("\nSelection mode:\n");
  printf("\t'LoadLabel label_file': import a label file into a selection.\n");
  printf("\t'SaveLabel label_file': save a selection as a label file.\n");
  printf("\tButton 2: add to selection\n");
  printf("\tButton 3: remove from selection\n" );

  printf("\nMode switching:\n");
  printf("\t'SelectMode': enter selection mode\n");
  printf("\t'CtrlPtMode': enter control point editing mode\n");
  printf("\t'EditMode': enter editing mode\n");

  printf("\nCursor visibility:\n");
  printf("\t'HideCursor': hides the cursor\n" );
  printf("\t'ShowCursor': shows the cursor\n" );

 printf("\n");
 PR;
 // end_kt

  /* dual event loop (interface window made now) */
  while(tk_NumMainWindows > 0) {
    while (Tk_DoOneEvent(TK_ALL_EVENTS|TK_DONT_WAIT)) {
      /* do all the tk events; non-blocking */
    }
    do_one_gl_event(interp);
    /*sginap((long)1);*/   /* block for 10 msec */
    usecnap(10000);        /* block for 10 msec */
  }

  Tcl_Eval(interp, "exit");

  // kt - delete the structs we inited before

  if ( FuncDis_IsDataLoaded() ) {
    FuncDis_DeleteData ();
  }

  DeleteSelectionModule ();

  theErr = VList_Delete ( &gSelectionList );
  if ( theErr )
    DebugPrint "Error in VList_Delete: %s\n",  
      VList_GetErrorString(theErr) EndDebugPrint;

  theErr = VSpace_Delete ( &gCtrlPtList );
  if ( theErr != kVSpaceErr_NoErr )
    DebugPrint "Error in VSpace_Delete: %s\n",
      VSpace_GetErrorString(theErr) EndDebugPrint;

  DeleteUndoList ();

  DeleteDebugging;

  // end_kt

  exit(0);
}

void usecnap(int usec)
{
  struct timeval delay;

  delay.tv_sec = 0;
  delay.tv_usec = (long)usec;
  select(0,NULL,NULL,NULL,&delay);
}

/*=== from TkMain.c ===================================================*/
static void StdinProc(clientData, mask)
  ClientData clientData;
  int mask;
{
#define BUFFER_SIZE 4000
  char input[BUFFER_SIZE+1];
  static int gotPartial = 0;
  char *cmd;
  int code, count;

  count = read(fileno(stdin), input, BUFFER_SIZE);
  if (count <= 0) {
    if (!gotPartial) {
      if (tty) {Tcl_Eval(interp, "exit"); exit(1);}
      else     {Tk_DeleteFileHandler(0);}
      return;
    }
    else count = 0;
  }
  cmd = Tcl_DStringAppend(&command, input, count);
  if (count != 0) {
    if ((input[count-1] != '\n') && (input[count-1] != ';')) {
      gotPartial = 1;
      goto prompt; }
    if (!Tcl_CommandComplete(cmd)) {
      gotPartial = 1;
      goto prompt; }
  }
  gotPartial = 0;
  Tk_CreateFileHandler(0, 0, StdinProc, (ClientData) 0);
  code = Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);
  Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) 0);
  Tcl_DStringFree(&command);
  if (*interp->result != 0)
    if ((code != TCL_OK) || (tty))
      puts(interp->result);
  prompt:
  if (tty)  Prompt(interp, gotPartial);
  Tcl_ResetResult(interp);
}

/*=== from TkMain.c ===================================================*/
static void Prompt(interp, partial)
  Tcl_Interp *interp;
  int partial;
{
  char *promptCmd;
  int code;

  promptCmd = Tcl_GetVar(interp,
      partial ? "tcl_prompt2" : "tcl_prompt1", TCL_GLOBAL_ONLY);
  if (promptCmd == NULL) {
    defaultPrompt:
    if (!partial)
      fputs("% ", stdout);
  }
  else {
    code = Tcl_Eval(interp, promptCmd);
    if (code != TCL_OK) {
      Tcl_AddErrorInfo(interp,
                    "\n    (script that generates prompt)");
      fprintf(stderr, "%s\n", interp->result);
      goto defaultPrompt;
    }
  }
  fflush(stdout);
}



// kt

// =========================================================== READING VOLUMES

void ReadVolumeWithMRIRead ( char * inFileOrPath ) {

  MRI * theUnconformedVolume, *theVolume;
  int theSlice, theRow, theCol;

  // pass the path to MRIRead
  theUnconformedVolume = MRIread ( inFileOrPath );

  // make sure the result is good.
  if ( NULL == theUnconformedVolume ) {
    OutputPrint "Couldn't read volume data at %s\n", 
      inFileOrPath EndOutputPrint;
    exit ( 1 );
  }

  // conform it.
  theVolume = MRIconform ( theUnconformedVolume );

  // grab all the data we need.
  imnr0 = theVolume->imnr0;
  imnr1 = theVolume->imnr1;
  ptype = theVolume->ptype;
  xnum = theVolume->width;
  ynum = theVolume->height;
  ps = theVolume->ps;
  st = theVolume->thick;
  xx0 = theVolume->xstart;
  xx1 = theVolume->xend;
  yy0 = theVolume->ystart;
  yy1 = theVolume->yend;
  zz0 = theVolume->zstart;
  zz1 = theVolume->zend;

  // grab the tal transforms.
  if ( NULL != theVolume->linear_transform ) {
    copy_general_transform ( &theVolume->transform, &talairach_transform );
    linear_transform = get_linear_transform_ptr ( &talairach_transform );
    inverse_linear_transform = 
      get_inverse_linear_transform_ptr ( &talairach_transform );
  }

  // if we got them, make note of it.
  if ( NULL != linear_transform  && NULL != inverse_linear_transform )
    transform_loaded = TRUE;
  else
    transform_loaded = FALSE;

  numimg = imnr1-imnr0+1; // really the number of slices

  // calc window dimensions according to already defined scale factor
  xdim= xnum * zf;
  ydim= ynum * zf;

  /*
  DebugPrint "\timnr0 = %d\n", imnr0 EndDebugPrint;
  DebugPrint "\timnr1 = %d\n", imnr1 EndDebugPrint;
  DebugPrint "\tnumimg = %d\n", numimg EndDebugPrint;
  DebugPrint "\tptype = %d\n", ptype EndDebugPrint;
  DebugPrint "\txnum = %d\n", xnum EndDebugPrint;
  DebugPrint "\tynum = %d\n", ynum EndDebugPrint;
  DebugPrint "\tps = %2.5f\n", ps EndDebugPrint;
  DebugPrint "\tst = %2.5f\n", st EndDebugPrint;
  DebugPrint "\txx0 = %2.5f\n", xx0 EndDebugPrint;
  DebugPrint "\txx1 = %2.5f\n", xx1 EndDebugPrint;
  DebugPrint "\tyy0 = %2.5f\n", yy0 EndDebugPrint;
  DebugPrint "\tyy1 = %2.5f\n", yy1 EndDebugPrint;
  DebugPrint "\tzz0 = %2.5f\n", zz0 EndDebugPrint;
  DebugPrint "\tzz1 = %2.5f\n", zz1 EndDebugPrint;
  DebugPrint "\ttransform_loaded = %d\n", transform_loaded EndDebugPrint;
  */

  // each slice is an array of rows. each row is an array of cols.
  // im[][][] is our volume buffer.
  for ( theSlice = 0 ; theSlice < numimg; theSlice++ ) {
    im[theSlice] = (BUFTYPE**)lcalloc ( ynum, sizeof(BUFTYPE*) );
    for ( theRow = 0; theRow < ynum; theRow++ ) {
      im[theSlice][theRow] = (BUFTYPE*)lcalloc ( xnum, sizeof(BUFTYPE) );
    }
  }

  // sim[][][] is a small volume buffer.
  for ( theSlice = 0 ; theSlice < 6; theSlice++ ) {
    sim[theSlice] = (BUFTYPE**)lcalloc ( ynum, sizeof(BUFTYPE*) );
    for ( theRow = 0; theRow < ynum; theRow++ ) {
      sim[theSlice][theRow] = (BUFTYPE*)lcalloc ( xnum, sizeof(BUFTYPE) );
    }
  }

  // read in all image data into im[], set changed[] for all slices to nil.
  for ( theSlice = 0; theSlice < numimg; theSlice++ ) {
    memcpy ( im[theSlice], theVolume->slices[theSlice], 
       xnum * ynum * sizeof(BUFTYPE) );
    changed[theSlice] = FALSE;
  }

  for ( theSlice = 0; theSlice < numimg; theSlice++ )
    for ( theRow = 0; theRow < ynum; theRow++ )
      for ( theCol = 0; theCol < xnum; theCol++ ) {
  if ( im[theSlice][theRow][theCol]/2 > sim[3][theRow][theCol]) 
    sim[3][theRow][theCol] = im[theSlice][theRow][theCol]/2;

  if ( im[theSlice][theRow][theCol]/2 > sim[4][theSlice][theCol]) 
    sim[4][theSlice][theCol] = im[theSlice][theRow][theCol]/2;

  if ( im[theSlice][theRow][theCol]/2 > sim[5][theRow][theSlice]) 
    sim[4][theRow][theSlice] = im[theSlice][theRow][theCol]/2;
      }

  for ( theRow = 0; theRow < ynum; theRow++ )
    for ( theCol = 0; theCol < xnum; theCol++ )
      for ( theSlice = 0; theSlice < 3; theSlice++ ) {
  sim[theSlice][theRow][theCol] = sim[theSlice+3][theRow][theCol];
      }

  // editing is always disabled in this mode (for the moment)
  editflag = FALSE;

  OutputPrint "NOTE: Editing is disabled. Do not try and enable it or I will crash.\n" EndOutputPrint;

  MRIfree ( &theVolume );
}


/* ================================================ Control point utilities */

                                       /* reads the control.dat file, 
                                          transforms all pts from RAS space 
                                          to voxel space, and adds them as 
                                          control pts */
void ProcessCtrlPtFile ( char * inDir ) {

  char theFilename [NAME_LENGTH];
  FILE * theFile;
  float theTempX, theTempY, theTempZ;
  Real theRASX, theRASY, theRASZ;
  int theVoxX, theVoxY, theVoxZ;
  int theNumPtsRead;
  char theResult;
  VoxelRef theVoxel;

  Voxel_New ( &theVoxel );

  // don't parse the file if we already have.
  if ( TRUE == gParsedCtrlPtFile )
    return;

  // if we're not in control point mode, return.
  if ( !IsInMode ( kMode_CtrlPt ) )
    return;

  // create the filename
  sprintf ( theFilename, "%s/control.dat", inDir );
  
  // open for reading. position file ptr at beginning of file.
  theFile = fopen ( theFilename, "r" );
  
  // check for success. if not, print an error and return.
  if ( NULL == theFile ) {
    DebugPrint "ProcessCtrlPtFile: Couldn't open %s for processing.\n",
      theFilename EndDebugPrint;
    return;
  }

  // while we have points left
  DebugPrint "ProcessCtrlPtFile: Opened %s, reading...\n", 
    theFilename EndDebugPrint;
  while ( !feof(theFile) ) {
    
    // read in some numbers
    theNumPtsRead = fscanf ( theFile, "%f %f %f", &theTempX, &theTempY, &theTempZ );

    // if not successful, file is wierd. print error, close file, and return.
    if ( theNumPtsRead < 3 &&
         theNumPtsRead != EOF ) {

      DebugPrint "ProcessCtrlPtFile: Error parsing file, expected three float values but didn't get them.\n" EndDebugPrint;
      fclose ( theFile );
      return;
    }

    // if we got 3 pts, we got a point.
    if ( 3 == theNumPtsRead ) {

      // cast to reals
      theRASX = (Real) theTempX;
      theRASY = (Real) theTempY;
      theRASZ = (Real) theTempZ;

      // transform from ras to voxel
      RASToVoxel ( theRASX, theRASY, theRASZ,
                   &theVoxX, &theVoxY, &theVoxZ );
      
      DebugPrint "\t%f %f %f -> %d %d %d\n", 
        theRASX, theRASY, theRASZ,
        (short)theVoxX, (short)theVoxY, (short)theVoxZ EndDebugPrint;
      
      // add it to our cntrl points space
      Voxel_Set ( theVoxel, theVoxX, theVoxY, theVoxZ );
      theResult = VSpace_AddVoxel ( gCtrlPtList, theVoxel );
      if ( theResult != kVSpaceErr_NoErr )
        DebugPrint "ProcessCtrlPtFile(): Error in VSpace_AddVoxel: %s\n", 
          VSpace_GetErrorString ( theResult ) EndDebugPrint;
    }
  }

  // close the file.
  fclose ( theFile );

  DebugPrint "\tDone.\n" EndDebugPrint;

  // mark that we have processed the file, and shouldn't do it again.
  gParsedCtrlPtFile = TRUE;

  Voxel_Delete ( &theVoxel );
}

                                       /* writes all control points to the
                                          control.dat file in RAS space */
void WriteCtrlPtFile ( char * inDir ) {

  char theFilename [NAME_LENGTH];
  FILE * theFile;
  int theVoxX, theVoxY, theVoxZ;
  Real theRASX, theRASY, theRASZ;
  char theResult;
  int theIndex;
  int theListCount, theListIndex;
  VoxelListRef theList;
  VoxelRef theVoxel;
  
  if ( !IsInMode ( kMode_CtrlPt ) ) {
    return;
  }

  Voxel_New ( &theVoxel );

  // create the filename.
  sprintf ( theFilename, "%s/control.dat", inDir );

  // open the file for writing.
  theFile = fopen ( theFilename, "w" );

  // check for success.
  if ( NULL == theFile ) {
    DebugPrint "WriteCtrlPtFile: Couldn't create %s for writing.\n",
      theFilename EndDebugPrint;
    return;
  }

   DebugPrint "WriteCtrlPtFile: Writing control points to %s...\n",
    theFilename EndDebugPrint;
   OutputPrint "Saving control points... " EndOutputPrint;

  // get the ctrl pts in the list...
  for ( theIndex = 0; theIndex < kVSpace_NumListsInPlane; theIndex++ ) {

    // get the list for this x value.
    theResult = VSpace_GetVoxelsInXPlane ( gCtrlPtList, theIndex, &theList );
    if ( theResult != kVSpaceErr_NoErr ) {
      DebugPrint "WriteCtrlPtFile(): Error in VSpace_GetVoxelsInXPlane: %s\n",
        VSpace_GetErrorString ( theResult ) EndDebugPrint;
      OutputPrint "\nError saving a point!\n" EndOutputPrint;
      continue;
    }

    // get the number of voxels in the list.
    theResult = VList_GetCount ( theList, &theListCount );
    if ( theResult != kVListErr_NoErr ) {
      DebugPrint "WriteCtrlPtFile(): Error in VList_GetCount: %s\n",
        VList_GetErrorString ( theResult ) EndDebugPrint;
      OutputPrint "\nError saving a point!\n" EndOutputPrint;
      continue;
    }

    // get each voxel...
    for ( theListIndex = 0; theListIndex < theListCount; theListIndex++ ) {

      theResult = VList_GetNthVoxel ( theList, theListIndex, theVoxel );
      if ( theResult != kVListErr_NoErr ) {
        DebugPrint "WriteCtrlPtFile(): Error in VList_GetNthVoxel: %s\n",
          VList_GetErrorString ( theResult ) EndDebugPrint;
  OutputPrint "\nError saving a point!\n" EndOutputPrint;
        continue;
      }
      
      // unpack it.
      theVoxX = Voxel_GetX ( theVoxel );
      theVoxY = Voxel_GetY ( theVoxel );
      theVoxZ = Voxel_GetZ ( theVoxel );

      // transform to ras space.
      VoxelToRAS ( theVoxX, theVoxY, theVoxZ,
                   &theRASX, &theRASY, &theRASZ );
      
      // write to the file
      fprintf ( theFile, "%f %f %f\n", theRASX, theRASY, theRASZ );
      
      DebugPrint "\t%d %d %d -> %f %f %f\n", 
        theVoxX, theVoxY, theVoxZ,
        theRASX, theRASY, theRASZ EndDebugPrint;
      
    }
    
  }

  // close file
  fclose ( theFile );

  DebugPrint "\tDone.\n" EndDebugPrint;
  OutputPrint " done.\n" EndOutputPrint;

  Voxel_Delete ( &theVoxel );
}

void ToggleCtrlPtDisplayStatus () {

  // toggle the status.
  SetCtrlPtDisplayStatus ( !gIsDisplayCtrlPts );
}

void SetCtrlPtDisplayStatus ( char inStatus ) {

  if ( inStatus != gIsDisplayCtrlPts ) {

    // toggle the status.
    gIsDisplayCtrlPts = inStatus;
    
    // print a little note.
    if ( gIsDisplayCtrlPts ) {
      OutputPrint "Control points display is ON.\n" EndOutputPrint;
    } else {
      OutputPrint "Control points display is OFF.\n" EndOutputPrint;
    }

    // if we're not in control points mode, we still won't be able to
    // view control points.
    if ( !IsInMode(kMode_CtrlPt) ) {
      OutputPrint "NOTE: You are in editing mode, not control point mode. You will not be able to see control points until you enter control point mode. This can be done by entering \"CtrlPtMode\" at the prompt.\n" EndOutputPrint;
    }
  }
}

void ToggleCtrlPtDisplayStyle () {

  // if it's one, change it to another.
  switch ( gCtrlPtDrawStyle ) {
  case kCtrlPtStyle_FilledVoxel: 
    SetCtrlPtDisplayStyle ( kCtrlPtStyle_Crosshair );
    break;
  case kCtrlPtStyle_Crosshair:
    SetCtrlPtDisplayStyle ( kCtrlPtStyle_FilledVoxel );
    break;
  }
}

void SetCtrlPtDisplayStyle ( int inStyle ) {

  if ( inStyle != gCtrlPtDrawStyle ) {
    
    // set the style
    gCtrlPtDrawStyle = inStyle;
    
    // notify the user of the new style
    switch ( gCtrlPtDrawStyle ) {
    case kCtrlPtStyle_FilledVoxel: 
      OutputPrint "Control point display style is FILLED VOXEL.\n" EndOutputPrint;
      break;
    case kCtrlPtStyle_Crosshair:
      OutputPrint "Control point display style is CROSSHAIR.\n" EndOutputPrint;
      break;
    }

    if ( !IsInMode(kMode_CtrlPt) ) {
      OutputPrint "NOTE: You are in editing mode, not control point mode. You will not be able to see control points until you enter control point mode. This can be done by entering \"CtrlPtMode\" at the prompt.\n" EndOutputPrint;
    }
  }
}

                                       /* draw control point. switches on 
                                          the current display style of the 
                                          point. */
void DrawCtrlPt ( char * inBuffer,  // video buffer to draw into
                  int inX, int inY, // x,y location in the buffer
                  long inColor ) {  // color to draw in

  switch ( gCtrlPtDrawStyle ) {

    // draw in pixels
  case kCtrlPtStyle_FilledVoxel:

    // if we're not in all3 view...
    if ( !all3flag ) {

      // fill in the entire pixel with dimensions of the product of
      // our two zoom factors.
      FillBoxInBuffer ( inBuffer, inX, inY, zf*gLocalZoom, inColor );

    } else {

      // otherwise, use half the zoom factor for our pixel size. 
      FillBoxInBuffer ( inBuffer, inX, inY, (zf/2)*gLocalZoom, inColor );
    }

    break;

    // crosshair
  case kCtrlPtStyle_Crosshair:

    // centered crosshair.
    DrawCenteredCrosshairInBuffer ( inBuffer, inX, inY, 
            kCtrlPtCrosshairRadius, inColor );
    break;
  }
}

                                       /* handles the clicking of ctrl pts.
                                          takes a screen pt and looks for the
                                          closest ctrl pt in the same plane
                                          displayed on the screen. if it
                                          finds one, it adds or removes it
                                          from the selection, depending on
                                          the ctrt and shift key. */
void SelectCtrlPt ( int inScreenX, int inScreenY,     // screen pt clicked
                    int inScreenZ, int inPlane,       // plane to search in
        unsigned int inState ) {

  int theVoxX, theVoxY, theVoxZ;
  int theListCount, theListIndex;
  char theResult;
  VoxelRef theVoxel;
  VoxelListRef theList;
  unsigned int theDistance, theClosestDistance;
  short theClosestIndex;

  Voxel_New ( &theVoxel );

  // find the voxel they clicked
  ScreenToVoxel ( inPlane, inScreenX, inScreenY, inScreenZ, 
                  &theVoxX, &theVoxY, &theVoxZ );

  // get the dimension to search in, based on our current plane, and
  // get the list of voxels for this slice.
  switch ( inPlane ) {
  case CORONAL: 
    theResult = VSpace_GetVoxelsInZPlane ( gCtrlPtList, theVoxZ, &theList );
    break;
  case HORIZONTAL: 
    theResult = VSpace_GetVoxelsInYPlane ( gCtrlPtList, theVoxY, &theList );
    break;
  case SAGITTAL: 
    theResult = VSpace_GetVoxelsInXPlane ( gCtrlPtList, theVoxX, &theList );
    break;
  default:
    theResult = 0;
  }
    
  if ( theResult != kVSpaceErr_NoErr ) {
    DebugPrint "SelectCtrlPt(): Error in VSpace_GetVoxelsInX/Y/ZPlane: %s\n", 
      VSpace_GetErrorString ( theResult ) EndDebugPrint;
    theList = NULL;
  }

  // if we got a list...
  if ( theList != NULL ) {

    // start with a large distance and no closest ctrl pt
    theClosestDistance = 0 - 1; 
    theClosestIndex = -1;

    // get the number of voxel and search through the list...
    theResult = VList_GetCount ( theList, &theListCount );
    if ( theResult != kVListErr_NoErr ) {
      DebugPrint "SelectCtrlPt(): Error in VList_GetCount: %s\n",
        VList_GetErrorString ( theResult ) EndDebugPrint;
      theListCount = 0;
    }
    
    for ( theListIndex = 0; theListIndex < theListCount; theListIndex++ ) {
 
      // grab a voxel
      theResult = VList_GetNthVoxel ( theList, theListIndex, theVoxel );
      if ( theResult != kVListErr_NoErr ) {
        DebugPrint "SelectCtrlPt(): Error in VList_GetNthVoxel: %s\n",
          VList_GetErrorString ( theResult ) EndDebugPrint;
        continue;
      }

      // get the distance to the clicked voxel...
      theDistance =
        ((theVoxX - Voxel_GetX(theVoxel))*(theVoxX - Voxel_GetX(theVoxel))) +
        ((theVoxY - Voxel_GetY(theVoxel))*(theVoxY - Voxel_GetY(theVoxel))) +
        ((theVoxZ - Voxel_GetZ(theVoxel))*(theVoxZ - Voxel_GetZ(theVoxel)));

      // if it's less than our max, mark the distance and vox index
      if ( theDistance < theClosestDistance ) {
        theClosestDistance = theDistance;
        theClosestIndex = theListIndex;
      }
    }

    // if we found a voxel
    if ( theClosestIndex != -1 ) {

      // get it back again
      theResult = VList_GetNthVoxel ( theList, theClosestIndex, theVoxel );
      if ( theResult != kVListErr_NoErr ) {
        DebugPrint "SelectCtrlPt(): Error in VList_GetNthVoxel: %s\n",
          VList_GetErrorString ( theResult ) EndDebugPrint;
        return;
      }


      // if shift key is down
      if ( inState & ShiftMask ) {

        // add this point to the selection
        theResult = VList_AddVoxel ( gSelectionList, theVoxel );
        if ( theResult != kVListErr_NoErr )
          DebugPrint "SelectCtrlPt(): Error in VList_AddVoxel: %s\n",
            VList_GetErrorString ( theResult ) EndDebugPrint;
    
        // else if shift key is not down...
      } else {

        // if ctrl key is down
        if ( inState & ControlMask ) {

          // remove this point from selection
          theResult = VList_RemoveVoxel ( gSelectionList, theVoxel );
          if ( theResult != kVListErr_NoErr )
            DebugPrint "SelectCtrlPt(): Error in VList_RemoveVoxel: %s\n",
              VList_GetErrorString ( theResult ) EndDebugPrint;
        
          // no shift and no ctrl key
        } else {

          // remove all points from selection and add this point
    DeselectAllCtrlPts ();

          theResult = VList_AddVoxel ( gSelectionList, theVoxel );
          if ( theResult != kVListErr_NoErr )
            DebugPrint "SelectCtrlPt(): Error in VList_AddVoxel: %s\n",
              VList_GetErrorString ( theResult ) EndDebugPrint;
          
        }
      }
    }
  }

  Voxel_Delete ( &theVoxel );

}

void DeselectAllCtrlPts () {

  char theResult;
  
  if ( IsInMode ( kMode_CtrlPt ) ) {

    theResult = VList_ClearList ( gSelectionList );
    if ( theResult != kVListErr_NoErr )
      DebugPrint "DeselectAllControlPoints(): Error in VList_ClearList: %s\n",
  VList_GetErrorString ( theResult ) EndDebugPrint;
  }
}

void NewCtrlPt ( VoxelRef inVoxel ) {

  char theResult;
  
  if ( IsInMode ( kMode_CtrlPt ) ) {
    
    // add the voxel to the ctrl pt space
    theResult = VSpace_AddVoxel ( gCtrlPtList, inVoxel );
    if ( theResult != kVSpaceErr_NoErr )
      DebugPrint "NewControlPoint(): Error in VSpace_AddVoxel: %s\n",
  VSpace_GetErrorString ( theResult ) EndDebugPrint;
    
    OutputPrint "Made control point (%d,%d,%d).\n",
      EXPAND_VOXEL_INT(inVoxel)  EndOutputPrint;
  }
}

void NewCtrlPtFromCursor () {

  VoxelRef theVoxel;

  Voxel_New ( &theVoxel );

  // get the cursor as an anatomical voxel and add it.
  GetCursorInVoxelCoords ( theVoxel );
  NewCtrlPt ( theVoxel );
  
  Voxel_Delete ( &theVoxel );
}

                                        /* remove the selected control points 
                                           from the control point space */
void DeleteSelectedCtrlPts () {

  char theResult;
  int theCount, theIndex;
  VoxelRef theCtrlPt;

  Voxel_New ( &theCtrlPt );

  // get the number of selected points we have
  theResult = VList_GetCount ( gSelectionList, &theCount );
  if ( theResult != kVListErr_NoErr ) {
    DebugPrint "Error in VList_GetCount: %s\n",
      VList_GetErrorString ( theResult ) EndDebugPrint;
    return;
  }

  DebugPrint "Deleting selected control points... " EndDebugPrint;

  // for each one...
  for ( theIndex = 0; theIndex < theCount; theIndex++ ) {

    // get it
    theResult = VList_GetNthVoxel ( gSelectionList, theIndex, theCtrlPt );
    if ( theResult != kVListErr_NoErr ) {
      DebugPrint "Error in VList_GetNthVoxel: %s\n",
        VList_GetErrorString ( theResult ) EndDebugPrint;
      continue;
    }
    
    // set its value in the space to 0
    theResult = VSpace_RemoveVoxel ( gCtrlPtList, theCtrlPt );
    if ( theResult != kVSpaceErr_NoErr ) {
      DebugPrint "DeleteSelectedCtrlPts(): Error in VSpace_RemoveVoxel: %s\n",
        VSpace_GetErrorString ( theResult ) EndDebugPrint;
      continue;
    }        
  }

  // remove all pts from the selection list
  theResult = VList_ClearList ( gSelectionList );
  if ( theResult != kVListErr_NoErr ) {
    DebugPrint "Error in VList_ClearList: %s\n",
      VList_GetErrorString ( theResult ) EndDebugPrint;
    return;
  }

  DebugPrint " done.\n" EndDebugPrint;

  printf ( "Deleted selected control points.\n" );
  PR;

  Voxel_Delete ( &theCtrlPt );
}

// ===========================================================================

// ========================================================= SELECTING REGIONS

void InitSelectionModule () {

  char theErr;
  theErr = VSpace_New ( &gSelectedVoxels );
  if ( theErr != kVSpaceErr_NoErr ) {
    DebugPrint "InitSelectionModule(): Error in VSpace_Init: %s\n",
      VSpace_GetErrorString ( theErr ) EndDebugPrint;
    gSelectedVoxels = NULL;
  }

  isDisplaySelectedVoxels = TRUE;
}

void DeleteSelectionModule () {

  char theErr;

  if ( NULL == gSelectedVoxels )
    return;
  
  theErr = VSpace_Delete ( &gSelectedVoxels );
  if ( theErr != kVSpaceErr_NoErr )
    DebugPrint "DeleteSelectionModule(): Error in VSpace_Delete: %s\n",
      VSpace_GetErrorString(theErr) EndDebugPrint;
}


void DrawSelectedVoxels ( char * inBuffer, int inPlane, int inPlaneNum ) {

  VoxelRef theVoxel;
  VoxelListRef theList;
  int theListCount, theListIndex;
  char theListErr;
  int theScreenX, theScreenY, theScreenZ, theScreenH, theScreenV;

  Voxel_New ( &theVoxel );

  switch ( inPlane ) {
  case CORONAL:
    theListErr = 
      VSpace_GetVoxelsInZPlane ( gSelectedVoxels, inPlaneNum, &theList );
    break;
  case SAGITTAL:
    theListErr = 
      VSpace_GetVoxelsInXPlane ( gSelectedVoxels, inPlaneNum, &theList );
    break;
  case HORIZONTAL:
    theListErr = 
      VSpace_GetVoxelsInYPlane ( gSelectedVoxels, inPlaneNum, &theList );
    break;
  }

  if ( theListErr != kVSpaceErr_NoErr ) {
    DebugPrint "DrawSelectedVoxels(): Error in VSpace_GetVoxelsInPlane: %s\n", 
      VSpace_GetErrorString ( theListErr ) EndDebugPrint;
    theList = NULL;
  }
  
  // if we have a list..
  if ( theList != NULL ) {
    
    // get its count
    theListErr = VList_GetCount ( theList, &theListCount );
    if ( theListErr != kVListErr_NoErr ) {
      DebugPrint "DrawSelectedVoxels: %s\n",
  VList_GetErrorString ( theListErr ) EndDebugPrint;
      theListCount = 0;
    }

    // get each voxel...
    for ( theListIndex = 0; 
    theListIndex < theListCount; 
    theListIndex++ ) {
      
      theListErr = VList_GetNthVoxel ( theList, theListIndex, theVoxel );
      if ( theListErr != kVListErr_NoErr ) {
  DebugPrint "DrawSelectedVoxels(): Error in VList_GetNthVoxel: %s\n",
    VList_GetErrorString ( theListErr ) EndDebugPrint;
  continue;
      }

      // go to screen points
      VoxelToScreen ( Voxel_GetX(theVoxel), Voxel_GetY(theVoxel),
          Voxel_GetZ(theVoxel), 
          inPlane, &theScreenX, &theScreenY, &theScreenZ );
      
      // if it's in the window...
      if ( IsScreenPtInWindow ( theScreenX, theScreenY, theScreenZ ) ) {
  
  // draw the point.
  ScreenToScreenXY ( theScreenX, theScreenY, theScreenZ,
         inPlane, &theScreenH, &theScreenV );
    
  FillBoxInBuffer ( inBuffer, theScreenH, theScreenV,
        gLocalZoom*zf, kRGBAColor_Green );
      }
    }
  }
  
}

void AllowSelectionModuleToRespondToClick ( VoxelRef inScreenVoxel ) {

}

void AddVoxelToSelection ( VoxelRef inVoxel, void * inData ) {

  char theErr;

  /*
  DebugPrint "AddVoxelToSelection ( (%d, %d, %d) )\n",
    EXPAND_VOXEL_INT(inVoxel) EndDebugPrint;
  */

  if ( NULL == gSelectedVoxels )
    return;
  
  theErr = VSpace_AddVoxel ( gSelectedVoxels, inVoxel );
  if ( theErr != kVSpaceErr_NoErr )
    DebugPrint "AddVoxelToSelection(): Error in VSpace_AddVoxel: %s\n",
      VSpace_GetErrorString(theErr) EndDebugPrint;
  
}

void RemoveVoxelFromSelection ( VoxelRef inVoxel, void * inData ) {

  char theErr;

  /*
  DebugPrint "RemoveVoxelToSelection ( (%d, %d, %d) )\n",
    EXPAND_VOXEL_INT(inVoxel) EndDebugPrint;
  */

  if ( NULL == gSelectedVoxels )
    return;
  
  theErr = VSpace_RemoveVoxel ( gSelectedVoxels, inVoxel );
  if ( theErr != kVSpaceErr_NoErr &&
       theErr != kVSpaceErr_VoxelNotInSpace )
    DebugPrint "RemoveVoxelFromSelection(): Error in VSpace_RemoveVoxel: %s\n",
      VSpace_GetErrorString(theErr) EndDebugPrint;
}


void ClearSelection () {

}

char IsDisplaySelectedVoxels () {

  return isDisplaySelectedVoxels;
}

void SaveSelectionToLabelFile ( char * inFileName ) {

  LABEL * theLabel;
  LABEL_VERTEX *theVertex;
  int theNumVoxels, theGlobalVoxelIndex, theListVoxelIndex, theListIndex,
    theListCount;
  Real theRASX, theRASY, theRASZ;
  VoxelRef theAnatomicalVoxel;
  VoxelListRef theList;
  int theLabelError;
  char theError;
  
  DebugPrint "SaveSelectionToLabelFile ( %s )\n",
    inFileName EndDebugPrint;

  Voxel_New ( &theAnatomicalVoxel );

  // get the number of selected voxels we have.
  theNumVoxels = 0;
  for ( theListIndex = 0; 
  theListIndex < kVSpace_NumListsInPlane; 
  theListIndex++ ) {
    theError = VSpace_GetVoxelsInXPlane ( gSelectedVoxels, 
            theListIndex, &theList );
    if ( kVSpaceErr_NoErr != theError ) {
      DebugPrint "\tError in VSpace_GetVoxelsInXPlane (%d): %s\n",
  theError, VSpace_GetErrorString ( theError ) EndDebugPrint;
      Voxel_Delete ( &theAnatomicalVoxel );
      return;
    }

    theError = VList_GetCount ( theList, &theListCount );
    if ( kVListErr_NoErr != theError ) {
      DebugPrint "\tError in VList_GetCount (%d): %s\n",
  theError, VList_GetErrorString ( theError ) EndDebugPrint;
      Voxel_Delete ( &theAnatomicalVoxel );
      return;
    }

    theNumVoxels += theListCount;
  }

  // allocate a label file with that number of voxels, our subject name,
  // and the passed in label file name.
  theLabel = LabelAlloc ( theNumVoxels, pname, inFileName );
  if ( NULL == theLabel ) {
    DebugPrint "\tCouldn't allocate label.\n" EndDebugPrint;
    OutputPrint "ERROR: Couldn't save label.\n" EndOutputPrint;
    Voxel_Delete ( &theAnatomicalVoxel );
    return;
  }

  // set the number of points in the label
  theLabel->n_points = theNumVoxels;

  // for every list in a plane of the space... 
  theGlobalVoxelIndex = 0;
  for ( theListIndex = 0; 
  theListIndex < kVSpace_NumListsInPlane; 
  theListIndex++ ) {

    // get the list
    theError = VSpace_GetVoxelsInXPlane ( gSelectedVoxels, 
            theListIndex, &theList );
    if ( kVSpaceErr_NoErr != theError ) {
      DebugPrint "\tError in VSpace_GetVoxelsInXPlane (%d): %s\n",
  theError, VSpace_GetErrorString ( theError ) EndDebugPrint;
      Voxel_Delete ( &theAnatomicalVoxel );
      return;
    }

    // get the num of voxels in the list.
    theError = VList_GetCount ( theList, &theListCount );
    if ( kVListErr_NoErr != theError ) {
      DebugPrint "\tError in VList_GetCount (%d): %s\n",
  theError, VList_GetErrorString ( theError ) EndDebugPrint;
      Voxel_Delete ( &theAnatomicalVoxel );
      return;
    }

    // note that this is only the index of the voxel within a list, not
    // global count. for each voxel in the list...
    for ( theListVoxelIndex = 0;
    theListVoxelIndex < theListCount;
    theListVoxelIndex++ ) {
      
      // get a voxel.
      theError = VList_GetNthVoxel ( theList, 
             theListVoxelIndex, theAnatomicalVoxel );
      if ( kVListErr_NoErr != theError ) {
  DebugPrint "\tError in VList_GetNthVoxel (%d): %s\n",
    theError, VList_GetErrorString ( theError ) EndDebugPrint;
  Voxel_Delete ( &theAnatomicalVoxel );
  return;
      }

      // get a ptr the vertex in the label file. use the global count to 
      // index.
      theVertex = &(theLabel->lv[theGlobalVoxelIndex]);
      
      // convert to ras
      VoxelToRAS ( EXPAND_VOXEL_INT(theAnatomicalVoxel),
       &theRASX, &theRASY, &theRASZ );

      // set the vertex
      theVertex->x = theRASX;
      theVertex->y = theRASY;
      theVertex->z = theRASZ;
      
      // set the vno and stat value to something decent and deleted to not
      theVertex->vno = theGlobalVoxelIndex;
      theVertex->stat = 0;
      theVertex->deleted = FALSE;

      // inc our global count.
      theGlobalVoxelIndex ++;
    }
  }

  // write the file
  theLabelError = LabelWrite ( theLabel, inFileName );
  if ( NO_ERROR != theLabelError ) {
    DebugPrint "Error in LabelWrite().\n" EndDebugPrint;
    OutputPrint "ERROR: Couldn't write label to file.\n" EndOutputPrint;
    Voxel_Delete ( &theAnatomicalVoxel );
    return;
  }

  // free it
  LabelFree ( &theLabel );

  Voxel_Delete ( &theAnatomicalVoxel );
}

void LoadSelectionFromLabelFile ( char * inFileName ) {

  LABEL *theLabel;
  LABEL_VERTEX *theVertex;
  int theNumVoxels, theVoxelIndex;
  int theVoxX, theVoxY, theVoxZ;
  VoxelRef theVoxel;

  Voxel_New ( &theVoxel );

  DebugPrint "LoadSelectionFromLabelFile ( %s )\n", inFileName EndDebugPrint;

  // read in the label
  theLabel = LabelRead ( pname, inFileName );
  if ( NULL == theLabel ) {
    DebugPrint "\tError reading label file.\n" EndDebugPrint;
    OutputPrint "ERROR: Couldn't read the label.\n" EndOutputPrint;
  }

  // for each vertex in there...
  theNumVoxels = theLabel->max_points;
  for ( theVoxelIndex = 0; theVoxelIndex < theNumVoxels; theVoxelIndex++ ) {

    // get the vertex.
    theVertex = &(theLabel->lv[theVoxelIndex]);
    
    // only process verticies that arn't deleted.
    if ( !(theVertex->deleted) ) {
      
      // covert from ras to voxel
      RASToVoxel ( theVertex->x, theVertex->y, theVertex->z,
       &theVoxX, &theVoxY, &theVoxZ );

      // add to the selection
      Voxel_Set ( theVoxel, theVoxX, theVoxY, theVoxZ );
      AddVoxelToSelection ( theVoxel, NULL );
    }
  }

  // dump the label
  LabelFree ( &theLabel );
  
  Voxel_Delete ( &theVoxel );
}

// ===========================================================================

// =============================================================== BRUSH UTILS

void PaintVoxels ( void(*inFunction)(VoxelRef,void*), void * inData,
       VoxelRef inStartingPoint ) {

  int theXRadius, theYRadius, theZRadius;
  int theX, theY, theZ;
  int theStartX, theStartY, theStartZ;
  VoxelRef theVoxel;

  Voxel_New ( &theVoxel );

  // set all radiuses to the global radius
  theXRadius = theYRadius = theZRadius = gBrushRadius;

  // if our 3d flag is not on, limit the radius of the plane we're in.
  if ( !gBrushIsIn3D ) {
    switch ( plane ) {
    case CORONAL:
      theZRadius = 0;
      break;
    case HORIZONTAL:
      theYRadius = 0;
      break;
    case SAGITTAL:
      theXRadius = 0; 
      break;
    }
  }
  
  theStartX = Voxel_GetX ( inStartingPoint );
  theStartY = Voxel_GetY ( inStartingPoint );
  theStartZ = Voxel_GetZ ( inStartingPoint );

  // loop around the starting point...
  for ( theZ = theStartZ - theZRadius; 
  theZ <= theStartZ + theZRadius; theZ++ ) {
    for ( theY = theStartY - theYRadius; 
    theY <= theStartY + theYRadius; theY++ ) {
      for ( theX = theStartX - theXRadius; 
      theX <= theStartX + theXRadius; theX++ ) {
  
        // if this voxel is valid..
        DisableDebuggingOutput;
        if ( IsVoxelInBounds ( theX, theY, theZ ) ) {
          
          EnableDebuggingOutput;
        
          // if we're drawing in a circle and this point is outside
          // our radius squared, skip this voxel.
          if ( kShape_Circular == gBrushShape && 
               (theStartX-theX)*(theStartX-theX) + 
               (theStartY-theY)*(theStartY-theY) + 
               (theStartZ-theZ)*(theStartZ-theZ) > 
         gBrushRadius*gBrushRadius ) {
            continue;
          }

    // run the function on this voxel.
    Voxel_Set ( theVoxel, theX, theY, theZ );
    inFunction ( theVoxel, inData );
        }

        EnableDebuggingOutput;
      }
    }
  }

  Voxel_Delete ( &theVoxel );
}

void SetBrushRadius ( int inRadius ) {

  DebugPrint "SetBrushRadius ( %d )\n", inRadius EndDebugPrint;

  gBrushRadius = inRadius;
}

void SetBrushShape ( Brush_Shape inShape ) {

  DebugPrint "SetBrushShape ( %d )\n", (int)inShape EndDebugPrint;

  gBrushShape = inShape;
}

void SetBrush3DStatus ( char inIsIn3D ) {

  DebugPrint "SetBrush3DStatus ( %d )\n", (int)inIsIn3D EndDebugPrint;

  gBrushIsIn3D = inIsIn3D;
}

// ===========================================================================

// ============================================================ EDITING VOXELS

void EditVoxel ( VoxelRef inVoxel, void * inData ) {

  int theEditAction;
  int theNewPixelValue, thePixelValue;

  // get the action from the data ptr.
  theEditAction = *(int*)inData;

  // get the pixel value at this voxel.
  thePixelValue = GetVoxelValue ( EXPAND_VOXEL_INT(inVoxel) );
  
  // choose a new color.
  if ( TO_WHITE == theEditAction && 
       thePixelValue < WM_MIN_VAL ) {
    
    theNewPixelValue = 255;
    
  } else if ( TO_BLACK == theEditAction ) {
    
    theNewPixelValue = WM_EDITED_OFF;
    
  } else {
    
    theNewPixelValue = thePixelValue;
  }
  
  
  // if this pixel is different from the new value..
  if ( theNewPixelValue != thePixelValue ) {
    
    // save this value in the undo list.
    AddVoxelAndValueToUndoList ( inVoxel, thePixelValue );
  }
  
  // set the pixel value.
  SetVoxelValue ( EXPAND_VOXEL_INT(inVoxel), theNewPixelValue );

  // mark the slice files that we changed.
  changed [ Voxel_GetZ(inVoxel) ] = TRUE;
  editedimage = imnr0 + Voxel_GetZ(inVoxel);
  
}

// ===========================================================================


/* ============================================= Coordinate transformations */

                                        /* convert a click to screen coords. 
                                           only converts the two coords that 
                                           correspond to the x/y coords on
                                           the visible plane, leaving the
                                           third untouched, so the out
                                           coords should be set to the current
                                           cursor coords before calling. */
void ClickToScreen ( int inH, int inV, int inPlane,
                     int *ioScreenJ, int *ioScreenI, int *ioScreenIM ) {

  long theOriginH, theOriginV, theSizeH, theSizeV;
  int theLocalH, theLocalV;

  getorigin ( &theOriginH, &theOriginV );
  getsize ( &theSizeH, &theSizeV );

  // if click is out of bounds for some reason, set to middle of screen.
  // probably not the best way to do this.... damn legacy code.
  if ( inH < theOriginH || inH > theOriginH+theSizeH ) 
    inH = theOriginH + theSizeH/2;
  if ( inV < theOriginV || inV > theOriginV+theSizeV )
    inV = theOriginV + theSizeV/2;

  // subtract the origin from the click to get local click
  theLocalH = inH - theOriginH;
  theLocalV = inV - theOriginV;

  // first find out what we clicked on. if we're in all3 mode...
  if ( all3flag ) {
    
    // first we find out what part of the screen we clicked in...
    if ( theLocalH < xdim/2 && theLocalV > ydim/2 ) {  /* COR */
      
      // then set our two coords accordingly. leave the other coords
      // untouched, as it will represent the z plane of the screen,
      // one that isn't affected by clicking.
      *ioScreenJ = 2 * theLocalH ;
      *ioScreenI = 2 * ( theLocalV - ydim/2 );

    } else if ( theLocalH < xdim/2 && theLocalV < ydim/2 ) {/* HOR */
      
      *ioScreenJ =  2 * theLocalH;
      *ioScreenIM = 2 * theLocalV;

    } else if ( theLocalH > xdim/2 && theLocalV > ydim/2 ) { /* SAG */
      
      *ioScreenIM = 2 * ( theLocalH - xdim/2 );
      *ioScreenI = 2 * ( theLocalV - ydim/2 );

    } else if ( theLocalH > xdim/2 && theLocalV < ydim/2 ) { // lowerleft quad
      
      *ioScreenJ = *ioScreenI = *ioScreenIM = xdim/2;
    }
    
  } else {
    
    // depending what plane we're in, use our screen coords to get
    // screenspace i/j/im coords.
    switch ( plane ) {
      
    case CORONAL: 
      *ioScreenJ = theLocalH;
      *ioScreenI = theLocalV;
      break;
      
    case HORIZONTAL:
      *ioScreenJ = theLocalH;
      *ioScreenIM = theLocalV;
      break;
      
    case SAGITTAL:
      *ioScreenIM = theLocalH;
      *ioScreenI = theLocalV;
      break;
    }
  }

}
                                       /* converts screen coords to 
                                          255 based voxel coords and back */
void ScreenToVoxel ( int inPlane,               // the plane we're in
                     int j, int i, int im,      // incoming screen coords
                     int *x, int *y, int *z) {  // outgoing voxel coords

  // if we're out of bounds...
  if ( ! IsScreenPointInBounds ( inPlane, j, i, im ) ) {
    
    DebugPrint "ScreenToVoxel(): Input screen pt was invalid.\n" EndDebugPrint;

    // just try not to crash. 
    *x = *y = *z = 0;
    return;
  }

  // if in all3 view, everything goes to regular non zoomed coords.
  if ( all3flag ) {

    *x = ScreenXToVoxelX ( j );
    *y = ScreenYToVoxelY ( i );
    *z = ScreenZToVoxelZ ( im );

  } else {    

    // we don't want to convert the coord that currently
    // represents the z axis in local zoom space - that just gets a normal
    // zf division. otherwise our 0-255 sliders don't work, since our 
    // range becomes 0-255*gLocalZoom.
    switch ( inPlane ) {
    case CORONAL:
      *x = LocalZoomXToVoxelX ( j );
      *y = LocalZoomYToVoxelY ( i );
      *z = ScreenZToVoxelZ ( im );
      break;
      
    case HORIZONTAL:
      *x = LocalZoomXToVoxelX ( j );
      *y = ScreenYToVoxelY ( i );
      *z = LocalZoomZToVoxelZ ( im );
      break;
      
    case SAGITTAL:
      *x = ScreenXToVoxelX ( j );
      *y = LocalZoomYToVoxelY ( i );
      *z = LocalZoomZToVoxelZ ( im );
      break;
    }
  }

  // check again...
  if ( ! IsVoxelInBounds ( *x, *y, *z ) ) {

    DebugPrint "ScreenToVoxel(): Valid screen point (%d,%d,%d) converted to invalid voxel.\n", j, i, im EndDebugPrint;

    // again, try not to crash.
    *x = *y = *z = 0;
  }
}

// screen to voxel and vice versa just switches between the zoomed and 
// unzoomed coords. when a window is zoomed in to show a particular portion
// of the model, we are in 'local zoom' mode. that conversion must consider
// the local zoom factor (different from the normal scale factor) and the 
// current center point.
// screen to local zoom: first scale the screen coord
// to voxel coord by dividing it by zf. then divide it by the local 
// zoom factor to zoom it up again. that gets us the right scale. then we
// move over according to the center. to do that we calc the left edge 
// of the centered display in local zoom mode, which involves subtracting
// half the number of voxels available in the screen from the central voxel
inline int ScreenXToVoxelX ( int j ) {
  return ( j / zf ); }

inline int LocalZoomXToVoxelX ( int j ) {
  return ( (j/zf) / gLocalZoom ) + ( gCenterX - (xdim/zf/2/gLocalZoom) ); }

inline int ScreenYToVoxelY ( int i ) {
  return ( ((ydim - 1) - i) / zf ); }
//return ( (ydim - i) / zf ); }

inline int LocalZoomYToVoxelY ( int i ) {
  return (((ydim-1-i)/zf)/gLocalZoom) + (gCenterY-(ydim/zf/2/gLocalZoom)); } 
//return (((ydim-i)/zf)/gLocalZoom) + (gCenterY-(ydim/zf/2/gLocalZoom)); } 

inline int ScreenZToVoxelZ ( int im ) {
  return ( im / zf ); }

inline int LocalZoomZToVoxelZ ( int im ) {
  return ( (im/zf) / gLocalZoom ) + ( gCenterZ - (xdim/zf/2/gLocalZoom) ); }

int TclScreenToVoxel ( ClientData inClientData, Tcl_Interp * inInterp,
                       int argc, char ** argv ) {

  int j, i, im, x, y, z, plane;
  char theNumStr[10];
  
  // check the number of args.
  if ( argc != 5 ) {
    inInterp->result = "Wrong # of args: ScreenToVoxel plane j i im";
    return TCL_ERROR;
  }

  // pull the input out.
  plane = atoi ( argv[1] );
  j = atoi ( argv[2] );
  i = atoi ( argv[3] );
  im = atoi ( argv[4] );

  // run the function.
  ScreenToVoxel ( plane, j, i, im, &x, &y, &z );

  // copy the input into the result string.
  sprintf ( theNumStr, "%d", x );
  Tcl_AppendElement ( interp, theNumStr );
  sprintf ( theNumStr, "%d", y );
  Tcl_AppendElement ( interp, theNumStr );
  sprintf ( theNumStr, "%d", z );
  Tcl_AppendElement ( interp, theNumStr );

  return TCL_OK;
}

void VoxelToScreen ( int x, int y, int z,       // incoming voxel coords
                     int inPlane,               // the plane we're in
                     int *j, int *i, int *im ){ // outgoing screen coords

  // voxel coords should be in good space
  if ( ! IsVoxelInBounds ( x, y, z ) ) {

    DebugPrint "VoxelToScreen(): Input voxel was invalid.\n" EndDebugPrint;

    // try not to crash.
    *j = *i = *im = 0;
    return;
  }
  
  // in all3 view, everything goes to unzoomed coords.
  if ( all3flag ) {

    *j = VoxelXToScreenX ( x );
    *i = VoxelYToScreenY ( y );
    *im = VoxelZToScreenZ ( z );

  } else {    

    // scales voxel coords up according to zoom level of the screen. flips
    // the i/y axis.
    switch ( inPlane ) {
    case CORONAL:
      *j = VoxelXToLocalZoomX ( x );
      *i = VoxelYToLocalZoomY ( y );
      *im = VoxelZToScreenZ ( z );
      break;
      
    case HORIZONTAL:
      *j = VoxelXToLocalZoomX ( x );
      *i = VoxelYToScreenY ( y );
      *im = VoxelZToLocalZoomZ ( z );
      break;
      
    case SAGITTAL:
      *j = VoxelXToScreenX ( x );
      *i = VoxelYToLocalZoomY ( y );
      *im = VoxelZToLocalZoomZ ( z );
      break;
    }
  }

  // check again
  if ( ! IsScreenPointInBounds ( inPlane, *j, *i, *im ) ) {

    DebugPrint "VoxelToScreen(): Valid voxel (%d,%d,%d) converted to invalid screen pt.\n", x, y, z EndDebugPrint;

    // try not to crash.
    *j = *i = *im = 0;
  }

}

inline int VoxelXToScreenX ( int x ) {
  return ( x * zf ); }

inline int VoxelXToLocalZoomX ( int x ) {
  return ( gLocalZoom * zf * (x - gCenterX) ) + (xdim/2); }

inline int VoxelYToScreenY ( int y ) {
  return ( (ydim - 1) - (zf * y) ); }
//return ( ydim - (zf * y) ); }

inline int VoxelYToLocalZoomY ( int y ) {
  return (ydim-1) - ( gLocalZoom * zf * (y - gCenterY) ) - (ydim/2); }
//return ydim - ( gLocalZoom * zf * (y - gCenterY) ) - (ydim/2); }

inline int VoxelZToScreenZ ( int z ) {
  return ( z * zf ); }

inline int VoxelZToLocalZoomZ ( int z ) {
  return ( gLocalZoom * zf * (z - gCenterZ) ) + (xdim/2); }


int TclVoxelToScreen ( ClientData inClientData, Tcl_Interp * inInterp,
                       int argc, char ** argv ) {

  int j, i, im, x, y, z, plane;
  char theNumStr[10];
  
  // check the number of args.
  if ( argc != 5 ) {
    inInterp->result = "Wrong # of args: VoxelToScreen j i im plane";
    return TCL_ERROR;
  }

  // pull the input out.
  x = atoi ( argv[1] );
  y = atoi ( argv[2] );
  z = atoi ( argv[3] );
  plane = atoi ( argv[4] );

  // run the function.
  VoxelToScreen ( x, y, z, plane, &j, &i, &im );

  // copy the input into the result string.
  sprintf ( theNumStr, "%d", j );
  Tcl_AppendElement ( interp, theNumStr );
  sprintf ( theNumStr, "%d", i );
  Tcl_AppendElement ( interp, theNumStr );
  sprintf ( theNumStr, "%d", im );
  Tcl_AppendElement ( interp, theNumStr );

  return TCL_OK;
}

void ScreenToScreenXY ( int x, int y, int z,
      int plane, int *outH, int *outV ) {

  int h, v;

  h = v = 0;

  // this just is.
  switch ( plane ) {
  case CORONAL:
    h = x;
    v = y;
    if ( all3flag ) {
      h = h/2;
      v = ydim/2 + v/2;
    }
    break;
  case SAGITTAL:
    h = z;
    v = y;
    if ( all3flag ) {
      h = xdim/2 + h/2;
      v = ydim/2 + v/2;
    }
    break;
  case HORIZONTAL:
    h = x;
    v = z;
    if ( all3flag ) {
      h = h/2;
      v = v/2;
    }
    break;
  }

  *outH = h;
  *outV = v;
}


                                            /* various bounds checking. */
inline
char IsScreenPointInBounds ( int inPlane, int j, int i, int im ) {

  int theMinX, theMinY, theMinZ, theMaxX, theMaxY, theMaxZ;

  // in testing bounds, we have to flip the y axis, so the upper voxel
  // bound gets us the lower screen bound. we use the plane to see which
  // space we use to find bound, locally zoomed or regular.
  if ( SAGITTAL == inPlane && !all3flag ) {
    theMinX = VoxelXToScreenX ( 0 );
    theMaxX = VoxelXToScreenX ( xnum );  
  } else {
    theMinX = VoxelXToLocalZoomX ( 0 );
    theMaxX = VoxelXToLocalZoomX ( xnum );  
  }

  if ( HORIZONTAL == inPlane && !all3flag ) {
    theMinY = VoxelYToScreenY ( ynum );
    theMaxY = VoxelYToScreenY ( 0 );
  } else {
    theMinY = VoxelYToLocalZoomY ( ynum );
    theMaxY = VoxelYToLocalZoomY ( 0 );
  }
  
  if ( CORONAL == inPlane && !all3flag ) {
    theMinZ = VoxelZToScreenZ ( 0 );
    theMaxZ = VoxelZToScreenZ ( xnum );
  } else {
    theMinZ = VoxelZToLocalZoomZ ( 0 );
    theMaxZ = VoxelZToLocalZoomZ ( xnum );
  }

  // our coords should be in good screen space.
  if ( j < theMinX || i < theMinY || im < theMinZ ||
       j >= theMaxX || i >= theMaxY || im >= theMaxZ ) {
    
    DebugPrint "!!! Screen coords out of bounds: (%d,%d,%d)\n", 
      j, i, im EndDebugPrint;
    DebugPrint "    j: %d, %d  i: %d, %d  im: %d, %d\n", 
      theMinX, theMaxX, theMinY, theMaxY, theMinZ, theMaxZ EndDebugPrint;
    
    return FALSE;
  }
  
  return TRUE;
}

inline
char IsVoxelInBounds ( int x, int y, int z ) {

  if ( x < 0 || y < 0 || z < 0 ||
       x >= xnum || y >= ynum || z >= xnum ) {

    DebugPrint "!!! Voxel coords out of bounds: (%d, %d, %d)\n",
      x, y, z EndDebugPrint;

    return FALSE;
  }

  return TRUE;
}

inline
char IsRASPointInBounds ( Real x, Real y, Real z ) {

  if ( x < xx0 || x > xx1 || y < yy0 || y > yy1 || z < zz0 || z > zz1 ) {

    DebugPrint "!!! RAS coords out of bounds: (%2.1f, %2.1f, %2.1f)\n",
      x, y, z EndDebugPrint;
    DebugPrint "    x: %2.1f, %2.1f  y: %2.1f, %2.1f  z: %2.1f, %2.1f\n",
      xx0, xx1, yy0, yy1, zz0, zz1 EndDebugPrint;

    return FALSE;
  }

  return TRUE;
}



inline
char IsScreenPtInWindow ( int j, int i, int im ){   // in screen coords

  if ( j < 0 || i < 0 || im < 0 ||
       j >= xdim || i >= ydim || im >= xdim )
    return FALSE;

  return TRUE;
}

inline
char IsTwoDScreenPtInWindow ( int j, int i ) {      // in screen coords

  if ( j < 0 || i < 0 || j >= xdim || i >= ydim )
    return FALSE;

  return TRUE;
}

void RASToVoxel ( Real x, Real y, Real z,        // incoming ras coords
                  int *xi, int *yi, int *zi ) {  // outgoing voxel coords

  // call the function in mritransform.
  trans_RASToVoxelIndex ( x, y, z, xi, yi, zi );

  // check our bounds...
  if ( ! IsVoxelInBounds ( *xi, *yi, *zi ) ) {

    // try not to crash.
    *xi = *yi = *zi = 0;
  }
}               

void VoxelToRAS ( int xi, int yi, int zi,        // incoming voxel coords
                  Real *x, Real *y, Real *z ) {  // outgoing RAS coords

  // check our bounds...
  if ( ! IsVoxelInBounds ( xi, yi, zi ) ) {

    // try not to crash.
    *x = *y = *z = 0;
    return;
  }

  // call the function in mritransform.
  trans_VoxelIndexToRAS ( xi, yi, zi, x, y, z );
}

                                       /* convert ras coords to the two
                                          two screen coords that are relevant
                                          for this plane. this is specifically
                                          used when we are zoomed in and need
                                          to get screen coords that are more
                                          precise than int voxels. */
void RASToFloatScreenXY ( Real x, Real y, Real z,    // incoming ras coords
                          int inPlane,               // plane to convert on
                          float *j, float *i ) {     // out float screen

  Real rLocalZoom, rCenterX, rCenterY, rCenterZ;
  Real rYDim, rXDim;

  // convert everything to reals.
  rLocalZoom = (Real) gLocalZoom;
  rCenterX = (Real) gCenterX;
  rCenterY = (Real) gCenterY;
  rCenterZ = (Real) gCenterZ;
  rYDim = (Real) ydim;
  rXDim = (Real) xdim;

  // we cheat a little here. to be really consistent, this function would
  // be similar to VoxelToScreen, doing all 3 conversions. but since some
  // processors don't like this much floating math, we'll only do the two
  // we need to do for our plane. we also add or subtract 0.5 to various 
  // coords. this prevents a bunch of half-off errors that i suspect are
  // due to rounding.
  switch ( inPlane ) {
  case CORONAL:
    *j = (float) ( rLocalZoom * fsf * 
                   ( ( (xx1-(x-0.5))/ps ) - rCenterX ) + 
                   ( rXDim/2.0 ) ); 
    
    *i = (float) ( (rYDim-1.0) - rLocalZoom * fsf * 
                   ( ((zz1-z)/ps) - 255.0 + ((rYDim - 1.0)/fsf) - rCenterY) - 
                   ( rYDim/2.0 ) );
    break;

  case HORIZONTAL:
    *j = (float) ( rLocalZoom * fsf * 
                   ( ( (xx1-(x-0.5))/ps ) - rCenterX ) + 
                   ( rXDim/2.0 ) ); 
    
    *i = (float) ( rLocalZoom * zf * 
                   ( ( ((y+0.5)-yy0) / st ) - rCenterZ ) +
                   ( rXDim / 2.0 ) );
    break;

  case SAGITTAL:
    *j = (float) ( rLocalZoom * zf * 
                   ( ( ((y+0.5)-yy0) / st ) - rCenterZ ) +
                   ( rXDim / 2.0 ) );

    *i = (float) ( (rYDim-1.0) - rLocalZoom * fsf * 
                   ( ((zz1-z)/ps) - 255.0 + ((rYDim - 1.0)/fsf) - rCenterY) - 
                   ( rYDim/2.0 ) );
    break;
  }

}
    
void RASToScreen ( Real inRASX, Real inRASY, Real inRASZ,
      int inPlane, 
       int *outScreenX, int *outScreenY, int *outScreenZ ) {

  Real rLocalZoom, rCenterX, rCenterY, rCenterZ;
  Real rYDim, rXDim;
  Real theRealVoxX, theRealVoxY, theRealVoxZ;

  // do some bounds checking
  if ( ! IsRASPointInBounds ( inRASX, inRASY, inRASZ ) ) {

    DebugPrint "RASToScreen(): Input RAS pt invalid.\n" EndDebugPrint;
    return;
  }

  // convert everything to reals.
  rLocalZoom = (Real) gLocalZoom;
  rCenterX = (Real) gCenterX;
  rCenterY = (Real) gCenterY;
  rCenterZ = (Real) gCenterZ;
  rYDim = (Real) ydim;
  rXDim = (Real) xdim;

  // first get our voxel x y and z in real form.
  theRealVoxX = (Real) ( (xx1-inRASX) / ps );
  theRealVoxY = (Real) ( (zz1-inRASZ) / ps );
  theRealVoxZ = (Real) ( (inRASY-yy0) / st );

  // then convert them to screen pts
  switch ( inPlane ) {
  case CORONAL:
    *outScreenX = (int)
      ( rLocalZoom * fsf * (theRealVoxX - rCenterX) ) + (rYDim/2.0);
    *outScreenY = (int)
      ( (rYDim-1.0) -
  ( rLocalZoom * fsf * (theRealVoxY - rCenterY) ) - (rYDim/2.0) );
    *outScreenZ = (int)
      ( theRealVoxZ * fsf );
    break;

  case HORIZONTAL:
    *outScreenX = (int)
      ( ( rLocalZoom * fsf * (theRealVoxX - rCenterX) ) + (rYDim/2.0) );
    *outScreenY = (int)
      ( (rYDim - 1.0) - (fsf * theRealVoxY) );
    *outScreenZ = (int)
      ( ( rLocalZoom * fsf * (theRealVoxZ - rCenterZ) ) + (rYDim/2.0) );
    break;

  case SAGITTAL:
    *outScreenX = (int)
      ( theRealVoxX * fsf );
    *outScreenY = (int)
      ( (rYDim-1.0) -
  ( rLocalZoom * fsf * (theRealVoxY - rCenterY) ) - (rYDim/2.0) );
    *outScreenZ = (int)
      ( ( rLocalZoom * fsf * (theRealVoxZ - rCenterZ) ) + (rYDim/2.0) );
    break;
  }

  // do some bounds checking
  if ( ! IsScreenPointInBounds ( plane, *outScreenX, *outScreenY, *outScreenZ ) ) {

    DebugPrint "RASToScreen(): Output screen pt invalid.\n" EndDebugPrint;
    *outScreenX = *outScreenY = *outScreenZ = 0;
  }

}

void ScreenToRAS ( int inPlane, int inScreenX, int inScreenY, int inScreenZ,
       Real * outRASX, Real * outRASY, Real * outRASZ ) {

  Real rScreenX, rScreenY, rScreenZ;
  Real rLocalZoom, rCenterX, rCenterY, rCenterZ;
  Real rYDim, rXDim, rHalfScreen;
  Real theRealVoxX, theRealVoxY, theRealVoxZ;

  // do some bounds checking
  if ( ! IsScreenPointInBounds ( plane, inScreenX, inScreenY, inScreenZ ) ) {

    DebugPrint "ScreenToRAS(): Input screen pt invalid.\n" 
      EndDebugPrint;
    return;
  }

  // convert everything to reals.
  rScreenX = (Real) inScreenX;
  rScreenY = (Real) inScreenY;
  rScreenZ = (Real) inScreenZ;
  rLocalZoom = (Real) gLocalZoom;
  rCenterX = (Real) gCenterX;
  rCenterY = (Real) gCenterY;
  rCenterZ = (Real) gCenterZ;
  rYDim = (Real) ydim;
  rXDim = (Real) xdim;
  rHalfScreen = (Real) rXDim/fsf/2.0/rLocalZoom;

  // screen to voxel.
  switch ( inPlane ) {
  case CORONAL:
    theRealVoxX =
      (rScreenX / fsf / rLocalZoom) + (rCenterX - rHalfScreen);
    theRealVoxY =
      ( ((rYDim-1.0)-rScreenY) / fsf / rLocalZoom) + (rCenterY - rHalfScreen);
    theRealVoxZ =
      ( rScreenZ / fsf );

    break;
    
  case HORIZONTAL:
    theRealVoxX =
      (rScreenX / fsf / rLocalZoom) + (rCenterX - rHalfScreen);
    theRealVoxY =
      ( ((rYDim-1.0)-rScreenY) / fsf );
    theRealVoxZ =
      (rScreenZ / fsf / rLocalZoom) + (rCenterZ - rHalfScreen);

    break;
    
  case SAGITTAL:
    theRealVoxX =
      ( rScreenX / fsf );
    theRealVoxY =
      ( ((rYDim-1.0)-rScreenY) / fsf / rLocalZoom) + (rCenterY - rHalfScreen);
    theRealVoxZ =
      (rScreenZ / fsf / rLocalZoom) + (rCenterZ - rHalfScreen);

    break;

  default:
    theRealVoxX = theRealVoxY = theRealVoxZ =0;
  }

  // voxel to ras
  *outRASX = (Real) (xx1 - (ps * theRealVoxX));
  *outRASY = (Real) (yy0 + (st * theRealVoxZ));
  *outRASZ = (Real) (zz1 - (ps * theRealVoxY));

  // do some bounds checking
  if ( ! IsRASPointInBounds ( *outRASX, *outRASY, *outRASZ ) ) {

    DebugPrint "ScreenToRAS(): Output RAS pt invalid.\n" EndDebugPrint;
    *outRASX = *outRASY = *outRASZ = 0;
    return;
  }
}

/* ===================================================== Graphics utilities */

                                      /* draws a crosshair cursor into a 
                                         video buffer. */
void DrawCrosshairInBuffer ( char * inBuffer,       // the buffer
                               int inX, int inY,      // location in buffer
                               int inSize,            // radius of crosshair
                               long inColor ) {       // color, should be a
                                                      // kRGBAColor_ value
 

  int x, y, theBufIndex;

  // go across the x line..
  for ( x = inX - inSize; x <= inX + inSize; x++ ) {

    // stay in good pixel space.
    if ( x < 0 || x >= xdim-1 )
      continue;

    // 4 bytes per pixel, cols * y down, x across
    theBufIndex = 4 * ( inY*xdim + x );

    // copy the color in.
    SetCompositePixelInBuffer ( inBuffer, theBufIndex, 1, inColor );
  }

  // same thing down the y line...
  for ( y = inY - inSize; y <= inY + inSize; y++ ) {
    if ( y < 0 || y >= ydim-1 ) 
      continue;
    theBufIndex = 4 * ( y*xdim +inX );
    SetCompositePixelInBuffer ( inBuffer, theBufIndex, 1, inColor );
  }
}

void DrawCenteredCrosshairInBuffer ( char * inBuffer,  
             int inX, int inY, 
             int inSize,       
             long inColor ) {   

  int x, y, theBufIndex;

  // add small offsets to put the cursor in the middle of a voxel.
  if ( gLocalZoom*zf > 0 ) {
    inX -= inX % (gLocalZoom*zf);
    inY -= inY % (gLocalZoom*zf);
    inX += gLocalZoom*zf/2;
    inY += gLocalZoom*zf/2;
  }

  // go across the x line..
  for ( x = inX - inSize; x <= inX + inSize; x++ ) {

    // stay in good pixel space.
    if ( x < 0 || x >= xdim-1 )
      continue;

    // 4 bytes per pixel, cols * y down, x across
    theBufIndex = 4 * ( inY*xdim + x );

    // copy the color in.
    SetCompositePixelInBuffer ( inBuffer, theBufIndex, 1, inColor );
  }

  // same thing down the y line...
  for ( y = inY - inSize; y <= inY + inSize; y++ ) {
    if ( y < 0 || y >= ydim-1 ) 
      continue;
    theBufIndex = 4 * ( y*xdim +inX );
    SetCompositePixelInBuffer ( inBuffer, theBufIndex, 1, inColor );
  }
}

                                       /* fills a box with a color.
                                          starts drawing with the input coords
                                          as the upper left and draws a box 
                                          the dimesnions of the size */
void FillBoxInBuffer ( char * inBuffer,       // the buffer
                       int inX, int inY,      // location in buffer
                       int inSize,            // width/height of pixel
                       long inColor ) {       // color, should be a
                                              // kRGBAColor_ value


  int x, y, theBufIndex;

  // from y-size+1 to y and x to x+size. the y is messed up because
  // our whole image is yflipped.
  for ( y = inY - inSize + 1; y <= inY; y++ ) {
    for ( x = inX; x < inX + inSize; x++ ) {

      // don't write out of bounds.
      if ( x < 0 || y < 0 || x >= xdim || y >= ydim )
        continue;

      // 4 bytes per pixel, cols * y down, x across
      theBufIndex = 4 * ( y*xdim + x );

      // copy the color in.
      SetCompositePixelInBuffer ( inBuffer, theBufIndex, 1, inColor );
    }
  }
}


                                                 /* fast pixel blitting */

inline
void SetGrayPixelInBuffer ( char * inBuffer, int inIndex, int inCount,
                            unsigned char inGray ) {

  // instead of copying each component one at a time, we combine
  // all components into a single long by bitshifting them to the
  // right place and then copying it all in at once.
  SetCompositePixelInBuffer ( inBuffer, inIndex, inCount,
    ( (unsigned char)kPixelComponentLevel_Max << kPixelOffset_Alpha |
      (unsigned char)inGray << kPixelOffset_Blue |
      (unsigned char)inGray << kPixelOffset_Green |
      (unsigned char)inGray << kPixelOffset_Red ) );
}

inline
void SetColorPixelInBuffer ( char * inBuffer, int inIndex, int inPixelSize,
                             unsigned char inRed, unsigned char inGreen,
                             unsigned char inBlue ) {

  SetCompositePixelInBuffer ( inBuffer, inIndex, inPixelSize,
    ( (unsigned char)kPixelComponentLevel_Max << kPixelOffset_Alpha |
      (unsigned char)inBlue << kPixelOffset_Blue |
      (unsigned char)inGreen<< kPixelOffset_Green |
      (unsigned char)inRed << kPixelOffset_Red ) );
}

inline
void SetCompositePixelInBuffer ( char * inBuffer, int inBaseIndex, int inPixelSize,
                                 long inPixel ) {
  int theRow, theCol, theIndex;
  for ( theRow = 0; theRow < inPixelSize; theRow ++ ) {
    for ( theCol = 0; theCol < inPixelSize;  theCol++ ) {

      theIndex = inBaseIndex +
  (sizeof(long) * theRow * xdim) + // rows
  (sizeof(long) * theCol);         // cols
      *(long*)&(inBuffer[theIndex]) = (long) inPixel;
    }
  }
}


/* ================================================ Center point utilities */
                                       /* sets the center voxel point */
void SetCenterVoxel ( int x, int y, int z ) {

  // check the bounds
  if ( x < 0 || y < 0 || z < 0 ||
       x >= xnum || y >= ynum || z >= xnum ) {
    fprintf ( stderr, "SetCenterVoxel(): Invalid voxel: (%d, %d, %d)\n",
              x, y, z );
    return;
  }

  // set the center
  gCenterX = x;
  gCenterY = y;
  gCenterZ = z;

  // make sure it's in bounds.
  CheckCenterVoxel ();
}

                                       /* checks for empty space on the  
                                          outside of the window with the 
                                          current center and zoom settings,
                                          and moves the center inward if
                                          necessary. */
void CheckCenterVoxel () {

#if 0
  int theEdge;

  // check left edge.
  theEdge = gCenterX - (xdim/zf/2/gLocalZoom);
  if ( theEdge < 0 )
    gCenterX += 0 - theEdge;

  // check right edge
  theEdge = gCenterX + (xdim/zf/2/gLocalZoom);
  if ( theEdge >= xdim/zf )
    gCenterX -= theEdge - xdim/zf;

  // check top edge.
  theEdge = gCenterY - (ydim/zf/2/gLocalZoom);
  if ( theEdge < 0 )
    gCenterY += 0 - theEdge;

  // check bottom edge
  theEdge = gCenterY + (ydim/zf/2/gLocalZoom);
  if ( theEdge >= ydim/zf )
    gCenterY -= theEdge - ydim/zf;

  // check the z edge.
  theEdge = gCenterZ - (xdim/zf/2/gLocalZoom);
  if ( theEdge < 0 )
    gCenterZ += 0 - theEdge;

  // check the other z edge
  theEdge = gCenterZ + (xdim/zf/2/gLocalZoom);
  if ( theEdge >= xdim/zf )
    gCenterZ -= theEdge - xdim/zf;
#endif

  // just make sure we're in vox bounds.
  if ( !IsVoxelInBounds ( gCenterX, gCenterY, gCenterZ ) ) {

    // if not, set it to middle.
    gCenterX = gCenterY = gCenterZ = xnum/2;
  }
}


void GetCursorInScreenCoords ( VoxelRef ioVoxel ) {

  Voxel_Set ( ioVoxel, jc, ic, imc );
}

void GetCursorInVoxelCoords ( VoxelRef ioVoxel ) {

  int x, y, z;

  ScreenToVoxel ( plane, jc, ic, imc, &x, &y, &z );
  Voxel_Set ( ioVoxel, x, y, z );
}

/* ===================================================== General utilities */


void PrintScreenPointInformation ( int j, int i, int ic ) {

  int theVoxX, theVoxY, theVoxZ;
  Real theTalX, theTalY, theTalZ;
  Real theRASX, theRASY, theRASZ;
  int thePixelValue;
  char theResult, theValue;
  VoxelRef theVoxel;
  float theFloatScreenX, theFloatScreenY;
  int theScreenX, theScreenY;
  float theFunctionalValue;

  Voxel_New ( &theVoxel );

  // get the voxel coords
  ScreenToVoxel ( plane, j, i, ic, &theVoxX, &theVoxY, &theVoxZ );

  // grab the value and print it
  thePixelValue = im[theVoxZ][theVoxY][theVoxX];
  printf ( "Value: %s = %d", imtype, thePixelValue );

  // we need to set selectedpixval because it's linked in the tcl script
  // and will change the title bar
  selectedpixval = thePixelValue;

  // if there's a second image, print that value too
  if ( second_im_allocated ) {
    thePixelValue = im2[theVoxZ][theVoxY][theVoxX];
    printf ( ", %s = %d", imtype2, thePixelValue );

    // we set secondpixval so redraw() will se the window title.
    secondpixval = thePixelValue;
  }

  printf ( "\n" );

  // get the ras coords
  ScreenToRAS ( plane, j, i, ic, &theRASX, &theRASY, &theRASZ );

  // print the screen, vox, and ras coords
  printf ( "Coords: Screen (%d,%d,%d) Voxel (%d,%d,%d)\n",
           j, i, ic, theVoxX, theVoxY, theVoxZ );
  printf ( "        RAS (%2.1f,%2.1f,%2.1f)",
           theRASX, theRASY, theRASZ );

  // if we have a transform
  if ( transform_loaded ) {

    // get that point too
    transform_point ( linear_transform,
                      theRASX, theRASY, theRASZ,
                      &theTalX, &theTalY, &theTalZ );

    // and print it
    printf ( " Talairach (%2.1f,%2.1f,%2.1f)",
             theTalX, theTalY, theTalZ );
  }


  // if we're doing an overlay..
  if ( TRUE == do_overlay &&
       getenv ( "I_WANT_FUNCTIONAL_OVERLAY_OUTPUT" ) ) {

    // transform RAS to screen xy
    RASToFloatScreenXY ( theRASX, theRASY, theRASZ, plane, 
       &theFloatScreenX, &theFloatScreenY );

    // cast to int.
    theScreenX = (int)theFloatScreenX;
    theScreenY = (int)theFloatScreenY;

    // get the overlay value out of the cache. this is twitzel's formula
    // from compose().
    theFunctionalValue = fcache[theScreenY*512 + theScreenX];

    printf ( "\nFunctional value: %2.5f\n", theFunctionalValue );
  }

  // check for the other functional value...
  Voxel_Set ( theVoxel, theVoxX, theVoxY, theVoxZ );
  if ( FuncDis_IsDataLoaded() ) {

    FuncDis_GetValueAtAnatomicalVoxel ( theVoxel, &theFunctionalValue );
    OutputPrint "\nFunctional value: %2.5f\n", 
      theFunctionalValue EndOutputPrint;
  }

  // get the value for this voxel
  theResult = VSpace_IsInList ( gCtrlPtList, theVoxel, &theValue );
  if ( theResult != kVSpaceErr_NoErr ) {
    DebugPrint"PrintScreenInformation(): Error in VSpace_IsInList: %s\n",
              VSpace_GetErrorString ( theResult ) EndDebugPrint;
    theValue = 0;
  }
  
  if ( theValue )
    printf ( "\nThis is a control point." );
  
  print ( "\n" );
  PR;

  Voxel_Delete ( &theVoxel );
}


void PrintZoomInformation () {

  int theLeft, theTop, theRight, theBottom;

  // if we're zoomed, print the center point and view rectangle
  if ( gLocalZoom > 1 ) {
    
    printf ( "\n\nZoom: Level: %dx Center voxel: (%d, %d, %d)",
             gLocalZoom, gCenterX, gCenterY, gCenterZ );
    
    switch ( plane ) {
    case CORONAL:
      theLeft = LocalZoomXToVoxelX ( 0 );
      theRight = LocalZoomXToVoxelX ( xdim-1 );
      theTop = LocalZoomYToVoxelY ( 0 );
      theBottom = LocalZoomYToVoxelY ( ydim-1 );
      
      printf ( "\n      Viewing voxel rect (x%d, y%d, x%d, y%d)", 
               theLeft, theTop, theRight, theBottom);
      break;
      
    case HORIZONTAL:
      theLeft = LocalZoomXToVoxelX ( 0 );
      theRight = LocalZoomXToVoxelX ( xdim-1 );
      theTop = LocalZoomZToVoxelZ ( 0 );
      theBottom = LocalZoomZToVoxelZ ( ydim-1 );
      
      printf ( "\n      Viewing voxel rect (x%d, z%d, x%d, z%d)", 
               theLeft, theTop, theRight, theBottom);
      break;
      
    case SAGITTAL:
      theTop = LocalZoomYToVoxelY ( 0 );
      theBottom = LocalZoomYToVoxelY ( ydim-1 );
      theLeft = LocalZoomZToVoxelZ ( 0 );
      theRight = LocalZoomZToVoxelZ ( xdim-1);
      
      printf ( "\n      Viewing voxel rect (z%d, y%d, z%d, y%d)", 
               theLeft, theTop, theRight, theBottom);
      break;
    }
  }

  printf ( "\n" );
  PR;
}


void SendUpdateMessageToTKWindow () {

  Tcl_Eval ( GetTCLInterp(), "unzoomcoords; sendupdate;" );
}

void SetTCLInterp ( Tcl_Interp * inInterp ) {

  gTCLInterp = inInterp;
}

Tcl_Interp * GetTCLInterp () {

  return gTCLInterp;
}

void SendTCLCommand ( char * inCommand ) {
  
  int theErr;
  Tcl_Interp * theInterp;

  // get the interp and send the command.
  theInterp = GetTCLInterp ();

  if ( NULL != theInterp ) {
    DebugPrint "Sending cmd: %s\n", inCommand EndDebugPrint;
    theErr = Tcl_Eval ( theInterp, inCommand );
    if ( *theInterp->result != 0 ) {
      DebugPrint "Cmd: %s\n", inCommand EndDebugPrint;
      DebugPrint "\tResult: %s\n", theInterp->result EndDebugPrint;
    }
    if ( TCL_OK != theErr ) {
      DebugPrint "Cmd: %s\n", inCommand EndDebugPrint;
      DebugPrint "\tCommand did not return OK.\n" EndDebugPrint;
      DebugPrint "\tResult: %s\n", theInterp->result EndDebugPrint;
    }
  }
}



inline
void SetVoxelValue ( int x, int y, int z, unsigned char inValue ) {

  if ( IsVoxelInBounds ( x, y, z ) ) {

    im[z][y][x] = inValue;
  }
}

inline
unsigned char GetVoxelValue ( int x, int y, int z ) {

  if ( IsVoxelInBounds ( x, y, z ) ) {

    return im[z][y][x];
  }

  return 0;
}

// ============================================================== EDITING UNDO

void InitUndoList () {

  xUndL_tErr theErr;

  // new our list.
  theErr = xUndL_New ( &gUndoList, 
           &UndoActionWrapper, &DeleteUndoEntryWrapper );
  if ( xUndL_tErr_NoErr != theErr ) {
    DebugPrint "InitUndoList(): Error in xUndL_New %d: %s\n",
      theErr, xUndL_GetErrorString ( theErr ) EndDebugPrint;
    gUndoList = NULL;
  }

  // set the print function so we can print if necessary.
  theErr = xUndL_SetPrintFunction ( gUndoList, &PrintEntryWrapper );
  if ( xUndL_tErr_NoErr != theErr ) {
    DebugPrint "InitUndoList(): Error in xUndL_SetPrintFunction %d: %s\n",
      theErr, xUndL_GetErrorString ( theErr ) EndDebugPrint;
    gUndoList = NULL;
  }
}

void DeleteUndoList () {

  xUndL_tErr theErr;

  // delete the list.
  theErr = xUndL_Delete ( &gUndoList );
  if ( xUndL_tErr_NoErr != theErr ) {
    DebugPrint "DeleteUndoList(): Error in xUndL_Delete %d: %s\n",
      theErr, xUndL_GetErrorString ( theErr ) EndDebugPrint;
  }
}

void NewUndoEntry ( UndoEntryRef* outEntry,
        VoxelRef inVoxel, unsigned char inValue ) {

  UndoEntryRef this = NULL;
  
  // assume failure.
  *outEntry = NULL;

  // allocate the entry.
  this = (UndoEntryRef) malloc ( sizeof(UndoEntry) );
  if ( NULL == this ) {
    DebugPrint "NewUndoEntry(): Error allocating entry.\n" EndDebugPrint;
    return;
  }

  // copy the voxel in.
  Voxel_New ( &(this->mVoxel) );
  Voxel_Copy ( this->mVoxel, inVoxel );

  // copy the value in.
  this->mValue = inValue;

  *outEntry = this;
}

void DeleteUndoEntry ( UndoEntryRef* ioEntry ) {

  UndoEntryRef this = NULL;

  this = *ioEntry;
  if ( NULL == this ) {
    DebugPrint "DeleteUndoEntry(): Got NULL entry.\n" EndDebugPrint;
    return;
  }
  
  // delete the voxel.
  Voxel_Delete ( &(this->mVoxel) );
  
  // delete the entry.
  free ( this );

  *ioEntry = NULL;
}

void PrintUndoEntry ( UndoEntryRef this ) {

  if ( NULL == this ) {
    DebugPrint "PrintUndoEntry() : INVALID ENTRY\n" EndDebugPrint;
    return;
  }

  DebugPrint "%p voxel (%d,%d,%d)  value = %d\n", this,
    EXPAND_VOXEL_INT(this->mVoxel), this->mValue EndDebugPrint;

}

void ClearUndoList () {

  xUndL_tErr theErr;

  // clear the list.
  theErr = xUndL_Clear ( gUndoList );
  if ( xUndL_tErr_NoErr != theErr ) {
    DebugPrint "ClearUndoList(): Error in xUndL_Clear %d: %s\n",
      theErr, xUndL_GetErrorString ( theErr ) EndDebugPrint;
  }
}

void AddVoxelAndValueToUndoList ( VoxelRef inVoxel, int inValue ) {

  UndoEntryRef theEntry = NULL;
  xUndL_tErr theErr;

  // make the entry.
  NewUndoEntry ( &theEntry, inVoxel, (unsigned char)inValue );
  if ( NULL == theEntry ) {
    DebugPrint "AddVoxelAndValueToUndoList(): Couldn't create entry.\n"
      EndDebugPrint;
    return;
  }

  // add the entry.
  theErr = xUndL_AddEntry ( gUndoList, theEntry );
  if ( xUndL_tErr_NoErr != theErr ) {
   DebugPrint "AddVoxelAndValueToUndoList(): Error in xUndL_AddEntry %d: %s\n",
      theErr, xUndL_GetErrorString ( theErr ) EndDebugPrint;
  }
}

void RestoreUndoList () {
  
  xUndL_tErr theErr;

  // restore the list.
  theErr = xUndL_Restore ( gUndoList );
  if ( xUndL_tErr_NoErr != theErr ) {
    DebugPrint "RestoreUndoList(): Error in xUndL_Restore %d: %s\n",
      theErr, xUndL_GetErrorString ( theErr ) EndDebugPrint;
  }
}

void DeleteUndoEntryWrapper ( xUndL_tEntryPtr* inEntryToDelete ) {

  if ( NULL == *inEntryToDelete ) {
    DebugPrint "DeleteUndoEntryWrapper(): Got null entry.\n" EndDebugPrint;
    return;
  }
    
  // just typecast and call.
  DeleteUndoEntry ( (UndoEntryRef*)inEntryToDelete );
  *inEntryToDelete = NULL;
}

void UndoActionWrapper      ( xUndL_tEntryPtr  inUndoneEntry, 
            xUndL_tEntryPtr* outNewEntry ) {

  UndoEntryRef theEntryToUndo, theUndoneEntry;
  unsigned char theVoxelValue;

                                   /* we're getting an undo entry that should
              be undone or restored. we also want to
              create a new entry and pass it back, so
              we can undo the undo. */

  // get the entry and check it.
  theEntryToUndo = (UndoEntryRef) inUndoneEntry;
  if ( NULL == theEntryToUndo ) {
    DebugPrint "UndoActionWrapper(): Got null entry.\n" EndDebugPrint;
    return;
  }

  // get the value at this voxel.
  theVoxelValue = GetVoxelValue ( EXPAND_VOXEL_INT(theEntryToUndo->mVoxel) );

  // create an entry for it.
  NewUndoEntry ( &theUndoneEntry, theEntryToUndo->mVoxel, theVoxelValue );

  // set the voxel value.
  SetVoxelValue ( EXPAND_VOXEL_INT(theEntryToUndo->mVoxel),
      theEntryToUndo->mValue );

  // pass back the new entry.
  *outNewEntry = theUndoneEntry;
}

void PrintEntryWrapper ( xUndL_tEntryPtr inEntry ) {

  PrintUndoEntry ( (UndoEntryRef) inEntry );
}

void SetMode ( Interface_Mode inMode ) {
  
  gMode = inMode;

  switch ( gMode ) {
  case kMode_Edit:

    control_points = -1;

    OutputPrint "Now in editing mode.\n" EndOutputPrint;
    break;
  case kMode_CtrlPt:
    OutputPrint "Now in control point mode.\n" EndOutputPrint;

    control_points = 1 ;
    
    // process the control pt file, adding all pts to it.
    ProcessCtrlPtFile ( tfname );
    
    if ( !gIsDisplayCtrlPts ) {

      OutputPrint "NOTE: Control points are currently not being displayed. "
  "To display control points, press ctrl-h.\n" EndOutputPrint;
    }
    
    // redraw new control pts
    redraw ();

    break;
  case kMode_Select:

    control_points = -1;

    OutputPrint "Now in selection mode.\n" EndOutputPrint;
    break;
  }
}

Interface_Mode GetMode () {

  return gMode;
}

char IsInMode ( Interface_Mode inMode ) {

  if ( gMode == inMode )
    return TRUE;

  return FALSE;
}

// end_kt
