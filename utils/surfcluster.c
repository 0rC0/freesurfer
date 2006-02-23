/*----------------------------------------------------------------
  surfcluster.c - routines for growing clusters on the surface
  based on intensity thresholds and area threshold. Note: this
  makes use of the undefval in the MRI_SURFACE structure.
  $Id: surfcluster.c,v 1.13 2006/02/23 22:18:33 greve Exp $
  ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "error.h"
#include "diag.h"
#include "mri.h"
#include "resample.h"
#include "matrix.h"

#include "volcluster.h"
#include "surfcluster.h"

static int sclustCompare(const void *a, const void *b);

/* ------------------------------------------------------------
   sclustMapSurfClusters() - grows a clusters on the surface.  The
   cluster is a list of contiguous vertices that that meet the
   threshold criteria. The cluster does not exist as a list at this
   point. Rather, the clusters are mapped using using the undefval
   element of the MRI_SURF structure. If a vertex meets the cluster
   criteria, then undefval is set to the cluster number.
   ------------------------------------------------------------ */
SCS *sclustMapSurfClusters(MRI_SURFACE *Surf, float thmin, float thmax, 
         int thsign, float minarea, int *nClusters,
         MATRIX *XFM)
{
  SCS *scs, *scs_sorted;
  int vtx, vtx_inrange, vtx_clustno, CurrentClusterNo;
  float vtx_val,ClusterArea;
  int nVtxsInCluster;

  /* initialized all cluster numbers to 0 */
  for(vtx = 0; vtx < Surf->nvertices; vtx++)
    Surf->vertices[vtx].undefval = 0; /* overloads this elem of struct */

  /* Go through each vertex looking for one that meets the threshold
     criteria and has not been previously assigned to a cluster. 
     When found, grow it out. */
  CurrentClusterNo = 1;
  for(vtx = 0; vtx < Surf->nvertices; vtx++){

    vtx_val     = Surf->vertices[vtx].val;
    vtx_clustno = Surf->vertices[vtx].undefval;
    vtx_inrange = clustValueInRange(vtx_val,thmin,thmax,thsign);

    if(vtx_clustno == 0 && vtx_inrange){
      sclustGrowSurfCluster(CurrentClusterNo,vtx,Surf,thmin,thmax,thsign);
      if(minarea > 0){
	/* If the cluster does not meet the area criteria, delete it */
	ClusterArea = sclustSurfaceArea(CurrentClusterNo, Surf, 
					&nVtxsInCluster) ;
	if(ClusterArea < minarea){
	  sclustZeroSurfaceClusterNo(CurrentClusterNo, Surf);
	  continue;
	}
      }
      CurrentClusterNo++;
    }
  }
  
  *nClusters = CurrentClusterNo-1;
  if(*nClusters == 0) return(NULL);

  /* Get a summary of the clusters */
  scs = SurfClusterSummary(Surf, XFM, nClusters);

  /* Sort the clusters by descending maxval */
  scs_sorted  = SortSurfClusterSum(scs, *nClusters);

  if(Gdiag_no > 1){
    printf("--- Surface Cluster Summary (unsorted) ---------------\n");
    DumpSurfClusterSum(stdout,scs,*nClusters);
    printf("---------- sorted ---------------\n");
    DumpSurfClusterSum(stdout,scs_sorted,*nClusters);
  }

  /* Remap the cluster numbers to match the sorted */
  sclustReMap(Surf, *nClusters, scs_sorted);

  free(scs);

  return(scs_sorted);
}
/* ------------------------------------------------------------ 
   sclustGrowSurfCluster() - grows a cluster on the surface from
   the SeedVtx. The cluster is a list of vertices that are 
   contiguous with the seed vertex and that meet the threshold
   criteria. The cluster map itself is defined using the
   undefval of the MRI_SURF structure. If a vertex meets the
   cluster criteria, then undefval is set to the ClusterNo.
   The ClustNo cannot be 0.
   ------------------------------------------------------------ */
int sclustGrowSurfCluster(int ClusterNo, int SeedVtx, MRI_SURFACE *Surf, 
        float thmin, float thmax, int thsign)
{
  int nbr, nbr_vtx, nbr_inrange, nbr_clustno;
  float nbr_val;

  if(ClusterNo == 0){
    printf("ERROR: clustGrowSurfCluster(): ClusterNo is 0\n");
    return(1);
  }

  Surf->vertices[SeedVtx].undefval = ClusterNo;

  for(nbr=0; nbr < Surf->vertices[SeedVtx].vnum; nbr++){

    nbr_vtx     = Surf->vertices[SeedVtx].v[nbr];
    nbr_clustno = Surf->vertices[nbr_vtx].undefval;
    if(nbr_clustno != 0) continue;
    nbr_val     = Surf->vertices[nbr_vtx].val;
    if(fabs(nbr_val) < thmin) continue;
    nbr_inrange = clustValueInRange(nbr_val,thmin,thmax,thsign);
    if(!nbr_inrange) continue;
    sclustGrowSurfCluster(ClusterNo, nbr_vtx, Surf, thmin,thmax,thsign);

  }
  return(0);
}
/*----------------------------------------------------------------
  sclustSurfaceArea() - computes the surface area (in mm^2) of a 
  cluster. Note:   MRIScomputeMetricProperties() must have been
  run on the surface. 
  ----------------------------------------------------------------*/
float sclustSurfaceArea(int ClusterNo, MRI_SURFACE *Surf, int *nvtxs) 
{
  int vtx, vtx_clusterno;
  float ClusterArea;

  *nvtxs = 0;
  ClusterArea = 0.0;
  for(vtx = 0; vtx < Surf->nvertices; vtx++){
    vtx_clusterno = Surf->vertices[vtx].undefval;
    if(vtx_clusterno != ClusterNo) continue;
    if(! Surf->group_avg_area_loaded)
      ClusterArea += Surf->vertices[vtx].area;
    else
      ClusterArea += Surf->vertices[vtx].group_avg_area;
    (*nvtxs) ++;
  }

  if(0 && Surf->group_avg_surface_area > 0){
    if(getenv("FIX_VERTEX_AREA") != NULL){
      printf("INFO: surfcluster: Fixing group surface area\n");
      ClusterArea *= (Surf->group_avg_surface_area/Surf->total_area);
    }
    else printf("INFO: surfcluster: NOT fixing group surface area\n");
  }

  return(ClusterArea);
}
/*----------------------------------------------------------------
  sclustSurfaceMax() - returns the maximum intensity value of
  inside a given cluster and the vertex at which it occured.
----------------------------------------------------------------*/
float sclustSurfaceMax(int ClusterNo, MRI_SURFACE *Surf, int *vtxmax) 
{
  int vtx, vtx_clusterno, first_hit;
  float vtx_val, vtx_val_max = 0;

  first_hit = 1;
  for(vtx = 0; vtx < Surf->nvertices; vtx++){
    vtx_clusterno = Surf->vertices[vtx].undefval;
    if(vtx_clusterno != ClusterNo) continue;

    vtx_val = Surf->vertices[vtx].val;
    if(first_hit){
      vtx_val_max = vtx_val;
      *vtxmax = vtx;
      first_hit = 0;
      continue;
    }

    if(fabs(vtx_val) > fabs(vtx_val_max)){
      vtx_val_max = vtx_val;
      *vtxmax = vtx;
    }

  }

  return(vtx_val_max);
}
/*----------------------------------------------------------------
  sclustZeroSurfaceClusterNo() - finds all the vertices with 
  cluster number equal to ClusterNo and sets the cluster number
  to zero (cluster number is the undefval member of the surface
  structure). Nothing is done to the surface value. This function
  is good for pruning clusters that do not meet some other
  criteria (eg, area threshold).
  ----------------------------------------------------------------*/
float sclustZeroSurfaceClusterNo(int ClusterNo, MRI_SURFACE *Surf)
{
  int vtx, vtx_clusterno;

  for(vtx = 0; vtx < Surf->nvertices; vtx++){
    vtx_clusterno = Surf->vertices[vtx].undefval;
    if(vtx_clusterno == ClusterNo) 
      Surf->vertices[vtx].undefval = 0;
  }

  return(0);
}
/*----------------------------------------------------------------
  sclustZeroSurfaceNonClusters() - zeros the value of all the vertices
  that are not assocated with a cluster. The cluster number is the
  undefval member of the surface structure.
  ----------------------------------------------------------------*/
float sclustZeroSurfaceNonClusters(MRI_SURFACE *Surf)
{
  int vtx, vtx_clusterno;

  for(vtx = 0; vtx < Surf->nvertices; vtx++){
    vtx_clusterno = Surf->vertices[vtx].undefval;
    if(vtx_clusterno == 0)  Surf->vertices[vtx].val = 0.0;
  }

  return(0);
}
/*----------------------------------------------------------------
  sclustSetSurfaceClusterToClusterNo() - sets the value of a vertex to the
  cluster number. The cluster number is the undefval member of the
  surface structure.
  ----------------------------------------------------------------*/
float sclustSetSurfaceValToClusterNo(MRI_SURFACE *Surf)
{
  int vtx, vtx_clusterno;

  for(vtx = 0; vtx < Surf->nvertices; vtx++){
    vtx_clusterno = Surf->vertices[vtx].undefval;
    Surf->vertices[vtx].val = vtx_clusterno;
  }

  return(0);
}
/*----------------------------------------------------------------
  sclustSetSurfaceClusterToCWP() - sets the value of a vertex to 
  -log10(cluster-wise pvalue).
  ----------------------------------------------------------------*/
float sclustSetSurfaceValToCWP(MRI_SURFACE *Surf, SCS *scs)
{
  int vtx, vtx_clusterno;
  float val;

  for(vtx = 0; vtx < Surf->nvertices; vtx++){
    vtx_clusterno = Surf->vertices[vtx].undefval;

    if(vtx_clusterno==0) val = 0;
    else val = -log10(scs[vtx_clusterno-1].pval_clusterwise) *
	   SIGN(scs[vtx_clusterno-1].maxval);
    Surf->vertices[vtx].val = val;
  }

  return(0);
}
/*----------------------------------------------------------------
  sclustCountClusters() - counts the number of clusters. Really
  just returns the largest cluster number, which will be the 
  number of clusters if there are no holes.
  ----------------------------------------------------------------*/
float sclustCountClusters(MRI_SURFACE *Surf)
{
  int vtx, vtx_clusterno, maxclusterno;

  maxclusterno = 0;
  for(vtx = 0; vtx < Surf->nvertices; vtx++){
    vtx_clusterno = Surf->vertices[vtx].undefval;
    if(maxclusterno < vtx_clusterno) maxclusterno = vtx_clusterno;
  }
  return(maxclusterno);
}


/*----------------------------------------------------------------*/
SCS *SurfClusterSummary(MRI_SURFACE *Surf, MATRIX *T, int *nClusters)
{
  int n;
  SURFCLUSTERSUM *scs;
  MATRIX *xyz, *xyzxfm;

  *nClusters = sclustCountClusters(Surf);
  if(*nClusters == 0) return(NULL);
  
  xyz    = MatrixAlloc(4,1,MATRIX_REAL);
  xyz->rptr[4][1] = 1;
  xyzxfm = MatrixAlloc(4,1,MATRIX_REAL);

  scs = (SCS *) calloc(*nClusters, sizeof(SCS));

  for(n = 0; n < *nClusters ; n++){
    scs[n].clusterno = n+1;
    scs[n].area   = sclustSurfaceArea(n+1, Surf, &scs[n].nmembers);
    scs[n].maxval = sclustSurfaceMax(n+1,  Surf, &scs[n].vtxmaxval);
    scs[n].x = Surf->vertices[scs[n].vtxmaxval].x;
    scs[n].y = Surf->vertices[scs[n].vtxmaxval].y;
    scs[n].z = Surf->vertices[scs[n].vtxmaxval].z;
    if(T != NULL){
      xyz->rptr[1][1] = scs[n].x;
      xyz->rptr[2][1] = scs[n].y;
      xyz->rptr[3][1] = scs[n].z;
      MatrixMultiply(T,xyz,xyzxfm);
      scs[n].xxfm = xyzxfm->rptr[1][1];
      scs[n].yxfm = xyzxfm->rptr[2][1];
      scs[n].zxfm = xyzxfm->rptr[3][1];
    }
  }

  MatrixFree(&xyz);
  MatrixFree(&xyzxfm);

  return(scs);
}
/*----------------------------------------------------------------*/
int DumpSurfClusterSum(FILE *fp, SCS *scs, int nClusters)
{
  int n;

  for(n=0; n < nClusters; n++){
    fprintf(fp,"%4d  %4d  %8.4f  %6d    %6.2f  %4d   %5.1f %5.1f %5.1f   "
      "%5.1f %5.1f %5.1f\n",n,scs[n].clusterno,
      scs[n].maxval, scs[n].vtxmaxval, 
      scs[n].area,scs[n].nmembers,
      scs[n].x, scs[n].y, scs[n].z,
      scs[n].xxfm, scs[n].yxfm, scs[n].zxfm);
  }
  return(0);
}
/*----------------------------------------------------------------*/
SCS *SortSurfClusterSum(SCS *scs, int nClusters)
{
  SCS *scs_sorted;
  int n;

  scs_sorted = (SCS *) calloc(nClusters, sizeof(SCS));

  for(n=0; n < nClusters; n++)
    memcpy(&scs_sorted[n],&scs[n],sizeof(SCS));

  /* Note: scs_sorted.clusterno does not changed */
  qsort((void *) scs_sorted, nClusters, sizeof(SCS), sclustCompare);

  return(scs_sorted);
}

/*----------------------------------------------------------------
  sclustReMap() - remaps the cluster numbers (ie, undefval) in the
  surface structure based on the sorted surface cluster summary
  (SCS). It is assumed that the scs.clusterno in the sorted SCS
  is the cluster id that corresponds to the original cluster id.
  ----------------------------------------------------------------*/
int sclustReMap(MRI_SURFACE *Surf, int nClusters, SCS *scs_sorted)
{
  int vtx, vtx_clusterno, c;

  if(Gdiag_no > 1){
    printf("sclustReMap:\n");
    for(c=1; c <= nClusters; c++)
      printf("new = %3d old = %3d\n",c,scs_sorted[c-1].clusterno);
  }

  for(vtx = 0; vtx < Surf->nvertices; vtx++){
    vtx_clusterno = Surf->vertices[vtx].undefval;
    for(c=1; c <= nClusters; c++){
      if(vtx_clusterno == scs_sorted[c-1].clusterno){
  Surf->vertices[vtx].undefval = c;
  break;
      }
    }
  }

  return(0);
}

/*----------------------------------------------------------------*/
/*--------------- STATIC FUNCTIONS BELOW HERE --------------------*/
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------
  sclustCompare() - compares two surface cluster summaries (for
  use with qsort().
  ----------------------------------------------------------------*/
static int sclustCompare(const void *a, const void *b)
{
  SCS sc1, sc2;

  sc1 = *((SURFCLUSTERSUM *)a);
  sc2 = *((SURFCLUSTERSUM *)b);

  if(fabs(sc1.maxval) > fabs(sc2.maxval) ) return(-1);
  if(fabs(sc1.maxval) < fabs(sc2.maxval) ) return(+1);

  if(sc1.area > sc2.area) return(-1);
  if(sc1.area < sc2.area) return(+1);

  return(0);
}

/*-------------------------------------------------------------------
  sclustMaxClusterArea() - returns the area of the cluster with the 
  maximum area. 
  -------------------------------------------------------------------*/
double sclustMaxClusterArea(SURFCLUSTERSUM *scs, int nClusters)
{
  int n;
  double maxarea;

  if(nClusters==0) return(0);

  maxarea = scs[0].area;
  for(n=0; n<nClusters; n++)
    if(maxarea < scs[n].area) maxarea = scs[n].area;
  return(maxarea);
}
