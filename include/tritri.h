#ifndef TRITRI_H
#define TRITRI_H

int tri_tri_intersect(double V0[3],double V1[3],double V2[3],
                      double U0[3],double U1[3],double U2[3]) ;


int triangle_ray_intersect(double orig_pt[3], double dir[3], double U0[3], 
                           double U1[3],double U2[3], double int_pt[3]) ;

#endif
