#! /bin/tcsh -ef

#
# nu_correct_one.csh
#
# REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
#
# Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR
# CVS Revision Info:
#    $Author: nicks $
#    $Date: 2007/01/06 00:01:14 $
#    $Revision: 1.2 $
#
# Copyright (C) 2002-2007,
# The General Hospital Corporation (Boston, MA).
# All rights reserved.
#
# Distribution, usage and copying of this software is covered under the
# terms found in the License Agreement file named 'COPYING' found in the
# FreeSurfer source code root directory, and duplicated here:
# https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
#
# General inquiries: freesurfer@nmr.mgh.harvard.edu
# Bug reports: analysis-bugs@nmr.mgh.harvard.edu
#


set s=$1
set iter=3

set mdir=$SUBJECTS_DIR/$s/mri
if (-e $mdir/orig == 0) then
		set ORIG = orig.mgz
else
		set ORIG = orig
endif

mri_convert $mdir/$ORIG /tmp/nu$$0.mnc
nu_correct -stop .0001 -iterations $iter -normalize_field -clobber /tmp/nu$$0.mnc /tmp/nu$$1.mnc
mkdir -p $mdir/nu
mri_convert /tmp/nu$$1.mnc $mdir/nu
rm /tmp/nu$$0.mnc /tmp/nu$$1.mnc
find  /tmp  -prune -name "*.imp" -user $LOGNAME -exec rm -f {} \;
#rm -f /tmp/*.imp


