#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mri.h"
#include "error.h"
#include "diag.h"
#include "utils.h"
#include "gca.h"
#include "proto.h"
#include "fio.h"
#include "transform.h"
#include "macros.h"
#include "utils.h"

#define UNKNOWN_DIST                4  /* within 4 mm of some known label */
#define GCA_VERSION                  1.0
#define DEFAULT_MAX_LABELS_PER_GCAN  4

GCA *gcaAllocMax(int ninputs, float spacing, int width, int height, int depth, 
                 int max_labels) ;
static int GCAupdateNode(GCA *gca, MRI *mri, int xt, int yt, int zt,
                         float val,int label);
static int different_nbr_labels(GCA *gca, int x, int y, int z, int wsize,
                                 int label) ;
static int gcaRegionStats(GCA *gca, int x0, int y0, int z0, int wsize,
                   float *priors, float *vars, float *means) ;
static int gcaFindBestSample(GCA *gca, int x, int y, int z, int best_label,
                             int wsize, GCA_SAMPLE *gcas);
static int gcaFindClosestMeanSample(GCA *gca, float mean, float min_prior,
                                    int x, int y, int z, 
                                    int label, int wsize, GCA_SAMPLE *gcas);
static GC1D *gcaFindHighestPriorGC(GCA *gca, int x, int y, int z,int label,
                                 int wsize) ;

static int mriFillRegion(MRI *mri, int x,int y,int z,int fill_val,int whalf);

int
GCAvoxelToNode(GCA *gca, MRI *mri, int xv, int yv, int zv, int *pxn, 
               int *pyn, int *pzn)
{
  float   xscale, yscale, zscale ;

  xscale = mri->xsize / gca->spacing ;
  yscale = mri->ysize / gca->spacing ;
  zscale = mri->zsize / gca->spacing ;
  *pxn = (int)(xv * xscale) ;
  if (*pxn < 0)
    *pxn = 0 ;
  else if (*pxn >= gca->width)
    *pxn = gca->width-1 ;

  *pyn = (int)(yv * yscale) ;
  if (*pyn < 0)
    *pyn = 0 ;
  else if (*pyn >= gca->height)
    *pyn = gca->height-1 ;

  *pzn = (int)(zv * zscale) ;
  if (*pzn < 0)
    *pzn = 0 ;
  else if (*pzn >= gca->depth)
    *pzn = gca->depth-1 ;

  return(NO_ERROR) ;
}

int
GCAsourceVoxelToNodePoint(GCA *gca, MRI *mri, LTA *lta,
                          Real xv, Real yv, Real zv, 
                          Real *pxn, Real *pyn, Real *pzn)
{
  static VECTOR *v_input, *v_canon = NULL ;
  float   xscale, yscale, zscale, xt, yt, zt ;

  if (!v_canon)
  {
    v_input = VectorAlloc(4, MATRIX_REAL) ;
    v_canon = VectorAlloc(4, MATRIX_REAL) ;
    *MATRIX_RELT(v_input, 4, 1) = 1.0 ;
    *MATRIX_RELT(v_canon, 4, 1) = 1.0 ;
  }

  V3_X(v_input) = xv ; V3_Y(v_input) = yv ; V3_Z(v_input) = zv ; 
  MatrixMultiply(lta->xforms[0].m_L, v_input, v_canon) ;
  xt = V3_X(v_canon) ; yt = V3_Y(v_canon) ; zt = V3_Z(v_canon) ; 

  xscale = mri->xsize / gca->spacing ;
  yscale = mri->ysize / gca->spacing ;
  zscale = mri->zsize / gca->spacing ;
  *pxn = (xt * xscale) ;
  if (*pxn < 0)
    *pxn = 0 ;
  else if (*pxn >= gca->width)
    *pxn = gca->width-1 ;

  *pyn = (yv * yscale) ;
  if (*pyn < 0)
    *pyn = 0 ;
  else if (*pyn >= gca->height)
    *pyn = gca->height-1 ;

  *pzn = (zv * zscale) ;
  if (*pzn < 0)
    *pzn = 0 ;
  else if (*pzn >= gca->depth)
    *pzn = gca->depth-1 ;

  return(NO_ERROR) ;
}

int
GCAnodeToVoxel(GCA *gca, MRI *mri, int xn, int yn, int zn, int *pxv, 
               int *pyv, int *pzv)
{
  float   xscale, yscale, zscale, offset ;

  xscale = mri->xsize / gca->spacing ;
  yscale = mri->ysize / gca->spacing ;
  zscale = mri->zsize / gca->spacing ;
  offset = (gca->spacing-1.0)/2.0 ;
  *pxv = (int)(xn / xscale + offset) ;
  if (*pxv < 0)
    *pxv = 0 ;
  else if (*pxv >= mri->width)
    *pxv = mri->width-1 ;

  *pyv = (int)(yn / yscale + offset) ;
  if (*pyv < 0)
    *pyv = 0 ;
  else if (*pyv >= mri->height)
    *pyv = mri->height-1 ;

  *pzv = (int)(zn / zscale + offset) ;
  if (*pzv < 0)
    *pzv = 0 ;
  else if (*pzv >= mri->depth)
    *pzv = mri->depth-1 ;

  return(NO_ERROR) ;
}


GCA  *
GCAalloc(int ninputs, float spacing, int width, int height, int depth)
{
  return(gcaAllocMax(ninputs, spacing, width,height,depth,
                     DEFAULT_MAX_LABELS_PER_GCAN));
}

GCA *
gcaAllocMax(int ninputs, float spacing, int width, int height, int depth, 
            int max_labels)
{
  GCA      *gca ;
  GCA_NODE *gcan ;
  int      x, y, z ;

  gca = calloc(1, sizeof(GCA)) ;
  if (!gca)
    ErrorExit(ERROR_NOMEMORY, "GCAalloc: could not allocate struct") ;

  gca->ninputs = ninputs ;
  gca->spacing = spacing ;

  gca->width = (int)ceil((float)width/spacing) ;
  gca->height = (int)ceil((float)height/spacing) ;
  gca->depth = (int)ceil((float)depth/spacing) ;

  gca->nodes = (GCA_NODE ***)calloc(gca->width, sizeof(GCA_NODE **)) ;
  if (!gca->nodes)
    ErrorExit(ERROR_NOMEMORY, "GCAalloc: could not allocate nodes") ;
  for (x = 0 ; x < gca->width ; x++)
  {
    gca->nodes[x] = (GCA_NODE **)calloc(gca->height, sizeof(GCA_NODE *)) ;
    if (!gca->nodes[x])
      ErrorExit(ERROR_NOMEMORY, "GCAalloc: could not allocate %dth **",x) ;

    for (y = 0 ; y < gca->height ; y++)
    {
      gca->nodes[x][y] = (GCA_NODE *)calloc(gca->depth, sizeof(GCA_NODE)) ;
      if (!gca->nodes[x][y])
        ErrorExit(ERROR_NOMEMORY,"GCAalloc: could not allocate %d,%dth *",x,y);
      for (z = 0 ; z < gca->depth ; z++)
      {
        gcan = &gca->nodes[x][y][z] ;
        gcan->max_labels = max_labels ;

        if (max_labels > 0)
        {
          /* allocate new ones */
          gcan->gcs = (GC1D *)calloc(gcan->max_labels, sizeof(GC1D)) ;
          if (!gcan->gcs)
            ErrorExit(ERROR_NOMEMORY, "GCANalloc: couldn't allocate gcs to %d",
                      gcan->max_labels) ;
          gcan->labels = (char *)calloc(gcan->max_labels, sizeof(char)) ;
          if (!gcan->labels)
            ErrorExit(ERROR_NOMEMORY,
                      "GCANalloc: couldn't allocate labels to %d",
                      gcan->max_labels) ;
        }
      }
    }
  }
  
  return(gca) ;
}

int
GCAfree(GCA **pgca)
{
  GCA  *gca ;
  int  x, y, z ;

  gca = *pgca ;
  *pgca = NULL ;

  for (x = 0 ; x < gca->width ; x++)
  {
    for (y = 0 ; y < gca->height ; y++)
    {
      for (z = 0 ; z < gca->depth ; z++)
      {
        GCANfree(&gca->nodes[x][y][z]) ;
      }
      free(gca->nodes[x][y]) ;
    }
    free(gca->nodes[x]) ;
  }

  free(gca->nodes) ;
  free(gca) ;
  return(NO_ERROR) ;
}

int
GCANfree(GCA_NODE *gcan)
{
  if (gcan->nlabels)
  {
    free(gcan->labels) ;
    free(gcan->gcs) ;
  }
  return(NO_ERROR) ;
}
int
GCAtrain(GCA *gca, MRI *mri_inputs, MRI *mri_labels, LTA *lta)
{
  int    x, y, z, xt, yt, zt, width, height, depth, label, val,xi,yi,zi ;
  MATRIX *m_L, *m_label_to_input, *m_voxel_to_ras, *m_ras_to_voxel ;
  VECTOR *v_input, *v_canon, *v_label ;

  v_input = VectorAlloc(4, MATRIX_REAL) ;
  v_canon = VectorAlloc(4, MATRIX_REAL) ;
  v_label = VectorAlloc(4, MATRIX_REAL) ;
  *MATRIX_RELT(v_input, 4, 1) = 1.0 ;
  *MATRIX_RELT(v_canon, 4, 1) = 1.0 ;
  *MATRIX_RELT(v_label, 4, 1) = 1.0 ;

  m_voxel_to_ras = MRIgetVoxelToRasXform(mri_labels) ;
  m_ras_to_voxel = MRIgetRasToVoxelXform(mri_inputs) ;
  m_label_to_input = MatrixMultiply(m_ras_to_voxel, m_voxel_to_ras, NULL) ;
  MatrixFree(&m_voxel_to_ras) ; MatrixFree(&m_ras_to_voxel) ;

  /* convert transform to voxel coordinates */
#if 0
  m_L = MRIrasXformToVoxelXform(mri_inputs,mri_inputs,lta->xforms[0].m_L,NULL);
#else
  m_L = lta->xforms[0].m_L ;
#endif

  /* go through each voxel in the input volume and find the canonical
     voxel (and hence the classifier) to which it maps. Then update the
     classifiers statistics based on this voxel's intensity and label.
  */
  width = mri_labels->width ; height = mri_labels->height; 
  depth = mri_labels->depth ;
  for (x = 0 ; x < width ; x++)
  {
    V3_X(v_label) = (float)x ;
    for (y = 0 ; y < height ; y++)
    {
      V3_Y(v_label) = (float)y ;
      for (z = 0 ; z < depth ; z++)
      {
        label = MRIvox(mri_labels, x, y, z) ;
#if 0
        if (!label)
          continue ;
#endif
        V3_Z(v_label) = (float)z ;
        MatrixMultiply(m_label_to_input, v_label, v_input) ;
        xi = nint(V3_X(v_input)) ;
        yi = nint(V3_Y(v_input)) ;
        zi = nint(V3_Z(v_input)) ;
        val = MRIvox(mri_inputs, xi, yi, zi) ;

        if (xi == 123 && yi == 86 && zi == 141)
          DiagBreak() ;  /* wm in 1332 */

        MatrixMultiply(m_L, v_input, v_canon) ;
        xt = nint(V3_X(v_canon)) ;
        yt = nint(V3_Y(v_canon)) ;
        zt = nint(V3_Z(v_canon)) ;
        if (xt < 0) xt = 0 ;
        if (yt < 0) yt = 0 ;
        if (zt < 0) zt = 0 ;
        if (xt >= width)  xt = width-1 ;
        if (yt >= height) yt = height-1 ;
        if (zt >= depth)  zt = depth-1 ;

        GCAupdateNode(gca, mri_inputs, xt, yt, zt, (float)val, label) ;
      }
    }
  }

  VectorFree(&v_input) ; VectorFree(&v_canon) ; VectorFree(&v_label) ;
#if 0
  MatrixFree(&m_L) ;
#endif
  return(NO_ERROR) ;
}
int
GCAwrite(GCA *gca, char *fname)
{
  FILE     *fp ;
  int      x, y, z, n ; 
  GCA_NODE *gcan ;
  GC1D     *gc ;

  fp  = fopen(fname, "wb") ;
  if (!fp)
    ErrorReturn(ERROR_NOFILE,
                (ERROR_NOFILE,
                 "GCAwrite: could not open GCA %s for writing",fname)) ;

  fwriteFloat(GCA_VERSION, fp) ;
  fwriteFloat(gca->spacing, fp) ;
  fwriteInt(gca->width,fp);fwriteInt(gca->height,fp); fwriteInt(gca->depth,fp);
  fwriteInt(gca->ninputs,fp) ;

  for (x = 0 ; x < gca->width ; x++)
  {
    for (y = 0 ; y < gca->height ; y++)
    {
      for (z = 0 ; z < gca->depth ; z++)
      {
        if (x == 25 && y == 59 && z == 35)
          DiagBreak() ;
        gcan = &gca->nodes[x][y][z] ;
        fwriteInt(gcan->nlabels, fp) ;
        fwriteInt(gcan->total_training, fp) ;
        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          gc = &gcan->gcs[n] ;
          fputc((int)gcan->labels[n],fp) ;
          fwriteFloat(gc->mean, fp) ;
          fwriteFloat(gc->var, fp) ;
          fwriteFloat(gc->prior, fp) ;
        }
      }
    }
  }

  fclose(fp) ;
  return(NO_ERROR) ;
}

GCA *
GCAread(char *fname)
{
  FILE     *fp ;
  int      x, y, z, n ; 
  GCA      *gca ;
  GCA_NODE *gcan ;
  GC1D     *gc ;
  float    version, spacing ;
  int      width, height, depth, ninputs ;

  fp  = fopen(fname, "rb") ;
  if (!fp)
    ErrorReturn(NULL,
                (ERROR_NOFILE,
                 "GCAread: could not open GCA %s for writing",fname)) ;

  version = freadFloat(fp) ;
  spacing = freadFloat(fp) ;
  width = freadInt(fp); height = freadInt(fp); depth = freadInt(fp);
  ninputs = freadInt(fp) ;

  gca = gcaAllocMax(ninputs, spacing, spacing*width, spacing*height, 
                    spacing*depth, 0) ;
  if (!gca)
    ErrorReturn(NULL, (Gdiag, NULL)) ;

  for (x = 0 ; x < gca->width ; x++)
  {
    for (y = 0 ; y < gca->height ; y++)
    {
      for (z = 0 ; z < gca->depth ; z++)
      {
        if (x == 28 && y == 39 && z == 39)
          DiagBreak() ;
        gcan = &gca->nodes[x][y][z] ;
        gcan->nlabels = freadInt(fp) ;
        gcan->total_training = freadInt(fp) ;
        gcan->labels = (char *)calloc(gcan->nlabels, sizeof(char)) ;
        if (!gcan->labels)
          ErrorExit(ERROR_NOMEMORY, "GCAread(%s): could not allocate %d "
                    "labels @ (%d,%d,%d)", fname, gcan->nlabels, x, y, z) ;
        gcan->gcs = (GC1D *)calloc(gcan->nlabels, sizeof(GC1D)) ;
        if (!gcan->gcs)
          ErrorExit(ERROR_NOMEMORY, "GCAread(%s); could not allocated %d gcs "
                    "@ (%d,%d,%d)", fname, gcan->nlabels, x, y, z) ;

        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          gc = &gcan->gcs[n] ;
          gcan->labels[n] = (char)fgetc(fp) ;
          gc->mean = freadFloat(fp) ;
          gc->var = freadFloat(fp) ;
          gc->prior = freadFloat(fp) ;
        }
      }
    }
  }

  fclose(fp) ;
  return(gca) ;
}

static int
GCAupdateNode(GCA *gca, MRI *mri, int xt, int yt, int zt, float val, int label)
{
  int      xn, yn, zn, n ;
  GCA_NODE *gcan ;
  GC1D     *gc ;

  GCAvoxelToNode(gca, mri, xt, yt, zt, &xn, &yn, &zn) ;

  if (xn == 77 && yn == 58 && zn == 64 && label == 5)
    DiagBreak() ;

  gcan = &gca->nodes[xn][yn][zn] ;

  gcan->total_training++ ;
  for (n = 0 ; n < gcan->nlabels ; n++)
  {
    if (gcan->labels[n] == label)
      break ;
  }
  if (n >= gcan->nlabels)  /* have to allocate a new classifier */
  {

    if (n >= gcan->max_labels)
    {
      int  old_max_labels ;
      char *old_labels ;
      GC1D *old_gcs ;

      old_max_labels = gcan->max_labels ; 
      gcan->max_labels += 2 ;
      old_labels = gcan->labels ;
      old_gcs = gcan->gcs ;

      /* allocate new ones */
      gcan->gcs = (GC1D *)calloc(gcan->max_labels, sizeof(GC1D)) ;
      if (!gcan->gcs)
        ErrorExit(ERROR_NOMEMORY, "GCANupdateNode: couldn't expand gcs to %d",
                  gcan->max_labels) ;
      gcan->labels = (char *)calloc(gcan->max_labels, sizeof(char)) ;
      if (!gcan->labels)
        ErrorExit(ERROR_NOMEMORY, "GCANupdateNode: couldn't expand labels to %d",
                  gcan->max_labels) ;

      /* copy the old ones over */
      memmove(gcan->gcs, old_gcs, old_max_labels*sizeof(GC1D)) ;
      memmove(gcan->labels, old_labels, old_max_labels*sizeof(char)) ;

      /* free the old ones */
      free(old_gcs) ; free(old_labels) ;
    }
    gcan->nlabels++ ;
  }

  gc = &gcan->gcs[n] ;

  /* these will be updated when training is complete */
  gc->mean += val ; gc->var += val*val ; gc->prior += 1.0f ;
  
  gcan->labels[n] = label ;

  return(NO_ERROR) ;
}
#define MIN_VAR  (2*2)   /* should make this configurable */
int
GCAcompleteTraining(GCA *gca)
{
  int      x, y, z, n, total_nodes, total_gcs ;
  float    nsamples ;
  GCA_NODE *gcan ;
  GC1D     *gc ;

  total_nodes = gca->width*gca->height*gca->depth ; total_gcs = 0 ;
  for (x = 0 ; x < gca->width ; x++)
  {
    for (y = 0 ; y < gca->height ; y++)
    {
      for (z = 0 ; z < gca->depth ; z++)
      {
        gcan = &gca->nodes[x][y][z] ;
        total_gcs += gcan->nlabels ;
        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          float var ;

          gc = &gcan->gcs[n] ;
          nsamples = gc->prior ;  /* prior is count of # of occurences now */
          gc->mean /= nsamples ;
          var = gc->var / nsamples - gc->mean*gc->mean ;
          if (var < -0.1)
            DiagBreak() ;
          if (var < MIN_VAR)
            var = MIN_VAR ;
          gc->var = var ;
          gc->prior /= (float)gcan->total_training ;
        }
      }
    }
  }
  printf("%d classifiers: %2.1f per node\n", 
         total_gcs, (float)total_gcs/(float)total_nodes) ;
  return(NO_ERROR) ;
}

MRI  *
GCAlabel(MRI *mri_inputs, GCA *gca, MRI *mri_dst, LTA *lta)
{
  int      x, y, z, xt, yt, zt, width, height, depth, label, val,
           xn, yn, zn, n ;
  MATRIX   *m_L ;
  VECTOR   *v_input, *v_canon ;
  GCA_NODE *gcan ;
  GC1D     *gc ;
  float    dist, max_p, p ;

  if (!mri_dst)
  {
    mri_dst = MRIclone(mri_inputs, NULL) ;
    if (!mri_dst)
      ErrorExit(ERROR_NOMEMORY, "GCAlabel: could not allocate dst") ;
    MRIcopyHeader(mri_inputs, mri_dst) ;
  }

  v_input = VectorAlloc(4, MATRIX_REAL) ;
  v_canon = VectorAlloc(4, MATRIX_REAL) ;
  *MATRIX_RELT(v_input, 4, 1) = 1.0 ;
  *MATRIX_RELT(v_canon, 4, 1) = 1.0 ;

  /* convert transform to voxel coordinates */
#if 0
  m_L = MRIrasXformToVoxelXform(mri_inputs,mri_inputs,lta->xforms[0].m_L,NULL);
#else
  m_L = lta->xforms[0].m_L ;
#endif

  /* go through each voxel in the input volume and find the canonical
     voxel (and hence the classifier) to which it maps. Then update the
     classifiers statistics based on this voxel's intensity and label.
  */
  width = mri_inputs->width ; height = mri_inputs->height; 
  depth = mri_inputs->depth ;
  for (x = 0 ; x < width ; x++)
  {
    V3_X(v_input) = (float)x ;
    for (y = 0 ; y < height ; y++)
    {
      V3_Y(v_input) = (float)y ;
      for (z = 0 ; z < depth ; z++)
      {
        if (x == 85 && y == 89 && z == 135)
          DiagBreak() ; 

        V3_Z(v_input) = (float)z ;
        val = MRIvox(mri_inputs, x, y, z) ;

        /* compute coordinates in canonical space */
        MatrixMultiply(m_L, v_input, v_canon) ;
        xt = nint(V3_X(v_canon)) ;
        yt = nint(V3_Y(v_canon)) ;
        zt = nint(V3_Z(v_canon)) ;
        if (xt < 0) xt = 0 ;
        if (yt < 0) yt = 0 ;
        if (zt < 0) zt = 0 ;
        if (xt >= width)  xt = width-1 ;
        if (yt >= height) yt = height-1 ;
        if (zt >= depth)  zt = depth-1 ;

        /* find the node associated with this coordinate and classify */
        GCAvoxelToNode(gca, mri_inputs, xt, yt, zt, &xn, &yn, &zn) ;
        gcan = &gca->nodes[xn][yn][zn] ;
        label = 0 ; max_p = -10000000.0 ;
        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          gc = &gcan->gcs[n] ;
          
          /* compute 1-d Mahalanobis distance */
          dist = (val-gc->mean) ;
          if (FZERO(gc->var))  /* make it a delta function */
          {
            if (FZERO(dist))
              p = 1.0 ;
            else
              p = 0.0 ;
          }
          else
            p = 1 / sqrt(gc->var * 2 * M_PI) * exp(-dist*dist/gc->var) ;

          p *= gc->prior ;
          if (p > max_p)
          {
            max_p = p ;
            label = gcan->labels[n] ;
          }
        }
        MRIvox(mri_dst, x, y, z) = label ;
      }
    }
  }

#if 0
  MatrixFree(&m_L) ;
#endif

  return(mri_dst) ;
}

GCA  *
GCAreduce(GCA *gca_src)
{
  GCA       *gca_dst ;
  int       xs, ys, zs, xd, yd, zd, swidth, sheight, sdepth,
            ns, nd, dwidth, dheight, ddepth, dof ;
  float     spacing = gca_src->spacing ;
  GCA_NODE  *gcan_src, *gcan_dst ;
  GC1D      *gc_src, *gc_dst ;

  swidth = gca_src->width ; sheight = gca_src->height; sdepth = gca_src->depth;
  
  gca_dst = 
    GCAalloc(gca_src->ninputs, spacing*2, spacing*swidth,
             spacing*gca_src->height,spacing*sdepth) ;


  dwidth = gca_dst->width ; dheight = gca_dst->height; ddepth = gca_dst->depth;

  for (zs = 0 ; zs < sdepth ; zs++)
  {
    for (ys = 0 ; ys < sheight ; ys++)
    {
      for (xs = 0 ; xs < swidth ; xs++)
      {
        xd = xs/2 ; yd = ys/2 ; zd = zs/2 ;
        if (xd == 15 && yd == 22 && zd == 35)
          DiagBreak() ;
        gcan_src = &gca_src->nodes[xs][ys][zs] ;
        gcan_dst = &gca_dst->nodes[xd][yd][zd] ;
        
        for (ns = 0 ; ns < gcan_src->nlabels ; ns++)
        {
          gc_src = &gcan_src->gcs[ns] ;
          dof = gc_src->prior * gcan_src->total_training ;
          
          /* find label in destination */
          for (nd = 0 ; nd < gcan_dst->nlabels ; nd++)
          {
            if (gcan_dst->labels[nd] == gcan_src->labels[ns])
              break ;
          }
          if (nd >= gcan_dst->nlabels)  /* couldn't find it */
          {
            if (nd >= gcan_dst->max_labels)
            {
              int  old_max_labels ;
              char *old_labels ;
              GC1D *old_gcs ;
              
              old_max_labels = gcan_dst->max_labels ; 
              gcan_dst->max_labels += 2 ;
              old_labels = gcan_dst->labels ;
              old_gcs = gcan_dst->gcs ;
              
              /* allocate new ones */
              gcan_dst->gcs = 
                (GC1D *)calloc(gcan_dst->max_labels, sizeof(GC1D)) ;
              if (!gcan_dst->gcs)
                ErrorExit(ERROR_NOMEMORY, 
                          "GCANreduce: couldn't expand gcs to %d",
                          gcan_dst->max_labels) ;
              gcan_dst->labels = 
                (char *)calloc(gcan_dst->max_labels, sizeof(char)) ;
              if (!gcan_dst->labels)
                ErrorExit(ERROR_NOMEMORY, 
                          "GCANupdateNode: couldn't expand labels to %d",
                          gcan_dst->max_labels) ;
              
              /* copy the old ones over */
              memmove(gcan_dst->gcs, old_gcs, old_max_labels*sizeof(GC1D));
              memmove(gcan_dst->labels, old_labels, 
                      old_max_labels*sizeof(char)) ;
              
              /* free the old ones */
              free(old_gcs) ; free(old_labels) ;
            }
            gcan_dst->nlabels++ ;
          }
          gc_dst = &gcan_dst->gcs[nd] ;
          gc_dst->mean += dof*gc_src->mean ;
          gc_dst->var += dof*(gc_src->var) ;
          gc_dst->prior += dof ;
          gcan_dst->total_training += dof ;
          gcan_dst->labels[nd] = gcan_src->labels[ns] ;
        }
      }
    }
  }

  for (xd = 0 ; xd < gca_dst->width ; xd++)
  {
    for (yd = 0 ; yd < gca_dst->height ; yd++)
    {
      for (zd = 0 ; zd < gca_dst->depth ; zd++)
      {
        gcan_dst = &gca_dst->nodes[xd][yd][zd] ;
        for (nd = 0 ; nd < gcan_dst->nlabels ; nd++)
        {
          float var ;

          gc_dst = &gcan_dst->gcs[nd] ;
          dof = gc_dst->prior ;/* prior is count of # of occurences now */
          gc_dst->mean /= dof ;
          var = gc_dst->var / dof ;
          if (var < -0.1)
            DiagBreak() ;
          if (var < MIN_VAR)
            var = MIN_VAR ;
          gc_dst->var = var ;
          gc_dst->prior /= (float)gcan_dst->total_training ;
        }
      }
    }
  }
  return(gca_dst) ;
}

#define MAX_LABELS_PER_GCAN              5
#define MAX_DIFFERENT_LABELS    500

typedef struct
{
  int   label ;
  float prob ;
} LABEL_PROB ;

static int compare_sort_probabilities(const void *plp1, const void *plp2);

MRI *
GCAclassify(MRI *mri_inputs,GCA *gca,MRI *mri_dst,LTA *lta,int max_labels)
{
  int        x, y, z, xt, yt, zt, width, height, depth, val,
             xn, yn, zn, n ;
  MATRIX     *m_L ;
  VECTOR     *v_input, *v_canon ;
  GCA_NODE   *gcan ;
  GC1D       *gc ;
  float      dist, max_p, p, total_p ;
  LABEL_PROB label_probs[1000] ;

  if (max_labels > MAX_LABELS_PER_GCAN)
    max_labels = MAX_LABELS_PER_GCAN ;

  if (!mri_dst)
  {
    int width = mri_inputs->width, height = mri_inputs->height, 
        depth = mri_inputs->depth ;

    mri_dst = MRIallocSequence(width, height, depth, MRI_UCHAR, max_labels) ;
    if (!mri_dst)
      ErrorExit(ERROR_NOMEMORY, "GCAlabel: could not allocate dst") ;
    MRIcopyHeader(mri_inputs, mri_dst) ;
  }

  v_input = VectorAlloc(4, MATRIX_REAL) ;
  v_canon = VectorAlloc(4, MATRIX_REAL) ;
  *MATRIX_RELT(v_input, 4, 1) = 1.0 ;
  *MATRIX_RELT(v_canon, 4, 1) = 1.0 ;

  /* convert transform to voxel coordinates */
#if 0
  m_L = MRIrasXformToVoxelXform(mri_inputs,mri_inputs,lta->xforms[0].m_L,NULL);
#else
  m_L = lta->xforms[0].m_L ;
#endif

  /* go through each voxel in the input volume and find the canonical
     voxel (and hence the classifier) to which it maps. Then update the
     classifiers statistics based on this voxel's intensity and label.
  */
  width = mri_inputs->width ; height = mri_inputs->height; 
  depth = mri_inputs->depth ;
  for (x = 0 ; x < width ; x++)
  {
    V3_X(v_input) = (float)x ;
    for (y = 0 ; y < height ; y++)
    {
      V3_Y(v_input) = (float)y ;
      for (z = 0 ; z < depth ; z++)
      {
        if (x == 85 && y == 89 && z == 135)
          DiagBreak() ; 

        V3_Z(v_input) = (float)z ;
        val = MRIvox(mri_inputs, x, y, z) ;

        /* compute coordinates in canonical space */
        MatrixMultiply(m_L, v_input, v_canon) ;
        xt = nint(V3_X(v_canon)) ;
        yt = nint(V3_Y(v_canon)) ;
        zt = nint(V3_Z(v_canon)) ;
        if (xt < 0) xt = 0 ;
        if (yt < 0) yt = 0 ;
        if (zt < 0) zt = 0 ;
        if (xt >= width)  xt = width-1 ;
        if (yt >= height) yt = height-1 ;
        if (zt >= depth)  zt = depth-1 ;

        /* find the node associated with this coordinate and classify */
        GCAvoxelToNode(gca, mri_inputs, xt, yt, zt, &xn, &yn, &zn) ;
        gcan = &gca->nodes[xn][yn][zn] ;
        max_p = -10000000.0 ;
        for (total_p = 0.0, n = 0 ; n < gcan->nlabels ; n++)
        {
          gc = &gcan->gcs[n] ;
          
          /* compute 1-d Mahalanobis distance */
          dist = (val-gc->mean) ;
          if (FZERO(gc->var))  /* make it a delta function */
          {
            if (FZERO(dist))
              p = 1.0 ;
            else
              p = 0.0 ;
          }
          else
            p = 1 / sqrt(gc->var * 2 * M_PI) * exp(-dist*dist/gc->var) ;

          p *= gc->prior ;
          total_p += p ;
          label_probs[n].prob = p ; label_probs[n].label = gcan->labels[n] ;
        }
        /* now sort the labels and probabilities */
        qsort(label_probs, gcan->nlabels, sizeof(LABEL_PROB), 
              compare_sort_probabilities) ;

        for (n = 0 ; n < max_labels ; n++)
        {
          MRIseq_vox(mri_dst, x, y, z, n*2) = label_probs[n].label ;
          MRIseq_vox(mri_dst, x, y, z, n*2+1) = label_probs[n].prob/total_p ;
        }
      }
    }
  }

#if 0
  MatrixFree(&m_L) ;
#endif
  
  return(mri_dst) ;
}
 
static int
compare_sort_probabilities(const void *plp1, const void *plp2)
{
  LABEL_PROB  *lp1, *lp2 ;

  lp1 = (LABEL_PROB *)plp1 ;
  lp2 = (LABEL_PROB *)plp2 ;

  if (lp1->prob > lp2->prob)
    return(1) ;
  else if (lp1->prob < lp2->prob)
    return(-1) ;

  return(0) ;
}

float
GCAcomputeLogSampleProbability(GCA *gca, GCA_SAMPLE *gcas, 
                               MRI *mri_inputs, MATRIX *m_L, int nsamples)
{
  int        x, y, z, xt, yt, zt, width, height, depth, val,
             xn, yn, zn, i ;
  MATRIX     *m_L_inv ;
  VECTOR     *v_input, *v_canon ;
  GCA_NODE   *gcan ;
  GC1D       *gc ;
  float      dist ;
  double     total_log_p ;

  m_L_inv = MatrixInverse(m_L, NULL) ;
  if (!m_L_inv)
  {
    MatrixPrint(stderr, m_L) ;
    ErrorExit(ERROR_BADPARM, 
              "GCAcomputeLogSampleProbability: matrix is not invertible") ;
  }
  v_input = VectorAlloc(4, MATRIX_REAL) ;
  v_canon = VectorAlloc(4, MATRIX_REAL) ;
  *MATRIX_RELT(v_input, 4, 1) = 1.0 ; *MATRIX_RELT(v_canon, 4, 1) = 1.0 ;

  /* go through each GC in the sample and compute the probability of
     the image at that point.
  */
  width = mri_inputs->width ; height = mri_inputs->height; 
  depth = mri_inputs->depth ;
  for (total_log_p = 0.0, i = 0 ; i < nsamples ; i++)
  {
    if (i == Gdiag_no)
      DiagBreak() ;

    xn = gcas[i].xn ; yn = gcas[i].yn ; zn = gcas[i].zn ; 
    GCAnodeToVoxel(gca, mri_inputs, xn, yn, zn, &xt, &yt, &zt) ;
    V3_X(v_canon) = (float)xt ;
    V3_Y(v_canon) = (float)yt ;
    V3_Z(v_canon) = (float)zt ;
    MatrixMultiply(m_L_inv, v_canon, v_input) ;
    x = nint(V3_X(v_input)); y = nint(V3_Y(v_input)); z = nint(V3_Z(v_input));
    if (x < 0) x = 0 ;
    if (y < 0) y = 0 ;
    if (z < 0) z = 0 ;
    if (x >= width)  x = width-1 ;
    if (y >= height) y = height-1 ;
    if (z >= depth)  z = depth-1 ;
    val = MRIvox(mri_inputs, x, y, z) ;

#if 1
    gcan = &gca->nodes[xn][yn][zn] ;
    gc = &gcan->gcs[gcas[i].n] ;
         
    dist = (val-gc->mean) ;
    total_log_p +=
      -log(sqrt(gc->var)) - 
      0.5 * (dist*dist/gc->var) +
      log(gc->prior) ;
#else
    dist = (val-gcas[i].mean) ;
    total_log_p +=
      -log(sqrt(gcas[i].var)) - 
      0.5 * (dist*dist/gcas[i].var) +
      log(gcas[i].prior) ;
#endif

    if (!finite(total_log_p))
    {
      DiagBreak() ;
      fprintf(stderr, 
              "total log p not finite at (%d, %d, %d) n = %d, "
              "var=%2.2f\n", x, y, z, gcas[i].n, gc->var) ;
    }
  }

  MatrixFree(&m_L_inv) ; MatrixFree(&v_canon) ; MatrixFree(&v_input) ;
  return((float)total_log_p) ;
}
/*
  compute the probability of the image given the transform and the class
  stats.
*/
float
GCAcomputeLogImageProbability(GCA *gca, MRI *mri_inputs, MRI *mri_labels,
                              LTA *lta)
{
  int        x, y, z, xt, yt, zt, width, height, depth, val,
             xn, yn, zn, n, label ;
  MATRIX     *m_L ;
  VECTOR     *v_input, *v_canon ;
  GCA_NODE   *gcan ;
  GC1D       *gc ;
  float      dist ;
  double     total_log_p ;

  v_input = VectorAlloc(4, MATRIX_REAL) ;
  v_canon = VectorAlloc(4, MATRIX_REAL) ;
  *MATRIX_RELT(v_input, 4, 1) = 1.0 ;
  *MATRIX_RELT(v_canon, 4, 1) = 1.0 ;

  /* convert transform to voxel coordinates */
  m_L = lta->xforms[0].m_L ;

  /* go through each voxel in the input volume and find the canonical
     voxel (and hence the classifier) to which it maps. Then update the
     classifiers statistics based on this voxel's intensity and label.
  */
  width = mri_inputs->width ; height = mri_inputs->height; 
  depth = mri_inputs->depth ;
  for (total_log_p = 0.0, x = 0 ; x < width ; x++)
  {
    V3_X(v_input) = (float)x ;
    for (y = 0 ; y < height ; y++)
    {
      V3_Y(v_input) = (float)y ;
      for (z = 0 ; z < depth ; z++)
      {
        if (x == 85 && y == 89 && z == 135)
          DiagBreak() ; 

        V3_Z(v_input) = (float)z ;
        val = MRIvox(mri_inputs, x, y, z) ;
        label = MRIvox(mri_labels, x, y, z) ;

        /* compute coordinates in canonical space */
        MatrixMultiply(m_L, v_input, v_canon) ;
        xt = nint(V3_X(v_canon)) ;
        yt = nint(V3_Y(v_canon)) ;
        zt = nint(V3_Z(v_canon)) ;
        if (xt < 0) xt = 0 ;
        if (yt < 0) yt = 0 ;
        if (zt < 0) zt = 0 ;
        if (xt >= width)  xt = width-1 ;
        if (yt >= height) yt = height-1 ;
        if (zt >= depth)  zt = depth-1 ;

        /* find the node associated with this coordinate and classify */
        GCAvoxelToNode(gca, mri_inputs, xt, yt, zt, &xn, &yn, &zn) ;
        gcan = &gca->nodes[xn][yn][zn] ;
        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          if (gcan->labels[n] == label)
            break ;
        }
        if (n < gcan->nlabels)
        {
          gc = &gcan->gcs[n] ;
         
          dist = (val-gc->mean) ;
          total_log_p +=
            -log(sqrt(gc->var)) - 
            0.5 * (dist*dist/gc->var) +
            log(gc->prior) ;
          if (!finite(total_log_p))
          {
            DiagBreak() ;
            fprintf(stderr, 
                    "total log p not finite at (%d, %d, %d) n = %d, "
                    "var=%2.2f\n", x, y, z, n, gc->var) ;
          }
        }
      }
    }
  }

  return((float)total_log_p) ;
}

#if 0
int
GCAsampleStats(GCA *gca, MRI *mri, int class, 
               Real x, Real y, Real z,
               Real *pmean, Real *pvar, Real *pprior)
{
  int    xm, xp, ym, yp, zm, zp, width, height, depth, n ;
  Real   val, xmd, ymd, zmd, xpd, ypd, zpd ;  /* d's are distances */
  Real   prior, mean, var, wt ;
  GCAN   *gcan ;
  GC1D   *gc ;

  width = gca->width ; height = gca->height ; depth = gca->depth ;

  xm = MAX((int)x, 0) ;
  xp = MIN(width-1, xm+1) ;
  ym = MAX((int)y, 0) ;
  yp = MIN(height-1, ym+1) ;
  zm = MAX((int)z, 0) ;
  zp = MIN(depth-1, zm+1) ;

  xmd = x - (float)xm ;
  ymd = y - (float)ym ;
  zmd = z - (float)zm ;
  xpd = (1.0f - xmd) ;
  ypd = (1.0f - ymd) ;
  zpd = (1.0f - zmd) ;

  prior = mean = var = 0.0 ;


  gcan = &gca->nodes[xm][ym][zm] ;
  wt = xpd * ypd * zpd 
  for (n = 0 ; n < gcan->nlabels ; n++)
  {
    if (gcan->labels[n] == class)
      break ;
  }
  if (n < gcan->nlabels)   /* found it */
  {
    gc = &gcan->gcs[n] ;
    prior += wt * gc->prior ;
  }

  gcan = &gca->nodes[xp][ym][zm] ;
  wt = xmd * ypd * zpd 

  gcan = &gca->nodes[xm][yp][zm] ;
  wt = xpd * ymd * zpd ;

  gcan = &gca->nodes[xp][yp][zm] ;
  wt = xmd * ymd * zpd ;

  gcan = &gca->nodes[xm][ym][zp] ;
  wt = xpd * ypd * zmd ;

  gcan = &gca->nodes[xp][ym][zp] ;
  wt = xmd * ypd * zmd ;

  gcan = &gca->nodes[xm][yp][zp] ;
  wt = xpd * ymd * zmd ;

  gcan = &gca->nodes[xp][yp][zp] ;
  wt = xmd * ymd * zmd ;
  return(NO_ERROR) ;
}
#endif  

#define MIN_SPACING   8

int
GCAtransformSamples(GCA *gca_src, GCA *gca_dst, GCA_SAMPLE *gcas, int nsamples)
{
  int       scale, i, label, xd, yd, zd, xs, ys, zs, n, xk, yk, zk, 
            xd0, yd0, zd0, min_y_i ;
  float     max_p, min_v, vscale, min_y ;
  GCA_NODE  *gcan ;
  GC1D      *gc ;
  MRI       *mri_found ;

  vscale = 1 ;
  mri_found = MRIalloc(gca_dst->width*vscale, gca_dst->height*vscale, 
                       gca_dst->depth*vscale, MRI_UCHAR) ;

  scale = gca_src->spacing / gca_dst->spacing ;
  min_y = 10000 ; min_y_i = -1 ;
  for (i = 0 ; i < nsamples ; i++)
  {
    label = gcas[i].label ; 
    xs = gcas[i].xn ; ys = gcas[i].yn ; zs = gcas[i].zn ; 
    xd0 = (int)(xs * scale) ; yd0 = (int)(ys * scale) ; 
    zd0 = (int)(zs * scale) ;
    max_p = -1.0 ;  /* find appropriate label with highest prior in dst */
    min_v =  10000000.0f ;
    for (xk = -scale/2 ; xk <= scale/2 ; xk++)
    {
      xd = MIN(MAX(0, xd0+xk), gca_dst->width-1) ;
      for (yk = -scale/2 ; yk <= scale/2 ; yk++)
      {
        yd = MIN(MAX(0, yd0+yk), gca_dst->height-1) ;
        for (zk = -scale/2 ; zk <= scale/2 ; zk++)
        {
          zd = MIN(MAX(0, zd0+zk), gca_dst->height-1) ;
          if (MRIvox(mri_found, (int)(xd*vscale), (int)(yd*vscale), 
                     (int)(zd*vscale)))
            continue ;
          gcan = &gca_dst->nodes[xd][yd][zd] ;
          for (n = 0 ; n < gcan->nlabels ; n++)
          {
            gc = &gcan->gcs[n] ;
            if (gcan->labels[n] == label && 
                (gc->prior > max_p || 
                 (FEQUAL(gc->prior,max_p) && gc->var < min_v)))
            {
              min_v = gc->var ;
              max_p = gc->prior ;
              gcas[i].xn = xd ; gcas[i].yn = yd ; gcas[i].zn = zd ; 
              gcas[i].n = n ;
            }
          }
        }
      }
    }
    if (max_p < 0)
    {
      fprintf(stderr, "WARNING: label %d not found at (%d,%d,%d)\n",
              label, gcas[i].xn, gcas[i].yn, gcas[i].zn) ;
      DiagBreak() ;
    }
    MRIvox(mri_found, (int)(gcas[i].xn*vscale),
           (int)(gcas[i].yn*vscale), (int)(gcas[i].zn*vscale))= 1;
    if (gcas[i].yn < min_y)
    {
      min_y = gcas[i].yn ;
      min_y_i = i ;
    }
  }

  i = min_y_i ;
  fprintf(stderr, "min_y = (%d, %d, %d) at i=%d, label=%d, n=%d\n",
          gcas[i].xn, gcas[i].yn, gcas[i].zn, i, gcas[i].label, gcas[i].n) ;
  MRIfree(&mri_found) ;
  return(NO_ERROR) ;
}

/* don't use a label with prior less than this */
#define MIN_MAX_PRIORS 0.5


static int exclude_classes[] = 
{
  0 /*1, 6, 21, 22, 23, 24, 25, 30, 57, 61, 62, 63*/
} ;


#define TILES     3
#define MAX_PCT   .1

static int compare_gca_samples(const void *pc1, const void *pc2) ;

static int
compare_gca_samples(const void *pgcas1, const void *pgcas2)
{
  register GCA_SAMPLE *gcas1, *gcas2 ;

  gcas1 = (GCA_SAMPLE *)pgcas1 ;
  gcas2 = (GCA_SAMPLE *)pgcas2 ;

/*  return(c1 > c2 ? 1 : c1 == c2 ? 0 : -1) ;*/
  if (FEQUAL(gcas1->prior, gcas2->prior))
  {
#if 0
    if (FEQUAL(gcas1->var, gcas2->var))
    {
      int  zv1, zv2 ;

      zv1 = gcas1->zn%10 ; zv2 = gcas2->zn%10 ;
      if (zv1 == zv2)
      {
        int  yv1, yv2 ;

        yv1 = gcas1->yn%10 ; zv2 = gcas2->yn%10 ;      
        return(yv1-yv2) ;
      }

      return(zv1-zv2) ;
    }
#endif

    if (gcas1->var > gcas2->var)
      return(1) ;
    else
      return(-1) ;
  }

  if (gcas1->prior > gcas2->prior)
    return(-1) ;
  else if (gcas1->prior < gcas2->prior)
    return(1) ;

  return(0) ;
}
#define MAX_SPACING   16  /* mm */

GCA_SAMPLE *
GCAfindStableSamplesByLabel(GCA *gca, int nsamples, float min_prior)
{
  GCA_SAMPLE *gcas, *ordered_labels[MAX_DIFFERENT_LABELS], *gcas2 ;
  GCA_NODE   *gcan ;
  GC1D       *gc ;
  int        found[MAX_DIFFERENT_LABELS], x, y, z, width, height, depth, n,
             label, nfound, samples_added[MAX_DIFFERENT_LABELS] ;
  float      histo[MAX_DIFFERENT_LABELS], spacing, scale ;
  int        volume[TILES][TILES], max_class, total, extra, total_found,
             label_counts[MAX_DIFFERENT_LABELS], 
             current_index[MAX_DIFFERENT_LABELS],
             ordered_label_counts[MAX_DIFFERENT_LABELS], i, index, 
             *x_indices, *y_indices, *z_indices, nindices ;

  memset(histo, 0, sizeof(histo)) ;
  memset(label_counts, 0, sizeof(label_counts)) ;
  memset(samples_added, 0, sizeof(samples_added)) ;
  memset(current_index, 0, sizeof(current_index)) ;
  memset(ordered_label_counts, 0, sizeof(ordered_label_counts)) ;
  memset(found, 0, sizeof(found)) ;
  memset(volume, 0, sizeof(volume)) ;
  gcas = calloc(nsamples, sizeof(GCA_SAMPLE )) ;
  width = gca->width ; height = gca->height ; depth = gca->depth ;

  /* compute the max priors and min variances for each class */
  for (x = 0 ; x < width ; x++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (z = 0 ; z < depth ; z++)
      {
        gcan = &gca->nodes[x][y][z] ;
        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          gc = &gcan->gcs[n] ;
          label = gcan->labels[n] ;
          if (label == Gdiag_no)
            DiagBreak() ;
          histo[label] += (float)nint(gcan->total_training*gc->prior) ;
          label_counts[label]++ ;
        }
      }
    }
  }

  for (max_class = MAX_DIFFERENT_LABELS ; max_class >= 0 ; max_class--)
    if (histo[max_class] > 0)
      break ;
  fprintf(stderr, "max class = %d\n", max_class) ;

  /* count total # of samples */
  for (total = 0, n = 1 ; n <= max_class ; n++)
    total += histo[n] ;

  fprintf(stderr, "%d total training samples found\n", total) ;

  /* turn histogram into samples/class */
  for (n = 1 ; n <= max_class ; n++)
    histo[n] = (float)nint(0.25+histo[n]*(float)nsamples/total) ;

  for (n = 0 ; n < sizeof(exclude_classes)/sizeof(exclude_classes[0]) ; n++)
    histo[exclude_classes[n]] = 0 ;

  /* crop max # per class */
  for (n = 1 ; n <= max_class ; n++)
    if (histo[n] > nint(MAX_PCT*nsamples))
      histo[n] = nint(MAX_PCT*nsamples) ;
      
  for (extra = 0, n = 1 ; n <= max_class ; n++)
    extra += histo[n] ;
  extra = nsamples-extra ;  /* from rounding */
  printf("%d extra samples for redistribution...\n", extra) ;

  /* first add to classes with only one sample */
  for (n = 1 ; extra > 0 && n <= max_class ; n++)
  {
    if (histo[n] == 1)
    {
      histo[n]++ ;
      extra-- ;
    }
  }  

  while (extra > 0)  /* add 1 to each class */
  {
    for (n = 1 ; extra > 0 && n <= max_class ; n++)
    {
      if (histo[n] >= 1)
      {
        histo[n]++ ;
        extra-- ;
      }
    }
  }

  {
    FILE *fp ;
    fp = fopen("classes.dat", "w") ;
    for (n = 1 ; n <= max_class ; n++)
      if (!FZERO(histo[n]))
        fprintf(fp, "%d  %2.1f\n", n, histo[n]) ;
    fclose(fp) ;
  }

  /* allocate arrays that will be used for sorting */
  for (n = 1 ; n <= max_class ; n++)
    if (histo[n] > 0)
    {
      ordered_labels[n] = 
        (GCA_SAMPLE *)calloc(label_counts[n], sizeof(GCA_SAMPLE)) ;
      if (!ordered_labels[n])
        ErrorExit(ERROR_NO_MEMORY, 
                  "GCAfindStableSamplesByLabel: could not allocate %d samples "
                  "for label %d", label_counts[n], n) ;
    }


  nindices = width*height*depth ;
  x_indices = (int *)calloc(nindices, sizeof(int)) ;
  y_indices = (int *)calloc(nindices, sizeof(int)) ;
  z_indices = (int *)calloc(nindices, sizeof(int)) ;

  for (i = 0 ; i < nindices ; i++)
  {
    x_indices[i] = i % width ;
    y_indices[i] = (i/width) % height ;
    z_indices[i] = (i / (width*height)) % depth ;
  }
  for (i = 0 ; i < nindices ; i++)
  {
    int tmp ;
    index = (int)randomNumber(0.0, (double)(nindices-0.0001)) ;

    tmp = x_indices[index] ; 
    x_indices[index] = x_indices[i] ; x_indices[i] = tmp ;

    tmp = y_indices[index] ; 
    y_indices[index] = y_indices[i] ; y_indices[i] = tmp ;

    tmp = z_indices[index] ; 
    z_indices[index] = z_indices[i] ; z_indices[i] = tmp ;
  }

  for (index = 0 ; index < nindices ; index++)
  {
    x = x_indices[index] ; y = y_indices[index] ; z = z_indices[index] ;
    gcan = &gca->nodes[x][y][z] ;
    for (n = 0 ; n < gcan->nlabels ; n++)
    {
      gc = &gcan->gcs[n] ;
      label = gcan->labels[n] ;
      if (histo[label] > 0)
      {
        i = ordered_label_counts[label] ;
        ordered_label_counts[label]++ ;
        ordered_labels[label][i].xn = x ;
        ordered_labels[label][i].yn = y ;
        ordered_labels[label][i].zn = z ;
        ordered_labels[label][i].n = n ;
        ordered_labels[label][i].label = label ;
        ordered_labels[label][i].prior = gc->prior ;
        ordered_labels[label][i].var = gc->var ;
        ordered_labels[label][i].mean = gc->mean ;
      }
    }
  }


  free(x_indices) ; free(y_indices) ; free(z_indices) ;

  /* sort each list of labels by prior, then by variance */
  for (n = 1 ; n <= max_class ; n++)
    if (histo[n] > 0)
    {
      qsort(ordered_labels[n], ordered_label_counts[n], sizeof(GCA_SAMPLE), 
            compare_gca_samples) ;
    }

  total_found = nfound = 0 ;

  for (spacing = MAX_SPACING ; spacing >= gca->spacing ; spacing /= 2)
  {
    MRI  *mri_found ;
    int  xv, yv, zv ;

    for (n = 1 ; n <= max_class ; n++) 
      current_index[n] = 0 ;

    scale = gca->spacing / spacing ;
    mri_found = MRIalloc(width*scale, height*scale, depth*scale, MRI_UCHAR);
    for (i = 0 ; i < total_found ; i++)
    {
      xv = gcas[i].xn*scale ; yv = gcas[i].yn*scale ; zv = gcas[i].zn*scale ;
      MRIvox(mri_found, xv, yv, zv) = 1 ;
    }

    do
    {

      nfound = 0 ;
      for (n = 1 ; n <= max_class ; n++) 
      {
        if (samples_added[n] < histo[n])/* add another sample from this class*/
        {
          while ((ordered_labels[n][current_index[n]].label < 0) &&
                 current_index[n] < ordered_label_counts[n])
            current_index[n]++ ;
          if (current_index[n] < ordered_label_counts[n])
          {
            gcas2 = &ordered_labels[n][current_index[n]] ;
            current_index[n]++ ; 
            xv = gcas2->xn*scale ; yv = gcas2->yn*scale ; zv = gcas2->zn*scale;
            
            if (n == Gdiag_no)
              DiagBreak() ;
            
            if (!MRIvox(mri_found, xv, yv, zv)) /* none at this location yet */
            {
              if (gcas2->label == Gdiag_no)
                DiagBreak() ;
              samples_added[n]++ ;
              gcas[total_found+nfound++] = *gcas2 ;
              MRIvox(mri_found, xv, yv, zv) = gcas2->label ;
              gcas2->label = -1 ;
              if (nfound+total_found >= nsamples)
                break ;
            }
          }
        }
      }

      total_found += nfound ;
    } while ((nfound > 0) && total_found < nsamples) ;
    MRIfree(&mri_found) ;
  } 

  if (total_found != nsamples)
  {
    ErrorPrintf(ERROR_BADPARM, "could only find %d samples!\n", total_found) ;
  }
  return(gcas) ;
}


GCA_SAMPLE *
GCAfindContrastSamples(GCA *gca, int *pnsamples, int min_spacing,
                       float min_prior)
{
  GCA_SAMPLE *gcas ;
  GC1D       *gc ;
  int        x, y, z, width, height, depth, label, nfound, node_stride,
             label_counts[MAX_DIFFERENT_LABELS], best_label, nzeros,
             xi, yi, zi, xk, yk, zk, i, j, k, labels[3][3][3], found ;
  float      max_prior, total_mean,
             priors[3][3][3][MAX_DIFFERENT_LABELS], best_mean, best_sigma,
             means[3][3][3][MAX_DIFFERENT_LABELS], mean, sigma,
             vars[3][3][3][MAX_DIFFERENT_LABELS] ;

  memset(label_counts, 0, sizeof(label_counts)) ;
  width = gca->width ; height = gca->height ; depth = gca->depth ;
  gcas = calloc(width*height*depth, sizeof(GCA_SAMPLE)) ;
  
  node_stride = min_spacing / gca->spacing ;

  total_mean = 0.0 ;

  /* compute the max priors and min variances for each class */
  for (nzeros = nfound = x = 0 ; x < width ; x += node_stride)
  {
    for (y = 0 ; y < height ; y += node_stride)
    {
      for (z = 0 ; z < depth ; z += node_stride)
      {
        if (abs(x-31)<=node_stride &&
            abs(y-22)<=node_stride &&
            abs(z-36)<=node_stride)
          DiagBreak() ;
        for (xk = -1 ; xk <= 1 ; xk++)
        {
          xi = x+xk ; i = xk+1 ;
          if (xi < 0 || xi >= width)
            continue ;
          for (yk = -1 ; yk <= 1 ; yk++)
          {
            yi = y+yk ; j = yk + 1 ;
            if (yi < 0 || yi >= height)
              continue ;
            for (zk = -1 ; zk <= 1 ; zk++)
            {
              zi = z+zk ; k = zk+1 ;
              if (zi < 0 || zi >= depth)
                continue ;
              gcaRegionStats(gca, xi, yi, zi, 
                             node_stride/3, 
                             priors[i][j][k], 
                             vars[i][j][k], 
                             means[i][j][k]) ;
              if (priors[i][j][k][4] > .5*min_prior ||  /* left lat ven */
                  priors[i][j][k][5] > .5*min_prior ||  /* inf left lat ven */
                  priors[i][j][k][14] > .5*min_prior ||  /* 3rd ven */
                  priors[i][j][k][15] > .5*min_prior ||  /* 4th ven */
                  priors[i][j][k][43] > .5*min_prior ||
                  priors[i][j][k][44] > .5*min_prior)
                DiagBreak() ;
              max_prior = 0 ;

              /* find highest prior label */
              for (label = 0 ; label < MAX_DIFFERENT_LABELS ; label++)
              {
                if (priors[i][j][k][label] > max_prior)
                {
                  max_prior = priors[i][j][k][label] ;
                  labels[i][j][k] = label ;
                }
              }
            }
          }
        }

        /* search nbrhd for high-contrast stable label */
        best_label = labels[1][1][1] ; found = 0 ;
        best_mean = means[1][1][1][best_label] ; 
        best_sigma = sqrt(vars[1][1][1][best_label]) ;

        gc = gcaFindHighestPriorGC(gca, x, y, z, best_label,node_stride/3);
        if (!gc || gc->prior < min_prior)
          continue ;

        for (xk = -1 ; xk <= 1 ; xk++)
        {
          xi = x+xk ; i = xk+1 ;
          if (xi < 0 || xi >= width)
            continue ;
          for (yk = -1 ; yk <= 1 ; yk++)
          {
            yi = y+yk ; j = yk + 1 ;
            if (yi < 0 || yi >= height)
              continue ;
            for (zk = -1 ; zk <= 1 ; zk++)
            {
              zi = z+zk ; k = zk+1 ;
              if (zi < 0 || zi >= depth)
                continue ;
              if (i == 1 && j == 1 && k == 1)
                continue ;
              label = labels[i][j][k] ;
              if (priors[i][j][k][label] < min_prior || label == best_label)
                continue ;
              
              mean = means[i][j][k][label] ; 
              sigma = sqrt(vars[i][j][k][label]) ;
              gc = gcaFindHighestPriorGC(gca, xi, yi, zi, label,node_stride/3);
              if (!gc || gc->prior < min_prior)
                continue ;
              if (abs(best_mean - mean) > 4*abs(best_sigma+sigma))
              {
#if 0
                if (gcaFindBestSample(gca, xi, yi, zi, label, node_stride/3, 
                         &gcas[nfound]) == NO_ERROR)
#else
                if (gcaFindClosestMeanSample(gca, mean, min_prior,
                                             xi, yi, zi, label, 
                                             node_stride/3, &gcas[nfound]) == 
                    NO_ERROR)
#endif
                {
                  if (label == Gdiag_no)
                    DiagBreak() ;
                  found = 1 ;
                  if (label > 0)
                    total_mean += means[i][j][k][label] ;
                  else
                    nzeros++ ;
                  label_counts[label]++ ;
                  nfound++ ;
                }
              }
            }
          }
        }

        if (found)   /* add central label */
        {
          if (best_label == Gdiag_no)
            DiagBreak() ;
#if 0
          if (gcaFindBestSample(gca, x, y, z, best_label, node_stride/3, 
                                &gcas[nfound]) == NO_ERROR)
#else
          if (gcaFindClosestMeanSample(gca, best_mean, min_prior, x, y, z, 
                                       best_label, node_stride/3, 
                                       &gcas[nfound]) == 
              NO_ERROR)
#endif
          {
            if (best_label > 0)
              total_mean += means[1][1][1][best_label] ;
            else
              nzeros++ ;
            label_counts[best_label]++ ;
            nfound++ ;
          }
        }
      }
    }
  }

  fprintf(stderr, "total sample mean = %2.1f\n", 
          total_mean/((float)nfound-nzeros)) ;

  {
    int  n ;
    FILE *fp ;

    fp = fopen("classes.dat", "w") ;
    for (n = 0 ; n < MAX_DIFFERENT_LABELS ; n++)
      if (label_counts[n] > 0)
        fprintf(fp, "%d  %d\n", n, label_counts[n]) ;
    fclose(fp) ;
  }

  *pnsamples = nfound ;

  return(gcas) ;
}
#if 1
GCA_SAMPLE *
GCAfindStableSamples(GCA *gca, int *pnsamples, int min_spacing,float min_prior)
{
  GCA_SAMPLE *gcas ;
  int        x, y, z, width, height, depth, label, nfound, node_stride,
             label_counts[MAX_DIFFERENT_LABELS], best_label, nzeros ;
  float      max_prior, total_mean, /*mean_dist,*/ best_mean_dist,
             priors[MAX_DIFFERENT_LABELS], means[MAX_DIFFERENT_LABELS], 
             vars[MAX_DIFFERENT_LABELS] ;
  MRI        *mri_filled ;

#define MIN_UNKNOWN_DIST  8

  memset(label_counts, 0, sizeof(label_counts)) ;
  width = gca->width ; height = gca->height ; depth = gca->depth ;
  gcas = calloc(width*height*depth, sizeof(GCA_SAMPLE)) ;

  mri_filled = MRIalloc(width*gca->spacing+1,height*gca->spacing+1, 
                        depth*gca->spacing+1,MRI_UCHAR);
  
  node_stride = min_spacing / gca->spacing ;

  total_mean = 0.0 ;

  /* compute the max priors and min variances for each class */
  for (nzeros = nfound = x = 0 ; x < width ; x += node_stride)
  {
    for (y = 0 ; y < height ; y += node_stride)
    {
      for (z = 0 ; z < depth ; z += node_stride)
      {
        if (abs(x-31)<=node_stride &&
            abs(y-22)<=node_stride &&
            abs(z-36)<=node_stride)
          DiagBreak() ;
        gcaRegionStats(gca, x, y, z, node_stride/2, priors, vars, means) ;

        if (priors[4] > .5*min_prior ||  /* left lat ven */
            priors[5] > .5*min_prior ||  /* inf left lat ven */
            priors[14] > .5*min_prior ||  /* 3rd ven */
            priors[15] > .5*min_prior ||  /* 4th ven */
            priors[43] > .5*min_prior ||
            priors[44] > .5*min_prior)
          DiagBreak() ;
            
        best_mean_dist = 0.0 ;
        max_prior = -1 ;

        if ((different_nbr_labels(gca, x, y, z, 
                                  ceil(UNKNOWN_DIST/gca->spacing),0) > 0) &&
            (priors[0] >= min_prior))
          best_label = 0 ;
        else 
          best_label = -1 ;

        for (label = 1 ; label < MAX_DIFFERENT_LABELS ; label++)
        {
          if (priors[label] < min_prior)
            continue ;

          if ((best_label == 0) ||
              (priors[label] > max_prior) ||
              (FEQUAL(priors[label], max_prior) && 
               label_counts[best_label] > label_counts[label]))
            /*              (mean_dist > 2*best_mean_dist))*/
          {
            best_label = label ;
            max_prior = priors[label] ;
          }
        }
        if (best_label >= 0)
        {
          if (best_label == 0)
          {
#if 1
            if (gcaFindBestSample(gca, x, y, z, best_label, node_stride/2, 
                                  &gcas[nfound]) == NO_ERROR)
#else
            gcaFindClosestMeanSample(gca, 255, min_prior, x, y, z, 0,
                                     node_stride/2, &gcas[nfound]);
            if (gcas[nfound].mean > 100)
#endif
            {
              int  xv, yv, zv ;
              
              xv = (int)((float)x*gca->spacing) ;
              yv = (int)((float)y*gca->spacing) ;
              zv = (int)((float)z*gca->spacing) ;
              if (MRIvox(mri_filled, xv, yv, zv) == 0)
              {
                mriFillRegion(mri_filled, xv, yv, zv, 1, MIN_UNKNOWN_DIST) ;
                /*                MRIvox(mri_filled, xv, yv, zv) = 1 ;*/
                nzeros++ ;
                label_counts[best_label]++ ;
                nfound++ ;
              }
            }
          }
          else
          {
            /* find best gc with this label */
            if (gcaFindBestSample(gca, x, y, z, best_label, node_stride/2, 
                                  &gcas[nfound]) == NO_ERROR)
            {
              total_mean += means[best_label] ;
              label_counts[best_label]++ ;
              nfound++ ;
            }
          }
        }
      }
    }
  }

  fprintf(stderr, "total sample mean = %2.1f\n", 
          total_mean/((float)nfound-nzeros)) ;

  {
    int  n ;
    FILE *fp ;

    fp = fopen("classes.dat", "w") ;
    for (n = 0 ; n < MAX_DIFFERENT_LABELS ; n++)
      if (label_counts[n] > 0)
        fprintf(fp, "%d  %d\n", n, label_counts[n]) ;
    fclose(fp) ;
  }

  *pnsamples = nfound ;

  MRIfree(&mri_filled) ;
  return(gcas) ;
}
#else
GCA_SAMPLE *
GCAfindStableSamples(GCA *gca, int *pnsamples, int min_spacing,float min_prior)
{
  GCA_SAMPLE *gcas ;
  GCA_NODE   *gcan ;
  GC1D       *gc, *best_gc ;
  int        x, y, z, width, height, depth, n, label, nfound, node_stride,
             label_counts[MAX_DIFFERENT_LABELS], best_n, best_label,
             xk, yk, zk, xi, yi, zi, best_x, best_y, best_z, nzeros ;
  float      max_prior, total_mean, mean_dist, best_mean_dist ;

  memset(label_counts, 0, sizeof(label_counts)) ;
  width = gca->width ; height = gca->height ; depth = gca->depth ;
  gcas = calloc(width*height*depth, sizeof(GCA_SAMPLE)) ;
  
  node_stride = min_spacing / gca->spacing ;

  total_mean = 0.0 ;

  /* compute the max priors and min variances for each class */
  for (nzeros = nfound = x = 0 ; x < width ; x += node_stride)
  {
    for (y = 0 ; y < height ; y += node_stride)
    {
      for (z = 0 ; z < depth ; z += node_stride)
      {
        if (abs(x-31)<=node_stride &&
            abs(y-22)<=node_stride &&
            abs(z-36)<=node_stride)
          DiagBreak() ;
        best_n = best_label = -1 ; best_gc = NULL ; best_mean_dist = 0.0 ;
        max_prior = -1 ; best_x = best_y = best_z = -1 ;
        for (xk = 0 ; xk < node_stride ; xk++)
        {
          xi = MIN(x+xk, width-1) ;
          for (yk = 0 ; yk < node_stride ; yk++)
          {
            yi = MIN(y+yk, height-1) ;
            for (zk = 0 ; zk < node_stride ; zk++)
            {
              zi = MIN(z+zk, depth-1) ;
              if (xi == 31 && yi == 22 && zi == 36)
                DiagBreak() ;
          
              gcan = &gca->nodes[xi][yi][zi] ;
              for (n = 0 ; n < gcan->nlabels ; n++)
              {
                gc = &gcan->gcs[n] ;
                label = gcan->labels[n] ;
                if (label == Gdiag_no)
                  DiagBreak() ;
                if ((label <= 0 && best_label >= 0) || gc->prior < min_prior)
                  continue ;

                if (label == 0 && 
                    !different_nbr_labels(gca, xi, yi, zi, 
                                        (int)ceil(UNKNOWN_DIST/gca->spacing),
                                          0))
                  continue ;
                
                if (nfound > nzeros)
                {
                  mean_dist = (gc->mean-total_mean/((float)nfound-nzeros)) ;
                  mean_dist = sqrt(mean_dist * mean_dist) ;
                  if (best_label >= 0 && 2*mean_dist < best_mean_dist)
                    continue ;
                }
                else
                  mean_dist = 0 ;

                if ((best_label == 0) ||
                    (gc->prior > max_prior) ||
                    (FEQUAL(gc->prior, max_prior) && 
                     label_counts[best_label] > label_counts[label]) ||
                    (mean_dist > 2*best_mean_dist)
#if 0
                    || label_counts[best_label] > 5*label_counts[label]
#endif
                    )
                {
                  if (label == Gdiag_no)
                    DiagBreak() ;
                  best_label = label ; best_gc = gc ;
                  max_prior = gc->prior ; best_n = n ;
                  if (nfound > nzeros)
                  {
                    best_mean_dist = 
                      (gc->mean-total_mean/(float)(nfound-nzeros)) ;
                    best_mean_dist = sqrt(best_mean_dist * best_mean_dist) ;
                  }
                  best_x = xi ; best_y = yi ; best_z = zi ;
                }
              }
            }
          }
        }

        if (best_label == 0 && 
            !different_nbr_labels(gca, best_x, best_y,best_z,
                                  (int)ceil(UNKNOWN_DIST/gca->spacing),0))
          continue ;
                
        if (x == 4 && y == 3 && z == 6)
          DiagBreak() ;
        if (best_n >= 0)
        {
          gcas[nfound].xn = best_x ; gcas[nfound].yn = best_y ;
          gcas[nfound].zn = best_z ;
          gcas[nfound].label = best_label ;
          gcas[nfound].n = best_n ;
          gcas[nfound].var = best_gc->var ;
          gcas[nfound].mean = best_gc->mean ;
          gcas[nfound].prior = best_gc->prior ;
          if (best_label > 0)
            total_mean += best_gc->mean ;
          else
            nzeros++ ;
          label_counts[best_label]++ ;
          nfound++ ;
        }
      }
    }
  }

  fprintf(stderr, "total sample mean = %2.1f\n", 
          total_mean/((float)nfound-nzeros)) ;

  {
    int  n ;
    FILE *fp ;

    fp = fopen("classes.dat", "w") ;
    for (n = 0 ; n < MAX_DIFFERENT_LABELS ; n++)
      if (label_counts[n] > 0)
        fprintf(fp, "%d  %d\n", n, label_counts[n]) ;
    fclose(fp) ;
  }

  *pnsamples = nfound ;

  return(gcas) ;
}
#endif
int
GCAwriteSamples(GCA *gca, MRI *mri, GCA_SAMPLE *gcas, int nsamples,
                char *fname)
{
  int    n, xv, yv, zv ;
  MRI    *mri_dst ;

  mri_dst = MRIclone(mri, NULL) ;

  for (n = 0 ; n < nsamples ; n++)
  {
    if (gcas[n].label == Gdiag_no)
      DiagBreak() ;
    GCAnodeToVoxel(gca, mri_dst, gcas[n].xn, gcas[n].yn, gcas[n].zn, 
                   &xv, &yv, &zv) ;
    if (gcas[n].label > 0)
      MRIvox(mri_dst, xv, yv, zv) = gcas[n].label ;
    else
      MRIvox(mri_dst, xv, yv, zv) = 29 ;  /* Left undetermined - visible */
    if (DIAG_VERBOSE_ON && Gdiag & DIAG_SHOW)
      printf("label %d: (%d, %d, %d) <-- (%d, %d, %d)\n",
             gcas[n].label,gcas[n].xn,gcas[n].yn,gcas[n].zn, xv, yv, zv) ;
  }
  MRIwrite(mri_dst, fname) ;
  MRIfree(&mri_dst) ;
  return(NO_ERROR) ;
}
MRI *
GCAmri(GCA *gca, MRI *mri)
{
  int      width, height, depth, x, y, z, xn, yn, zn, n ;
  float    val ;
  GC1D     *gc ;
  GCA_NODE *gcan ;

  if (!mri)
    mri = MRIalloc(gca->width, gca->height, gca->depth, MRI_UCHAR) ;
  width = mri->width ; height = mri->height ; depth = mri->depth ;

  for (x = 0 ; x < width ; x++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (z = 0 ; z < depth ; z++)
      {
        GCAvoxelToNode(gca, mri, x, y, z, &xn, &yn, &zn) ;
        gcan = &gca->nodes[xn][yn][zn] ;
        for (val = 0.0, n = 0 ; n < gcan->nlabels ; n++)
        {
          gc = &gcan->gcs[n] ;
          val += gc->mean*gc->prior ;
        }
        MRIvox(mri, x, y, z) = (unsigned char)val ;
      }
    }
  }
  return(mri) ;
}

static int 
different_nbr_labels(GCA *gca, int x, int y, int z, int wsize, int label)
{
  int      xk, yk, zk, xi, yi, zi, nbrs, n, width, height, depth ;
  GCA_NODE *gcan ;

  width = gca->width ; height = gca->height ; depth = gca->depth ;
  for (nbrs = 0, xk = -wsize ; xk <= wsize ; xk++)
  {
    xi = x+xk ;
    if (xi < 0 || xi >= width)
      continue ;

    for (yk = -wsize ; yk <= wsize ; yk++)
    {
      yi = y+yk ;
      if (yi < 0 || yi >= height)
        continue ;

      for (zk = -wsize ; zk <= wsize ; zk++)
      {
        zi = z+zk ;
        if (zi < 0 || zi >= depth)
          continue ;
        
        gcan = &gca->nodes[xi][yi][zi] ;

        for (n = 0 ; n < gcan->nlabels ; n++)
          if (gcan->labels[n] != label)
            nbrs++ ;
      }
    }
  }
  return(nbrs) ;
}
static int
gcaRegionStats(GCA *gca, int x, int y, int z, int wsize,
               float *priors, float *vars, float *means)
{
  int        xk, yk, zk, xi, yi, zi, width, height, depth, label, n,
             total_training ;
  GCA_NODE  *gcan  ;
  GC1D      *gc ;
  float     dof ;

  if (x == 28 && y == 24 && z == 36)
    DiagBreak() ;

  memset(priors, 0, MAX_DIFFERENT_LABELS*sizeof(float)) ;
  memset(vars, 0, MAX_DIFFERENT_LABELS*sizeof(float)) ;
  memset(means, 0, MAX_DIFFERENT_LABELS*sizeof(float)) ;

  width = gca->width ; height = gca->height ; depth = gca->depth ;
  total_training = 0 ;
  for (xk = -wsize ; xk <= wsize ; xk++)
  {
    xi = x+xk ;
    if (xi < 0 || xi >= width)
      continue ;

    for (yk = -wsize ; yk <= wsize ; yk++)
    {
      yi = y+yk ;
      if (yi < 0 || yi >= height)
        continue ;

      for (zk = -wsize ; zk <= wsize ; zk++)
      {
        zi = z+zk ;
        if (zi < 0 || zi >= depth)
          continue ;
        
        gcan = &gca->nodes[xi][yi][zi] ;
        total_training += gcan->total_training ;

        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          label = gcan->labels[n] ;
          if (label == Gdiag_no)
            DiagBreak() ;
          gc = &gcan->gcs[n] ;
          dof = (gc->prior * (float)gcan->total_training) ;
          
          vars[label] +=  dof * gc->var ;
          means[label] += dof * gc->mean ;
          priors[label] += dof ;
        }
      }
    }
  }

  for (label = 0 ; label < MAX_DIFFERENT_LABELS ; label++)
  {
    dof = priors[label] ;
    if (dof > 0)
    {
      vars[label] /= dof ;
      priors[label] /= total_training ;
      means[label] /= dof ;
    }
  }
  return(NO_ERROR) ;
}
static int
gcaFindBestSample(GCA *gca, int x, int y, int z,int label,int wsize,
                  GCA_SAMPLE *gcas)
{
  int        xk, yk, zk, xi, yi, zi, width, height, depth, n,
             best_n, best_x, best_y, best_z ;
  GCA_NODE   *gcan  ;
  GC1D       *gc, *best_gc ;
  float      max_prior, min_var ;

  width = gca->width ; height = gca->height ; depth = gca->depth ;
  max_prior = 0.0 ; min_var = 10000.0f ; best_gc = NULL ;
  best_x = best_y = best_z = -1 ; best_n = -1 ;
  for (xk = -wsize ; xk <= wsize ; xk++)
  {
    xi = x+xk ;
    if (xi < 0 || xi >= width)
      continue ;

    for (yk = -wsize ; yk <= wsize ; yk++)
    {
      yi = y+yk ;
      if (yi < 0 || yi >= height)
        continue ;

      for (zk = -wsize ; zk <= wsize ; zk++)
      {
        zi = z+zk ;
        if (zi < 0 || zi >= depth)
          continue ;
        
        gcan = &gca->nodes[xi][yi][zi] ;

        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          if (gcan->labels[n] != label)
            continue ;
          DiagBreak() ;
          gc = &gcan->gcs[n] ;
          if (gc->prior > max_prior ||
              (FEQUAL(gc->prior, max_prior) && (gc->var < min_var)) ||
              (label == 0 && gc->mean > best_gc->mean))
          {
            max_prior = gc->prior ; min_var = gc->var ;
            best_gc = gc ; best_x = xi ; best_y = yi ; best_z = zi ; 
            best_n = n ;
          }
        }
      }
    }
  }

  if (best_x < 0)
  {
    ErrorPrintf(ERROR_BADPARM, 
                "could not find GC1D for label %d at (%d,%d,%d)\n", 
                label, x, y, z) ;
    return(ERROR_BADPARM) ;
  }
  if (best_x == 145/4 && best_y == 89/4 && best_z == 125/4)
    DiagBreak() ;
  gcas->xn = best_x ; gcas->yn = best_y ; gcas->zn = best_z ;
  gcas->label = label ; gcas->n = best_n ; gcas->mean = best_gc->mean ;
  gcas->var = best_gc->var ; gcas->prior = best_gc->prior ;

  return(NO_ERROR) ;
}
static GC1D *
gcaFindHighestPriorGC(GCA *gca, int x, int y, int z,int label,int wsize)
{
  int        xk, yk, zk, xi, yi, zi, width, height, depth, n ;
  GCA_NODE   *gcan  ;
  GC1D       *gc, *best_gc ;
  float      max_prior ;

  width = gca->width ; height = gca->height ; depth = gca->depth ;
  max_prior = 0.0 ; best_gc = NULL ;
  for (xk = -wsize ; xk <= wsize ; xk++)
  {
    xi = x+xk ;
    if (xi < 0 || xi >= width)
      continue ;

    for (yk = -wsize ; yk <= wsize ; yk++)
    {
      yi = y+yk ;
      if (yi < 0 || yi >= height)
        continue ;

      for (zk = -wsize ; zk <= wsize ; zk++)
      {
        zi = z+zk ;
        if (zi < 0 || zi >= depth)
          continue ;
        
        gcan = &gca->nodes[xi][yi][zi] ;

        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          if (gcan->labels[n] != label)
            continue ;
          gc = &gcan->gcs[n] ;
          if (gc->prior > max_prior)
          {
            max_prior = gc->prior ; 
            best_gc = gc ; 
          }
        }
      }
    }
  }

  if (best_gc == NULL)
  {
    ErrorPrintf(ERROR_BADPARM, 
                "could not find GC for label %d at (%d,%d,%d)\n", 
                label, x, y, z) ;
    return(NULL) ;
  }

  return(best_gc) ;
}

static int
gcaFindClosestMeanSample(GCA *gca, float mean, float min_prior, int x, int y, 
                         int z, int label, int wsize, GCA_SAMPLE *gcas)
{
  int        xk, yk, zk, xi, yi, zi, width, height, depth, n,
             best_n, best_x, best_y, best_z ;
  GCA_NODE   *gcan  ;
  GC1D       *gc, *best_gc ;
  float      min_dist ;

  width = gca->width ; height = gca->height ; depth = gca->depth ;
  min_dist = 1000000.0f ; best_gc = NULL ;
  best_x = best_y = best_z = -1 ; best_n = -1 ;
  for (xk = -wsize ; xk <= wsize ; xk++)
  {
    xi = x+xk ;
    if (xi < 0 || xi >= width)
      continue ;

    for (yk = -wsize ; yk <= wsize ; yk++)
    {
      yi = y+yk ;
      if (yi < 0 || yi >= height)
        continue ;

      for (zk = -wsize ; zk <= wsize ; zk++)
      {
        zi = z+zk ;
        if (zi < 0 || zi >= depth)
          continue ;
        
        gcan = &gca->nodes[xi][yi][zi] ;

        for (n = 0 ; n < gcan->nlabels ; n++)
        {
          gc = &gcan->gcs[n] ;
          if (gcan->labels[n] != label || gc->prior < min_prior)
            continue ;
          if (abs(gc->mean - mean) < min_dist)
          {
            min_dist = abs(gc->mean-mean) ;
            best_gc = gc ; best_x = xi ; best_y = yi ; best_z = zi ; 
            best_n = n ;
          }
        }
      }
    }
  }

  if (best_x < 0)
  {
    ErrorPrintf(ERROR_BADPARM, 
                "could not find GC for label %d at (%d,%d,%d)\n", 
                label, x, y, z) ;
    return(ERROR_BADPARM) ;
  }
  if (best_x == 141/4 && best_y == 37*4 && best_z == 129*4)
    DiagBreak() ;
  gcas->xn = best_x ; gcas->yn = best_y ; gcas->zn = best_z ;
  gcas->label = label ; gcas->n = best_n ; gcas->mean = best_gc->mean ;
  gcas->var = best_gc->var ; gcas->prior = best_gc->prior ;

  return(NO_ERROR) ;
}
int
GCAtransformAndWriteSamples(GCA *gca, MRI *mri, GCA_SAMPLE *gcas, 
                            int nsamples,char *fname,LTA *lta)
{
  int    n, xv, yv, zv ;
  MRI    *mri_dst ;
  MATRIX *m_L_inv ;
  VECTOR *v_input, *v_canon ;

  m_L_inv = MatrixInverse(lta->xforms[0].m_L, NULL) ;
  if (!m_L_inv)
  {
    MatrixPrint(stderr, lta->xforms[0].m_L) ;
    ErrorExit(ERROR_BADPARM, 
              "GCAcomputeLogSampleProbability: matrix is not invertible") ;
  }
  mri_dst = MRIclone(mri, NULL) ;
  
  v_input = VectorAlloc(4, MATRIX_REAL) ;
  v_canon = VectorAlloc(4, MATRIX_REAL) ;
  *MATRIX_RELT(v_input, 4, 1) = 1.0 ; *MATRIX_RELT(v_canon, 4, 1) = 1.0 ;

  for (n = 0 ; n < nsamples ; n++)
  {
    if (gcas[n].label == Gdiag_no)
      DiagBreak() ;
    GCAnodeToVoxel(gca, mri_dst, gcas[n].xn, gcas[n].yn, gcas[n].zn, 
                   &xv, &yv, &zv) ;
    V3_X(v_canon) = (float)xv ; V3_Y(v_canon) = (float)yv ;
    V3_Z(v_canon) = (float)zv ;
    MatrixMultiply(m_L_inv, v_canon, v_input) ;
    xv = nint(V3_X(v_input)); yv = nint(V3_Y(v_input)); zv=nint(V3_Z(v_input));
    if (xv < 0) xv = 0 ;
    if (xv >= mri->width) xv = mri->width-1 ;
    if (yv < 0) xv = 0 ;
    if (yv >= mri->height) xv = mri->height-1 ;
    if (zv < 0) xv = 0 ;
    if (zv >= mri->depth) xv = mri->depth-1 ;
    if (gcas[n].label > 0)
      MRIvox(mri_dst, xv, yv, zv) = gcas[n].label ;
    else
      MRIvox(mri_dst, xv, yv, zv) = 29 ;  /* Left undetermined - visible */
    if (DIAG_VERBOSE_ON && Gdiag & DIAG_SHOW)
      printf("label %d: (%d, %d, %d) <-- (%d, %d, %d)\n",
             gcas[n].label,gcas[n].xn,gcas[n].yn,gcas[n].zn, xv, yv, zv) ;
  }
  fprintf(stderr, "writing samples to %s...\n", fname) ;
  MRIwrite(mri_dst, fname) ;
  MRIfree(&mri_dst) ;

  MatrixFree(&m_L_inv) ; MatrixFree(&v_canon) ; MatrixFree(&v_input) ;

  return(NO_ERROR) ;
}

static int
mriFillRegion(MRI *mri, int x, int y, int z, int fill_val, int whalf)
{
  int   xi, xk, yi, yk, zi, zk ;

  for (xk = -whalf ; xk <= whalf ; xk++)
  {
    xi = mri->xi[x+xk] ;
    for (yk = -whalf ; yk <= whalf ; yk++)
    {
      yi = mri->xi[y+yk] ;
      for (zk = -whalf ; zk <= whalf ; zk++)
      {
        zi = mri->zi[z+zk] ;
        MRIvox(mri, xi, yi, zi) = fill_val ;
      }
    }
  }
  return(NO_ERROR) ;
}

