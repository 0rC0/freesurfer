/**
 * @file  autoencoder.c
 * @brief header file for creating and training a stacked autoencoder for feature extraction.
 *
H.-C. Shin, M. R. Orton, D. J. Collins, S. J. Doran, and M. O. Leach,
"Stacked Autoencoders for
Unsupervised Feature Learning and Multiple Organ Detectionin a Pilot Study
Using 4D Patient Data,"
IEEE Transaction on Pattern Analysis and Machine Intelligence, 2012.

 */
/*
 * Original Author: Bruce Fischl
 * CVS Revision Info:
 *    $Author: fischl $
 *    $Date: 2013/11/03 19:56:01 $
 *    $Revision: 1.1 $
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
#include <stdio.h>
#include "error.h"
#include "diag.h"
#include "utils.h"
#include "autoencoder.h"
#include "macros.h"


static int aeApplyGradient(AE *ae, SAE_INTEGRATION_PARMS *parms, double dt) ;
static double AEcomputeRMS(AE *ae) ;
static int AEsaveState(AE *ae) ;
static int AErestoreState(AE *ae) ;
static AE *AEalloc(AE *prev, int ninputs, int nhidden) ;
static AE *SAEfindLastLayer(SAE *sae, AE *ae) ;
static int AEactivateLayer(AE *ae, VECTOR *v_input) ;
static double AEtrain(AE *ae, SAE_INTEGRATION_PARMS *parms);
static int  AEwrite(AE *ae, FILE *fp)  ;
static AE *AEread(FILE *fp, AE *prev)  ;

static AE *
SAEfindLastLayer(SAE *sae, AE *ae)
{
  if (ae == NULL)
    return(SAEfindLastLayer(sae, sae->first)) ;
  else if (ae->next)
    return(SAEfindLastLayer(sae, ae->next)) ;
  else
    return(ae) ;
}


SAE *
SAEalloc(int whalf, double scale)
{
  SAE *sae ;
  int ninputs, wsize, nhidden ;

  wsize = 2*whalf+1 ; 
  ninputs = wsize*wsize*wsize ;
  nhidden = nint(scale*ninputs) ;
  printf("allocating SAE with inputs/hidden %d/%d\n", ninputs, nhidden) ;

  sae = (SAE *)calloc(1, sizeof(SAE)) ;
  if (sae == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEalloc(%s, %d): could not alloc sae", whalf)) ;
  sae->whalf = whalf ;
  sae->nencoders = 1 ;
  sae->first = AEalloc(NULL, ninputs, nhidden) ;
  sae->scale = scale ;
  if (whalf == 0)
  {
    sae->first->v_input->rptr[1][1] = 0 ;
    SAEactivateNetwork(sae) ;
    AEdump(sae->first) ;
    sae->first->v_input->rptr[1][1] = 1 ;
    SAEactivateNetwork(sae) ;
    AEdump(sae->first) ;
  }
  return(sae) ;
}

static AE *
AEalloc(AE *prev, int ninputs, int nhidden)
{
  AE     *ae ;
  int    i, j, k ;
  double norm, w ;


  ae = (AE *)calloc(1, sizeof(AE)) ;
  if (ae == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc ae", ninputs, nhidden)) ;


  // the input and ouptut layers for an AE is the hidden layer of the previous AE
  if (prev)
  {
    ae->v_input = prev->v_hidden ;
    ae->v_output = prev->v_hidden ;
  }
  else
  {
    ae->v_input = VectorAlloc(ninputs, MATRIX_REAL) ;
    ae->v_output = VectorAlloc(ninputs, MATRIX_REAL) ;
  }

  if (ae->v_input == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc v_input", ninputs, nhidden)) ;

  ae->m_input_to_hidden = MatrixAlloc(nhidden, ninputs, MATRIX_REAL) ;
  if (ae->m_input_to_hidden == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc m_input_to_hidden", ninputs, nhidden)) ;
  ae->m_hidden_to_output = MatrixAlloc(ninputs, nhidden, MATRIX_REAL) ;
  if (ae->m_hidden_to_output == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc m_hidden_output_to", ninputs, nhidden));
  ae->v_output = VectorAlloc(ninputs, MATRIX_REAL) ;
  if (ae->v_output == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc v_output", ninputs, nhidden)) ;
  ae->v_hidden_bias = VectorAlloc(nhidden, MATRIX_REAL) ;
  if (ae->v_hidden_bias == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc v_hidden_bias", ninputs, nhidden)) ;
  ae->v_hidden_net = VectorAlloc(nhidden, MATRIX_REAL) ;
  if (ae->v_hidden_net == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc v_hidden_net", ninputs, nhidden)) ;
  ae->v_hidden = VectorAlloc(nhidden, MATRIX_REAL) ;
  if (ae->v_hidden == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc v_hidden", ninputs, nhidden)) ;
  ae->v_output_bias = VectorAlloc(ninputs, MATRIX_REAL) ;
  if (ae->v_output_bias == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "AEaalloc(%d, %d): could not alloc v_output_bias", ninputs, nhidden)) ;
  ae->prev = prev ;
  if (ae->prev)
    ae->prev->next = ae ;
  ae->next = NULL ;

  for (norm = 0.0, i = 1 ; i <= ninputs ; i++)
    for (j = 1 ; j <= nhidden ; j++)
    {
      w = randomNumber(-1, 1) ;
      norm += (w*w) ;
      *MATRIX_RELT(ae->m_input_to_hidden, j, i) = w ;
    }
  norm = sqrt(norm) ;
  for (i = 1 ; i <= ninputs ; i++)
    for (j = 1 ; j <= nhidden ; j++)
      *MATRIX_RELT(ae->m_input_to_hidden, j, i) /= norm ;

  for (norm = 0.0, j = 1 ; j <= nhidden ; j++)
    for (k = 1 ; k <= ninputs ; k++)
    {
      w = randomNumber(-1, 1) ;
      norm += (w*w) ;
      *MATRIX_RELT(ae->m_hidden_to_output, k, j) = w ;
    }
  norm = sqrt(norm) ;
  for (j = 1 ; j <= nhidden ; j++)
    for (k = 1 ; k <= ninputs ; k++)
      *MATRIX_RELT(ae->m_hidden_to_output, k, j) /= norm ;

  for (norm = 0.0, j = 1 ; j <= nhidden ; j++)
  {
    w = randomNumber(-1, 1) ;
    norm += (w*w) ;
    VECTOR_ELT(ae->v_hidden_bias, j) = w ;
  }
  norm = sqrt(norm) ;
  for (j = 1 ; j <= nhidden ; j++)
    VECTOR_ELT(ae->v_hidden_bias, j) /= norm ;

  for (norm = 0.0, k = 1 ; k <= ninputs ; k++)
  {
    w = randomNumber(-1, 1) ;
    norm += (w*w) ;
    VECTOR_ELT(ae->v_output_bias, k) = w ;
  }
  norm = sqrt(norm) ;
  for (k = 1 ; k <= ninputs ; k++)
    VECTOR_ELT(ae->v_output_bias, k) /= norm ;

  if (ninputs == 1)  // diags - set weights to correct values
  {
    double f0, f1 ;

    f0 = SIGMOID(0) ; f1 = SIGMOID(1) ;
//    ae->v_hidden_bias->rptr[1][1] = 0 ;
//    ae->m_input_to_hidden->rptr[1][1] = 1 ;
//    ae->m_hidden_to_output->rptr[1][1] = 1 / (f1-f0) ;
//    ae->v_output_bias->rptr[1][1] = 0 - (1 / (f1-f0)) * f0 ;
  }

  return(ae) ;
}

static void 
AEfree(AE **pae)
{
  AE *ae = *pae ;

  *pae = NULL ;
  MatrixFree(&ae->m_input_to_hidden) ;
  MatrixFree(&ae->m_hidden_to_output) ;
  VectorFree(&ae->v_output_bias) ;
  VectorFree(&ae->v_hidden_bias) ;
  VectorFree(&ae->v_hidden) ;
  VectorFree(&ae->v_hidden_net) ;
  if (ae->v_error)
    VectorFree(&ae->v_error) ;
  if (ae->prev == NULL)
  {
    VectorFree(&ae->v_input) ;
    VectorFree(&ae->v_output) ;
  }
  if (ae->m_grad_input_to_hidden)   // free training stuff
  {
    VectorFree(&ae->v_grad_hidden_bias) ;
    VectorFree(&ae->v_grad_output_bias) ;
    VectorFree(&ae->v_previous_step_hidden_bias) ;
    VectorFree(&ae->v_previous_step_output_bias) ;

    MatrixFree(&ae->m_grad_input_to_hidden) ;
    MatrixFree(&ae->m_grad_hidden_to_output) ;
    MatrixFree(&ae->m_previous_step_input_to_hidden) ;
    MatrixFree(&ae->m_previous_step_hidden_to_output) ;
  }
}


void
SAEfree(SAE **psae) 
{
  SAE *sae = *psae ;
  AE  *ae, *next ;

  *psae = NULL ;

  ae = sae->first ;
  do
  {
    next = ae->next ;
    AEfree(&ae) ;
    ae = next ;
  } while(ae) ;

  free(sae) ;
}
AE *
SAEaddLayer(SAE *sae, float scale)
{
  AE  *ae, *last ;


  last = SAEfindLastLayer(sae, sae->first) ;
  ae =  AEalloc(last, last->v_input->rows, last->v_input->rows/2) ;
  return(ae) ;
}
 
SAE  *
SAEtrainLayer(SAE *sae, AE *layer, MRI *mri, double tol) 
{
  int   x, y, z ;

  for (x = 0 ; x < mri->width ; x++)
    for (y = 0 ; y < mri->height ; y++)
      for (z = 0 ; z < mri->depth ; z++)
      {
	SAEfillInputVector(mri, x, y, z, sae->whalf, sae->first->v_input) ;
	SAEactivateNetwork(sae) ;
      }
  return(sae) ;
}
double
SAEcomputeTotalRMS(SAE *sae, MRI *mri)
{
  int     x, y, z, nvox ;
  double  rms, total_rms ;

  total_rms = 0.0 ; nvox = 0 ;
  for (x = 0 ; x < mri->width ; x++)
    for (y = 0 ; y < mri->height ; y++)
      for (z = 0 ; z < mri->depth ; z++)
      {
	if (FZERO(MRIgetVoxVal(mri, x, y, z, 0)))
	  continue ;
	SAEfillInputVector(mri, x, y, z, sae->whalf, sae->first->v_input) ;
	SAEactivateNetwork(sae) ;
	rms = SAEcomputeRMS(sae) ;
	if (!devFinite(rms))
	  DiagBreak() ;
	if (rms > 1000)
	  DiagBreak() ;
	total_rms += rms ;
	nvox++ ;
      }
    return(total_rms/nvox) ;
}

VECTOR *
SAEactivateNetwork(SAE *sae)
{
  AE     *ae ;

  ae = sae->first ;
  AEactivateLayer(sae->first, sae->first->v_input) ;
  return(sae->first->v_output) ;
}

static int
AEactivateLayer(AE *ae, VECTOR *v_input)
{
  int    row ;
  double o,net ;

  MatrixCopy(v_input, ae->v_input) ;
  MatrixMultiply(ae->m_input_to_hidden, ae->v_input, ae->v_hidden_net) ;
  MatrixAdd(ae->v_hidden_net, ae->v_hidden_bias, ae->v_hidden_net) ;
  for (row = 1 ; row <= ae->v_hidden->rows ; row++)   // apply hidden layer nonlinearity
  {
    net = VECTOR_ELT(ae->v_hidden_net, row) ;
    if (!devFinite(net))
      DiagBreak() ;
    o = SIGMOID(net) ;
    if (!devFinite(o))
      DiagBreak() ;
    VECTOR_ELT(ae->v_hidden, row) = o ;
  }
  if (ae->next)
  {
    AEactivateLayer(ae->next, ae->v_hidden) ;       // recurse - this hidden unit is next layers input
    MatrixCopy(ae->next->v_output, ae->v_hidden) ;  // output of next layer is hidden activation of this one since it autoencodes
  }

  MatrixMultiply(ae->m_hidden_to_output, ae->v_hidden, ae->v_output) ;
  MatrixAdd(ae->v_output, ae->v_output_bias, ae->v_output) ;
  return(NO_ERROR) ;
}

VECTOR *
SAEactivateLastHiddenLayer(SAE *sae, MRI *mri) 
{
  return(sae->first->v_output) ;
}

double
SAEcomputeRMS(SAE *sae)
{
  VECTOR *v_output ;
  int    row ;
  double error, rms  ;

  v_output = SAEactivateNetwork(sae) ;

  for (rms = 0.0, row = 1 ; row <= sae->first->v_input->rows ; row++)
  {
    error = VECTOR_ELT(sae->first->v_input, row) - VECTOR_ELT(sae->first->v_output, row) ;
    rms += error*error ;
  }
  return(sqrt(rms/row)) ;
}
static double
AEcomputeRMS(AE *ae)
{
  int    row ;
  double error, rms  ;

  for (rms = 0.0, row = 1 ; row <= ae->v_input->rows ; row++)
  {
    error = VECTOR_ELT(ae->v_input, row) - VECTOR_ELT(ae->v_output, row) ;
    rms += error*error ;
  }
  return(sqrt(rms/row)) ;
}



// NOTE: input MRI must be type float and scaled to be 0->1 (that is, scale down MRI_UCHAR vols by 255)
double
SAEtrainFromMRI(SAE *sae, MRI *mri, SAE_INTEGRATION_PARMS *parms)
{
  double error = 0.0, rms, last_rms, total_rms, last_total_rms, pct_decrease, running_last_rms, G_rms, G_last_rms = 1e10 ;
  int    x, y, z, iter = 0, visited, ind, nvox, calls = 0 ;
  short  *x_ind, *y_ind, *z_ind ;
  double tol ; 
  double dt, acceptance_sigma, proposal_sigma ;
  char *out_fname ;

  acceptance_sigma = parms->acceptance_sigma ;
  proposal_sigma = parms->proposal_sigma ;
  tol = parms->tol ; dt = parms->dt ; out_fname = parms->out_fname ;

  if (mri->type != MRI_FLOAT)
    ErrorExit(ERROR_BADPARM, "SAEtrainFromMRI: input type must be MRI_FLOAT scaled to [0->1]") ;
  nvox = mri->width * mri->height * mri->depth ;

  x_ind = (short *)calloc(nvox, sizeof(short)) ; y_ind = (short *)calloc(nvox, sizeof(short)) ; z_ind = (short *)calloc(nvox, sizeof(short)) ;
  if (!x_ind || !y_ind || !z_ind)
    ErrorExit(ERROR_NOMEMORY, "SAEtrainFromMRI: could not allocate permutation indices") ;

  last_total_rms = 0 ;
  last_total_rms = SAEcomputeTotalRMS(sae, mri) ;
  printf("%3.3d: rms = %2.4f\n", iter, last_total_rms) ;
  if (Gx >= 0)
  {
    int wsize, ind ;
    wsize = sae->whalf*2+1 ; ind = (wsize*wsize*wsize)/2 + 1 ;
    SAEfillInputVector(mri, Gx, Gy, Gz, sae->whalf, sae->first->v_input) ;
    SAEactivateNetwork(sae) ;
    G_last_rms = SAEcomputeRMS(sae) ;
    printf("voxel (%d, %d, %d), I = %2.2f, o = %2.2f, rms = %2.4f\n", Gx, Gy, Gz, 
	   sae->first->v_input->rptr[ind][1],
	   sae->first->v_output->rptr[ind][1],
	   G_last_rms) ;
  }
  do
  {
    MRIcomputeVoxelPermutation(mri, x_ind, y_ind, z_ind);
    for (running_last_rms = total_rms = 0.0, visited = ind = 0 ; ind < nvox ; ind++)
    {
      if (ind && !(ind % MAX((nvox/500),2)))
      {
	double running_pct_dec ;
	running_pct_dec = 100 * (running_last_rms/visited - total_rms/visited) / (running_last_rms/visited + total_rms/visited) ;
	pct_decrease = 100 * (last_total_rms - total_rms/visited) / (last_total_rms + total_rms/visited) ;
	printf("%d of %d (%2.2f%%): rms = %2.4f (%2.3f%%) (%2.3f%%)\n", ind, nvox, 100*ind/(float)nvox, total_rms/visited, pct_decrease,running_pct_dec) ;
      }
      x = x_ind[ind] ;  y = y_ind[ind] ; z = z_ind[ind] ; 
      if (x < sae->whalf || y < sae->whalf || z < sae->whalf ||
	  x >= mri->width-sae->whalf || y >= mri->height-sae->whalf || z >= mri->depth-sae->whalf)
	continue ;
      if (!FZERO(MRIgetVoxVal(mri, x, y, z, 0)))   // there is real data at this spot
      {
	visited++ ;
	if (++calls == Gdiag_no)
	  DiagBreak() ;
	SAEfillInputVector(mri, x, y, z, sae->whalf, sae->first->v_input) ;
	SAEactivateNetwork(sae) ;
	last_rms = SAEcomputeRMS(sae) ;
	running_last_rms += last_rms ;
	AEtrain(sae->first, parms) ;
	SAEactivateNetwork(sae) ;
	rms = SAEcomputeRMS(sae) ;
	total_rms += rms ;
	if (rms > last_rms)
	  DiagBreak() ;
	if (Gx >= 0)
	{
	  int wsize, ind ;
	  float in, out ;
	  wsize = sae->whalf*2+1 ; ind = (wsize*wsize*wsize)/2 + 1 ;

	  SAEfillInputVector(mri, Gx, Gy, Gz, sae->whalf, sae->first->v_input) ;
	  SAEactivateNetwork(sae) ;
	  G_rms = SAEcomputeRMS(sae) ;
	  if (G_rms > G_last_rms)
	    DiagBreak() ;
	  G_last_rms = G_rms ;
	  in = sae->first->v_input->rptr[ind][1] ;
	  out = sae->first->v_output->rptr[ind][1] ;
	  DiagBreak() ;
	}
      }
      if (out_fname && ind && !(ind % MAX((nvox/50),2)))
      {
	char   fname[STRLEN], path[STRLEN] ;

	if (sae->first->v_input->rows != 1 || !(ind % MAX((nvox/500),2)))
	{
	  FileNameRemoveExtension(out_fname, path) ;
	  sprintf(fname, "%s.%2.2d.ae", path, iter) ;
	  printf("writing SAE after %dth iteration to %s\n", iter, fname) ;
	  SAEwrite(sae, fname) ;
	}
      }
    }
    total_rms = SAEcomputeTotalRMS(sae, mri) ;
    pct_decrease = 100 * (last_total_rms - total_rms) / (last_total_rms + total_rms) ;
    last_total_rms = total_rms ;
    printf("%3.3d: rms = %2.4f (%2.3f%%)\n", ++iter, total_rms, pct_decrease) ;
    if (out_fname)
    {
      char   fname[STRLEN], path[STRLEN] ;
      FileNameRemoveExtension(out_fname, path) ;
      sprintf(fname, "%s.%2.2d.ae", path, iter) ;
      printf("writing SAE after %dth iteration to %s\n", iter, fname) ;
      SAEwrite(sae, fname) ;
    }
    if (parms->integration_type == INTEGRATE_BOLTZMANN_MACHINE)
    {
      if (iter < 30)
	pct_decrease = 2*tol ;
      parms->acceptance_sigma *= .8 ;
      parms->proposal_sigma *= .8 ;
      printf("setting acceptance/proposal sigma to %2.3f/%2.3f\n", parms->acceptance_sigma, parms->proposal_sigma) ;
    }
  } while (pct_decrease > tol) ;

  if (Gx >= 0)
  {
    int wsize, ind, i, j ;
    float in, out, total_rms, init_total_rms ;
    wsize = sae->whalf*2+1 ; ind = (wsize*wsize*wsize)/2 + 1 ;
    
    init_total_rms = SAEcomputeTotalRMS(sae, mri) ;
    for (j = 0 ; j < 10 ; j++)
    {
      total_rms = SAEcomputeTotalRMS(sae, mri) ;
      SAEfillInputVector(mri, Gx, Gy, Gz, sae->whalf, sae->first->v_input) ;
      SAEactivateNetwork(sae) ;
      last_rms = SAEcomputeRMS(sae) ;

      for (i = 0 ; i < 100 ; i++)
      {
	AEtrain(sae->first, parms) ;
	SAEactivateNetwork(sae) ;
	rms = SAEcomputeRMS(sae) ;
      }
    }

    G_rms = SAEcomputeRMS(sae) ;
    if (G_rms > G_last_rms)
      DiagBreak() ;
    G_last_rms = G_rms ;
    in = sae->first->v_input->rptr[ind][1] ;
    out = sae->first->v_output->rptr[ind][1] ;
    DiagBreak() ;
  }

  parms->acceptance_sigma = acceptance_sigma ;
  parms->proposal_sigma = proposal_sigma ;
  free(x_ind) ; free(y_ind) ; free(z_ind) ;
  return(error) ;
}
/*

 Simplify[D[1 / (1+Exp[-(w*x+b)]),w]]

           b + w x
          E        x
Out[7]= ---------------
              b + w x 2
        (1 + E       )
 Simplify[D[1 / (1+Exp[-(w*x+b)]),b]]

            b + w x
           E
Out[8]= ---------------
              b + w x 2
        (1 + E       )


*/

static double
AEtrain(AE *ae, SAE_INTEGRATION_PARMS *parms) 
{
  double rms = 0, error, dt, momentum, Egrad, Erandom ;
  int    i, j, k, ninputs, nhidden, noutputs ;

  dt = parms->orig_dt ; momentum = parms->momentum ;
  ninputs = ae->v_input->rows ; noutputs = ae->v_output->rows ; nhidden = ae->v_hidden->rows ;
  if (ae->v_error == NULL)
  {
    ae->v_error = VectorClone(ae->v_output) ;
    ae->m_grad_input_to_hidden = MatrixClone(ae->m_input_to_hidden) ;
    ae->m_grad_hidden_to_output = MatrixClone(ae->m_hidden_to_output) ;
    ae->v_grad_hidden_bias = VectorClone(ae->v_hidden) ;
    ae->v_grad_output_bias = VectorClone(ae->v_output) ;

    ae->m_previous_step_input_to_hidden = MatrixClone(ae->m_input_to_hidden) ;
    ae->m_previous_step_hidden_to_output = MatrixClone(ae->m_hidden_to_output) ;
    ae->v_previous_step_hidden_bias = VectorClone(ae->v_hidden) ;
    ae->v_previous_step_output_bias = VectorClone(ae->v_output) ;
  }
  for (k = 1 ; k <= ae->v_input->rows ; k++)
  {
    error = VECTOR_ELT(ae->v_error, k) =  VECTOR_ELT(ae->v_output, k) - VECTOR_ELT(ae->v_input, k) ;
    rms += error*error ;
  }


  // NOTE: compute all GRADIENTS first. Will multiply by -dt at the end to get right direction of update

  // update compute output weights grad
#ifdef HAVE_OPENMP
#pragma omp parallel for firstprivate(noutputs, nhidden, dt) shared(ae) schedule(static,1) 
#endif
  for (k = 1 ; k <= noutputs ; k++)
  {
    double error, hidden ;
    int    j ;

    error = VECTOR_ELT(ae->v_error, k) ;
    VECTOR_ELT(ae->v_grad_output_bias, k) = error ;
    if (!devFinite(VECTOR_ELT(ae->v_grad_output_bias, k)))
      DiagBreak() ;
    for (j = 1 ; j <= nhidden ; j++)
    {
      hidden = VECTOR_ELT(ae->v_hidden, j) ;
      *MATRIX_RELT(ae->m_grad_hidden_to_output, k, j) = error * hidden ;  // *fprime fprime?
      if (!devFinite(*MATRIX_RELT(ae->m_grad_hidden_to_output, k, j)))
	DiagBreak() ;
    }
  }

  // compute hidden bias grad
#ifdef HAVE_OPENMP
#pragma omp parallel for firstprivate(noutputs, nhidden, dt) shared(ae) schedule(static,1) 
#endif
  for (j = 1 ; j <= nhidden ; j++)
  {
    double dE_dbj, net, fprime, error, wjk, o ;
    int    k ;

    net = VECTOR_ELT(ae->v_hidden_net, j) ;
    o = SIGMOID(net) ;
    fprime = D_SIGMOID(o) ;
    if (!devFinite(fprime) || !devFinite(o))
      DiagBreak() ;
    for (dE_dbj = 0.0, k = 1 ; k <= noutputs ; k++)
    {
      error = VECTOR_ELT(ae->v_error, k) ;
      wjk = *MATRIX_RELT(ae->m_hidden_to_output, k, j) ;
      dE_dbj += error * wjk * fprime ;
      if (!devFinite(dE_dbj))
	DiagBreak() ;
    }
    VECTOR_ELT(ae->v_grad_hidden_bias, j) = dE_dbj ;
    if (!devFinite(VECTOR_ELT(ae->v_grad_hidden_bias, j)))
	DiagBreak() ;
  }

  // compute hidden weight grad
#ifdef HAVE_OPENMP
#pragma omp parallel for firstprivate(noutputs, nhidden, dt) shared(ae) schedule(static,1) 
#endif
  for (i = 1 ; i <= noutputs ; i++)
  {
    int    j ;
    double Ii ;

    Ii = VECTOR_ELT(ae->v_input, i) ;
    for (j = 1 ; j <= nhidden ; j++)
    { 
      double netj, fprimej, dE_dwij, wjk, o ;
      int    k  ;

      netj = VECTOR_ELT(ae->v_hidden_net, j) ;
      o = SIGMOID(netj) ;
      fprimej = D_SIGMOID(o) ;
      if (!devFinite(fprimej) || !devFinite(o))
	DiagBreak() ;

      for (dE_dwij = 0.0, k = 1 ; k <= noutputs ; k++)
      {
	error = VECTOR_ELT(ae->v_error, k) ;
	wjk = *MATRIX_RELT(ae->m_hidden_to_output, k, j) ;
	dE_dwij += error * wjk ;
      }
      dE_dwij *= Ii*fprimej ;
      if (dE_dwij < 0)
	DiagBreak() ;
      *MATRIX_RELT(ae->m_grad_input_to_hidden, j, i) = dE_dwij ;
      if (!devFinite(*MATRIX_RELT(ae->m_grad_input_to_hidden, j, i)))
	DiagBreak() ;
    }
  }

  if (parms->integration_type != INTEGRATE_CONJUGATE_GRADIENT)  // don't scale grads by -dt for conjugate gradient
  {
    MatrixScalarMul(ae->v_grad_output_bias, -dt, ae->v_grad_output_bias) ; 
    MatrixScalarMul(ae->v_grad_hidden_bias, -dt, ae->v_grad_hidden_bias) ; 
    MatrixScalarMul(ae->m_grad_hidden_to_output, -dt, ae->m_grad_hidden_to_output) ; 
    MatrixScalarMul(ae->m_grad_input_to_hidden, -dt, ae->m_grad_input_to_hidden) ; 
  }
  if (!FZERO(momentum))   // use momentum
  {

    MatrixScalarMul(ae->v_previous_step_output_bias, momentum, ae->v_previous_step_output_bias) ;
    MatrixAdd(ae->v_grad_output_bias, ae->v_previous_step_output_bias, ae->v_grad_output_bias) ;

    MatrixScalarMul(ae->v_previous_step_hidden_bias, momentum, ae->v_previous_step_hidden_bias) ;
    MatrixAdd(ae->v_grad_hidden_bias, ae->v_previous_step_hidden_bias, ae->v_grad_hidden_bias) ;

    MatrixScalarMul(ae->m_previous_step_input_to_hidden, momentum, ae->m_previous_step_input_to_hidden) ;
    MatrixAdd(ae->m_grad_input_to_hidden, ae->m_previous_step_input_to_hidden, ae->m_grad_input_to_hidden) ;

    MatrixScalarMul(ae->m_previous_step_hidden_to_output, momentum, ae->m_previous_step_hidden_to_output) ;
    MatrixAdd(ae->m_grad_hidden_to_output, ae->m_previous_step_hidden_to_output, ae->m_grad_hidden_to_output) ;
  }

  AEsaveState(ae) ;
  if (parms->integration_type == INTEGRATE_GRADIENT_DESCENT)
  {
    MatrixAdd(ae->v_grad_output_bias, ae->v_output_bias, ae->v_output_bias) ;
    MatrixAdd(ae->v_grad_hidden_bias, ae->v_hidden_bias, ae->v_hidden_bias) ;
    MatrixAdd(ae->m_grad_hidden_to_output, ae->m_hidden_to_output, ae->m_hidden_to_output) ;
    MatrixAdd(ae->m_grad_input_to_hidden, ae->m_input_to_hidden, ae->m_input_to_hidden) ;
    
    MatrixCopy(ae->v_grad_output_bias, ae->v_previous_step_output_bias) ;
    MatrixCopy(ae->v_grad_hidden_bias, ae->v_previous_step_hidden_bias) ;
    MatrixCopy(ae->m_grad_hidden_to_output, ae->m_previous_step_hidden_to_output) ;
    MatrixCopy(ae->m_grad_input_to_hidden, ae->m_previous_step_input_to_hidden) ;
    rms = sqrt(rms / ninputs) ;
  }
  else if (parms->integration_type == INTEGRATE_CONJUGATE_GRADIENT)
  {
    double   beta, best_dt, orig_dt, rms, best_rms ;
    static int callno = 0 ;

    if (++callno == Gdiag_no)
      DiagBreak() ;

    if (parms->v_dir_hidden_bias == NULL)   // allocate everything the first time
    {
      parms->v_prev_grad_change_hidden_bias  = MatrixClone(ae->v_hidden_bias) ;
      parms->v_prev_grad_change_output_bias  = MatrixClone(ae->v_output_bias) ;
      parms->m_prev_grad_change_input_to_hidden  = MatrixClone(ae->m_input_to_hidden) ;
      parms->m_prev_grad_change_hidden_to_output  = MatrixClone(ae->m_hidden_to_output) ;
      parms->v_prev_grad_hidden_bias  = MatrixClone(ae->v_hidden_bias) ;
      parms->v_prev_grad_output_bias  = MatrixClone(ae->v_output_bias) ;
      parms->m_prev_grad_input_to_hidden  = MatrixClone(ae->m_input_to_hidden) ;
      parms->m_prev_grad_hidden_to_output  = MatrixClone(ae->m_hidden_to_output) ;
      parms->v_dir_hidden_bias  = MatrixClone(ae->v_hidden_bias) ;
      parms->v_dir_output_bias  = MatrixClone(ae->v_output_bias) ;
      parms->m_dir_input_to_hidden  = MatrixClone(ae->m_input_to_hidden) ;
      parms->m_dir_hidden_to_output  = MatrixClone(ae->m_hidden_to_output) ;
      parms->norm_hidden_bias = parms->norm_output_bias = parms->norm_hidden_to_output = parms->norm_input_to_hidden = 1.0 ;
    }

    if (DZERO(parms->norm_hidden_bias) || !devFinite(parms->norm_hidden_bias))
      DiagBreak() ;
    beta = VectorDot(parms->v_prev_grad_change_hidden_bias, parms->v_prev_grad_hidden_bias) / parms->norm_hidden_bias ;
    if (!devFinite(beta))
      DiagBreak() ;
    MatrixScalarMul(parms->v_dir_hidden_bias, beta, parms->v_dir_hidden_bias) ;
    MatrixSubtract(parms->v_dir_hidden_bias, ae->v_grad_hidden_bias, parms->v_dir_hidden_bias) ;
//    MatrixAdd(parms->v_dir_hidden_bias, ae->v_hidden_bias, ae->v_hidden_bias) ;

    AEsaveState(ae) ;
    best_dt = 0 ; best_rms = AEcomputeRMS(ae) ;
    for (orig_dt = dt, dt = dt*.1 ; dt <= 1000 ; dt *= 2 )
    {
      aeApplyGradient(ae, parms, dt) ;
      AEactivateLayer(ae, ae->v_input) ;
      rms = AEcomputeRMS(ae) ;
      if (rms < best_rms)
      {
	best_rms = rms ;
	best_dt = dt ;
      }
      AErestoreState(ae) ;
    }
    aeApplyGradient(ae, parms, best_dt) ;
    parms->dt = best_dt ;
#if 0
    parms->v_prev_grad_change_hidden_bias = VectorSubtract(ae->v_grad_hidden_bias, parms->v_prev_grad_hidden_bias, parms->v_prev_grad_change_hidden_bias);
    VectorCopy(ae->v_grad_hidden_bias, parms->v_prev_grad_hidden_bias) ;
    parms->norm_hidden_bias = VectorLen(parms->v_prev_grad_hidden_bias) ; parms->norm_hidden_bias *= parms->norm_hidden_bias ;
#endif
  }
  else if (parms->integration_type == INTEGRATE_BOLTZMANN_MACHINE)
  {
    static MATRIX *m_hidden_to_output_delta = NULL, *m_input_to_hidden_delta = NULL, *v_hidden_bias_delta = NULL, *v_output_bias_delta = NULL ;
    double acceptance_val ;

    MatrixAdd(ae->v_grad_output_bias, ae->v_output_bias, ae->v_output_bias) ;
    MatrixAdd(ae->v_grad_hidden_bias, ae->v_hidden_bias, ae->v_hidden_bias) ;
    MatrixAdd(ae->m_grad_hidden_to_output, ae->m_hidden_to_output, ae->m_hidden_to_output) ;
    MatrixAdd(ae->m_grad_input_to_hidden, ae->m_input_to_hidden, ae->m_input_to_hidden) ;
    
    AEactivateLayer(ae, ae->v_input) ;
    Egrad = AEcomputeRMS(ae) ;
    AErestoreState(ae) ;          // undo gradient steop
    m_hidden_to_output_delta = MatrixDRand48ZeroMean(ae->m_hidden_to_output->rows, ae->m_hidden_to_output->cols, m_hidden_to_output_delta);
    m_input_to_hidden_delta = MatrixDRand48ZeroMean(ae->m_input_to_hidden->rows, ae->m_input_to_hidden->cols, m_input_to_hidden_delta);
    v_hidden_bias_delta = MatrixDRand48ZeroMean(ae->v_hidden_bias->rows, ae->v_hidden_bias->cols, v_hidden_bias_delta);
    v_output_bias_delta = MatrixDRand48ZeroMean(ae->v_output_bias->rows, ae->v_output_bias->cols, v_output_bias_delta);
    
    MatrixScalarMul(m_hidden_to_output_delta, dt*parms->proposal_sigma, m_hidden_to_output_delta) ;
    MatrixScalarMul(m_input_to_hidden_delta, dt*parms->proposal_sigma, m_input_to_hidden_delta) ;
    MatrixScalarMul(v_hidden_bias_delta, dt*parms->proposal_sigma, v_hidden_bias_delta) ;
    MatrixScalarMul(v_output_bias_delta, dt*parms->proposal_sigma, v_output_bias_delta) ;

    MatrixAdd(v_output_bias_delta, ae->v_output_bias, ae->v_output_bias) ;
    MatrixAdd(v_hidden_bias_delta, ae->v_hidden_bias, ae->v_hidden_bias) ;
    MatrixAdd(m_hidden_to_output_delta, ae->m_hidden_to_output, ae->m_hidden_to_output) ;
    MatrixAdd(m_input_to_hidden_delta, ae->m_input_to_hidden, ae->m_input_to_hidden) ;
    AEactivateLayer(ae, ae->v_input) ;
    Erandom = AEcomputeRMS(ae) ;
    acceptance_val = exp((Egrad-Erandom)/parms->acceptance_sigma) ;
    if (randomNumber(0.0, 1.0) > acceptance_val) // take gradient step
    {
      AErestoreState(ae) ;          // undo gradient steop
      MatrixAdd(ae->v_grad_output_bias, ae->v_output_bias, ae->v_output_bias) ;
      MatrixAdd(ae->v_grad_hidden_bias, ae->v_hidden_bias, ae->v_hidden_bias) ;
      MatrixAdd(ae->m_grad_hidden_to_output, ae->m_hidden_to_output, ae->m_hidden_to_output) ;
      MatrixAdd(ae->m_grad_input_to_hidden, ae->m_input_to_hidden, ae->m_input_to_hidden) ;
    }
    else
      DiagBreak() ;
  }

  return(rms) ;
}

static int
aeApplyGradient(AE *ae, SAE_INTEGRATION_PARMS *parms, double dt)
{
  static MATRIX *m_grad_hidden_to_output, *m_grad_input_to_hidden ;
  static VECTOR *v_grad_hidden_bias, *v_grad_output_bias ;

  if (v_grad_output_bias == NULL)
  {
    v_grad_output_bias = VectorCopy(ae->v_grad_output_bias, NULL) ;
    v_grad_hidden_bias = VectorCopy(ae->v_grad_hidden_bias, NULL) ;
    m_grad_hidden_to_output = MatrixCopy(ae->m_grad_hidden_to_output, NULL) ;
    m_grad_input_to_hidden = MatrixCopy(ae->m_grad_input_to_hidden, NULL) ;
  }


  v_grad_output_bias = MatrixScalarMul(parms->v_dir_output_bias, dt, v_grad_output_bias) ; 
  v_grad_hidden_bias = MatrixScalarMul(parms->v_dir_hidden_bias, dt, v_grad_hidden_bias) ; 
  m_grad_hidden_to_output = MatrixScalarMul(parms->m_dir_hidden_to_output, dt, m_grad_hidden_to_output) ; 
  m_grad_input_to_hidden = MatrixScalarMul(parms->m_dir_input_to_hidden, dt, m_grad_input_to_hidden) ; 
  
  MatrixAdd(v_grad_output_bias, ae->v_output_bias, ae->v_output_bias) ;
  MatrixAdd(v_grad_hidden_bias, ae->v_hidden_bias, ae->v_hidden_bias) ;
  MatrixAdd(m_grad_hidden_to_output, ae->m_hidden_to_output, ae->m_hidden_to_output) ;
  MatrixAdd(m_grad_input_to_hidden, ae->m_input_to_hidden, ae->m_input_to_hidden) ;
  
  return(NO_ERROR) ;
}

int
SAEfillInputVector(MRI *mri, int x, int y, int z, int whalf, VECTOR *v_input) 
{
  int  xi, yi, zi, xk, yk, zk, i ;


  for (xk = -whalf, i = 1 ; xk <= whalf ; xk++)
  {
    xi = mri->xi[x+xk] ;
    for (yk = -whalf ; yk <= whalf ; yk++)
    {
      yi = mri->yi[y+yk] ;
      for (zk = -whalf ; zk <= whalf ; zk++, i++)
      {
	zi = mri->zi[z+zk] ;
	VECTOR_ELT(v_input, i) = MRIgetVoxVal(mri, xi, yi, zi, 0) ;
      }
    }
  }

  return(NO_ERROR) ;
}

MRI *
SAEvectorToMRI(VECTOR *v_input, int whalf, MRI *mri) 
{
  int  xk, yk, zk, i, wsize ;

  wsize = (2*whalf)+1 ;

  if (mri == NULL)
    mri = MRIalloc(wsize, wsize, wsize, MRI_FLOAT) ;

  for (xk = 0, i = 1 ; xk < wsize ; xk++)
    for (yk = 0 ; yk < wsize ; yk++)
      for (zk = 0 ; zk < wsize ; zk++, i++)
	MRIsetVoxVal(mri, xk, yk, zk, 0, VECTOR_ELT(v_input, i) ) ;

  return(mri) ;
}

MRI *
SAEinputWeightsToMRI(SAE *sae, MRI *mri) 
{
  int  xk, yk, zk, i, wsize, frame ;

  wsize = (2*sae->whalf)+1 ;

  if (mri == NULL)
    mri = MRIallocSequence(wsize, wsize, wsize, MRI_FLOAT, sae->first->v_hidden->rows) ;

  for (frame = 0 ; frame < mri->nframes ; frame++)
    for (xk = 0, i = 1 ; xk < wsize ; xk++)
      for (yk = 0 ; yk < wsize ; yk++)
	for (zk = 0 ; zk < wsize ; zk++, i++)
	  MRIsetVoxVal(mri, xk, yk, zk, frame, *MATRIX_RELT(sae->first->m_input_to_hidden, frame+1, i)) ;

  return(mri) ;
}

int
SAEwrite(SAE *sae, char *fname) 
{
  FILE *fp ;
  AE   *ae ;
  int   i ;
  
  fp = fopen(fname, "wb") ;
  if (fp == NULL)
    ErrorReturn(ERROR_NOFILE, (ERROR_NOFILE, "SAEwrite(%s): could not open file", fname)) ;

  fprintf(fp, "%d %d %lf\n", sae->whalf, sae->nencoders, sae->scale) ;
  AEwrite(sae->first, fp) ;
  ae = sae->first ;
  for (i = 1 ; i < sae->nencoders && ae->next; i++)
  {
    AEwrite(ae->next, fp) ;
    ae = ae->next ;
  }
  fclose(fp) ;
  return(NO_ERROR) ;
}
static int 
AEwrite(AE *ae, FILE *fp) 
{
  MatrixWriteInto(fp, ae->m_input_to_hidden) ;
  MatrixWriteInto(fp, ae->v_output_bias) ;
  MatrixWriteInto(fp, ae->v_hidden_bias) ;
  MatrixWriteInto(fp, ae->m_hidden_to_output) ;
  MatrixWriteInto(fp, ae->v_input) ;
  MatrixWriteInto(fp, ae->v_hidden) ;
  MatrixWriteInto(fp, ae->v_output) ;
  return(NO_ERROR) ;
}
static AE *
AEread(FILE *fp, AE *prev) 
{
  AE *ae ;
  MATRIX *m_input_to_hidden, *v_output_bias ;

  if (feof(fp))
    return(NULL) ;
  m_input_to_hidden = MatrixReadFrom(fp, NULL) ;
  v_output_bias = MatrixReadFrom(fp, NULL) ;
  ae = AEalloc(prev, v_output_bias->rows, m_input_to_hidden->rows) ;
  MatrixCopy(v_output_bias, ae->v_output_bias) ;
  MatrixCopy(m_input_to_hidden, ae->m_input_to_hidden) ;
  MatrixFree(&m_input_to_hidden) ;
  VectorFree(&v_output_bias) ;

  MatrixReadFrom(fp, ae->v_hidden_bias) ;
  MatrixReadFrom(fp, ae->m_hidden_to_output) ;
  MatrixReadFrom(fp, ae->v_input) ;
  MatrixReadFrom(fp, ae->v_hidden) ;
  MatrixReadFrom(fp, ae->v_output) ;

  return(ae) ;
}

SAE *
SAEread(char *fname) 
{
  FILE *fp ;
  int   whalf, nencoders, i ;
  SAE   *sae ;
  AE    *ae ;
  double scale ;

  fp = fopen(fname, "rb") ;
  if (fp == NULL)
    ErrorReturn(NULL, (ERROR_NOFILE, "SAEread(%s): could not open file", fname)) ;

  fscanf(fp, "%d %d %lf\n", &whalf, &nencoders, &scale) ;
  sae = SAEalloc(whalf, scale) ;
  ae = sae->first = AEread(fp, NULL) ;
  for (i = 1 ; i < nencoders ; i++)
  {
    ae->next = AEread(fp, ae) ;
    ae = ae->next ;
  }
  fclose(fp) ;
  return(sae) ;
  
}

void
AEdump(AE *ae) 
{
  if (ae->v_input->rows == 1)
  {
    printf("f(I=%2.2f * wij=%2.2f + bj=%2.2f = %2.2f) = %2.2f * wjk=%2.2f + bk=%2.2f = %2.2f, E=%2.2f\n",
	   ae->v_input->rptr[1][1], 
	   ae->m_input_to_hidden->rptr[1][1], 
	   ae->v_hidden_bias->rptr[1][1], 
	   ae->v_hidden_net->rptr[1][1], 
	   ae->v_hidden->rptr[1][1], 
	   ae->m_hidden_to_output->rptr[1][1], 
	   ae->v_output_bias->rptr[1][1],
	   ae->v_output->rptr[1][1], 
	   ae->v_output->rptr[1][1]-ae->v_input->rptr[1][1]) ;
    if (ae->m_grad_input_to_hidden)
      printf("grad: wij=%2.4f, bj=%2.4f, wjk=%2.4f, bk=%2.4f\n",
	     ae->m_grad_input_to_hidden->rptr[1][1],
	     ae->v_grad_hidden_bias->rptr[1][1],
	     ae->m_grad_hidden_to_output->rptr[1][1],
	     ae->v_grad_output_bias->rptr[1][1]) ;

    return ;
  }
  printf("v_input\n") ;
  MatrixPrint(stdout, ae->v_input);
  printf("v_output\n") ;
  MatrixPrint(stdout, ae->v_output) ;
  printf("v_error\n") ;
  MatrixPrint(stdout, ae->v_error) ;

  printf("v_hidden_bias\n") ;
  MatrixPrint(stdout, ae->v_hidden_bias) ;
  printf("v_hidden_net\n") ;
  MatrixPrint(stdout, ae->v_hidden_net) ;
  printf("v_hidden\n") ;
  MatrixPrint(stdout, ae->v_hidden) ;
  printf("m_input_to_hidden\n") ;
  MatrixPrint(stdout, ae->m_input_to_hidden) ;

  printf("v_output_bias\n") ;
  MatrixPrint(stdout, ae->v_output_bias) ;
  printf("m_hidden_to_output\n") ;
  MatrixPrint(stdout, ae->m_hidden_to_output) ;

  if (ae->m_grad_input_to_hidden && 0)
  {
    printf("m_grad_input_to_hidden\n") ;
    MatrixPrint(Gstdout, ae->m_grad_input_to_hidden) ;
    printf("v_grad_hidden_bias\n") ;
    MatrixPrint(Gstdout, ae->v_grad_hidden_bias) ;
    printf("m_grad_hidden_to_output\n") ;
    MatrixPrint(Gstdout, ae->m_grad_hidden_to_output) ;
    printf("v_grad_output_bias\n") ;
    MatrixPrint(Gstdout, ae->v_grad_output_bias) ;
  }
}
static int
AEsaveState(AE *ae) 
{
  ae->m_saved_input_to_hidden = MatrixCopy(ae->m_input_to_hidden, ae->m_saved_input_to_hidden) ;
  ae->m_saved_hidden_to_output = MatrixCopy(ae->m_hidden_to_output, ae->m_saved_hidden_to_output) ;
  ae->v_saved_hidden_bias = MatrixCopy(ae->v_hidden_bias, ae->v_saved_hidden_bias) ;
  ae->v_saved_output_bias = MatrixCopy(ae->v_output_bias, ae->v_saved_output_bias) ;
  return(NO_ERROR) ;
}

static int 
AErestoreState(AE *ae) 
{
  MatrixCopy(ae->m_saved_input_to_hidden, ae->m_input_to_hidden) ;
  MatrixCopy(ae->m_saved_hidden_to_output, ae->m_hidden_to_output) ;
  MatrixCopy(ae->v_saved_hidden_bias, ae->v_hidden_bias) ;
  MatrixCopy(ae->v_saved_output_bias, ae->v_output_bias) ;
  return(NO_ERROR) ;
}
