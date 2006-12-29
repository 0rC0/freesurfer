/**
 * @file  evschutils.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 02:08:59 $
 *    $Revision: 1.7 $
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



#ifndef EVSCHUTILS_H
#define EVSCHUTILS_H

#ifdef X
#undef X
#endif

#define EVS_COST_UNKNOWN   0
#define EVS_COST_EFF       1
#define EVS_COST_VRFAVG    2
#define EVS_COST_VRFSTD    3
#define EVS_COST_VRFAVGSTD 4
#define EVS_COST_IDEALXTX  5

typedef struct
{

  int    nevents;
  float *tevent;
  int   *eventid;
  float *weight;

  int    nEvTypes;
  int   *nEvReps;
  float  *EvDur;

  int   nthsearched;
  float cb1err;
  float eff;
  float vrfavg;
  float vrfstd;
  float vrfmin;
  float vrfmax;
  float vrfrange;
  float idealxtxerr;
  float cost;

}
EVENT_SCHEDULE, EVSCH;

EVENT_SCHEDULE *EVSAlloc(int nevents, int allocweight);
int EVSfree(EVENT_SCHEDULE **ppEvSch);
int EVSPrint(FILE *fp, EVENT_SCHEDULE *EvSch);

EVENT_SCHEDULE *EVSreadPar(char *parfile);
int EVSwritePar(char *parfile, EVENT_SCHEDULE *EvSch, char **labels,
                float tPreScan, float tMax);

int EVSmaxId(EVSCH *EvSch);

EVENT_SCHEDULE *EVSsynth(int nEvTypes, int *nPer, float *tPer,
                         float tRes, float tMax, float tPreScan,
                         int nCB1Search, float tNullMin, float tNullMax);

EVENT_SCHEDULE *RandEvSch(int nevents, int ntypes, float dtmin,
                          float dtnullavg, int randweights);
EVENT_SCHEDULE *SynthEvSch(int nEvTypes, int *nPer, float *tPer,
                           float tRes, float tMax, float tPreScan);
MATRIX *EVS2FIRmtx(int EvId, EVSCH *EvSch, float tDelay, float TR,
                   int Ntps, float PSDMin, float PSDMax, float dPSD,
                   MATRIX *X);
MATRIX *EVSfirMtxAll(EVSCH *EvSch, float tDelay, float TR, int Ntps,
                     float PSDMin, float PSDMax, float dPSD);


int EVSsort(EVSCH **EvSchList, int nList);

MATRIX *EVScb1ProbMatrix(EVSCH *EvSch);
MATRIX *EVScb1IdealProbMatrix(EVSCH *EvSch);
MATRIX *EVScb1Matrix(EVSCH *EvSch);
float   EVScb1Error(EVSCH *EvSch);

EVSCH *EVSRandSequence(int nEvTypes, int *nEvReps);
int    EVSRandTiming(EVSCH *EvSch, float *EvDur,
                     float tRes, float tMax, float tPreScan);
EVSCH *EVScb1Optimize(int nEvTypes, int *nEvReps, int nSearch);
char  *EVScostString(int CostId);
int    EVScostId(char *CostString);

int EVSdesignMtxStats(MATRIX *Xtask, MATRIX *Xnuis, EVSCH *EvSch,
                      MATRIX *C, MATRIX *W);
float EVScost(EVSCH *EvSch, int CostId, float *params);

int *RandPerm(int N, int *v);
int  RandPermList(int N, int *v);
int RandPermListLimit0(int N, int *v, int lim, int nitersmax);

MATRIX *EVSfirXtXIdeal(int nEvTypes, int *nEvReps, float *EvDur,
                       float TR, int Ntp,
                       float PSDMin, float PSDMax, float dPSD);
int EVSrefractory(EVSCH *sch, double alpha, double T, double dtmin);

#endif //#ifndef EVSCHUTILS_H


