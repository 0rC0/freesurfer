/**
 * @file  romp_support.h
 * @brief prototypes and structures for getting reprodiucible results from and for timing omp loops.
 *
 */
/*
 * Original Author: Bevin Brett
 * CVS Revision Info:
 *    $Author: fischl $
 *    $Date: 2017/12 $
 *    $Revision: 1.0 $
 *
 * Copyright © 2012 The General Hospital Corporation (Boston, MA) "MGH"
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


	    }           // serial loop
        
        }               // parallel loop
    } // poison ROMP_SUMREDUCTION0 etc.
    
#undef ROMP_VARIABLE
#undef ROMP_LO
#undef ROMP_HI

#ifdef ROMP_FOR_LEVEL
#undef ROMP_FOR_LEVEL
#endif

#ifdef ROMP_SUMREDUCTION0
#undef ROMP_SUMREDUCTION0
#endif

#ifdef ROMP_SUMREDUCTION1
#undef ROMP_SUMREDUCTION1
#endif

#ifdef ROMP_SUMREDUCTION2
#undef ROMP_SUMREDUCTION2
#endif

       ROMP_Distributor_end(&ROMP_distributor);
    }
