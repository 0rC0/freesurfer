#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "error.h"
#include "colortab.h"
#include "fio.h"

COLOR_TABLE *
CTABread(char *fname)
{
	COLOR_TABLE *ct ;
	COLOR_TABLE_ENTRY *cte ;
	char        line[STRLEN], *cp ;
	int         nbins ;
	FILE        *fp ;

	fp = fopen(fname, "r") ;
	if (fp == NULL)
		ErrorReturn(NULL, (ERROR_NOFILE, "CTABread(%s): could not open file", fname)) ;

	nbins = 0 ;
	while ((cp = fgetl(line, STRLEN, fp)) != NULL)
		nbins++ ;

	if (nbins <= 0)
	{
		fclose(fp) ;
		ErrorReturn(NULL, (ERROR_NOFILE, "CTABread(%s): badly formed file", fname)) ;
	}

	ct = (COLOR_TABLE *)calloc(1, sizeof(COLOR_TABLE)) ;
	if (ct == NULL)
		ErrorReturn(NULL, (ERROR_NOFILE, "CTABread(%s): could not allocate table", fname)) ;
	ct->bins = (COLOR_TABLE_ENTRY *)calloc(nbins, sizeof(COLOR_TABLE_ENTRY)) ;
	if (ct->bins == NULL)
		ErrorReturn(NULL, (ERROR_NOFILE, "CTABread(%s): could not allocate %d bin table", fname, nbins)) ;
	strcpy(ct->fname, fname) ;
	ct->nbins = nbins ;  

	rewind(fp) ; nbins = 0 ;
	while ((cp = fgetl(line, STRLEN, fp)) != NULL)
	{
		cte = &ct->bins[nbins] ;
		if (sscanf(line, "%d %s %d %d %d %d", &cte->index,cte->name,&cte->r,&cte->g,&cte->b,&cte->flag) != 6)
		{
			printf("CTABread(%s): could not scan 6 parms from line %s (%d)\n",fname,cp, nbins) ;
			continue ;
		}
		nbins++ ;
	}


	fclose(fp) ;
	return(ct) ;
}

int
CTABfree(COLOR_TABLE **pct)
{
	COLOR_TABLE *ct = *pct ;
	*pct = NULL ;
	free(ct->bins) ;
	free(ct) ;
	return(NO_ERROR) ;
}

int
CTABwriteInto(FILE *fp, COLOR_TABLE *ct)
{
	int                i ;
	COLOR_TABLE_ENTRY  *cte ;

	fwriteInt(ct->nbins, fp) ;
	fwriteInt(strlen(ct->fname)+1, fp) ;
	fwrite(ct->fname, sizeof(char), strlen(ct->fname)+1, fp) ;
	for (i = 0 ; i < ct->nbins ; i++)
	{
		cte = &ct->bins[i] ;
		fwriteInt(strlen(cte->name)+1, fp) ;
		fwrite(cte->name, sizeof(char), strlen(cte->name)+1, fp) ;
		fwriteInt(cte->r, fp) ;
		fwriteInt(cte->g, fp) ;
		fwriteInt(cte->b, fp) ;
		fwriteInt(cte->flag, fp) ;
	}
	return(NO_ERROR) ;
}

COLOR_TABLE *
CTABreadFrom(FILE *fp)
{
	COLOR_TABLE        *ct ;
	int                i, nbins, len ;
	COLOR_TABLE_ENTRY  *cte ;

	nbins = freadInt(fp) ;
	ct = (COLOR_TABLE *)calloc(1, sizeof(COLOR_TABLE)) ;
	if (ct == NULL)
		ErrorReturn(NULL, (ERROR_NOFILE, "CTABreadFrom: could not allocate table")) ;
	ct->bins = (COLOR_TABLE_ENTRY *)calloc(nbins, sizeof(COLOR_TABLE_ENTRY)) ;
	if (ct->bins == NULL)
		ErrorReturn(NULL, (ERROR_NOFILE, "CTABreadFrom: could not allocate %d bin table", nbins)) ;
	ct->nbins = nbins ;

	len = freadInt(fp) ;
	fread(ct->fname, sizeof(char), len, fp) ;
	for (i = 0 ; i < ct->nbins ; i++)
	{
		cte = &ct->bins[i] ;
		len = freadInt(fp) ;
		fread(cte->name, sizeof(char), len, fp) ;
		cte->r = freadInt(fp) ;
		cte->g = freadInt(fp) ;
		cte->b = freadInt(fp) ;
		cte->flag = freadInt(fp) ;
	}
	return(ct) ;
}

int
CTABcolorToIndex(COLOR_TABLE *pct, int r, int g, int b, int*index)
{
  CTE *bin;
  int nbin;

  if (NULL == pct)
    return(ERROR_BAD_PARM);

  nbin = 0;
  while (nbin < pct->nbins)
    {
      bin = &(pct->bins[nbin]);
      if (bin->r == r &&
	  bin->g == g &&
	  bin->b == b)
	{
	  *index = nbin;
	  return(NO_ERROR);
	}
      
      nbin++;
    }
  
  *index = -1;
  return(ERROR_BAD_PARM);
}

int
CTABindexToColor(COLOR_TABLE *pct, int index, int*r, int*g, int*b)
{
  CTE *bin;

  if (NULL == pct)
    return(ERROR_BAD_PARM);

  if (index < 0 || index >= pct->nbins)
    return(ERROR_BAD_PARM);
    
  bin = &(pct->bins[index]);
  *r = bin->r;
  *g = bin->g;
  *b = bin->b;

  return(NO_ERROR);
}

int
CTABcopyName(COLOR_TABLE *pct, int index, char *name)
{
  CTE *bin;

  if (NULL == pct)
    return(ERROR_BAD_PARM);

  if (index < 0 || index >= pct->nbins)
    return(ERROR_BAD_PARM);
    
  bin = &(pct->bins[index]);
  strcpy (name, bin->name);

  return(NO_ERROR);
}

int
CTABannotationToIndex(COLOR_TABLE *ctab, int annotation)
{
  int   r, g, b, index ;

	r = annotation & 0x0000ff ;
	g = (annotation >> 8) & 0x0000ff ;
	b = (annotation >> 16) & 0x0000ff ;
	CTABcolorToIndex(ctab, r, g, b, &index) ;
	return(index) ;
}
