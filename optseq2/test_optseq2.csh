#!/bin/tcsh -ef

#############################################################################
#
# Name:    test_optseq2.csh
# Purpose: execute optseq2 and check results against expected results
# Usage:
#
#   test_optseq2.csh
#
#############################################################################

set VERSION='$Id: test_optseq2.csh,v 2.3 2006/09/22 17:31:29 nicks Exp $'

umask 002

set WD=$PWD

#
# gunzip arch-specific expected results and 
# set location where test data is to be created
#

set EXPECTED=$WD/test_data
cd $EXPECTED
if (-e emot.sum) rm -Rf emot*
set PROC=`uname -p`
set cmd=(tar zxvf $EXPECTED/$PROC-emot.tar.gz)
echo $cmd
$cmd
if ($status != 0) then
  echo "Unable to extract architecture-specific test data!"
  exit 1
endif
chmod 666 emot*

set ACTUAL=/tmp/optseq2
if (-e $ACTUAL) rm -Rf $ACTUAL
mkdir -p $ACTUAL
if (! -e $ACTUAL) then
    echo "test_optseq2 FAILED to create directory $ACTUAL"
    exit 1
endif

#
# run optseq2 using typical input args (and the same as those used
# to produce the expected results)
#

set cmd=($WD/test_data/create_optseq2_data.csh)
cd $ACTUAL
echo $cmd
$cmd
if ($status != 0) then
  echo "create_optseq2_data FAILED"
  chmod -R 777 $ACTUAL
  exit 1
endif
chmod -R 777 $ACTUAL

#
# compare expected results with actual (produced) results
#

set TEST_FILES=( emot-001.par emot-002.par emot-003.par emot-004.par emot.sum emot.log )
foreach tstfile ($TEST_FILES)
  set cmd=(diff $EXPECTED/$tstfile $ACTUAL/$tstfile);
  echo $cmd
  $cmd
  set diff_status=$status
  if ($diff_status != 0) then
    echo "$cmd FAILED (exit status=$diff_status)"
    exit 1
  endif
end

echo "test_optseq2 passed all tests"
exit 0
