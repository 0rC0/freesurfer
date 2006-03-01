#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#ifdef Darwin
// Mac OS X doesnt have gnulib, so it doesn't have getline, 
// so the source for getline is found here in utils and built on the Mac
#include "getline.h"
#endif 
#include "path.h"
#include "error.h"

int PathReadMany (char *fname, int *num_read, PATH ***returned_paths)
{
  FILE* fp;
  int num_scanf;
  int line_number;
  int path_pno;
  int num_points;
  char* line = NULL;
  size_t line_size = 1024;
  int version;
  PATH* path = NULL;
  PATH** paths = NULL;
  int num_paths = 0;
  float x, y, z;

  /* Try opening the file. */
  fp = fopen (fname, "r");
  if (NULL == fp)
    {
      ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
                                 "Couldn't open %s\n",fname));
    }
  line_number = 0;

  line = (char*) malloc (line_size);
  
  /* Look for keywords... */
  while (!feof(fp))
    {
      getline (&line, &line_size, fp);
      line_number++;

      /* Skip comments. */
      if (line[0] == '#')
	continue;
      
      /* If this is the end of file, go to the end. */
      if (feof(fp))
	continue;

      /* VERSION keyword */
      if (0 == strncmp (line, "VERSION", 7))
	{
	  /* See if we recognize this version number. */
	  num_scanf = sscanf (line, "VERSION %d", &version);
	  if (1 != num_scanf)
	    {
	      fclose (fp);
	      free (line);
	      if (paths) free (paths);
	      if (path) free (path);
	      ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
					 "Error reading file %s\n"
					 "     line number %d\n"
					 "     couldn't read version number\n",
					 fname, line_number));
	    }
	  if (1 != version)
	    {
	      fclose (fp);
	      free (line);
	      if (paths) free (paths);
	      if (path) free (path);
	      ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
					 "Error reading file %s\n"
					 "     wrong version %d\n",
					 fname, version));
	    }
	}
      else if (0 == strncmp (line, "BEGINPATH", 9))
	{
	  /* Start a new path decsription. */
	  getline (&line, &line_size, fp);
	  line_number++;
	  if (0 != strncmp (line, "NUMVERTICES", 11) &&
	      0 != strncmp (line, "NUMPOINTS", 9))
	    {
	      fclose (fp);
	      free (line);
	      if (paths) free (paths);
	      if (path) free (path);
	      ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
					 "Error reading file %s\n"
					 "     line number %d\n"
					 "     expected NUMVERTICES\n",
					 fname, line_number));
	    }

	  /* Scan for the number of vertices. */
	  num_scanf = sscanf (line, "NUMVERTICES %d", &num_points);
	  if (1 != num_scanf)
	    num_scanf = sscanf (line, "NUMPOINTS %d", &num_points);
	  if (1 != num_scanf || feof(fp))
	    {
	      fclose (fp);
	      free (line);
	      if (paths) free (paths);
	      if (path) free (path);
	      ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
					 "Error reading file %s\n"
					 "     line number %d\n"
				       "     couldn't read NUMPOINTS number\n",
					 fname, line_number));
	    }

	  /* Allocate our path object. */
	  path = PathAlloc (num_points, fname);
	  if (NULL == path)
	    {
	      fclose (fp);
	      free (line);
	      if (paths) free (paths);
	      if (path) free (path);
	      ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
					 "Error creating path of\n"
					 "     size %d\n"
					 "     line number %d\n"
				       "     couldn't read NUMPOINTS number\n",
					 num_points, fname, line_number));
	    }

	  /* Read in a line of coordinates for every point we
	     have. */
	  for (path_pno = 0; path_pno < num_points; path_pno++)
	    {
	      getline (&line, &line_size, fp);
	      line_number++;
	      num_scanf = sscanf (line, "%f %f %f", &x, &y, &z);
	      if (3 != num_scanf || feof(fp))
		{
		  fclose (fp);
		  free (line);
		  free (path);
		  if (paths) free (paths);
		  if (path) free (path);
		  ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
					     "Error reading file %s\n"
					     "     line number %d\n"
					   "     couldn't read three floats\n",
					     fname, line_number));
		}

	      /* Add this coordinate to our label. */
	      path->points[path_pno].x = x;
	      path->points[path_pno].y = y;
	      path->points[path_pno].z = z;
	    }

	  /* Make sure we got the ENDPATH keyword. */
	  getline (&line, &line_size, fp);
	  line_number++;
	  if (0 != strncmp (line, "ENDPATH", 7))
	    {
	      fclose (fp);
	      free (line);
	      if (paths) free (paths);
	      if (path) free (path);
	      ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
					 "Error reading file %s\n"
					 "     line number %d\n"
					 "     expected ENDPATH\n",
					 fname, line_number));
	    }

	  /* Add the path to our array. */
	  if (NULL == paths) 
	    {
	      paths = (PATH**) calloc (num_paths+1, sizeof(PATH*));
	    } else {
	      paths = (PATH**) realloc (paths, (num_paths+1) * sizeof(PATH*));
	    }
	  paths[num_paths] = path;
	  num_paths++;
	  path = NULL;
	}
      else
	{
	  /* Didn't get a keyword we're looking for. */
	  ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
				     "Error reading file %s\n"
				     "     line number %d\n"
				     "     no expected keyword found\n",
				     fname, line_number));
	}
    }

  free (line);
  fclose (fp);

  *num_read = num_paths;
  *returned_paths = paths;
  
  return (ERROR_NONE);
}


int PathWriteMany (char *fname, int num_paths, PATH **paths)
{
  FILE* fp;
  int path;
  int path_pno;

  /* Try to open the file. */  
  fp = fopen (fname, "w");
  if (NULL == fp)
    {
      ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,
                                 "Couldn't open %s\n",fname));
    }

  /* Write some header info. */
  fprintf (fp, "# Path file\n");

  /* Version keyword. */
  fprintf (fp, "VERSION 1\n");

  /* For each path... */
  for (path = 0; path < num_paths; path++)
    {
      /* Add BEGINPATH and NUMVERTICES keywords and info. */
      fprintf (fp, "BEGINPATH\n");
      fprintf (fp, "NUMPOINTS %d\n", paths[path]->n_points);

      /* For each vertex, write a line with the coordinate on it. */
      for (path_pno = 0; path_pno < paths[path]->n_points; path_pno++)
	{
	  fprintf (fp, "%f %f %f\n", 
		   paths[path]->points[path_pno].x,
		   paths[path]->points[path_pno].y,
		   paths[path]->points[path_pno].z);
	}
      
      /* ENDPATH keyword. */
      fprintf (fp, "ENDPATH\n");
    }

  fclose (fp);
  
  return (ERROR_NONE);
}

PATH* PathAlloc (int n_points, char* name)
{
  PATH* path;

  if (n_points < 0)
    return NULL;
  
  /* Allocate path struct. */
  path = (PATH*) malloc (sizeof(PATH));
  if (NULL == path)
    {
      printf ("ERROR: Couldn't allocate path.\n");
      return NULL;
    }

  /* Set the number of points. */
  path->n_points = n_points;

  /* Copy in a name. */
  if (NULL != name )
    strncpy (path->name, name, 100);
  else
    strcpy (path->name, "");

  /* Allocate the point storage. */
  path->points = (PATH_POINT*) calloc (n_points, sizeof(PATH_POINT));
  if (NULL == path)
    {
      printf ("ERROR: Couldn't allocate %d points in path.\n", n_points);
      free (path);
      return NULL;
    }
  
  return path;
}

int PathFree (PATH** path)
{
  if (NULL == path || NULL == (*path))
    ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,"No path supplied."));

  /* Free the points array, then the path struct. */
  free ((*path)->points);
  free ((*path));
  *path = NULL;
      
  return (ERROR_NONE);
}
