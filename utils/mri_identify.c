#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "mri.h"
#include "proto.h"
#include "mri_identify.h"
#include "analyze.h"
#include "volume_io.h"
#include "machine.h"
#include "fio.h"

extern int errno;

#ifdef SunOS
int stricmp(char *str1, char *str2) ;
#endif

int mri_identify(char *fname)
{

  if(is_cor(fname))
    return(MRI_CORONAL_SLICE_DIRECTORY);
  else if(is_genesis(fname))
    return(GENESIS_FILE);
  else if(is_ge_lx(fname))
    return(GE_LX_FILE);
  else if(is_sdt(fname))
    return(SDT_FILE);
  else if(is_mgh(fname))
    return(MRI_MGH_FILE);
  else if(is_mnc(fname))
    return(MRI_MINC_FILE);
  else if(is_analyze(fname))
    return(MRI_ANALYZE_FILE);
  else if(is_siemens(fname))
    return(SIEMENS_FILE);
  else if(is_brik(fname))
    return(BRIK_FILE);
  else if(is_bshort(fname))
    return(BSHORT_FILE);
  else
    return(-1);

}  /*  end mri_identify()  */

int is_cor(char *fname)
{

  struct stat stat_buf;

  if(stat(fname, &stat_buf) < 0)
    return(0);

  if(S_ISDIR(stat_buf.st_mode))
    return(1);

  return(0);

}  /*  end is_cor()  */

int is_brik(char *fname)
{

  char *dot;

  dot = strrchr(fname, '.');
  if(dot)
  {
    dot++;
    if(!stricmp(dot, "BRIK"))
      return(1);
  }

  return(0);

} /* end is_brik() */

int is_siemens(char *fname)
{

  FILE *fp;
  char string[4];
  char *dot;

  dot = strrchr(fname, '.');
  if(dot)
  {
    if(!stricmp(dot+1, "ima"))
      return(1);
  }

  if((fp = fopen(fname, "r")) == NULL)
  {
    errno = 0;
    return(0);
  }

  fseek(fp, 5790, SEEK_SET);
  if(fread(string, 2, 1, fp) < 1)
  {
    errno = 0;
    fclose(fp);
    return(0);
  }
  string[2] = '\0';
  if(strcmp(string, "SL"))
    {
    fclose(fp);
    return(0);
    }
  fseek(fp, 5802, SEEK_SET);
  if(fread(string, 2, 1, fp) < 1)
  {
    errno = 0;
    fclose(fp);
    return(0);
  }
  string[2] = '\0';
  if(strcmp(string, "SP"))
    {
    fclose(fp);
    return(0);
    }
  fseek(fp, 5838, SEEK_SET);
  if(fread(string, 3, 1, fp) < 1)
  {
    errno = 0;
    fclose(fp);
    return(0);
  }
  string[3] = '\0';
  if(strcmp(string, "FoV"))
    {
    fclose(fp);
    return(0);
    }

  fclose(fp);

  return(1);

} /* end is_siemens() */

int is_genesis(char *fname)
{

  FILE *fp;
  long magic;
  char *dot;

  if(!strncmp(fname, "I.", 2))
    return(1);

  dot = strrchr(fname, '.');
  if(dot)
  {
    if(!strcmp(dot+1, "MR"))
      return(1);
  }

  if((fp = fopen(fname, "r")) == NULL)
  {
    errno = 0;
    return(0);
  }

  if(fread(&magic, 4, 1, fp) < 1)
  {
    errno = 0;
    fclose(fp);
    return(0);
  }
  magic = orderLongBytes(magic);

  fclose(fp);

  if(magic == GE_MAGIC)
    return(1);

  return(0);

}  /*  end is_genesis()  */

int is_ge_lx(char *fname)
{

  FILE *fp;
  long magic;

  if((fp = fopen(fname, "r")) == NULL)
  {
    errno = 0;
    return(0);
  }

  fseek(fp, 3228, SEEK_CUR);
  if(fread(&magic, 4, 1, fp) < 0)
  {
    errno = 0;
    fclose(fp);
    return(0);
  }
  magic = orderLongBytes(magic);

  fclose(fp);

  if(magic == GE_MAGIC)
    return(1);

  return(0);

}  /*  end is_ge_lx()  */

int is_analyze(char *fname)
{

  FILE *fp;
  dsr hdr;
  char hfname[STRLEN], *dot;
  long hdr_length;

  strcpy(hfname, fname);

  dot = strrchr(fname, '.') ;
  if (dot)
  {
    if (!stricmp(dot+1, "img"))
      return(1) ;
    return(0);
  }

  dot = '\0';
  sprintf(hfname, "%s.hdr", hfname);

  if((fp = fopen(hfname, "r")) == NULL)
  {
    errno = 0;
    return(0);
  }

  if(fread(&hdr, sizeof(hdr), 1, fp) < 1)
  {
    errno = 0;
    fclose(fp);
    return(0);
  }

  fseek(fp, 0, SEEK_END);
  hdr_length = ftell(fp);
  fclose(fp);

  if(hdr_length != orderIntBytes(hdr.hk.sizeof_hdr))
    return(0);
  if(orderIntBytes(hdr.hk.extents) != 16384)
    return(0);
  if(hdr.hk.regular != 'r')
    return(0);

  return(1);

}  /*  end is_analyze()  */

int is_mnc(char *fname)
{

  char buf[3];
  FILE *fp;
  char *dot;

  dot = strrchr(fname, '.') ;
  if (dot)
  {

    if(!stricmp(dot+1, "mnc"))
      return(1) ;

    if(!stricmp(dot+1, "gz"))
    {
      /* --- get the next to last dot or the beginning of the file name --- */
      for(dot--;*dot != '.' && dot > fname;dot--);
      if(stricmp(dot, ".mnc.gz") == 0)
        return(1);
    }

  }

  if((fp = fopen(fname, "r")) == NULL)
  {
    errno = 0;
    return(0);
  }

  if(fread(buf, 1, 3, fp) < 3)
  {
    errno = 0;
    fclose(fp);
    return(0);
  }

  fclose(fp);

  if(strncmp(buf, "CDF", 3) == 0)
    return(1);

  return(0);

}  /*  end is_mnc()  */

int is_mgh(char *fname)
{
  FILE *fp;
  int version, width, height, depth, nframes, type, dof;
  char *dot ;

  dot = strrchr(fname, '.') ;
  if (dot)
  {
    if (!stricmp(dot+1, "mgh"))
      return(1) ;
  }

  if((fp = fopen(fname, "r")) == NULL)
  {
    errno = 0;
    return(0);
  }

  version = freadInt(fp) ;
  width = freadInt(fp) ;
  height = freadInt(fp) ;
  depth =  freadInt(fp) ;
  nframes = freadInt(fp) ;
  type = freadInt(fp) ;
  dof = freadInt(fp) ;

  fclose(fp);

/* my estimates (ch) */
  if(width < 64 || height < 64 || width > 1024 || height > 1024)
    return(0);
  if(depth > 2000)
    return(0);
  if(nframes > 2000)
    return(0);

  return(1);

}  /*  end is_mgh()  */

int is_bshort(char *fname)
{

  char *dot;

  dot = strrchr(fname, '.');
  if(dot)
  {
    dot++;
    if(!strcmp(dot, "bshort"))
      return(1);
  }

  return(0);

}  /*  end is_bshort()  */

int is_sdt(char *fname)
{

  char header_fname[STR_LEN];
  char *dot;
  FILE *fp;

  if((fp = fopen(fname, "r")) == NULL)
  {
    errno = 0;
    return(0);
  }

  fclose(fp);

  strcpy(header_fname, fname);

  if((dot = strrchr(header_fname, '.')))
    sprintf(dot+1, "spr");
  else
    strcat(header_fname, ".spr");

  if((fp = fopen(header_fname, "r")) == NULL)
  {
    errno = 0;
    return(0);
  }

  fclose(fp);

  return(1);

} /* end is_sdt() */

/* EOF */
