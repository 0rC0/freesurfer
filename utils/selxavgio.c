/***************************************************************
  Name:    selxavgio.c
  $Id: selxavgio.c,v 1.1 2001/02/06 18:13:11 greve Exp $
  Author:  Douglas Greve
  Purpose: Routines for handling header files for data created by
  selxavg or selavg (selectively averaged).
 ****************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <strings.h>

#include "selxavgio.h"

extern int errno;

/* ---------------------------------------------------- */
int sv_sxadat_by_stem(SXADAT *sxadat ,char *volstem)
{
  char tmpstr[1000];
  int err;
  sprintf(tmpstr,"%s.dat",volstem);
  err = sv_sxadat(sxadat,tmpstr);
  return(err);
}
/* ---------------------------------------------------- */
SXADAT * ld_sxadat_from_stem(char *volstem)
{
  char tmpstr[1000];
  SXADAT *sxa;

  if(! is_sxa_volume(volstem) ) return(NULL);

  sprintf(tmpstr,"%s.dat",volstem);

  sxa = ld_sxadat(tmpstr);
  return(sxa);
}


/* ---------------------------------------------------- */
float *sxa_framepower(SXADAT *sxa, int *nframes)
{
  int frame;
  int h, statid, condition;
  float *framepower;

  *nframes = 2 * sxa->Nc * sxa->Nh;
  framepower = (float *)calloc(*nframes,sizeof(float));

  frame = 0;
  for(condition = 0; condition < sxa->Nc; condition++){
    for(statid = 0; statid < 2; statid++){
      for(h = 0; h < sxa->Nh; h++){
	if(statid == 0) framepower[frame] = 1.0; /* avereage */
	if(statid == 1) framepower[frame] = 2.0; /* std/var */
	frame++;
      }
    }
  }
  return(framepower);
}

/* ---------------------------------------------------- */
int is_sxa_volume(char *volstem)
{
  char tmpstr[1000];
  FILE *fp;

  sprintf(tmpstr,"%s.dat",volstem);
  fp = fopen(tmpstr,"r");
  if(fp==NULL) return(0);
  fclose(fp);

  return(1);
}

/* ---------------------------------------------------- */
SXADAT * ld_sxadat(char *sxadatfile)
{
  FILE *fp;
  SXADAT *sxa;
  int n, r, c, Nch;

  fp = fopen(sxadatfile,"r");
  if(fp==NULL){
    perror("ldsxdat():");
    fprintf(stderr,"Could not open %s\n",sxadatfile);
    return(NULL);
  }

  sxa = (SXADAT *) calloc(1,sizeof(SXADAT));
  if(sxa == NULL){
    fprintf(stderr,"Could not alloc SXADAT\n");
    fclose(fp);
    return(NULL);
  }
    
  fscanf(fp,"%*s %f",&sxa->TR);
  fscanf(fp,"%*s %f",&sxa->TimeWindow);
  fscanf(fp,"%*s %f",&sxa->TPreStim);
  fscanf(fp,"%*s %d",&sxa->Nc);
  fscanf(fp,"%*s %d",&sxa->Nh);
  sxa->Nnnc = sxa->Nc - 1;
  n = fscanf(fp,"%*s %d",&sxa->version);
  if(n==0){
    sxa->version = 0;
    return(sxa);
  }
  fscanf(fp,"%*s %f",&sxa->TER);
  fscanf(fp,"%*s %d",&sxa->DOF);

  fscanf(fp,"%*s");
  sxa->npercond = (int *)calloc(sxa->Nc,sizeof(int));
  for(n=0; n < sxa->Nc; n++)
    fscanf(fp,"%d",&sxa->npercond[n]);
  fscanf(fp,"%*s %d",&sxa->nruns);
  fscanf(fp,"%*s %d",&sxa->ntp);
  fscanf(fp,"%*s %d",&sxa->nrows);
  fscanf(fp,"%*s %d",&sxa->ncols);
  fscanf(fp,"%*s %d",&sxa->nskip);
  fscanf(fp,"%*s %d",&sxa->DTOrder);
  fscanf(fp,"%*s %f",&sxa->RescaleFactor);
  fscanf(fp,"%*s %f",&sxa->HanningRadius);
  fscanf(fp,"%*s %d",&sxa->nNoiseAC);
  fscanf(fp,"%*s %d",&sxa->BrainAirSeg);
  fscanf(fp,"%*s %d",&sxa->GammaFit);
  if(sxa->GammaFit){
    fscanf(fp,"%*s %f",&sxa->gfDelta);
    fscanf(fp,"%*s %f",&sxa->gfTau);
  }
  fscanf(fp,"%*s %d",&sxa->NullCondId);

  Nch = sxa->Nh * sxa->Nnnc;

  fscanf(fp,"%*s");
  sxa->SumXtX = (float *)calloc(Nch*Nch,sizeof(float));
  n = 0;
  for(r=0; r < Nch; r++){
    for(c=0; c < Nch; c++){
      fscanf(fp,"%f",&sxa->SumXtX[n]);
      n++;
    }
  }

  if(sxa->version == 1) return(sxa);

  fscanf(fp,"%*s");
  sxa->hCovMtx = (float *)calloc(Nch*Nch,sizeof(float));
  n = 0;
  for(r=0; r < Nch; r++){
    for(c=0; c < Nch; c++){
      fscanf(fp,"%f",&sxa->hCovMtx[n]);
      n++;
    }
  }

  fscanf(fp,"%*s");
  sxa->CondIdMap = (int *) calloc(sxa->Nc, sizeof(int));
  for(n=0; n < sxa->Nc; n++)
    fscanf(fp,"%d",&sxa->CondIdMap[n]);

  return(sxa);
}

/* ---------------------------------------------------- */
int sv_sxadat(  SXADAT *sxa, char *sxadatfile )
{
  FILE *fp;
  int n, r, c, Nch;

  fp = fopen(sxadatfile,"w");
  if(fp==NULL){
    perror("sv_sxdat():");
    fprintf(stderr,"Could not open %s\n",sxadatfile);
    return(1);
  }

  fprintf(fp,"TR         %f\n",sxa->TR);
  fprintf(fp,"TimeWindow %f\n",sxa->TimeWindow);
  fprintf(fp,"TPreStim   %f\n",sxa->TPreStim);
  fprintf(fp,"nCond      %d\n",sxa->Nc);
  fprintf(fp,"Nh         %d\n",sxa->Nh);
  if(sxa->version == 0){
    fclose(fp);
    return(0);
  }
  fprintf(fp,"version %d\n",sxa->version);
  fprintf(fp,"TER %f\n",sxa->TER);
  fprintf(fp,"DOF %d\n",sxa->DOF);

  fprintf(fp,"NPerCond ");
  for(n=0; n < sxa->Nc; n++) fprintf(fp,"%d ",sxa->npercond[n]);
  fprintf(fp,"\n");

  fprintf(fp,"nruns %d\n",sxa->nruns);
  fprintf(fp,"ntp %d\n",sxa->ntp);
  fprintf(fp,"nrows %d\n",sxa->nrows);
  fprintf(fp,"ncols %d\n",sxa->ncols);
  fprintf(fp,"nskip %d\n",sxa->nskip);
  fprintf(fp,"DTOrder %d\n",sxa->DTOrder);
  fprintf(fp,"RescaleFactor %f\n",sxa->RescaleFactor);
  fprintf(fp,"HanningRadius %f\n",sxa->HanningRadius);
  fprintf(fp,"nNoiseAC %d\n",sxa->nNoiseAC);
  fprintf(fp,"BrainAirSeg %d\n",sxa->BrainAirSeg);
  fprintf(fp,"GammaFit %d\n",sxa->GammaFit);
  if(sxa->GammaFit){
    fprintf(fp,"gfDelta %f\n",sxa->gfDelta);
    fprintf(fp,"gfTau   %f\n",sxa->gfTau);
  }
  fprintf(fp,"NullCondId %d\n",sxa->NullCondId);

  fprintf(fp,"SumXtX\n");
  Nch = sxa->Nh * sxa->Nnnc;

  n = 0;
  for(r=0; r < Nch; r++){
    for(c=0; c < Nch; c++){
      fprintf(fp,"%f \n",sxa->SumXtX[n]);
      n++;
    }
  }

  if(sxa->version == 1) return(0);

  fprintf(fp,"hCovMtx\n");
  n = 0;
  for(r=0; r < Nch; r++){
    for(c=0; c < Nch; c++){
      fprintf(fp,"%f \n",sxa->hCovMtx[n]);
      n++;
    }
  }

  fprintf(fp,"CondIdMap ");  
  for(n=0; n < sxa->Nc; n++)
    fprintf(fp,"%d ",sxa->CondIdMap[n]);
  fprintf(fp,"\n");

  return(0);
}

