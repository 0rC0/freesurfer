#ifndef REGION_H
#define REGION_H

#include <stdio.h>
#include "mri.h"


MRI_REGION *REGIONsubtract(MRI_REGION *reg1, MRI_REGION *reg2, 
                           MRI_REGION *rdst) ;
MRI_REGION *REGIONadd(MRI_REGION *reg1, MRI_REGION *reg2, MRI_REGION *rdst);
MRI_REGION *REGIONclear(MRI_REGION *r) ;
MRI_REGION *REGIONintersect(MRI_REGION *reg1, MRI_REGION *reg2, 
                            MRI_REGION *rdst) ;
MRI_REGION *REGIONunion(MRI_REGION *reg1, MRI_REGION *reg2, MRI_REGION *rdst);
MRI_REGION *REGIONalloc(void) ;
MRI_REGION *REGIONcopy(MRI_REGION *rsrc, MRI_REGION *rdst) ;
int        REGIONinside(MRI_REGION *reg, int x, int y, int z) ;
MRI_REGION *REGIONexpand(MRI_REGION *rsrc, MRI_REGION *rdst, int n) ;
float      REGIONminCornerDistance(MRI_REGION *r1, MRI_REGION *r2) ;


#define REGION_INSIDE      1
#define REGION_ON_BORDER   -1
#define REGION_OUTSIDE     0

#endif
