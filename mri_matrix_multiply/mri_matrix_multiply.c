#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "machine.h"
#include "fio.h"
#include "utils.h"
#include "mri.h"
#include "matrix.h"

#define IN_OUT_NAMES  100

char *prog_name;
int verbose_flag = 0;

int read_mat(int argc, char *argv[], int i, MATRIX *in_mat);
int write_mat(int argc, char *argv[], int i, MATRIX *in_mat);

char subject_name[STR_LEN];
float ipr, st, brightness;
int register_stuff_defined = 0;

static void usage(int exit_val)
{

  fprintf(stderr, "usage: %s [options]\n", prog_name);
  fprintf(stderr, "options are:\n");
  fprintf(stderr, "  -v verbose\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "'-im file' specifies input matrix files\n");
  fprintf(stderr, "'-iim file' specifies input matrix files to be inverted before multiplication\n");
  fprintf(stderr, "'-om file' specifies output matrix files\n");
  fprintf(stderr, "input and output files may be .dat or .xfm files\n");
  fprintf(stderr, "\n");
  exit(exit_val);

} /* end usage() */

int main(int argc, char *argv[])
{

  int i;
  int in_names[IN_OUT_NAMES], n_in = 0;
  int out_names[IN_OUT_NAMES], n_out = 0;
  MATRIX *in_mat, *result;

  /* ----- get the base executable name ----- */
  prog_name = strrchr(argv[0], '/');
  prog_name = (prog_name == NULL ? argv[0] : prog_name + 1);

  if(argc == 1)
    usage(1);

  for(i = 0;i < IN_OUT_NAMES;i++)
    in_names[i] = out_names[i] = 0;

  /* ----- get options ----- */
  for(i = 1;i < argc;i++)
  {
    if(strcmp(argv[i], "-im") == 0)
    {
      in_names[n_in] = i+1;
      n_in++;
      i++;
    }
    else if(strcmp(argv[i], "-iim") == 0)
    {
      in_names[n_in] = -(i+1);
      n_in++;
      i++;
    }
    else if(strcmp(argv[i], "-om") == 0)
    {
      out_names[n_out] = i+1;
      n_out++;
      i++;
    }
    else if(strcmp(argv[i], "-v") == 0)
    {
      verbose_flag = 1;
    }
    else
    {
      fprintf(stderr, "%s: unknown option %s\n", prog_name, argv[i]);
    }
  }

  if(n_in == 0)
  {
    fprintf(stderr, "%s: no input files specified\n", prog_name);
    usage(1);
  }

  if(n_out == 0)
  {
    fprintf(stderr, "%s: no output files specified\n", prog_name);
    usage(1);
  }

  in_mat = MatrixAlloc(4, 4, MATRIX_REAL);
  result = MatrixAlloc(4, 4, MATRIX_REAL);

  *MATRIX_RELT(result, 1, 1) = 1;  *MATRIX_RELT(result, 1, 2) = 0;  *MATRIX_RELT(result, 1, 3) = 0;  *MATRIX_RELT(result, 1, 4) = 0;
  *MATRIX_RELT(result, 2, 1) = 0;  *MATRIX_RELT(result, 2, 2) = 1;  *MATRIX_RELT(result, 2, 3) = 0;  *MATRIX_RELT(result, 2, 4) = 0;
  *MATRIX_RELT(result, 3, 1) = 0;  *MATRIX_RELT(result, 3, 2) = 0;  *MATRIX_RELT(result, 3, 3) = 1;  *MATRIX_RELT(result, 3, 4) = 0;
  *MATRIX_RELT(result, 4, 1) = 0;  *MATRIX_RELT(result, 4, 2) = 0;  *MATRIX_RELT(result, 4, 3) = 0;  *MATRIX_RELT(result, 4, 4) = 1;

  /* ----- read input files and keep a running product ----- */
  for(i = 0;i < n_in;i++)
  {
    if(read_mat(argc, argv, (in_names[i] < 0 ? -in_names[i] : in_names[i]), in_mat) == -1)
    {
      fprintf(stderr, "%s: exiting...\n", prog_name);
      exit(1);
    }
    if(in_names[i] < 0)
    {

      if(verbose_flag)
        printf("inverting matrix\n");

      if(MatrixInverse(in_mat, in_mat) == NULL)
      {
        fprintf(stderr, "%s: couldn't invert matrix from file %s\n", prog_name, argv[in_names[i]]);
        exit(0);
      }
    }
    MatrixMultiply(result, in_mat, result);
  }

  /* ----- write output files ----- */
  for(i = 0;i < n_out;i++)
  {
    if(write_mat(argc, argv, out_names[i], result) == -1)
    {
    }
  }

  exit(0);

} /* end main() */

int read_mat(int argc, char *argv[], int i, MATRIX *in_mat)
{

  FILE *fin;
  char line[STR_LEN];

  line[0] = '\0';

  if(i > argc)
  {
    fprintf(stderr, "%s: missing input matrix\n", prog_name);
    return(-1);
  }

  if(strcmp(&argv[i][strlen(argv[i])-4], ".dat") == 0)
  {
    if((fin = fopen(argv[i], "r")) == NULL)
    {
      fprintf(stderr, "%s: error opening file %s\n", prog_name, argv[i]);
      return(-1);
    }

    if(verbose_flag)
      printf("reading transform from .dat file %s\n", argv[i]);

    fscanf(fin, "%s", subject_name);
    fscanf(fin, "%f", &ipr);
    fscanf(fin, "%f", &st);
    fscanf(fin, "%f", &brightness);
    fscanf(fin, "%f %f %f %f", MATRIX_RELT(in_mat, 1, 1), MATRIX_RELT(in_mat, 1, 2), MATRIX_RELT(in_mat, 1, 3), MATRIX_RELT(in_mat, 1, 4));
    fscanf(fin, "%f %f %f %f", MATRIX_RELT(in_mat, 2, 1), MATRIX_RELT(in_mat, 2, 2), MATRIX_RELT(in_mat, 2, 3), MATRIX_RELT(in_mat, 2, 4));
    fscanf(fin, "%f %f %f %f", MATRIX_RELT(in_mat, 3, 1), MATRIX_RELT(in_mat, 3, 2), MATRIX_RELT(in_mat, 3, 3), MATRIX_RELT(in_mat, 3, 4));
    fscanf(fin, "%f %f %f %f", MATRIX_RELT(in_mat, 4, 1), MATRIX_RELT(in_mat, 4, 2), MATRIX_RELT(in_mat, 4, 3), MATRIX_RELT(in_mat, 4, 4));
    fclose(fin);
    register_stuff_defined = 1;
  }
  else if(strcmp(&argv[i][strlen(argv[i])-4], ".xfm") == 0)
  {
    if((fin = fopen(argv[i], "r")) == NULL)
    {
      fprintf(stderr, "%s: error opening file %s\n", prog_name, argv[i]);
      return(-1);
    }

    if(verbose_flag)
      printf("reading transform from .xfm file %s\n", argv[i]);

    while(strncmp(line, "Linear_Transform", 16) != 0)
    {
      if(fgets(line, STR_LEN, fin) == NULL)
      {
      fclose(fin);
      fprintf(stderr, "%s: premature EOF in file %s\n", prog_name, argv[i]);
      return(-1);
      }
    }
    fscanf(fin, "%f %f %f %f", MATRIX_RELT(in_mat, 1, 1), MATRIX_RELT(in_mat, 1, 2), MATRIX_RELT(in_mat, 1, 3), MATRIX_RELT(in_mat, 1, 4));
    fscanf(fin, "%f %f %f %f", MATRIX_RELT(in_mat, 2, 1), MATRIX_RELT(in_mat, 2, 2), MATRIX_RELT(in_mat, 2, 3), MATRIX_RELT(in_mat, 2, 4));
    fscanf(fin, "%f %f %f %f;", MATRIX_RELT(in_mat, 3, 1), MATRIX_RELT(in_mat, 3, 2), MATRIX_RELT(in_mat, 3, 3), MATRIX_RELT(in_mat, 3, 4));
    fclose(fin);
  }
  else
  {
    fprintf(stderr, "%s: unknown input matrix file type for file %s\n", prog_name, argv[i]);
    return(-1);
  }

  return(0);

} /* end read_mat() */

int write_mat(int argc, char *argv[], int i, MATRIX *out_mat)
{

  FILE *fout;

  if(i > argc)
  {
    fprintf(stderr, "%s: missing output matrix\n", prog_name);
    return(-1);
  }

  if(strcmp(&argv[i][strlen(argv[i])-4], ".dat") == 0)
  {

    if(verbose_flag)
      printf("writing transform to .dat file %s\n", argv[i]);

    if((fout = fopen(argv[i], "w")) == NULL)
    {
      fprintf(stderr, "%s: error opening file %s\n", prog_name, argv[i]);
      return(-1);
    }

    if(!register_stuff_defined)
    {
      fprintf(stderr, "%s: extra parameters for .dat file %s undefined\n", prog_name, argv[i]);
      fprintf(stderr, "%s: (not writing this file)\n", prog_name);
      fclose(fout);
      return(-1);
    }

    fprintf(fout, "%s\n", subject_name);
    fprintf(fout, "%f\n", ipr);
    fprintf(fout, "%f\n", st);
    fprintf(fout, "%f\n", brightness);
    fprintf(fout, "%f %f %f %f\n", *MATRIX_RELT(out_mat, 1, 1), *MATRIX_RELT(out_mat, 1, 2), *MATRIX_RELT(out_mat, 1, 3), *MATRIX_RELT(out_mat, 1, 4));
    fprintf(fout, "%f %f %f %f\n", *MATRIX_RELT(out_mat, 2, 1), *MATRIX_RELT(out_mat, 2, 2), *MATRIX_RELT(out_mat, 2, 3), *MATRIX_RELT(out_mat, 2, 4));
    fprintf(fout, "%f %f %f %f\n", *MATRIX_RELT(out_mat, 3, 1), *MATRIX_RELT(out_mat, 3, 2), *MATRIX_RELT(out_mat, 3, 3), *MATRIX_RELT(out_mat, 3, 4));
    fprintf(fout, "%f %f %f %f\n", *MATRIX_RELT(out_mat, 4, 1), *MATRIX_RELT(out_mat, 4, 2), *MATRIX_RELT(out_mat, 4, 3), *MATRIX_RELT(out_mat, 4, 4));

    fclose(fout);

  }
  else if(strcmp(&argv[i][strlen(argv[i])-4], ".xfm") == 0)
  {

    if((fout = fopen(argv[i], "w")) == NULL)
    {
      fprintf(stderr, "%s: error opening file %s\n", prog_name, argv[i]);
      return(-1);
    }

    if(verbose_flag)
      printf("writing transform to .xfm file %s\n", argv[i]);

    fprintf(fout, "MNI Transform File\n");
    fprintf(fout, "%% Generated by %s\n", prog_name);
    fprintf(fout, "\n");
    fprintf(fout, "Transform_Type = Linear;\n");
    fprintf(fout, "Linear_Transform =\n");
    fprintf(fout, "   %e %e %e %e\n",  *MATRIX_RELT(out_mat, 1, 1), *MATRIX_RELT(out_mat, 1, 2), *MATRIX_RELT(out_mat, 1, 3), *MATRIX_RELT(out_mat, 1, 4));
    fprintf(fout, "   %e %e %e %e\n",  *MATRIX_RELT(out_mat, 2, 1), *MATRIX_RELT(out_mat, 2, 2), *MATRIX_RELT(out_mat, 2, 3), *MATRIX_RELT(out_mat, 2, 4));
    fprintf(fout, "   %e %e %e %e;\n", *MATRIX_RELT(out_mat, 3, 1), *MATRIX_RELT(out_mat, 3, 2), *MATRIX_RELT(out_mat, 3, 3), *MATRIX_RELT(out_mat, 3, 4));

    fclose(fout);

  }
  else
  {
    fprintf(stderr, "%s: unknown output matrix file type for file %s\n", prog_name, argv[i]);
    return(-1);
  }

  return(0);

} /* end read_mat() */

/* EOF */
