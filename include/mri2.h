
#include "mri.h"


MRI *mri_load_bvolume(char *bfstem);
int  mri_save_as_bvolume(MRI *vol, char *stem, int svendian, int svtype);
MRI *mri_load_bvolume_frame(char *bfstem, int frameno);
int  mri_framepower(MRI *vol, float *framepower);
MRI *mri_binarize(MRI *vol, float thresh, char *sign, MRI *volbin, int *nover);
MRI *mri_rescale(MRI *invol, float min, float max, MRI *outvol);
int  mri_save_as_cor(MRI *vol,  char *cordir, int frame, int rescale);
int mri_minmax(MRI *vol, float *min, float *max);
MRI *mri_load_cor_as_float(char *cordir);
MRI *mri_load_wfile(char *wfile);
