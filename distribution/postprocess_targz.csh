#!/bin/tcsh -ef

set ID='$Id: postprocess_targz.csh,v 1.4 2006/05/24 15:15:14 nicks Exp $'

set echo=1

setenv SPACE_FS /space/freesurfer
# if /space/freesurfer is down, or if there is a need to run
# the build scripts outside of /usr/local/freesurfer, then 
# the var USE_SPACE_MINERVA can be set
if ($?USE_SPACE_MINERVA) then
  setenv SPACE_FS /space/minerva/1/users/nicks
endif

cd $MINERVA_HOME/tmp
if (-e freesurfer) sudo rm -Rf freesurfer
sudo tar zxvf ${SPACE_FS}/build/pub-releases/$1.tar.gz
if ($status) exit 1
sudo chmod -R a+rw freesurfer
if ($status) exit 1
sudo chmod -R a-w freesurfer/subjects/fsaverage
if ($status) exit 1
sudo chown -R root freesurfer
if ($status) exit 1
sudo chown -R root freesurfer/*.*
if ($status) exit 1
sudo chgrp -R root freesurfer
if ($status) exit 1
sudo chgrp -R root freesurfer/*.*
if ($status) exit 1
tar -X ${SPACE_FS}/build/scripts/exclude_from_targz cvf $1.tar freesurfer
if ($status) exit 1
gzip $1.tar
if ($status) exit 1
md5sum $1.tar.gz >> ${SPACE_FS}/build/pub-releases/md5sum.txt
mv $1.tar.gz ${SPACE_FS}/build/pub-releases/
if ($status) exit 1

exit 0
