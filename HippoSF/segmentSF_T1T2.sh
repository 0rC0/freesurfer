#!/bin/tcsh

if (  $#argv < 6 ) then
  echo "Software requires 6 arguments"
  echo echo "segmentSF_T1T2.sh matlabRuntimeDirectory FShomeDirectory subjectName subjectDir additionalImageVolume side(must be left or right, in lower case) "
  exit 1
endif

# Absolute name of script
set rootdir = `dirname $0`
set SCRIPTPATH = `cd $rootdir && pwd`

# Parameters
set RUNTIME=$1;
set FREESURFER_HOME=$2;
set SUBJECTNAME=$3;
set SUBJECTDIR = `cd $4 && pwd`

set AUX = `dirname $5`
set T2DIR = `cd $AUX && pwd`
set T2NAME = `basename $5`
set T2VOL=$T2DIR/$T2NAME

set RESOLUTION="0.333333333333333333333333333333333333";
set ATLASMESH="$FREESURFER_HOME/average/HippoSF/atlas/AtlasMesh.gz";
set ATLASDUMP="$FREESURFER_HOME/average/HippoSF/atlas/AtlasDump.mgz";
set LUT="$FREESURFER_HOME/average/HippoSF/atlas/compressionLookupTable.txt";
set K="0.05";
set SIDE=$6;
set OPTIMIZER="ConjGrad";
set MRFCONSTANT="0";
set SUFFIX="T1_${T2NAME}_v1.0";
set BYPASSBF="1";
set USEWHOLEBRAININHP="0";

# command
set cmd="$SCRIPTPATH/run_segmentSubjectT1T2_autoEstimateAlveusML.sh $RUNTIME $SUBJECTNAME $SUBJECTDIR $T2VOL $RESOLUTION $ATLASMESH $ATLASDUMP $LUT $K $SIDE $OPTIMIZER $SUFFIX ${FREESURFER_HOME}/bin/ $MRFCONSTANT $BYPASSBF $USEWHOLEBRAININHP"

# echo $cmd

eval $cmd

exit

