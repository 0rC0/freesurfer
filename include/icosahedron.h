/**
 * @file  icosahedron.h
 * @brief Icosahedron utils
 *
 */
/*
 * Original Author: Bruce Fischl
 * CVS Revision Info:
 *    $Author: fischl $
 *    $Date: 2009/11/19 18:57:48 $
 *    $Revision: 1.11 $
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


#ifndef ICOSAHEDRON_H
#define ICOSAHEDRON_H

#include "mrisurf.h"

#define ICO_VERTICES_PER_FACE  3

typedef struct
{
  float x, y, z ;
}
IC_VERTEX ;

typedef struct
{
  int vno[ICO_VERTICES_PER_FACE] ;
}
IC_FACE ;

typedef struct
{
  int       nvertices ;
  int       nfaces ;
  IC_VERTEX *vertices ;
  IC_FACE   *faces ;
}
ICOSAHEDRON ;

MRI_SURFACE *ic642_make_surface(int max_vertices, int max_faces) ;
MRI_SURFACE *ic42_make_surface(int max_vertices, int max_faces) ;
MRI_SURFACE *ic40962_make_surface(int max_vertices, int max_faces) ;
MRI_SURFACE *ic10242_make_surface(int max_vertices, int max_faces) ;
MRI_SURFACE *ic2562_make_surface(int max_vertices, int max_faces) ;
MRI_SURFACE *ic162_make_surface(int max_vertices, int max_faces) ;
MRI_SURFACE *ic163842_make_surface(int max_vertices, int max_faces) ;
MRI_SURFACE *ic12_make_surface(int max_vertices, int max_faces) ;
MRI_SURFACE *ICOread(const char *fname) ;
MRI_SURFACE *ICOreadOverAlloc(const char *fname, double pct_over) ;
int          ICOreadVertexPositions(MRI_SURFACE *mris, 
                                    const char *fname, 
                                    int which) ;
MRI_SURFACE *ReadIcoByOrder(int IcoOrder, float RescaleFactor);
MRI_SURFACE *ReadIcoByNVtxs(int nIcoVtxs, float RescaleFactor);
int          IcoOrderFromNVtxs(int nIcoVtxs);
int          IcoNVtxsFromOrder(int IcoOrder);

#define ICO4_NVERTICES    2562
#define ICO4_NFACES       5120
#define ICO0_NVERTICES    12

extern IC_VERTEX ic2562_vertices[] ;
extern IC_FACE   ic2562_faces[] ;
extern IC_VERTEX ic0_vertices[12] ;
extern IC_FACE ic0_faces[20] ;


// version of ic2562 used for testing mrishash.c, contributed by G. Wideman
MRI_SURFACE *ic2562_make_two_icos(float x1, float y1, float z1, float r1,
                                  float x2, float y2, float z2, float r2 );

ICOSAHEDRON *read_icosahedron(const char *fname) ;
ICOSAHEDRON *read_icosahedron_by_order(int order) ;
int IcoFindClosestVertex(IC_VERTEX *vertices, int nvertices, float nx, float ny, float nz) ;
int IcoFindNClosestVertices(IC_VERTEX *vertices, int nvertices, float nx, float ny, float nz, int num, int *pv) ;


#define MAX_ICP_LEVELS 8
typedef struct
{
  int  nfaces ;   // total # of faces (first dimension of faces)
  int  *nmapped ;  // # of finer scale faces within this face
  int  **faces ;
} ICO_FACE_LIST, ICF ;

typedef struct 
{
  int         min_level ;
  int         nlevels ;
  MRI_SURFACE *icos[MAX_ICP_LEVELS] ;
  ICF         *icfs[MAX_ICP_LEVELS] ;
} ICO_PYRAMID, ICP ;

ICO_PYRAMID *ICPread(int min_level, int max_level) ;
int          ICPfree(ICP **picp) ;

#endif

