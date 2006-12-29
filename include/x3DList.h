/**
 * @file  x3DList.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 02:09:01 $
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


#ifndef x3DList_H
#define x3DList_H

#include "xTypes.h"
#include "xList.h"
#include "xVoxel.h"

typedef enum {
  x3Lst_tPlane_X = 0,
  x3Lst_tPlane_Y,
  x3Lst_tPlane_Z,
  x3Lst_knNumPlanes
} x3Lst_tPlane;


typedef struct
{

  tSignature mSignature;

  int        mnPlaneSize;
  xListRef*  mPlane[ x3Lst_knNumPlanes ];

}
x3DList, *x3DListRef;

#define x3Lst_kSignature 0x780123bc

typedef enum {
  x3Lst_tErr_NoErr = 0,
  x3Lst_tErr_InvalidPtr,
  x3Lst_tErr_InvalidSignature,
  x3Lst_tErr_ErrorAccessingList,
  x3Lst_tErr_ItemNotInSpace,
  x3Lst_tErr_InvalidSpace,
  x3Lst_tErr_InvalidLocation,
  x3Lst_tErr_InvalidPlaneNumber,
  x3Lst_tErr_CouldntAllocateSpace,
  x3Lst_tErr_CouldntAllocateList,
  x3Lst_tErr_InvalidErrorCode,
  x3Lst_knNumErrorCodes
} x3Lst_tErr;

/* allocate and init the space */
x3Lst_tErr x3Lst_New ( x3DListRef* op3DList,
                       int         inListSize );

/* delete the space */
x3Lst_tErr x3Lst_Delete ( x3DListRef* iop3DList );

x3Lst_tErr x3Lst_GetPlaneSize ( x3DListRef this,
                                int*       onPlaneSize );

/* add an item to the space */
x3Lst_tErr x3Lst_AddItem ( x3DListRef this,
                           xVoxelRef  iWhere,
                           void*      ipItem );

/* remove an item from the space */
x3Lst_tErr x3Lst_RemoveItem ( x3DListRef this,
                              xVoxelRef  iWhere,
                              void**     iopItem );

/* clears all the items in the
space. */
x3Lst_tErr x3Lst_Clear ( x3DListRef this );

/* determine whether or not an item
   is in the space */
x3Lst_tErr x3Lst_IsInList ( x3DListRef this,
                            void*      ipData,
                            tBoolean* outIsInList );

/* return a pointer to a list of
   items in a particular plane. */
x3Lst_tErr x3Lst_GetItemsInXPlane ( x3DListRef this,
                                    int        inXPlane,
                                    xListRef*  opList );
x3Lst_tErr x3Lst_GetItemsInYPlane ( x3DListRef this,
                                    int        inYPlane,
                                    xListRef*  opList );
x3Lst_tErr x3Lst_GetItemsInZPlane ( x3DListRef this,
                                    int        inZPlane,
                                    xListRef*  opList );
x3Lst_tErr x3Lst_GetItemsInPlane_ ( x3DListRef   this,
                                    x3Lst_tPlane iPlane,
                                    int          inPlaneIndex,
                                    xListRef*    opList );

x3Lst_tErr x3Lst_SetComparator ( x3DListRef this,
                                 xList_tCompare(*iComparator)(void*,void*) );

x3Lst_tErr x3Lst_Verify ( x3DListRef this );
x3Lst_tErr x3Lst_VerifyLocation ( x3DListRef this,
                                  xVoxelRef  iWhere );

char* x3Lst_GetErrorString ( x3Lst_tErr ieCode );


#endif
