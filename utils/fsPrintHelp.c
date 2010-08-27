/**
 * @file  fsPrintHelp.c
 * @brief utility to read an .xml file and output it in a readable format
 *
 */
/*
 * Original Author: Greg Terrono
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2010/08/27 15:25:20 $
 *    $Revision: 1.4 $
 *
 * Copyright (C) 2010,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 *
 */

//-------------------------------------------------------------------
 
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xinclude.h>
#include <libxml/xmlIO.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

#ifdef BUILD_MAIN
//-------------------------------------------------------------------
int main(int argc, char *argv[])
{
  return outputHelp(argv[1]);
  exit(0);
}
#endif

/*-------------------------------------------------------------------

 routines for printing of help text formated in xml

-------------------------------------------------------------------*/

static void toUpperCase(char *sPtr);
static void printName(xmlNodePtr cur);
static void replaceUnderscore(char *c);
static void printContents(xmlDocPtr doc, xmlNodePtr cur);
static int tagNameIs(char *c, xmlNodePtr cur);
static int wrdLen(char *c);

int outputHelp(char *name)
{
  xmlDocPtr doc;
  xmlNodePtr cur;
  //Checks for the .xml file name
  if (name==NULL)
  {
    fprintf(stderr, "No file name passed.\n");
    return -1;
  }
  char *fshome = getenv("FREESURFER_HOME");
  if (NULL == fshome) return -1;
  char *temp = name;
  name = malloc(strlen(name)+strlen(fshome)+strlen("/docs/xml/.help.xml"));
  strcpy(name,fshome);
  strcat(name,"/docs/xml/");
  strcat(name,temp);
  strcat(name,".help.xml");
  //Opens the .xml file given as an argument
  doc = xmlParseFile(name);
  //Catches an error in parsing the file
  if (doc == NULL )
  {
    fprintf(stderr,"Document not parsed successfully. \n");
    return -1;
	}

  cur = xmlDocGetRootElement(doc);
	
  //Catches the error of passing a blank document
  if (cur == NULL)
  {
    fprintf(stderr,"Empty document.\n");
		xmlFreeDoc(doc);
		return -1;
	}

  //Catches the error of passing a document with a different root tag
  if (!tagNameIs("help",cur)) 
  {
		fprintf(stderr,
            "Document id of the wrong type,the root node is not help.\n");
		xmlFreeDoc(doc);
		return -1;
	}

  cur=cur->xmlChildrenNode;

  printf("\t\t\t\tHelp");
  
  int exampleNum=1;
  //Outputs the file
  while (cur != NULL) 
  {
    //Outputs the arugments of the file
    if (tagNameIs("arguments",cur))
    {
      xmlNodePtr argumentType;
      argumentType=cur->xmlChildrenNode;
      while (argumentType!=NULL)
      {
        if (!tagNameIs("text",argumentType))
        {
          printName(argumentType);
          printf(" ARGUMENTS");
          xmlNodePtr argumentElement;
          argumentElement=argumentType->xmlChildrenNode;
          int first=1;
          while (argumentElement!=NULL)
          { 
            if (!tagNameIs("text",argumentElement))
            {
              if (tagNameIs("intro",argumentElement) || 
                  tagNameIs("argument",argumentElement))
              {
                if (first==1)
                  first=0;
                else
                  printf("\n");
              }
              printContents(doc,argumentElement);
            } 
            argumentElement= argumentElement->next;
          }
        }
        argumentType=argumentType->next;
      }
    }
    //Outputs all the other tags
    else if (!tagNameIs("text",cur))
    {
      printName(cur);

      if (tagNameIs("EXAMPLE",cur))
        printf(" %d",exampleNum++);
      if (tagNameIs("outputs", cur))
      {
        xmlNodePtr outputElement;
        outputElement=cur->xmlChildrenNode;
        int first=1;
        while (outputElement!=NULL)
        {
          if (!tagNameIs("text",outputElement))
          {
            if (tagNameIs("intro",outputElement) ||
                tagNameIs("output",outputElement))
            {
              if (first==1)
                first=0;
              else
                printf("\n");
            }
            printContents(doc,outputElement);
          }
          outputElement= outputElement->next;
        }
      }
      else
        printContents(doc,cur);
    }
    cur=cur->next;
  }
  
  printf("\n\n\n");

  return 0; // success
}

//Changes a string to unpper case
static void toUpperCase(char *c)
{
  int i;
  for (i=0; *(c+i)!='\0'; i++)
  {
    *(c+i) = toupper(*(c+i));
  }
}

//Prints the name of a tag in the correct format
static void printName(xmlNodePtr cur)
{
  char *n= malloc(strlen((char *)cur->name));
  strcpy(n,(char *)cur->name);
  toUpperCase(n);
  replaceUnderscore(n);
  printf("\n\n%s",n);
}

//Replaces the first underscore in a string with a space
static void replaceUnderscore(char *c)
{
  char *pos=strchr(c, '_');
  if (pos!=NULL)
    *pos=' ';
}

//Prints the text of a tag in the corect format
//at most 78 characters per line with the correct number of tabs
static void printContents(xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *contents;
  contents = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
  int i=0,j;
  while(i<strlen((char *)contents))
  {
    int tabNum=1;
    printf ("\n\t");
    if (tagNameIs("explanation",cur))
    {
      printf("\t");
      tabNum=2;
    }
    if (*(contents+i)==' ')
      i++;
    for(j=i; 
        j< 78-tabNum*8+i &&
          j<strlen((char *)contents) &&
          (wrdLen((char *)(contents+j))>78-tabNum*8 ||
           wrdLen((char *)(contents+j))<=78-tabNum*8+i-j); j++)
    {
      if (*(contents+j)=='\n')
      {
        j++;
        break;
      }
      else if (wrdLen((char *)(contents+j))>78-tabNum*8)
        for(j=j;j<78-tabNum*8+i;j++)
          printf("%c",*(contents+j));
      else
        printf("%c",*(contents+j));
    }
    i=j;
  }
  xmlFree(contents);
}

//Checks if the name of the tag is the same as the string parameter
static int tagNameIs(char *c, xmlNodePtr cur)
{
  return !(xmlStrcmp(cur->name, (const xmlChar *)c));
}

//Counts the number of chars until the next space, dash, underscore, slash,
//or end of the string
//Used to keep a word from running on two lines
static int wrdLen(char *c)
{
  int i;
  for (i=0; i<strlen(c); i++)
    if (*(c+i)==' '||*(c+i)=='_'||*(c+i)=='/')
      return i;
  return i;
}


