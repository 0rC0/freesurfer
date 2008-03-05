/**
 * @file  gifti_local.c
 * @brief local utilities for GIFTI library
 *
 * This file has some some extra functions for use with the GIFTI
 * utilities. The official utilities reside in gifti_io.c and gifti_xml.c
 *
 */
/*
 * Original Author: Kevin Teich
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2008/03/05 22:50:25 $
 *    $Revision: 1.3 $
 *
 * Copyright (C) 2007-2008,
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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>

#include "error.h" // return codes
#include "utils.h" // strcpyalloc
#include "nifti1.h"
#include "gifti_local.h"


/*
 *
 */
static giiDataArray* gifti_alloc_and_add_darray (gifti_image* image)
{
  if (!image)
  {
    fprintf (stderr,"** gifti_alloc_and_add_darray: NULL image\n");
    return NULL;
  }

  /* Try to add an empty array. */
  if (gifti_add_empty_darray(image,1))
  {
    fprintf (stderr,"** gifti_alloc_and_add_darray: gifti_add_empty_darray "
             "failed\n");
    return NULL;
  }

  /* Return the array we just allocated. */
  return image->darray[image->numDA-1];
}


/*
 *
 */
static double gifti_get_DA_value_2D (giiDataArray* da, int row, int col)
{
  int dim0_index, dim1_index;

  if (!da || !da->data)
  {
    fprintf (stderr,"** gifti_get_DA_value_2D, invalid params: data=%p\n",
             da);
    return 0;
  }

  if (da->num_dim != 2)
  {
    fprintf (stderr,"** gifti_get_DA_value_2D, array dim is %d\n",
             da->num_dim);
    return 0;
  }

  /* Get the dim0 and dims[1] indices based on our order. */
  if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
  {
    dim0_index = row;
    dim1_index = col;
  }
  else if (GIFTI_IND_ORD_COL_MAJOR == da->ind_ord)
  {
    dim0_index = col;
    dim1_index = row;
  }
  else
  {
    fprintf (stderr,"** gifti_get_DA_value_2D, unknown ind_ord: %d\n",
             da->ind_ord);
    return 0;
  }

  /* Check the indices. */
  if (dim0_index < 0 || dim0_index >= da->dims[0] ||
      dim1_index < 0 || dim1_index >= da->dims[1])
  {
    fprintf(stderr,"** gifti_get_DA_value_2D, invalid params: "
            "dim0_index=%d (max=%d), dim1_index=%d (max=%d)\n",
            dim0_index, da->dims[0], dim1_index, da->dims[1]);
    return 0;
  }

  /* Switch on the data type and return the appropriate
     element. Indexing depends on the data order. */
  switch (da->datatype)
  {
  default :
    fprintf(stderr,"** gifti_get_DA_value_2D, unsupported type %d-"
            "unknown, or can't convert to double\n",da->datatype);
    return 0;
  case NIFTI_TYPE_UINT8:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      return (double)*((unsigned char*)
                       (da->data) + (dim0_index*da->dims[1]) + dim1_index);
    else
      return (double)*((unsigned char*)
                       (da->data) + dim0_index + (dim1_index*da->dims[0]));
    break;
  }
  case NIFTI_TYPE_INT16:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      return (double)*((short*)
                       (da->data) + (dim0_index*da->dims[1]) + dim1_index);
    else
      return (double)*((short*)
                       (da->data) + dim0_index + (dim1_index*da->dims[0]));
    break;
  }
  case NIFTI_TYPE_INT32:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      return (double)*((int*)
                       (da->data) + (dim0_index*da->dims[1]) + dim1_index);
    else
      return (double)*((int*)
                       (da->data) + dim0_index + (dim1_index*da->dims[0]));
    break;
  }
  case NIFTI_TYPE_FLOAT32:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      return (double)*((float*)
                       (da->data) + (dim0_index*da->dims[1]) + dim1_index);
    else
      return (double)*((float*)
                       (da->data) + dim0_index + (dim1_index*da->dims[0]));
    break;
  }
  case NIFTI_TYPE_INT8:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      return (double)*((char*)
                       (da->data) + (dim0_index*da->dims[1]) + dim1_index);
    else
      return (double)*((char*)
                       (da->data) + dim0_index + (dim1_index*da->dims[0]));
    break;
  }
  case NIFTI_TYPE_UINT16:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      return (double)*((unsigned short*)
                       (da->data) + (dim0_index*da->dims[1]) + dim1_index);
    else
      return (double)*((unsigned short*)
                       (da->data) + dim0_index + (dim1_index*da->dims[0]));
    break;
  }
  case NIFTI_TYPE_UINT32:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      return (double)*((unsigned int*)
                       (da->data) + (dim0_index*da->dims[1]) + dim1_index);
    else
      return (double)*((unsigned int*)
                       (da->data) + dim0_index + (dim1_index*da->dims[0]));
    break;
  }
  }

  return 0;
}


/*
 *
 */
static void gifti_set_DA_value_2D (giiDataArray* da,
                                   int row, int col, double value)
{
  int dim0_index, dim1_index;

  if (!da || !da->data)
  {
    fprintf (stderr,"** gifti_set_DA_value_2D, invalid params: data=%p\n",
             da);
    return;
  }

  if (da->num_dim != 2)
  {
    fprintf (stderr,"** gifti_set_DA_value_2D, array dim is %d\n",
             da->num_dim);
    return;
  }

  /* Get the dim0 and dims[1] indices based on our order. */
  if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
  {
    dim0_index = row;
    dim1_index = col;
  }
  else
  {
    dim0_index = col;
    dim1_index = row;
  }

  /* Check the indices. */
  if (dim0_index < 0 || dim0_index >= da->dims[0] ||
      dim1_index < 0 || dim1_index >= da->dims[1])
  {
    fprintf(stderr,"** gifti_set_DA_value_2D, invalid params: "
            "dim0_index=%d (max=%d), dim1_index=%d (max=%d)\n",
            dim0_index, da->dims[0], dim1_index, da->dims[1]);
    return;
  }

  /* Switch on the data type and write the appropriate
     element. Indexing depends on the data order. */
  switch (da->datatype)
  {
  default :
    fprintf(stderr,"** gifti_set_DA_value_2D, unsupported type %d-"
            "unknown, or can't convert to double\n",da->datatype);
    return;
  case NIFTI_TYPE_UINT8:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      *((unsigned char*)(da->data) + (dim0_index*da->dims[1]) + dim1_index) =
        (unsigned char)value;
    else
      *((unsigned char*)(da->data) + dim0_index + (dim1_index*da->dims[0])) =
        (unsigned char)value;
    break;
  }
  case NIFTI_TYPE_INT16:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      *((short*)(da->data) + (dim0_index*da->dims[1]) + dim1_index) =
        (short)value;
    else
      *((short*)(da->data) + dim0_index + (dim1_index*da->dims[0])) =
        (short)value;
    break;
  }
  case NIFTI_TYPE_INT32:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      *((int*)(da->data) + (dim0_index*da->dims[1]) + dim1_index) =
        (int)value;
    else
      *((int*)(da->data) + dim0_index + (dim1_index*da->dims[0])) =
        (int)value;
    break;
  }
  case NIFTI_TYPE_FLOAT32:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      *((float*)(da->data) + (dim0_index*da->dims[1]) + dim1_index) =
        (float)value;
    else
      *((float*)(da->data) + dim0_index + (dim1_index*da->dims[0])) =
        (float)value;
    break;
  }
  case NIFTI_TYPE_INT8:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      *((char*)(da->data) + (dim0_index*da->dims[1]) + dim1_index) =
        (char)value;
    else
      *((char*)(da->data) + dim0_index + (dim1_index*da->dims[0])) =
        (char)value;
    break;
  }
  case NIFTI_TYPE_UINT16:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      *((unsigned short*)(da->data) + (dim0_index*da->dims[1]) + dim1_index) =
        (unsigned short)value;
    else
      *((unsigned short*)(da->data) + dim0_index + (dim1_index*da->dims[0])) =
        (unsigned short)value;
    break;
  }
  case NIFTI_TYPE_UINT32:
  {
    if ( GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord )
      *((unsigned int*)(da->data) + (dim0_index*da->dims[1]) + dim1_index) =
        (unsigned int)value;
    else
      *((unsigned int*)(da->data) + dim0_index + (dim1_index*da->dims[0])) =
        (unsigned int)value;
    break;
  }
  }

  return;
}



/*-----------------------------------------------------
  Parameters:    input file name of GIFTI file

  Returns value: freesurfer surface structure

  Description:   reads a GIFTI file, putting vertices, 
                 faces, and face normals into an 
                 MRIS_SURFACE structure.
  ------------------------------------------------------*/
MRI_SURFACE * mrisReadGIFTIfile(char *fname)
{
  /* Attempt to read the file. */
  gifti_image* image = gifti_read_image (fname, 1);
  if (NULL == image)
  {
    fprintf (stderr,"mrisReadGIFTIfile: gifti_read_image() returned NULL\n");
    return NULL;
  }

  /* check for compliance */
  int valid = gifti_valid_gifti_image (image, 1);
  if (valid == 0)
  {
    fprintf (stderr,"mrisReadGIFTIfile: GIFTI file %s is invalid!\n", fname);
    gifti_free_image (image);
    return NULL;
  }

  /* Make sure we got at least coords and faces. */
  giiDataArray* coords = gifti_find_DA (image, NIFTI_INTENT_POINTSET, 0);
  if (NULL == coords)
  {
    fprintf (stderr,"mrisReadGIFTIfile: no coordinates in file %s\n", fname);
    gifti_free_image (image);
    return NULL;
  }
  giiDataArray* faces = gifti_find_DA (image,  NIFTI_INTENT_TRIANGLE, 0);
  if (NULL == faces)
  {
    fprintf (stderr,"mrisReadGIFTIfile: no topology in file %s\n", fname);
    gifti_free_image (image);
    return NULL;
  }

  /* Get normals if we can. */
  giiDataArray* normals = gifti_find_DA (image, NIFTI_INTENT_NORMAL, 0);

  /* Check the number of vertices and faces. */
  long long num_vertices = 0;
  long long num_cols = 0;
  gifti_DA_rows_cols (coords, &num_vertices, &num_cols);
  if (num_vertices <= 0 || num_cols != 3)
  {
    fprintf (stderr,"mrisReadGIFTIfile: malformed coords data array in file "
             "%s: num_vertices=%d num_cols=%d\n",
             fname, (int)num_vertices, (int)num_cols);
    gifti_free_image (image);
    return NULL;
  }
  long long num_faces = 0;
  num_cols = 0;
  gifti_DA_rows_cols (faces, &num_faces, &num_cols);
  if (num_faces <= 0 || num_cols != 3)
  {
    fprintf (stderr,"mrisReadGIFTIfile: malformed faces data array in file "
             "%s: num_faces=%d num_cols=%d\n",
             fname, (int)num_faces, (int)num_cols);
    gifti_free_image (image);
    return NULL;
  }

  /* If we got normals, check they match the number of verts. If they
     don't match, just don't use them.*/
  if (NULL != normals)
  {
    long long num_normals = 0;
    num_cols = 0;
    gifti_DA_rows_cols (normals, &num_normals, &num_cols);
    if (num_normals != num_vertices || num_cols != 3)
    {
      fprintf (stderr,"mrisReadGIFTIfile: malformed normals data array in "
               "file %s: num_normals=%d (num_vertices=%d) num_cols=%d\n",
               fname, (int)num_normals, (int)num_vertices, (int)num_cols);
      normals = NULL;
    }
  }

  /* Try to allocate a surface. */
  MRIS* mris = MRISalloc (num_vertices, num_faces);
  if (NULL == mris)
  {
    fprintf (stderr,"mrisReadGIFTIfile: failed to allocate an MRIS with "
             "%d vertices and %d faces\n",
             (int)num_vertices,(int) num_faces);
    gifti_free_image (image);
    return NULL;
  }

  /* Set some meta data in the mris. */
  mris->type = MRIS_TRIANGULAR_SURFACE;

  /* Copy in the vertices. */
  int vertex_index;
  for (vertex_index = 0; vertex_index < num_vertices; vertex_index++)
  {
    mris->vertices[vertex_index].x =
      (float) gifti_get_DA_value_2D (coords, vertex_index, 0);
    mris->vertices[vertex_index].y =
      (float) gifti_get_DA_value_2D (coords, vertex_index, 1);
    mris->vertices[vertex_index].z =
      (float) gifti_get_DA_value_2D (coords, vertex_index, 2);
    mris->vertices[vertex_index].num = 0;

    /* If we have normals, set normals values. */
    if (NULL != normals)
    {
      mris->vertices[vertex_index].nx =
        (float) gifti_get_DA_value_2D (normals, vertex_index, 0);
      mris->vertices[vertex_index].ny =
        (float) gifti_get_DA_value_2D (normals, vertex_index, 1);
      mris->vertices[vertex_index].nz =
        (float) gifti_get_DA_value_2D (normals, vertex_index, 2);
    }
  }

  /* Copy in the faces. */
  int face_index;
  for (face_index = 0; face_index < num_faces; face_index++)
  {
    int face_vertex_index;
    for (face_vertex_index = 0;
         face_vertex_index < VERTICES_PER_FACE;
         face_vertex_index++)
    {
      vertex_index =
        (int) gifti_get_DA_value_2D (faces, face_index, face_vertex_index);
      mris->faces[face_index].v[face_vertex_index] = vertex_index;
      mris->vertices[vertex_index].num++;
    }
  }

  /* And we're done. */
  gifti_free_image (image);

  return mris;
}


/*-----------------------------------------------------
  Parameters:    MRIS_SURFACE structure,
                 output file name of GIFTI file,
                 optional input scalar data file

  Returns value: 0 if passed, else error code

  Description:   writes a GIFTI file, putting vertices, 
                 faces, and face normals from input 
                 MRIS_SURFACE structure.
  ------------------------------------------------------*/
int MRISwriteGIFTI(MRIS* mris, char *fname, char *scalar_fname)
{
  if (NULL == mris || NULL == fname)
  {
    fprintf (stderr,"MRISwriteGIFTI: invalid parameter\n");
    return ERROR_BADPARM;
  }

  gifti_image* image = (gifti_image *)calloc(1,sizeof(gifti_image));
  if (NULL == image)
  {
    fprintf (stderr,"MRISwriteGIFTI: couldn't allocate image\n");
    return ERROR_NOMEMORY;
  }
  image->version = strcpyalloc(GIFTI_XML_VERSION);

  /* Make coords array. */
  giiDataArray* coords = gifti_alloc_and_add_darray (image);
  if (NULL == coords)
  {
    fprintf (stderr,"MRISwriteGIFTI: couldn't allocate giiDataArray\n");
    gifti_free_image (image);
    return ERROR_NOMEMORY;
  }

  /* Set its attributes. */
  coords->intent = NIFTI_INTENT_POINTSET;
  coords->datatype = NIFTI_TYPE_FLOAT32;
  coords->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
  coords->num_dim = 2;
  coords->dims[0] = mris->nvertices; /* In highest first, dim0 = rows */
  coords->dims[1] = 3;               /* In highest first, dim1 = cols */
  coords->encoding = GIFTI_ENCODING_B64GZ; // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
  coords->endian = GIFTI_ENDIAN_LITTLE;
#else
  coords->endian = GIFTI_ENDIAN_BIG;
#endif

  if (mris->fname)
  {
    const char *primary=NULL, *secondary=NULL, *geotype=NULL;
    char *name = mris->fname;
    char *topotype="Closed";
    if (strstr(name, "lh.")) primary = "CortexLeft";
    if (strstr(name, "rh.")) primary = "CortexRight";
    if (strstr(name, ".orig"))     secondary = "GrayWhite";
    if (strstr(name, ".smoothwm")) secondary = "GrayWhite";
    if (strstr(name, ".white"))    secondary = "GrayWhite";
    if (strstr(name, ".pial"))     secondary = "Pial";
    if (strstr(name, ".orig"))     geotype = "Reconstruction";
    if (strstr(name, ".smoothwm")) geotype = "Anatomical";
    if (strstr(name, ".white"))    geotype = "Anatomical";
    if (strstr(name, ".pial"))     geotype = "Anatomical";
    if (strstr(name, ".inflated")) geotype = "Inflated";
    if (strstr(name, ".sphere"))   geotype = "Sphere";
    if (strstr(name, ".qsphere"))  geotype = "Sphere";
    if (strstr(name,"pial-outer")) geotype = "Hull";
    if (mris->patch)
    {
      geotype = "Flat";
      topotype = "Cut";
    }

    if (primary) gifti_add_to_meta( &coords->meta,
                                    "AnatomicalStructurePrimary",
                                    primary,
                                    1 );
    if (secondary) gifti_add_to_meta( &coords->meta,
                                      "AnatomicalStructureSecondary",
                                      secondary,
                                      1 );
    if (geotype) gifti_add_to_meta( &coords->meta,
                                    "GeometricType",
                                    geotype,
                                    1 );
    gifti_add_to_meta( &coords->meta,
                       "TopologicalType",
                       topotype,
                       1 );
    gifti_add_to_meta( &coords->meta,
                       "Name",
                       name,
                       1 );
  }

  if (strlen(mris->subject_name))
  {
    gifti_add_to_meta( &coords->meta,
                       "SubjectID",
                       mris->subject_name,
                       1 );
  }

#if 0
#include <uuid/uuid.h>
  uuid_t uuid;
  char uuidstr[2048];
  uuid_generate(uuid);
  uuid_unparse(uuid, uuidstr);
  gifti_add_to_meta( &coords->meta,
                     "UniqueID",
                     uuidstr,
                     1 );
#endif

  struct passwd *pw = getpwuid(geteuid());
  if ((pw != NULL) && (pw->pw_name != NULL))
  {
    gifti_add_to_meta( &coords->meta,
                       "UserName",
                       pw->pw_name,
                       1 );
  }

  time_t tyme = time(NULL);
  struct tm *lt = localtime(&tyme);
  char *date = asctime(lt);
  char *chr = strchr(date,'\r');
  if (chr) *chr = 0; // remove carriage return
  chr = strchr(date,'\n');
  if (chr) *chr = 0; // remove linefeed
  gifti_add_to_meta( &coords->meta, "Date", date, 1 );

  coords->coordsys = NULL; // empty, unless we find something here...
  if (mris->SRASToTalSRAS_ &&
      mris->SRASToTalSRAS_->rows==4 &&
      mris->SRASToTalSRAS_->cols==4)
  { // found a valid xform, so use it...
    coords->coordsys->dataspace = strcpyalloc("NIFTI_XFORM_UNKNOWN");
    coords->coordsys->xformspace = strcpyalloc("NIFTI_XFORM_TALAIRACH");
    MATRIX *xform = mris->SRASToTalSRAS_;
    int r,c;
    for (r=1; r <= 4; r++)
      for (c=1; c <= 4; c++)
        coords->coordsys->xform[r-1][c-1] = xform->rptr[r][c];
  }

  coords->nvals = gifti_darray_nvals (coords);
  gifti_datatype_sizes (coords->datatype, &coords->nbyper, NULL);

  /* Allocate the data array. */
  coords->data = NULL;
  coords->data = (void*) calloc (coords->nvals, coords->nbyper);
  if (NULL == coords->data)
  {
    fprintf (stderr,"MRISwriteGIFTI: couldn't allocate coords data of "
             "length %d, element size %d\n",
             (int)coords->nvals, coords->nbyper);
    gifti_free_image (image);
    return ERROR_NOMEMORY;
  }

  /* Copy in all our data. */
  int vertex_index;
  for (vertex_index = 0; vertex_index < mris->nvertices; vertex_index++)
  {
    if (mris->vertices[vertex_index].ripflag) continue;
    gifti_set_DA_value_2D (coords, vertex_index, 0,
                           mris->vertices[vertex_index].x);
    gifti_set_DA_value_2D (coords, vertex_index, 1,
                           mris->vertices[vertex_index].y);
    gifti_set_DA_value_2D (coords, vertex_index, 2,
                           mris->vertices[vertex_index].z);
  }

  /* -------------------------------------------------------
   * Faces array. 
   */
  giiDataArray* faces = gifti_alloc_and_add_darray (image);
  if (NULL == faces)
  {
    fprintf (stderr,"MRISwriteGIFTI: couldn't allocate giiDataArray\n");
    gifti_free_image (image);
    return ERROR_NOMEMORY;
  }

  /* Set its attributes. */
  faces->intent = NIFTI_INTENT_TRIANGLE;
  faces->datatype = NIFTI_TYPE_INT32;
  faces->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
  faces->num_dim = 2;
  faces->dims[0] = mris->nfaces;    /* In highest first, dim0 = rows */
  faces->dims[1] = 3;               /* In highest first, dim1 = cols */
  faces->encoding = GIFTI_ENCODING_B64GZ; // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
  faces->endian = GIFTI_ENDIAN_LITTLE;
#else
  faces->endian = GIFTI_ENDIAN_BIG;
#endif
  faces->coordsys = NULL;
  faces->nvals = gifti_darray_nvals (faces);
  gifti_datatype_sizes (faces->datatype, &faces->nbyper, NULL);

  /* Allocate the data array. */
  faces->data = NULL;
  faces->data = (void*) calloc (faces->nvals, faces->nbyper);
  if (NULL == faces->data)
  {
    fprintf (stderr,"MRISwriteGIFTI: couldn't allocate faces data of "
             "length %d, element size %d\n", (int)faces->nvals, faces->nbyper);
    gifti_free_image (image);
    return ERROR_NOMEMORY;
  }

  /* Copy in all our data. */
  int face_index;
  for (face_index = 0; face_index < mris->nfaces; face_index++)
  {
    gifti_set_DA_value_2D (faces, face_index, 0,
                           mris->faces[face_index].v[0]);
    gifti_set_DA_value_2D (faces, face_index, 1,
                           mris->faces[face_index].v[1]);
    gifti_set_DA_value_2D (faces, face_index, 2,
                           mris->faces[face_index].v[2]);
  }

  /* -------------------------------------------------------
   * Normals array. 
   */
  giiDataArray* normals = gifti_alloc_and_add_darray (image);
  if (NULL == normals)
  {
    fprintf (stderr,"MRISwriteGIFTI: couldn't allocate giiDataArray\n");
    gifti_free_image (image);
    return ERROR_NOMEMORY;
  }

  /* Set its attributes. */
  normals->intent = NIFTI_INTENT_NORMAL;
  normals->datatype = NIFTI_TYPE_FLOAT32;
  normals->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
  normals->num_dim = 2;
  normals->dims[0] = mris->nvertices; /* In highest first, dim0 = rows */
  normals->dims[1] = 3;               /* In highest first, dim1 = cols */
  normals->encoding = GIFTI_ENCODING_B64GZ; // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
  normals->endian = GIFTI_ENDIAN_LITTLE;
#else
  normals->endian = GIFTI_ENDIAN_BIG;
#endif
  normals->coordsys = NULL;
  normals->nvals = gifti_darray_nvals (normals);
  gifti_datatype_sizes (normals->datatype, &normals->nbyper, NULL);

  /* Allocate the data array. */
  normals->data = NULL;
  normals->data = (void*) calloc (normals->nvals, normals->nbyper);
  if (NULL == normals->data)
  {
    fprintf (stderr,"MRISwriteGIFTI: couldn't allocate normals data of "
             "length %d, element size %d\n",
             (int)normals->nvals,normals->nbyper);
    gifti_free_image (image);
    return ERROR_NOMEMORY;
  }

  /* Copy in all our data. */
  int normal_index;
  for (normal_index = 0; normal_index < mris->nvertices; normal_index++)
  {
    if (mris->vertices[normal_index].ripflag) continue;
    gifti_set_DA_value_2D (normals, normal_index, 0,
                           mris->vertices[normal_index].nx);
    gifti_set_DA_value_2D (normals, normal_index, 1,
                           mris->vertices[normal_index].ny);
    gifti_set_DA_value_2D (normals, normal_index, 2,
                           mris->vertices[normal_index].nz);
  }

  /* -------------------------------------------------------
   * Scalars array (if optionally specified in mris_convert)
   */
  if (scalar_fname)
  {
    if (MRISreadCurvatureFile(mris, scalar_fname))
    {
      fprintf (stderr,"MRISwriteGIFTI: couldn't read %s\n",scalar_fname);
      gifti_free_image (image);
      return ERROR_BADFILE;
    }

    giiDataArray* scalars = gifti_alloc_and_add_darray (image);
    if (NULL == scalars)
    {
      fprintf (stderr,"MRISwriteGIFTI: couldn't allocate giiDataArray\n");
      gifti_free_image (image);
      return ERROR_NOMEMORY;
    }

    /* Set its attributes. */
    scalars->intent = NIFTI_INTENT_SHAPE;
    scalars->datatype = NIFTI_TYPE_FLOAT32;
    scalars->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    scalars->num_dim = 2;
    scalars->dims[0] = mris->nvertices;
    scalars->dims[1] = 1;
    scalars->encoding = GIFTI_ENCODING_B64GZ; // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    scalars->endian = GIFTI_ENDIAN_LITTLE;
#else
    scalars->endian = GIFTI_ENDIAN_BIG;
#endif
    scalars->coordsys = NULL;
    scalars->nvals = gifti_darray_nvals (scalars);
    gifti_datatype_sizes (scalars->datatype, &scalars->nbyper, NULL);

    /* include some metadata describing this thing */
    char *meta=NULL;
    if (strstr(scalar_fname, ".thickness")) meta = "Thickness";
    if (strstr(scalar_fname, ".curv"))      meta = "CurvatureRadial";
    if (strstr(scalar_fname, ".sulc"))      meta = "SulcalDepth";
    if (strstr(scalar_fname, ".area"))      meta = "Area";
    if (strstr(scalar_fname, ".volume"))    meta = "Volume";
    if (strstr(scalar_fname, ".jacobian"))  meta = "Jacobian";
    if (meta) gifti_add_to_meta( &scalars->meta, "ScalarDataType", meta, 1 );

    /* Allocate the data array. */
    scalars->data = NULL;
    scalars->data = (void*) calloc (scalars->nvals, scalars->nbyper);
    if (NULL == scalars->data)
    {
      fprintf (stderr,"MRISwriteGIFTI: couldn't allocate scalars data of "
               "length %d, element size %d\n",
               (int)scalars->nvals,scalars->nbyper);
      gifti_free_image (image);
      return ERROR_NOMEMORY;
    }

    /* Copy in all our data. */
    int scalar_index;
    for (scalar_index = 0; scalar_index < mris->nvertices; scalar_index++)
    {
      if (mris->vertices[scalar_index].ripflag) continue;
      gifti_set_DA_value_2D (scalars, scalar_index, 0,
                             mris->vertices[scalar_index].curv);
    }
  }

  /* check for compliance */
  int valid = gifti_valid_gifti_image (image, 1);
  if (valid == 0)
  {
    fprintf (stderr,"MRISwriteGIFTI: GIFTI file %s is invalid!\n", fname);
    gifti_free_image (image);
    return ERROR_BADFILE;
  }

  /* Write the file. */
  if (gifti_write_image (image, fname, 1))
  {
    fprintf (stderr,"MRISwriteGIFTI: couldn't write image\n");
    gifti_free_image (image);
    return ERROR_BADFILE;
  }

  gifti_free_image (image);

  return ERROR_NONE;
}


