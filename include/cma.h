#ifndef CMA_H
#define CMA_H

#define    Unknown   0 /*                                 0    0    0     0*/
#define    Left_Cerebral_Exterior  1/*              205   62   78     0 */
#define    Left_Cerebral_White_Matter  2/*          225  225  225     0 */
#define    Left_Cerebral_Cortex  3/*                205   62   78     0 */
#define    Left_Lateral_Ventricle  4/*              120   18  134     0 */
#define    Left_Inf_Lat_Vent  5/*                    196   58  250     0 */
#define    Left_Cerebellum_Exterior  6/*               0  148    0     0 */
#define    Left_Cerebellum_White_Matter  7/*         220  248  164     0 */
#define    Left_Cerebellum_Cortex  8/*               230  148   34     0 */
#define    Left_Thalamus  9/*                          0  118   14     0 */
#define   Left_Thalamus_Proper  10/*                   0  118   14     0 */
#define   Left_Caudate  11/*                         122  186  220    0  */
#define   Left_Putamen  12/*                         236   13  176    0  */
#define   Left_Pallidum  13/*                         12   48  255    0  */
#define   Third_Ventricle  14/*                        204  182  142    0  */
#define   Fourth_Ventricle  15/*                         42  204  164    0  */
#define   Brain_Stem  16/*                           119  159  176    0  */
#define   Left_Hippocampus  17/*                     220  216   20     0 */
#define   Left_Amygdala  18/*                        103  255  255    0  */
#define   Left_Insula  19/*                           80  196   98     0 */
#define   Left_Operculum  20/*                        60   58  210    0  */
#define   Line_1  21/*                                60   58  210    0  */
#define   Line_2  22/*                                60   58  210    0  */
#define   Line_3  23/*                                60   58  210    0  */
#define   CSF  24/*                                   60   60   60     0 */
#define   Left_Lesion  25/*                          255  165    0    0  */
#define   Left_Accumbens_area  26/*                  255  165    0    0  */
#define   Left_Substancia_Nigra  27/*                  0  255  127    0  */
#define   Left_VentralDC  28/*                       165   42   42    0  */
#define   Left_undetermined  29/*                    135  206  235    0  */
#define   Left_vessel  30/*                          160   32  240    0  */
#define   Left_choroid_plexus  31/*                    0  255  255    0  */
#define   Left_F3orb  32/*                           100   50  100    0  */
#define   Left_lOg  33/*                             135   50   74     0 */
#define   Left_aOg  34/*                             122  135   50     0 */
#define   Left_mOg  35/*                              51   50  135    0  */
#define   Left_pOg  36/*                              74  155   60     0 */
#define   Left_Stellate  37/*                        120   62   43    0  */
#define   Left_Porg  38/*                             74  155   60     0 */
#define   Left_Aorg  39/*                            122  135   50      0 */
#define   Right_Cerebral_Exterior  40/*              205   62   78      0 */
#define   Right_Cerebral_White_Matter  41/*            0  225    0     0  */
#define   Right_Cerebral_Cortex  42/*                205   62   78      0  */
#define   Right_Lateral_Ventricle  43/*              120   18  134     0  */
#define   Right_Inf_Lat_Vent  44/*                   196   58  250     0   */
#define   Right_Cerebellum_Exterior  45/*              0  148    0     0 */
#define   Right_Cerebellum_White_Matter  46/*        220  248  164     0  */
#define   Right_Cerebellum_Cortex  47/*              230  148   34      0  */
#define   Right_Thalamus  48/*                         0  118   14      0  */
#define   Right_Thalamus_Proper  49/*                  0  118   14      0  */
#define   Right_Caudate  50/*                        122  186  220     0  */
#define   Right_Putamen  51/*                        236   13  176     0  */
#define   Right_Pallidum  52/*                       255   48  255     0   */
#define   Right_Hippocampus  53/*                    220  216   20      0  */
#define   Right_Amygdala  54/*                       103  255  255     0   */
#define   Right_Insula  55/*                          80  196   98      0  */
#define   Right_Operculum  56/*                       60   58  210     0   */
#define   Right_Lesion  57/*                         255  165    0     0   */
#define   Right_Accumbens_area  58/*                 255  165    0     0   */
#define   Right_Substancia_Nigra  59/*                 0  255  127     0   */
#define   Right_VentralDC  60/*                      165   42   42     0   */
#define   Right_undetermined  61/*                   135  206  235     0   */
#define   Right_vessel  62/*                         160   32  240     0   */
#define   Right_choroid_plexus  63/*                   0  255  255     0   */
#define   Right_F3orb  64/*                          100   50  100     0   */
#define   Right_lOg  65/*                            135   50   74     0   */
#define   Right_aOg  66/*                            122  135   50     0   */
#define   Right_mOg  67/*                             51   50  135     0   */
#define   Right_pOg  68/*                             74  155   60     0   */
#define   Right_Stellate  69/*                       120   62   43     0   */
#define   Right_Porg  70/*                            74  155   60     0   */
#define   Right_Aorg  71/*                          122  135   50     0*/
#define   Fifth_Ventricle 72/*                        156   25  250    0 */
#define   Left_Interior 73  /*                       122  135   50    0*/
#define   Right_Interior 74  /*                      122  135   50    0*/
#define   Left_Lateral_Ventricles 75  /*             120   18  134    0*/
#define   Right_Lateral_Ventricles 76  /*            120   18  134    0*/
#define   WM_hypointensities 77  /*                  124  140  178    0*/
#define   Left_WM_hypointensities 78  /*             124  140  178    0*/
#define   Right_WM_hypointensities 79  /*            124  140  178    0*/
#define   non_WM_hypointensities 80  /*              164  108  226    0*/
#define   Left_non_WM_hypointensities 81  /*         164  108  226    0*/
#define   Right_non_WM_hypointensities 82  /*        164  108  226    0*/
#define   Left_F1 83  /*                             255  218  185    0*/
#define   Right_F1 84  /*                            255  218  185    0*/
#define   Optic_Chiasm   85  /*                        234  169   30    0*/

/* no brain labels after this please unless you fix the IS_BRAIN macro */
#define   Dura           98
#define   Epidermis      118
#define   Conn_Tissue    119
#define   SC_FAT_MUSCLE  120
#define   Cranium        121
#define   CSF_SA         122
#define   Muscle         123
#define   Ear            124
#define   Fatty_Tissue   125
#define   Spinal_Cord    126
#define   Soft_Tissue    127
#define   Nerve          128
#define   Bone           129                            
#define   Air            130
#define   Orbit          131
#define   Tongue         132
#define   Nasal_Structures 133
#define   Globe          134
#define   Teeth          135

#define   Right_Temporal_Cerebral_White_Matter  186      /*  240  240  240     0 */
#define   Left_Temporal_Cerebral_White_Matter  187       /* 240  240  240     0 */
#define   Fat         189 /*  255 255 255 0 */
#define   Bright_Unknown  190   /* 100 240 240 0 */
#define   Dark_Unknown    191   /* 20  100  100     0  */
#define   Corpus_Callosum 192   /* 255 255 255 */
#define   Not_Set         255


#define IS_UNKNOWN(label)  (((label) == Unknown) || (label == 255) || (label == Bright_Unknown) || (label == Dark_Unknown))

#define IS_BRAIN(label)  (!IS_UNKNOWN(label) && label < Dura)

#define IS_WM(label) (((label) == Left_Cerebral_White_Matter) || ((label) == Right_Cerebral_White_Matter) || ((label) == Left_Temporal_Cerebral_White_Matter) || ((label) == Right_Temporal_Cerebral_White_Matter))
#define IS_GM(label) (((label) == Left_Cerebral_Cortex) || ((label) == Right_Cerebral_Cortex))

#define IS_CEREBELLAR_WM(label) (((label) == Left_Cerebellum_White_Matter) || ((label) == Right_Cerebellum_White_Matter))

#define IS_HIPPO(l) (((l) == Left_Hippocampus) || ((l) == Right_Hippocampus))
#define IS_AMYGDALA(l) (((l) == Left_Amygdala) || ((l) == Right_Amygdala))
#define IS_CORTEX(l) (((l) == Left_Cerebral_Cortex) || \
                      ((l) == Right_Cerebral_Cortex))
#define IS_LAT_VENT(l) (((l) == Left_Lateral_Ventricle) || \
                        ((l) == Right_Lateral_Ventricle) || \
                        ((l) == Right_Inf_Lat_Vent) || \
                        ((l) == Left_Inf_Lat_Vent))


#define MAX_CMA_LABEL (Corpus_Callosum+1)
#define MAX_CMA_LABELS (MAX_CMA_LABEL+1)
#define LABEL_WITH_NO_TOPOLOGY_CONSTRAINT(l) (\
   ((l) == Right_non_WM_hypointensities) || \
   ((l) == Left_non_WM_hypointensities) || \
   ((l) == Left_WM_hypointensities) || \
   ((l) == Right_WM_hypointensities) || \
   ((l) == WM_hypointensities) || \
   ((l) == Dura) || \
   ((l) == Bone) || \
   ((l) == CSF_SA) || \
   ((l) == Epidermis) || \
   ((l) == SC_FAT_MUSCLE) || \
   ((l) == non_WM_hypointensities))

/* --- below: see cma.c --- */

#define MAX_OUTLINE_CLAIMS 10

#define CMA_FILL_NONE     0
#define CMA_FILL_OUTLINE  1
#define CMA_FILL_INTERIOR 2

typedef struct {
  int n_claims;
  int interior_claim_flag;
  short claim_labels[MAX_OUTLINE_CLAIMS];
  float claim_values[MAX_OUTLINE_CLAIMS];
  float no_label_claim;
} CMAoutlineClaim;

typedef struct {
  int width, height;
  CMAoutlineClaim **claim_field;
  unsigned char **fill_field;
  unsigned char **outline_points_field;
} CMAoutlineField;

CMAoutlineField *CMAoutlineFieldAlloc(int width, int height);
int CMAfreeOutlineField(CMAoutlineField **of);

int CMAclearFillField(CMAoutlineField *field);
int CMAfill(CMAoutlineField *field, short seed_x, short seed_y);

int CMAclaimPoints(CMAoutlineField *field, short label, short *points, int n_points, short seed_x, short seed_y);
int CMAassignLabels(CMAoutlineField *field);

int CMAvalueClaims(CMAoutlineClaim *claim);
int CMAvalueAllClaims(CMAoutlineField *field);

/* the weights to give claims of nearby points -- see cma.c */
#define OTL_CLAIM_WEIGHT_CENTER   1.000
#define OTL_CLAIM_WEIGHT_SIDE     0.500
#define OTL_CLAIM_WEIGHT_DIAGONAL 0.707

short CMAtotalClaims(CMAoutlineField *field, int x, int y);
int CMAaddWeightedTotals(CMAoutlineClaim *claim, float weight, float *claim_totals);

int CMAzeroOutlines(CMAoutlineField *field);

#endif
