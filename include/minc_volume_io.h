/**
 * @file  minc_volume_io.h
 * @brief Wrapper for MNI's volume_io.h, to decouple from MNI lib
 *
 */
/*
 * Original Author: Nick Schmansky
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2011/03/02 00:04:10 $
 *    $Revision: 1.5 $
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


#ifndef MINC_VOLUME_IO_H
#define MINC_VOLUME_IO_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(BEVIN_EXCLUDE_MINC)

#include "netcdf.h"
#include "minc_multidim_arrays.h"


// The following is a replacement for some portions of
// mni/1.5/include/volume_io/basic.h
//
// As such, it needs the following Copyright notice
/*
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
*/
typedef enum { 
               OK,
               ERROR,
               INTERNAL_ERROR,
               END_OF_FILE,
               QUIT,
	       VIO_Status__end
             } VIO_Status;

char* statusToString(VIO_Status status);


// The following is a replacement for some portions of
// mni/1.5/include/volume_io/multidim.h
//
// As such, it needs the following Copyright notice
/*
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
*/

#define  VIO_MAX_DIMENSIONS     5


// The following is a replacement for some portions of
// mni/1.5/include/volume_io/geom_structs.h
//
// As such, it needs the following Copyright notice
//
/* ----------------------------------------------------------------------------
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
@VERSION    : $Header: /private-cvsroot/minc/volume_io/Include/volume_io/geom_structs.h,v 1.20.2.3 2006/11/30 09:15:13 rotor Exp $
---------------------------------------------------------------------------- */

#define VIO_N_DIMENSIONS 3

typedef struct
{
    double m[4][4];
} Transform;

#define  Transform_elem( t, i, j ) ((t).m[j][i])


// The following is a replacement for some portions of
// mni/1.5/include/volume_io/transforms.h
//
// As such, it needs the following Copyright notice
//
/* ----------------------------------------------------------------------------
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
@VERSION    : $Header: /private-cvsroot/minc/volume_io/Include/volume_io/transforms.h,v 1.12.2.2 2005/03/31 17:39:49 bert Exp $
---------------------------------------------------------------------------- */

typedef  void   (*User_transform_function)( void  *user_data,
                                            double  x,
                                            double  y,
                                            double  z,
                                            double  *x_trans,
                                            double  *y_trans,
                                            double  *z_trans );

typedef enum { 
	LINEAR,
#if defined(BEVIN_ALL_TYPES_SUPPORTED)
	THIN_PLATE_SPLINE, 
	USER_TRANSFORM,
#endif
        CONCATENATED_TRANSFORM
#if defined(BEVIN_ALL_TYPES_SUPPORTED)
	,
	GRID_TRANSFORM 
#endif
} Transform_types;


typedef struct General_transform
{
    Transform_types             type;
    bool                    	inverse_flag;

    /* --- linear transform */

    Transform               *linear_transform;
    Transform               *inverse_linear_transform;

    /* --- non-linear transform */

    int                         n_points;
    int                         n_dimensions;
    double                    **points;
    double                    **displacements;   /* n_points + n_dim + 1 by */
                                                   /* n_dim */

    /* --- grid transform */

    void                        *displacement_volume;

    /* --- user_defined */

    void                        *user_data;
    size_t                      size_user_data;
    User_transform_function     user_transform_function;
    User_transform_function     user_inverse_transform_function;

    /* --- concatenated transform */

    int                         n_transforms;
    struct General_transform    *transforms;

} General_transform;


// The following is a replacement for some portions of
// mni/1.5/include/volume_io/volume.h
// As such, it needs the following Copyright notice
/*
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
*/

typedef struct volume_struct
{
        int                     spatial_axes[VIO_N_DIMENSIONS];

        nc_type                 nc_data_type;
        double                  voxel_min;
        double                  voxel_max;

        bool                    real_range_set;
        double                  real_value_scale;
        double                  real_value_translation;
	
        double                  separations[VIO_MAX_DIMENSIONS];
	double			starts[VIO_MAX_DIMENSIONS];
        double                  direction_cosines[VIO_MAX_DIMENSIONS][VIO_N_DIMENSIONS];

        bool                    voxel_to_world_transform_uptodate;
	General_transform       voxel_to_world_transform;

        VIO_multidim_array      array;
        bool                    signed_flag;

        const char*             dimension_names[VIO_MAX_DIMENSIONS];
        bool                    is_rgba_data;
  
        const char*             coordinate_system_name;
  

//#define   BEVIN_ALL_VOLUME_MEMBERS
#if defined(BEVIN_ALL_VOLUME_MEMBERS)
	//
	// When moving one of these out, make sure the current conditionalized uses
	// have the condition removed!
	//
        bool                    is_cached_volume;
        //VIO_volume_cache_struct cache;
  
        double                 *irregular_starts[VIO_MAX_DIMENSIONS];
        double                 *irregular_widths[VIO_MAX_DIMENSIONS];
#endif

} volume_struct;

typedef volume_struct* Volume;


// The following is a replacement for some portions of
// mni/1.5/include/volume_io/vol_io_prototypes.h
// which did not have its own Copyright notice
//

void  make_identity_transform( Transform   *transform );

bool close_to_identity(
    Transform   *transform );
    
void   concat_transforms(
    Transform   *result,
    Transform   *t1,
    Transform   *t2 );

Transform* get_linear_transform_ptr(
    General_transform   *transform );

Transform* get_inverse_linear_transform_ptr(
    General_transform   *transform );

void concat_general_transforms(
    General_transform   *first,
    General_transform   *second,
    General_transform   *result );
    
void copy_general_transform(
    General_transform   *transform,
    General_transform   *copy );

void delete_general_transform(
    General_transform   *transform );

VIO_Status input_transform_file(
    const char* filename,
    General_transform   *transform );

void transform_point(
    Transform  *transform,
    double 	x,
    double	y,
    double 	z,
    double	*x_trans,
    double	*y_trans,
    double	*z_trans );


#else


/*
 * Wrapper for MNI's volume_io.h, which has some annoyances which
 * must be circumvented.
 */

/* remove unwanted warnings between hips_basic.h vs. volume_io/basic.h */
#undef ABS
#undef SIGN
#ifdef Darwin
// The result of not defining __MACTYPES__ is a scuba2 build error
// complaining about QT conflicting with MNI over the Point typedef.
// Notes from the file <mni installation>/include/volume_io/geom_structs.h:
/* Th 'Point' typedef is annoying to Mac OS users, since Point has been
 * a basic type on Macs since the beginning.  Testing __MACTYPES__ should
 * work at least with the OS X codebase, I don't know if it existed in
 * earlier versions of the MacTypes.h header.
 */
#define __MACTYPES__
#endif
#ifdef Status
// avoid conflicts with usage of 'Status' in volume_io/basic.h
#undef Status
#endif
#ifdef Windows_NT
#undef ERROR
#endif // Windows_NT

#include <volume_io.h> //from MNI

/* remove unwanted warnings between hips_basic.h vs. volume_io/basic.h */
#undef ABS
#undef SIGN

void  test_convert_transform_to_starts_and_steps(
    General_transform  *transform,
    int                n_volume_dimensions,
    double             step_signs[],
    int                spatial_axes[],
    double             starts[],
    double             steps[],
    double             dir_cosines[][VIO_N_DIMENSIONS] );
    
#endif

#endif // MINC_VOLUME_IO_H
