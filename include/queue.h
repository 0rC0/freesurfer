/*
   @(#)queue.h  1.4
   3/31/94
*/
/*------------------------------------------------------------------------
      File Name:  queue.h

         Author:  Bruce Fischl

        Created:  Jan. 1994

    Description:  

------------------------------------------------------------------------*/


#ifndef QUEUE_H
#define QUEUE_H


typedef struct _qelt_
{
  struct _qelt_ *next ;
  struct _qelt_ *prev ;
  void          *data ;
} QELT ;

typedef struct
{
  QELT  *head ;
  QELT  *tail ;
  int   nelts ;      /* # of elements in queue */
  int   mode ;
} QUEUE ;

#define Q_WAIT_FOR_DATA   1
#define Q_DONT_WAIT       2

#ifdef ANSI
int   Qput(QUEUE *q, void *data) ;
void  *Qget(QUEUE *q, int mode) ;
QUEUE *Qalloc(int max_elts) ;
void  Qfree(QUEUE *q);
#else
int   Qput() ;
void  *Qget() ;
QUEUE *Qalloc() ;
void  Qfree();
#endif


#define Qempty(q)    (((q)->head == NULL))
#define Qfirst(q)    ((q)->head ? (q)->head->data : NULL)
#define Qnelts(q)    ((q)->nelts)

#endif



