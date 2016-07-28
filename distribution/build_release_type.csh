#!/bin/tcsh -f

set ID='$Id: build_release_type.csh,v 1.152 2016/07/28 14:26:12 zkaufman Exp $'

unsetenv echo
if ($?SET_ECHO_1) set echo=1

umask 002

# usage:
#  build_release_type dev
#  build_release_type stable
#  build_release_type stable-pub
set RELEASE_TYPE=$1
set FORCE_BUILD=0
if ($#argv == 2 && $2 == "--force") then
    set FORCE_BUILD=1
endif

set STABLE_VER_NUM="v6-beta"
set STABLE_PUB_VER_NUM="v6.0.0"

set HOSTNAME=`hostname -s`

# note: Mac's need full email addr
set SUCCESS_MAIL_LIST=(\
    zkaufman@nmr.mgh.harvard.edu)
set FAILURE_MAIL_LIST=(\
    fischl@nmr.mgh.harvard.edu \
    greve@nmr.mgh.harvard.edu \
    rpwang@nmr.mgh.harvard.edu \
    mreuter@nmr.mgh.harvard.edu \
    koen@nmr.mgh.harvard.edu \
    lzollei@nmr.mgh.harvard.edu \
    rudolph@nmr.mgh.harvard.edu \
    ayendiki@nmr.mgh.harvard.edu \
    zkaufman@nmr.mgh.harvard.edu)
set FAILURE_MAIL_LIST=(\
    zkaufman@nmr.mgh.harvard.edu)

setenv OSTYPE `uname -s`
if ("$OSTYPE" == "linux") setenv OSTYPE Linux
if ("$OSTYPE" == "Linux") setenv OSTYPE Linux
if ("$OSTYPE" == "darwin") setenv OSTYPE Darwin
if ("$OSTYPE" == "Darwin") setenv OSTYPE Darwin
set OS=${OSTYPE}

if ("$OSTYPE" == "Darwin") then
  # Mac OS X chmod and chgrp need -L flag to follow symbolic links
  #set change_flags=(-RL)
  set change_flags=(-R)
else
  set change_flags=(-R)
endif

#
# Set up directories.
######################################################################
#
setenv SPACE_FS /space/freesurfer
setenv LOCAL_FS /usr/local/freesurfer
# if /space/freesurfer is down, or if there is a need to install
# outside of /usr/local/freesurfer, 
# then these can override
if ($?USE_SPACE_FS) then
  setenv SPACE_FS $USE_SPACE_FS
endif
if ($?USE_LOCAL_FS) then
  setenv LOCAL_FS $USE_LOCAL_FS
endif

setenv BUILD_HOSTNAME_DIR      ${SPACE_FS}/build/$HOSTNAME
setenv PLATFORM       "`cat ${LOCAL_FS}/PLATFORM`"
setenv BUILD_PLATFORM "`cat ${BUILD_HOSTNAME_DIR}/PLATFORM`"

# SRC_DIR is the CVS checkout of either the main trunk and the stable branch.
# BUILD_DIR is where the build occurs (doesnt have to be SRC_DIR)
# INSTALL_DIR is where the build will be installed.
if ("$RELEASE_TYPE" == "dev") then
  set BRANCH=master
  set SRC_DIR=${BUILD_HOSTNAME_DIR}/trunk/dev
  set BUILD_DIR=${SRC_DIR}
  set INSTALL_DIR=${LOCAL_FS}/dev
else if ("$RELEASE_TYPE" == "stable") then
  set BRANCH=stable6
  set SRC_DIR=${BUILD_HOSTNAME_DIR}/stable/dev
  set BUILD_DIR=${SRC_DIR}
  set INSTALL_DIR=${LOCAL_FS}/${BRANCH}
else if ("$RELEASE_TYPE" == "stable-pub") then
  set BRANCH=stable6
  set SRC_DIR=${BUILD_HOSTNAME_DIR}/stable/dev
  set BUILD_DIR=${SRC_DIR}
  set INSTALL_DIR=${LOCAL_FS}/${BRANCH}-pub
else
  echo "ERROR: release_type must be either dev, stable or stable-pub"
  echo ""
  echo "Examples: "
  echo "  build_release_type dev"
  echo "  build_release_type stable"
  echo "  build_release_type stable-pub"
  exit 1
endif
set SCRIPT_DIR=${SPACE_FS}/build/scripts
set LOG_DIR=${SPACE_FS}/build/logs


# dev build use latest-and-greatest package libs
# stable build use explicit package versions (for stability).
# Linux uses the /usr/pubsw symlink that exist on all linux
# platforms here at the Center. But as of OSX v10.10 (El Capitan)
# Sips protects the /usr directory, so we use /usr/local instead.
set PUBSW_DIR="/usr/pubsw"
if ("$PLATFORM" =~ OSX*) then
  set PUBSW_DIR="/usr/local/pubsw"
endif
if (("${RELEASE_TYPE}" == "stable") || ("${RELEASE_TYPE}" == "stable-pub")) then
  set MNIDIR=${PUBSW_DIR}/packages/mni/1.5
  set TCLDIR=${PUBSW_DIR}/packages/tcltktixblt/8.4.6
  set TIXWISH=${TCLDIR}/bin/tixwish8.1.8.4
  set VTKDIR=${PUBSW_DIR}/packages/vtk/5.6
  set KWWDIR=${PUBSW_DIR}/packages/KWWidgets/CVS
  set GCCDIR=${PUBSW_DIR}/packages/gcc/4.4.7
  set HIPPO_MATLAB_DIR=${PUBSW_DIR}/packages/matlab/8.0
  setenv FSLDIR ${PUBSW_DIR}/packages/fsl/4.1.9
  setenv DCMTKDIR ${PUBSW_DIR}/packages/dcmtk/3.6.0
  setenv GDCMDIR ${PUBSW_DIR}/packages/gdcm/2.4.1
  set CPPUNITDIR=${PUBSW_DIR}/packages/cppunit/1.10.2
  if ( ! -d ${CPPUNITDIR} ) unset CPPUNITDIR
  setenv AFNIDIR ${PUBSW_DIR}/packages/AFNI/2011_12_21_1014
else
  # dev build uses most current
  set MNIDIR=${PUBSW_DIR}/packages/mni/current
  set TCLDIR=${PUBSW_DIR}/packages/tcltktixblt/current
  set TIXWISH=${TCLDIR}/bin/tixwish8.1.8.4
  set VTKDIR=${PUBSW_DIR}/packages/vtk/current
  set KWWDIR=${PUBSW_DIR}/packages/KWWidgets/current
  set GCCDIR=${PUBSW_DIR}/packages/gcc/current
  set HIPPO_MATLAB_DIR=${PUBSW_DIR}/packages/matlab/8.0
  setenv FSLDIR ${PUBSW_DIR}/packages/fsl/current
  setenv DCMTKDIR ${PUBSW_DIR}/packages/dcmtk/current
  setenv GDCMDIR ${PUBSW_DIR}/packages/gdcm/current
  set CPPUNITDIR=${PUBSW_DIR}/packages/cppunit/current
  if ( ! -d ${CPPUNITDIR} ) unset CPPUNITDIR
  setenv AFNIDIR ${PUBSW_DIR}/packages/AFNI/current
endif

# GSL and Qt are no longer used, so they're not defined
unsetenv QTDIR
unsetenv GSLDIR

# on Mac OS X Tiger, need /sw/bin (Fink) to get latex and dvips.
#if ("$OSTYPE" == "Darwin") then
#  setenv PATH "/sw/bin":"$PATH"
#  rehash
#endif


#
# Output log files (OUTPUTF and CVSUPDATEF)
######################################################################
#
set FAILED_FILE=${BUILD_HOSTNAME_DIR}/${RELEASE_TYPE}-build-FAILED
set OUTPUTF=${LOG_DIR}/build_log-${RELEASE_TYPE}-${HOSTNAME}.txt
set GITSTATUSF=${LOG_DIR}/git-status-output-${RELEASE_TYPE}-${HOSTNAME}.txt
rm -f $FAILED_FILE $OUTPUTF $GITSTATUSF
echo "$HOSTNAME $RELEASE_TYPE build" >& $OUTPUTF
chmod g+w $OUTPUTF
set BEGIN_TIME=`date`
echo $BEGIN_TIME >>& $OUTPUTF
set TIME_STAMP=`date +%Y%m%d`
set INSTALL_DIR_TEMP=${INSTALL_DIR}_${TIME_STAMP}


#
# Sanity checks
######################################################################
#
set CURRENT_TIME=`date`
echo "Running Sanity checks: $CURRENT_TIME" >>& $OUTPUTF
if(! -d $SCRIPT_DIR) then 
  echo "$SCRIPT_DIR doesn't exist" >>& $OUTPUTF
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED - sanity"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  exit 1  
endif
if(! -d $SRC_DIR) then 
  echo "$SRC_DIR doesn't exist" >>& $OUTPUTF
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED - sanity"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  exit 1  
endif
if(! -d $BUILD_DIR) then 
  echo "$BUILD_DIR doesn't exist" >>& $OUTPUTF
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED - sanity"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  exit 1  
endif
if(! -d $INSTALL_DIR) then 
  echo "$INSTALL_DIR doesn't exist" >>& $OUTPUTF
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED - sanity"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  exit 1  
endif
if ("${BUILD_PLATFORM}" != "${PLATFORM}") then
  echo "PLATFORM mismatch!" >>& $OUTPUTF
  echo "${LOCAL_FS}/PLATFORM=${PLATFORM}" >>& $OUTPUTF
  echo "${BUILD_HOSTNAME_DIR}/PLATFORM=${BUILD_PLATFORM}" >>& $OUTPUTF
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED - sanity"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  exit 1  
endif


# Source the source_before_building file if it exists
if( -f ${BUILD_HOSTNAME_DIR}/source_before_building.csh ) then
  echo "source ${BUILD_HOSTNAME_DIR}/source_before_building.csh" >>& $OUTPUTF
  source ${BUILD_HOSTNAME_DIR}/source_before_building.csh
endif
echo "##########################################################" >>& $OUTPUTF
echo "Settings" >>& $OUTPUTF
echo "PLATFORM $PLATFORM" >>& $OUTPUTF
echo "HOSTNAME $HOSTNAME" >>& $OUTPUTF
if ($?CC) then
  if ("$CC" == "icc") then
    echo "ICC      `icc --version | grep icc`" >>& $OUTPUTF
  else
    echo "GCC      `gcc --version | grep gcc`" >>& $OUTPUTF
  endif
else
  echo "GCC      `gcc --version | grep gcc`" >>& $OUTPUTF
endif
echo "BUILD_HOSTNAME_DIR $BUILD_HOSTNAME_DIR" >>& $OUTPUTF
echo "SCRIPT_DIR         $SCRIPT_DIR" >>& $OUTPUTF
echo "LOG_DIR            $LOG_DIR" >>& $OUTPUTF
echo "SRC_DIR            $SRC_DIR" >>& $OUTPUTF
echo "BUILD_DIR          $BUILD_DIR" >>& $OUTPUTF
echo "INSTALL_DIR        $INSTALL_DIR" >>& $OUTPUTF
if( $?CFLAGS ) then 
  echo "CFLAGS $CFLAGS" >>& $OUTPUTF
endif
if( $?CPPFLAGS ) then 
  echo "CPPFLAGS $CPPFLAGS" >>& $OUTPUTF
endif
if( $?CXXFLAGS ) then 
  echo "CXXFLAGS $CXXFLAGS" >>& $OUTPUTF
endif
if( $?LDFLAGS ) then 
  echo "LDFLAGS $LDFLAGS" >>& $OUTPUTF
endif
echo "" >>& $OUTPUTF

# in case a new dev dir needed to be created, and the old one cannot
# be deleted because of permissions, then name that old dir 'devold',
# and it will get deleted here:
set DEVOLD=${BUILD_HOSTNAME_DIR}/trunk/devold
if (-e ${DEVOLD}) then
  echo "CMD: rm -Rf ${DEVOLD}" >>& $OUTPUTF
  rm -rf ${DEVOLD} >>& $OUTPUTF
endif
set DEVOLD=${BUILD_HOSTNAME_DIR}/stable/devold
if (-e ${DEVOLD}) then
  echo "CMD: rm -Rf ${DEVOLD}" >>& $OUTPUTF
  rm -rf ${DEVOLD} >>& $OUTPUTF
endif


#
# make distclean
######################################################################
#
set CURRENT_TIME=`date`
echo "Running make distclean: $CURRENT_TIME" >>& $OUTPUTF
echo "##########################################################" >>& $OUTPUTF
echo "" >>& $OUTPUTF
echo "CMD: cd $BUILD_DIR" >>& $OUTPUTF
cd ${BUILD_DIR} >>& $OUTPUTF
if ( "${BUILD_DIR}" == "${SRC_DIR}" ) then
  echo "CMD: make distclean" >>& $OUTPUTF
  if (-e Makefile) make distclean >>& $OUTPUTF
else
  echo "CMD: rm -Rf ${BUILD_DIR}/*" >>& $OUTPUTF
  rm -Rf ${BUILD_DIR}/* >>& $OUTPUTF
endif


#
# Git update
######################################################################
# UPDATE THIS COMMENT
# Go to dev directory and check status. If there are local commits 
# or modified files than exit as there should be neither. 
# Then fetch the origin to update our local repo. If the working copy
# and the local repo do not match than things have changed so
# perform a hard reset (set local working copy to the repo) and build.
# Also, if local files are missing then build. This makes it easy 
# to ensure a build is performed at the next cycle.
# Don't quit if the file FAILED exists, because that means 
# that the last build failed.
set CURRENT_TIME=`date`
set GIT_VERSION=`git --version`
echo "Running git update: $CURRENT_TIME" >>& $OUTPUTF
echo "$GIT_VERSION" >>& $OUTPUTF
echo "##########################################################" >>& $OUTPUTF
echo "Updating $SRC_DIR" >>& $OUTPUTF
echo "" >>& $OUTPUTF
echo "CMD: cd $SRC_DIR" >>& $OUTPUTF
cd ${SRC_DIR} >>& $OUTPUTF
echo "CMD: git status \>\& $GITSTATUSF" >>& $OUTPUTF
git status >& $GITSTATUSF
echo "CMD: grep -E "^\#\? Your branch is ahead of " $GITSTATUSF" >>& $OUTPUTF
grep -E "^#? Your branch is ahead of " $GITSTATUSF >& /dev/null
if ($status == 0) then
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED -  git status: local repo not pushed!"
  echo "$msg" >>& $OUTPUTF
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  exit 1  
endif

echo "CMD: grep -E "^\#\?\[\[:space:\]\]modified:" $GITSTATUSF" >>& $OUTPUTF
grep -E "^#?[[:space:]]modified:" $GITSTATUSF >& /dev/null
if ($status == 0) then
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED - git update: file modified!"
  echo "$msg" >>& $OUTPUTF
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  exit 1  
endif


set local_deletions=0
echo "CMD: grep -E "^\#\?\[\[:space:\]\]deleted:" $GITSTATUSF" >>& $OUTPUTF
grep -E "^#?[[:space:]]deleted:" $GITSTATUSF >& /dev/null
if ($status == 0) then
  set local_deletions=1
  set msg="git status: file deleted!"
  echo "$msg" >>& $OUTPUTF
endif


echo "CMD: git fetch origin \>\>\& $GITSTATUSF" >>& $OUTPUTF
git fetch origin >>& $GITSTATUSF
set SHA_HEAD=`git rev-parse HEAD`
set SHA_HEAD_SHORT=`git rev-parse --short HEAD`
set SHA_ORIGIN=`git rev-parse origin/${BRANCH}`
echo "git rev-parse HEAD = $SHA_HEAD" >>& $OUTPUTF
echo "git rev-parse origin/${BRANCH} = $SHA_ORIGIN" >>& $OUTPUTF

if ($SHA_HEAD == $SHA_ORIGIN && \
    ! -e ${FAILED_FILE} && \
    ! ${FORCE_BUILD} && \
    ! ${local_deletions}) then
  echo "Nothing changed in repository, SKIPPED building" >>& $OUTPUTF
  set msg="$HOSTNAME $RELEASE_TYPE build skipped - no changes since last build"
  mail -s "$msg" $SUCCESS_MAIL_LIST < $OUTPUTF
  rm -f $GITSTATUSF
  exit 0
endif

# Perform a hard reset of the local workspace and repo to the origin/master.
# This step probably isnt required, but it seems safe to me. Annexed files
# that changed are lost when performing a reset. So we must get those as well.
echo "CMD: git reset --hard origin/${BRANCH}  \>\>\& $GITSTATUSF" >>& $OUTPUTF
git reset --hard origin/${BRANCH} >>& $GITSTATUSF
echo "CMD: git annex get . \>\>\& $GITSTATUSF" >>& $OUTPUTF
git annex get . >>& $GITSTATUSF

# assume failure (file removed only after successful build)
rm -f ${FAILED_FILE}
touch ${FAILED_FILE}

echo "CMD: cat $GITSTATUSF \>\>\& $OUTPUTF" >>& $OUTPUTF
cat $GITSTATUSF >>& $OUTPUTF
echo "CMD: rm -f $GITSTATUSF" >>& $OUTPUTF
rm -f $GITSTATUSF
# Git update is now complete


#
# configure
######################################################################
#
set CURRENT_TIME=`date`
echo "Running configure: $CURRENT_TIME" >>& $OUTPUTF
echo "##########################################################" >>& $OUTPUTF
echo "" >>& $OUTPUTF
echo "CMD: cd $BUILD_DIR" >>& $OUTPUTF
cd ${BUILD_DIR} >>& $OUTPUTF
echo "CMD: rm -rf autom4te.cache" >>& $OUTPUTF
if (-e autom4te.cache) rm -rf autom4te.cache >>& $OUTPUTF
echo "CMD: libtoolize --force" >>& $OUTPUTF
if ( "${OSTYPE}" == "Linux") libtoolize --force >>& $OUTPUTF
if ( "${OSTYPE}" == "Darwin") glibtoolize --force >>& $OUTPUTF
echo "CMD: aclocal" >>& $OUTPUTF
aclocal --version >>& $OUTPUTF
aclocal >>& $OUTPUTF
echo "CMD: automake" >>& $OUTPUTF
automake --version >>& $OUTPUTF
automake --add-missing -Wno-portability >>& $OUTPUTF
echo "CMD: autoreconf --force" >>& $OUTPUTF
autoreconf --force >>& $OUTPUTF
echo "CMD: autoconf" >>& $OUTPUTF
autoconf --version >>& $OUTPUTF
autoconf -Wno-portability >>& $OUTPUTF

if ($status != 0) then
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED after automake"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  rm -f ${FAILED_FILE}
  touch ${FAILED_FILE}
  # set group write bit on files changed by make tools:
  echo "CMD: chgrp ${change_flags} fsbuild ${BUILD_DIR}" >>& $OUTPUTF
  chgrp ${change_flags} fsbuild ${BUILD_DIR} >>& $OUTPUTF
  echo "CMD: chmod ${change_flags} g+rw ${BUILD_DIR}" >>& $OUTPUTF
  chmod ${change_flags} g+rw ${BUILD_DIR} >>& $OUTPUTF
  chmod g+rw ${BUILD_DIR}/autom4te.cache >>& $OUTPUTF
  chgrp fsbuild ${BUILD_DIR}/config.h.in >>& $OUTPUTF
  exit 1
endif
echo "CMD: ./configure..." >>& $OUTPUTF
# notice that the configure command sets 'prefix' to ${INSTALL_DIR_TEMP}.  
# later, after make install, ${INSTALL_DIR_TEMP} is moved to ${INSTALL_DIR}.
# this ensure a clean installation and minimizes disruption of machines 
# running recon-all.
set ENAB_NMR="--enable-nmr-install"
if ("${RELEASE_TYPE}" == "stable-pub") then
  # public build doesn't get the extra special stuff
  set ENAB_NMR=""
endif
set cnfgr=(${SRC_DIR}/configure)
set cnfgr=($cnfgr --prefix=${INSTALL_DIR_TEMP})
set cnfgr=($cnfgr $ENAB_NMR)
if (("${RELEASE_TYPE}" == "stable") || \
    ("${RELEASE_TYPE}" == "stable-pub")) then
  set cnfgr=($cnfgr `cat ${BUILD_HOSTNAME_DIR}/stable-configure_options.txt`)
else
  set cnfgr=($cnfgr `cat ${BUILD_HOSTNAME_DIR}/dev-configure_options.txt`)
endif
set cnfgr=($cnfgr --with-mni-dir=${MNIDIR})
if ($?VTKDIR) then
  set cnfgr=($cnfgr --with-vtk-dir=${VTKDIR})
endif
if ($?KWWDIR) then
  set cnfgr=($cnfgr --with-kwwidgets-dir=${KWWDIR})
endif
if ($?EXPATDIR) then
  set cnfgr=($cnfgr --with-expat-dir=${EXPATDIR})
endif
set cnfgr=($cnfgr --with-tcl-dir=${TCLDIR})
set cnfgr=($cnfgr --with-tixwish=${TIXWISH})
if ($?CPPUNITDIR) then
    set cnfgr=($cnfgr --with-cppunit-dir=${CPPUNITDIR})
endif
echo "$cnfgr" >>& $OUTPUTF
$cnfgr >>& $OUTPUTF
if ($status != 0) then
  echo "########################################################" >>& $OUTPUTF
  echo "config.log" >>& $OUTPUTF
  echo "" >>& $OUTPUTF
  cat ${BUILD_DIR}/config.log >>& $OUTPUTF
  set msg="$HOSTNAME $RELEASE_TYPE build FAILED after configure"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  rm -f ${FAILED_FILE}
  touch ${FAILED_FILE}
  # set group write bit on files changed by make tools:
  echo "CMD: chgrp ${change_flags} fsbuild ${BUILD_DIR}" >>& $OUTPUTF
  chgrp ${change_flags} fsbuild ${BUILD_DIR} >>& $OUTPUTF
  echo "CMD: chmod ${change_flags} g+rw ${BUILD_DIR}" >>& $OUTPUTF
  chmod ${change_flags} g+rw ${BUILD_DIR} >>& $OUTPUTF
  chmod g+rw ${BUILD_DIR}/autom4te.cache >>& $OUTPUTF
  chgrp fsbuild ${BUILD_DIR}/config.h.in >>& $OUTPUTF
  exit 1
endif
# save-away the configure string for debug:
head config.log | grep configure | grep prefix > conf
chmod 777 conf


#
# make clean
######################################################################
#
set CURRENT_TIME=`date`
echo "Running make clean: $CURRENT_TIME" >>& $OUTPUTF
echo "##########################################################" >>& $OUTPUTF
echo "" >>& $OUTPUTF
echo "CMD: make clean" >>& $OUTPUTF
if (-e Makefile) make clean >>& $OUTPUTF


#
# make
######################################################################
#
set CURRENT_TIME=`date`
echo "Running make: $CURRENT_TIME" >>& $OUTPUTF
echo "##########################################################" >>& $OUTPUTF
echo "Making $BUILD_DIR" >>& $OUTPUTF
echo "" >>& $OUTPUTF
echo "CMD: cd $BUILD_DIR" >>& $OUTPUTF
cd ${BUILD_DIR} >>& $OUTPUTF
if ("$OSTYPE" == "Darwin") then
  # parallel make (-j 9) seems to cause NFS problems on Mac
  echo "CMD: make -s" >>& $OUTPUTF
  make -s >>& $OUTPUTF
  set mstat = $status
  # stupid Mac OS NFS has intermittent file access failures,
  # resulting in make failures because it cant find stuff in
  # /usr/pubsw/packages, so we will retry make a couple times...
  if ($mstat != 0) then
    sleep 60
    make -s >>& $OUTPUTF
    set mstat = $status
    if ($mstat != 0) then
      sleep 60
      make -s >>& $OUTPUTF
      set mstat = $status
    endif
  endif
else
  echo "CMD: make -j 9 -s" >>& $OUTPUTF
  make -j 9 -s >>& $OUTPUTF
  set mstat = $status
endif
if ($mstat != 0) then
  # note: /usr/local/freesurfer/dev/bin/ dirs have not 
  # been modified (bin/ gets written after make install)
  set msg="$HOSTNAME $RELEASE_TYPE build (make) FAILED"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  rm -f ${FAILED_FILE}
  touch ${FAILED_FILE}
  # set group write bit on files changed by make tools:
  echo "CMD: chgrp ${change_flags} fsbuild ${BUILD_DIR}" >>& $OUTPUTF
  chgrp ${change_flags} fsbuild ${BUILD_DIR} >>& $OUTPUTF
  echo "CMD: chmod ${change_flags} g+rw ${BUILD_DIR}" >>& $OUTPUTF
  chmod ${change_flags} g+rw ${BUILD_DIR} >>& $OUTPUTF
  chmod g+rw ${BUILD_DIR}/autom4te.cache >>& $OUTPUTF
  chgrp fsbuild ${BUILD_DIR}/config.h.in >>& $OUTPUTF
  exit 1  
endif


#
# make check (run available unit tests)
######################################################################
#
set CURRENT_TIME=`date`
echo "Running make check: $CURRENT_TIME" >>& $OUTPUTF
if ($?SKIP_ALL_MAKE_CHECKS) goto make_check_done
#goto make_check_done
if ("$RELEASE_TYPE" != "stable-pub") then
  echo "########################################################" >>& $OUTPUTF
  echo "Make check $BUILD_DIR" >>& $OUTPUTF
  echo "" >>& $OUTPUTF
  echo "CMD: cd $BUILD_DIR" >>& $OUTPUTF
  cd ${BUILD_DIR} >>& $OUTPUTF
  echo "CMD: make check" >>& $OUTPUTF
  make check >>& $OUTPUTF
  set check_status = $status
  if ("$OSTYPE" == "Darwin") then
    # stupid Mac OS NFS has intermittent file access failures,
    # resulting in make failures because it cant find stuff in
    # /usr/pubsw/packages, so we will retry make a couple times...
    if ($check_status != 0) then
      sleep 60
      make check >>& $OUTPUTF
      set check_status = $status
      if ($check_status != 0) then
        sleep 60
        make check >>& $OUTPUTF
        set check_status = $status
      endif
    endif
  endif
  if ($check_status != 0) then
    # note: /usr/local/freesurfer/dev/bin/ dirs have not 
    # been modified (bin/ gets written after make install)
    set msg="$HOSTNAME $RELEASE_TYPE build (make check) FAILED unit tests"
    tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
    rm -f ${FAILED_FILE}
    touch ${FAILED_FILE}
    # set group write bit on files changed by make tools:
    echo "CMD: chgrp ${change_flags} fsbuild ${BUILD_DIR}" >>& $OUTPUTF
    chgrp ${change_flags} fsbuild ${BUILD_DIR} >>& $OUTPUTF
    echo "CMD: chmod ${change_flags} g+rw ${BUILD_DIR}" >>& $OUTPUTF
    chmod ${change_flags} g+rw ${BUILD_DIR} >>& $OUTPUTF
    chmod g+rw ${BUILD_DIR}/autom4te.cache >>& $OUTPUTF
    chgrp fsbuild ${BUILD_DIR}/config.h.in >>& $OUTPUTF
    exit 1  
  endif
endif
make_check_done:


#
# make install
######################################################################
# (recall that configure sets $prefix to $INSTALL_DIR_TEMP instead of $INSTALL_DIR, 
# to minimize disruption of machines using contents of $INSTALL_DIR)
set CURRENT_TIME=`date`
echo "Running make install: $CURRENT_TIME" >>& $OUTPUTF
echo "CMD: rm -Rf ${INSTALL_DIR_TEMP}" >>& $OUTPUTF
if (-e ${INSTALL_DIR_TEMP}) rm -rf ${INSTALL_DIR_TEMP} >>& $OUTPUTF
if ("${RELEASE_TYPE}" == "stable-pub") then
  # make release does make install, and runs some extra commands that
  # remove stuff not intended for public release
  echo "Building public stable" >>& $OUTPUTF
  set make_cmd=(make release)
else
  set make_cmd=(make install)
endif
echo "CMD: cd $BUILD_DIR" >>& $OUTPUTF
cd ${BUILD_DIR} >>& $OUTPUTF
echo "$make_cmd" >>& $OUTPUTF
$make_cmd >>& $OUTPUTF
set makestatus=($status)
if ("$OSTYPE" == "Darwin") then
  # stupid Mac OS NFS has intermittent file access failures,
  # resulting in make failures because it cant find stuff in
  # /usr/pubsw/packages, so we will retry make a couple times...
  if ($makestatus != 0) then
    sleep 60
    $make_cmd >>& $OUTPUTF
    set makestatus=($status)
    if ($makestatus != 0) then
      sleep 60
      $make_cmd >>& $OUTPUTF
      set makestatus=($status)
    endif
  endif
endif
if ($makestatus != 0) then
  set msg="$HOSTNAME $RELEASE_TYPE build ($make_cmd) FAILED"
  tail -n 20 $OUTPUTF | mail -s "$msg" $FAILURE_MAIL_LIST
  rm -f ${FAILED_FILE}
  touch ${FAILED_FILE}
  # set group write bit on files changed by make tools:
  echo "CMD: chgrp ${change_flags} fsbuild ${BUILD_DIR}" >>& $OUTPUTF
  chgrp ${change_flags} fsbuild ${BUILD_DIR} >>& $OUTPUTF
  echo "CMD: chmod ${change_flags} g+rw ${BUILD_DIR}" >>& $OUTPUTF
  chmod ${change_flags} g+rw ${BUILD_DIR} >>& $OUTPUTF
  chmod g+rw ${BUILD_DIR}/autom4te.cache >>& $OUTPUTF
  chgrp fsbuild ${BUILD_DIR}/config.h.in >>& $OUTPUTF
  # and the fsaverage in the subjects dir...
  echo "CMD: chmod ${change_flags} g+rw ${INSTALL_DIR_TEMP}/subjects/fsaverage" \
    >>& $OUTPUTF
  chmod ${change_flags} g+rw ${INSTALL_DIR_TEMP}/subjects/fsaverage >>& $OUTPUTF
  chgrp ${change_flags} fsbuild ${INSTALL_DIR_TEMP}/subjects/fsaverage >>& $OUTPUTF
  exit 1  
endif
# delete the files in the EXCLUDE_FILES_rm_cmds list which were created from
# the EXCLUDE_FILES section of each Makefile.am by the top-level Makefile.extra
# when make release was run (which is done only during a stable-pub build)
if ( -e {$INSTALL_DIR_TEMP}/EXCLUDE_FILES_rm_cmds ) then
  echo "CMD: cat ${INSTALL_DIR_TEMP}/EXCLUDE_FILES_rm_cmds" >>& $OUTPUTF
  cat ${INSTALL_DIR_TEMP}/EXCLUDE_FILES_rm_cmds >>& $OUTPUTF
  echo "CMD: source ${INSTALL_DIR_TEMP}/EXCLUDE_FILES_rm_cmds" >>& $OUTPUTF
  source ${INSTALL_DIR_TEMP}/EXCLUDE_FILES_rm_cmds >>& $OUTPUTF
  echo "CMD: mv -f ${INSTALL_DIR_TEMP}/EXCLUDE_FILES_rm_cmds /tmp" >>& $OUTPUTF
  mv -f ${INSTALL_DIR_TEMP}/EXCLUDE_FILES_rm_cmds /tmp
endif

# strip symbols from binaries, greatly reducing their size
# do not strip eugenios segment subject binaries because
# the 'strip' command corrupts them.
echo "CMD: strip -v ${INSTALL_DIR_TEMP}/bin/*" >>& $OUTPUTF
cd ${INSTALL_DIR_TEMP}/bin
foreach f ( `ls -d *` )
  if (("$f" =~ segmentSubject*) || ("$f" =~ SegmentSubject*) || ("$f" =~ *glnxa64)) then
    echo "Not Stripping $f" >>& ${OUTPUTF}
  else
    strip -v $f >>& ${OUTPUTF}
  endif
end
cd -

#
# Move INSTALL_DIR to INSTALL_DIR.bak 
# Move newly created INSTALL_DIR_TEMP to INSTALL_DIR
# This series of mv's minimizes the time window where the INSTALL_DIR directory
# would appear empty to a machine trying to reference its contents in recon-all
if ( -d ${INSTALL_DIR}.bak ) then
  echo "CMD: rm -Rf ${INSTALL_DIR}.bak" >>& $OUTPUTF
  rm -Rf ${INSTALL_DIR}.bak >>& $OUTPUTF
endif
if ( -d ${INSTALL_DIR} ) then
  echo "CMD: mv ${INSTALL_DIR} ${INSTALL_DIR}.bak" >>& $OUTPUTF
  mv ${INSTALL_DIR} ${INSTALL_DIR}.bak >>& $OUTPUTF
endif
echo "CMD: mv ${INSTALL_DIR_TEMP} ${INSTALL_DIR}" >>& $OUTPUTF
mv ${INSTALL_DIR_TEMP} ${INSTALL_DIR} >>& $OUTPUTF

# 
# Point to a  license file
#
setenv LICENSE_FILE "../../.license"
echo "CMD: ln -s ${LICENSE_FILE} ${INSTALL_DIR}/.license" >>& $OUTPUTF
ln -s ${LICENSE_FILE} ${INSTALL_DIR}/.license >>& $OUTPUTF

#
# fix qdec.bin for centos6_x86_64: for as yet unknown reasons, qdec core
# dumps on centos6_x86_64 (std::length_error) but the centos4_x86_64
# version works, so copy it over
if ("$PLATFORM" == "centos6_x86_64") then
  echo "cp /space/freesurfer/centos4.0_x86_64/${RELEASE_TYPE}/bin/qdec.bin ${INSTALL_DIR}/bin" >>& $OUTPUTF
  cp /space/freesurfer/centos4.0_x86_64/${RELEASE_TYPE}/bin/qdec.bin ${INSTALL_DIR}/bin
endif


#
# make install is now complete, and /bin dir is now setup with new code
#
echo "##########################################################" >>& $OUTPUTF
echo "Setting permissions" >>& $OUTPUTF
echo "" >>& $OUTPUTF
echo "CMD: chgrp ${change_flags} fsbuild ${INSTALL_DIR}" >>& $OUTPUTF
chgrp ${change_flags} fsbuild ${INSTALL_DIR} >>& $OUTPUTF
echo "CMD: chmod ${change_flags} g+rw ${INSTALL_DIR}" >>& $OUTPUTF
chmod ${change_flags} g+rw ${INSTALL_DIR} >>& $OUTPUTF
echo "CMD: chgrp ${change_flags} fsbuild ${BUILD_DIR}" >>& $OUTPUTF
chgrp ${change_flags} fsbuild ${BUILD_DIR} >>& $OUTPUTF
echo "CMD: chmod ${change_flags} g+rw ${BUILD_DIR}" >>& $OUTPUTF
chmod ${change_flags} g+rw ${BUILD_DIR} >>& $OUTPUTF
chmod g+rw ${BUILD_DIR}/autom4te.cache >>& $OUTPUTF
chgrp fsbuild ${BUILD_DIR}/config.h.in >>& $OUTPUTF
echo "CMD: chmod ${change_flags} g+rw ${LOG_DIR}" >>& $OUTPUTF
chmod ${change_flags} g+rw ${LOG_DIR} >>& $OUTPUTF


#
# make distcheck
######################################################################
# make distcheck creates a source tarball (as does make dist) and then
# checks that the tarball works by untarring into a _build directory,
# runs make, then make check, make install, and make uninstall.
#goto make_distcheck_done
set CURRENT_TIME=`date`
echo "Running make distcheck: $CURRENT_TIME" >>& $OUTPUTF
if (("$RELEASE_TYPE" == "stable") || \
    ("$RELEASE_TYPE" == "dev")) then
# just run on terrier
if ("$HOSTNAME" == "terrier") then
# just do this once a week, as it takes a few hours to run
date | grep "Sat " >& /dev/null
if ( ! $status ) then
  echo "########################################################" >>& $OUTPUTF
  echo "Make distcheck $BUILD_DIR" >>& $OUTPUTF
  echo "" >>& $OUTPUTF
  echo "CMD: cd $BUILD_DIR" >>& $OUTPUTF
  cd ${BUILD_DIR} >>& $OUTPUTF
  echo "CMD: make distcheck" >>& $OUTPUTF
  make distcheck >>& $OUTPUTF
  if ($status != 0) then
    set msg="$HOSTNAME $RELEASE_TYPE build FAILED make distcheck"
#
# HACK: mail to zkaufman for now, until it passes regularly
#
    tail -n 20 $OUTPUTF | mail -s "$msg" zkaufman@nmr.mgh.harvard.edu
    rm -f ${FAILED_FILE}
    touch ${FAILED_FILE}
    # set group write bit on files changed by make tools:
    echo "CMD: chgrp ${change_flags} fsbuild ${BUILD_DIR}" >>& $OUTPUTF
    chgrp ${change_flags} fsbuild ${BUILD_DIR} >>& $OUTPUTF
    echo "CMD: chmod ${change_flags} g+rw ${BUILD_DIR}" >>& $OUTPUTF
    chmod ${change_flags} g+rw ${BUILD_DIR} >>& $OUTPUTF
    chmod g+rw ${BUILD_DIR}/autom4te.cache >>& $OUTPUTF
    chgrp fsbuild ${BUILD_DIR}/config.h.in >>& $OUTPUTF
# HACK: dont exit:
#    exit 1
  else
    set msg="$HOSTNAME $RELEASE_TYPE build PASSED make distcheck"
    tail -n 20 $OUTPUTF | mail -s "$msg" zkaufman@nmr.mgh.harvard.edu
  endif
endif
endif
endif
make_distcheck_done:


#
# library and sample subject symlinks
######################################################################
# ensure that the symlinks to the necessary packages are in place
#
set CURRENT_TIME=`date`
echo "Running simlinks: $CURRENT_TIME" >>& $OUTPUTF
symlinks:

  # first remove existing links
  rm -f ${INSTALL_DIR}/mni
  rm -f ${INSTALL_DIR}/fsl
  rm -f ${INSTALL_DIR}/lib/tcltktixblt
  rm -f ${INSTALL_DIR}/lib/vtk
  rm -f ${INSTALL_DIR}/lib/vxl
  rm -f ${INSTALL_DIR}/lib/misc
  # then setup for proper installation
  set cmd1=(ln -s ${MNIDIR} ${INSTALL_DIR}/mni)
  set cmd2=
  set cmd3=(ln -s ${TCLDIR} ${INSTALL_DIR}/lib/tcltktixblt)
  set cmd4=
  if ($?VTKDIR) then
    set cmd5=(ln -s ${VTKDIR} ${INSTALL_DIR}/lib/vtk)
  else
    set cmd5=
  endif
  set cmd6=(ln -s ${HIPPO_MATLAB_DIR} ${INSTALL_DIR}/MCRv80)
  set cmd7=
  set cmd8=
  # execute the commands
  echo "$cmd1" >>& $OUTPUTF
  $cmd1
  echo "$cmd2" >>& $OUTPUTF
  $cmd2
  echo "$cmd3" >>& $OUTPUTF
  $cmd3
  echo "$cmd4" >>& $OUTPUTF
  $cmd4
  echo "$cmd5" >>& $OUTPUTF
  $cmd5
  echo "$cmd6" >>& $OUTPUTF
  $cmd6
  echo "$cmd7" >>& $OUTPUTF
  $cmd7
  echo "$cmd8" >>& $OUTPUTF
  $cmd8
  # also setup sample subject:
  rm -f ${INSTALL_DIR}/subjects/bert
  set cmd=(ln -s ${SPACE_FS}/subjects/test/bert.${OSTYPE} ${INSTALL_DIR}/subjects/bert)
  echo "$cmd" >>& $OUTPUTF
  $cmd

# 
# Platform dependant symlinking.
######################################################################
#
set CURRENT_TIME=`date`
if ("$OSTYPE" == "Darwin") then
  rm ${INSTALL_DIR}/lib/qt
  ln -s ${GCCDIR} ${INSTALL_DIR}/lib/gcc
  cp ${INSTALL_DIR}/lib/gcc/lib/* ${INSTALL_DIR}/Freeview.app/Contents/Frameworks
  install_name_tool -change /opt/local/lib/gcc44/libgcc_s.1.dylib \
                            @executable_path/../Frameworks/libgcc_s.1.dylib \
                            ${INSTALL_DIR}/Freeview.app/Contents/MacOS/Freeview
  install_name_tool -change /opt/local/lib/libgcc/libstdc++.6.dylib \
                            @executable_path/../Frameworks/libstdc++.6.dylib \
                            ${INSTALL_DIR}/Freeview.app/Contents/MacOS/Freeview
  install_name_tool -change /opt/local/lib/libgcc/libgomp.1.dylib \
                            @executable_path/../Frameworks/libgomp.1.dylib \
                            ${INSTALL_DIR}/Freeview.app/Contents/MacOS/Freeview
  # As of Mac 10.11 DYLD_LIBRARY_PATH is no longer repected. So we must
  # checnge the location within the library to point to the local lib.
  foreach i (`find ${INSTALL_DIR} -type f`)
    file $i | grep "Mach-O 64-bit executable" >& /dev/null
    if ($status == 0) then
      install_name_tool -change /opt/local/lib/libgcc/libgomp.1.dylib @executable_path/../lib/gcc/lib/libgomp.1.dylib ${i}
      install_name_tool -change /opt/local/lib/libgcc/libstdc++.6.dylib @executable_path/../lib/gcc/lib/libstdc++.6.dylib ${i}
      install_name_tool -change /opt/local/lib/gcc44/libgcc_s.1.dylib @executable_path/../lib/gcc/lib/libgcc_s.1.dylib ${i}
      install_name_tool -change /opt/local/lib/libgcc/libgfortran.3.dylib @executable_path/../lib/gcc/lib/libgfortran.3.dylib ${i}
    endif
  end
endif

# Until we are building 64-bit GUIs on the Mac, we need to link
# to the 32-bit versions of vtk, and KWWidgets.
if ("$PLATFORM" =~ OSX*) then
  echo "executing fix_mac_libs.csh" >>& $OUTPUTF
  ${SPACE_FS}/build/scripts/fix_mac_libs.csh ${RELEASE_TYPE}
endif

#
# create build-stamp.txt
######################################################################
# create a build-stamp file, containing some basic info on this build
# which is displayed when FreeSurferEnv.csh is executed.  
# its also used by create_targz to name the tarball.
# Note: the stable build version info is hard-coded here! so it
# should be updated here with each release update
set DATE_STAMP="`date +%Y%m%d`"
set FS_PREFIX="freesurfer-${OSTYPE}-${PLATFORM}-${RELEASE_TYPE}"
if ("$RELEASE_TYPE" == "dev") then
  echo "${FS_PREFIX}-${DATE_STAMP}-${SHA_HEAD_SHORT}" > ${INSTALL_DIR}/build-stamp.txt
else if ("$RELEASE_TYPE" == "stable") then
  echo "${FS_PREFIX}-${STABLE_VER_NUM}-${DATE_STAMP}" \
    > ${INSTALL_DIR}/build-stamp.txt
else if ("$RELEASE_TYPE" == "stable-pub") then
  echo "${FS_PREFIX}-${STABLE_PUB_VER_NUM}" > ${INSTALL_DIR}/build-stamp.txt
else
  echo "ERROR: unknown RELEASE_TYPE: $RELEASE_TYPE"
  exit 1
endif


#
# create tarball
######################################################################
# If building stable-pub, then create a tarball
set CURRENT_TIME=`date`
echo "Running create tarball: $CURRENT_TIME" >>& $OUTPUTF
if (("$RELEASE_TYPE" == "stable-pub") || \
    ("$RELEASE_TYPE" == "dev") || \
    ("$STABLE_VER_NUM" == "v6-beta") || \
    ( -e ${BUILD_HOSTNAME_DIR}/TARBALL)) then
  if ( ! $?SKIP_CREATE_TARBALL) then
    set cmd=($SCRIPT_DIR/create_targz.csh $PLATFORM $RELEASE_TYPE)
    echo "$cmd" >>& $OUTPUTF
    $cmd >>& $OUTPUTF
    if ($status) then
      echo "create_targz.csh failed to create tarball!"
      # don't exit with error, since create_targz can be re-run manually
    endif
    rm -f ${BUILD_HOSTNAME_DIR}/TARBALL >& /dev/null
  endif
endif


#
# copy libraries and include filed needed by those who want to 
# build against the freesurfer enviro (NMR center only)
######################################################################
#
set CURRENT_TIME=`date`
echo "Running copy libraries: $CURRENT_TIME" >>& $OUTPUTF
if ("$RELEASE_TYPE" == "dev") then
  # remove existing 
  set cmd=(rm -Rf ${INSTALL_DIR}/lib/dev)
  echo "$cmd" >>& $OUTPUTF
  $cmd >>& $OUTPUTF
  set cmd=(rm -Rf ${INSTALL_DIR}/include)
  echo "$cmd" >>& $OUTPUTF
  $cmd >>& $OUTPUTF
  set cmd=(mkdir -p ${INSTALL_DIR}/lib/dev)
  echo "$cmd" >>& $OUTPUTF
  $cmd >>& $OUTPUTF
  # cp necessary libs
  set dirlist=(${BUILD_DIR}/dicom/libdicom.a \
	${BUILD_DIR}/hipsstubs/libhipsstubs.a \
	${BUILD_DIR}/rgb/librgb.a \
	${BUILD_DIR}/unix/libunix.a \
	${BUILD_DIR}/utils/libutils.a \
	${BUILD_DIR}/fsgdf/libfsgdf.a )
  set cmd=(cp $dirlist ${INSTALL_DIR}/lib/dev)
  echo "$cmd" >>& $OUTPUTF
  $cmd >>& $OUTPUTF
  # cp include dir
  set cmd=(cp -r ${BUILD_DIR}/include ${INSTALL_DIR})
  echo "$cmd" >>& $OUTPUTF
  $cmd >>& $OUTPUTF
  # make sure all files in INSTALL_DIR are group writable
  set cmd=(chmod ${change_flags} g+rw ${INSTALL_DIR})
  echo "$cmd" >>& $OUTPUTF
  $cmd >>& $OUTPUTF
endif


# Success, so remove fail indicator:
rm -rf ${FAILED_FILE}


done:

# Finish up
######################################################################
#
echo "##########################################################" >>& $OUTPUTF
echo "Done." >>& $OUTPUTF
set END_TIME=`date`
echo $END_TIME >>& $OUTPUTF

# Move log file to stamped version.
chmod g+w $OUTPUTF
mv $OUTPUTF ${LOG_DIR}/build_log-$RELEASE_TYPE-$HOSTNAME-$TIME_STAMP.txt
gzip -f ${LOG_DIR}/build_log-$RELEASE_TYPE-$HOSTNAME-$TIME_STAMP.txt

# Send email.
echo "Begin ${BEGIN_TIME}, end ${END_TIME}" >& $LOG_DIR/message-$HOSTNAME.txt
set msg="$HOSTNAME $RELEASE_TYPE build is wicked awesome."
mail -s "$msg" $SUCCESS_MAIL_LIST < $LOG_DIR/message-$HOSTNAME.txt
rm $LOG_DIR/message-$HOSTNAME.txt

#
# Now for a cheap way to build stable-pub, which is normally only run
# when a public distribution is needed.  Just create an empty file
# called PUB in the BUILD_HOSTNAME_DIR, and stable-pub will be built.
if ("$RELEASE_TYPE" == "stable") then
  if (-e ${BUILD_HOSTNAME_DIR}/PUB) then
    rm -f ${BUILD_HOSTNAME_DIR}/PUB
    # force stable build to run again by removing a CVS'd file:
    rm -f ${SRC_DIR}/setup_configure
    ${SCRIPT_DIR}/build_stable-pub.csh
  endif
endif
