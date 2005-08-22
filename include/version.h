// Warning: Do not edit the following four lines.  CVS maintains them.
// Revision Author: $Author $
// Revision Date  : $Date $
// Revision       : $Revision $


#ifndef VERSION_H
#define VERSION_H

/* This function looks for the --version, or -version tag in the
   argv and if found, prints out version information. This can be used
   in any binary. It will return the number of options processed and
   copy the remaining items in argv back, so the caller can shorten
   their own argc variable, like this:

    nargs = handle_version_option (argc, argv, 
                                   "dollarIDdollar", "dollarNamedollar");
    argc -= nargs ;

   (This is done so that it can easily be used with older programs
   that make assumptions about argument counts instead of scanning for
   options.)

   It will print out the id string passed in, which should just be
   dollar sign + ID + dollar sign, which CVS will expand to include
   the CVS information including CVS file, revision number, date,
   peson who checked it in, and tag, as well as the OS and GCC
   information.

   The binary may also want to exit if there are no other options to
   handle, i.e.

    nargs = handle_version_option (argc, argv, 
                                   "dollarIDdollar", "dollarNamedollar");
    if (nargs && argc - nargs == 1)
      exit (0);
    argc -= nargs;

   This will also handle the --all-info option which prints out
   more information in the format necessary for the BIRN provenance
   spec, including the version_string.

*/

int handle_version_option (int argc, char** argv, 
			   char* id_string, char* version_string);
int make_cmd_version_string (int argc, char** argv,  char* id_string,
			     char* version_string, char *return_string) ;
char *argv2cmdline(int argc, char *argv[]);

#endif
