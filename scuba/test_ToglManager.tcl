#!/bin/sh
# the next line restarts using wish \
exec wish "$0" "$@"

##
## test_ToglManager.tcl
##
## CVS Revision Info:
##    $Author: nicks $
##    $Date: 2007/01/05 00:21:50 $
##    $Revision: 1.3 $
##
## Copyright (C) 2002-2007,
## The General Hospital Corporation (Boston, MA). 
## All rights reserved.
##
## Distribution, usage and copying of this software is covered under the
## terms found in the License Agreement file named 'COPYING' found in the
## FreeSurfer source code root directory, and duplicated here:
## https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
##
## General inquiries: freesurfer@nmr.mgh.harvard.edu
## Bug reports: analysis-bugs@nmr.mgh.harvard.edu
##

load [file dirname [info script]]/test_ToglManager[info sharedlibextension] Test_toglmanager

set gCurWindowID 0
proc GetNewWindowID { } {
    global gCurWindowID
    incr gCurWindowID
    return $gCurWindowID;
}

proc CreateWindow { } {

    set windowID [GetNewWindowID]

    set ww         .ww$windowID
    set fwTop      $ww.fwTop
    set twMain     $fwTop.twMain

    set fwControls  $ww.fwControls
    set bwNewWindow $fwControls.bwNewWindow

    toplevel $ww

    frame $fwTop
    togl $twMain -width 300 -height 300 -rgba true -ident $windowID

    frame $fwControls
    button $bwNewWindow -text "New Window" \
	-command CreateWindow

    bind $twMain <Motion> "%W MouseMotionCallback %x %y %b"
    bind $twMain <ButtonPress> "%W MouseDownCallback %x %y %b"
    bind $twMain <ButtonRelease> "%W MouseUpCallback %x %y %b"
    bind $twMain <KeyRelease> "%W KeyUpCallback %x %y %K"
    bind $twMain <KeyPress> "%W KeyDownCallback %x %y %K"
    bind $twMain <Enter> "focus $twMain"
    bind $twMain <FocusOut> "%W ExitCallback"

    pack $twMain -fill both -expand true
    pack $bwNewWindow -side left

    pack $fwTop -fill both -expand true
    pack $fwControls -fill x -expand true

    puts "tcl: created window $windowID"
}


CreateWindow


wm withdraw .
