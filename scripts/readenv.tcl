#! /usr/local/bin/tclsh7.4
###########################################################################
# read names/values of non-cap setenv vars to override default parms
# in csh scripts:
#   global:     setenv parm value
#   singlerun:  setenv $rgbname.parm value
# NOTE:
#   setenv (but not set!) can set varname longer than 20char csh limit
#   tcl (but not csh!) can read such over-20-char-long env vars
#   set,setenv (but not tcl set!) create empty var w/o arg (tcl returns err!)
###########################################################################

### first read lower-case global env vars
puts "readenv.tcl: ==> read global setenv vars"
foreach var [array names env] {
  if { [string range $var 0 0] >= "a" } {
    set root [file root $var]
    set extension [string trimleft [file extension $var] .]
    if { [string length $extension] == 0 } {    # no extension: global
      set $var $env($var)
      puts "readenv: set $var $env($var)"
    }
  }
}

### then read lower-case rgbname-specific env vars (overwrite global)
if [info exists rgbname] {
  puts "readenv.tcl: ==> read rgbname-specific setenv vars"
  foreach var [array names env] {
    if { [string range $var 0 0] >= "a" } {
      set root [file root $var]
      set extension [string trimleft [file extension $var] .]
      if { [string length $extension] > 0 } {           # if there is ext
        if { [string compare $rgbname $root] == 0 } {   # if correct root
          set $extension $env($var)
          puts "readenv: set $extension $env($var)"
        }
      }
    }
  }
}

### maybe just print defaults
if [info exists justvars] {
  puts "readenv.tcl: ==> print defaults"
  source $env(FREESURFER_HOME)/lib/tcl/printdef.tcl
  exit
}

prompt


