#include <stdio.h>
#include <image.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "proto.h"
#include "macros.h"
#include "utils.h"

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Simple.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

static int nocolor = 0 ;

#define NONE  0
#define LOOP  1
#define SWING 2

#define FORWARD 0
#define REVERSE 1

#define INTERVAL_INC 15

char *Progname ;

/* -------- Prototypes -------- */
#ifdef __sun__ 
int XShmQueryExtension(Display *disp) ;
#endif 
void useage(void);
void mark_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void update_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void quit_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void repaint_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void loop_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void stop_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void timer_proc(XtPointer ptr, XtIntervalId *id);
void swing_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void slower_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void faster_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void forward_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void back_proc(Widget w, XEvent *event, String *pars, Cardinal *npars);
void start_timer(void);
static void XInit(int *argc, char ***argv);
static int highbit(unsigned long ul);
static void XSetupDisplay(int nframes);
void rgb2xcol(IMAGE *I, byte *ximgdata, int fnum);
void ConvertImages(int nframes, char **argv);
void MakeDispNames(int argc, char **argv);
/* -------- End Prototypes -------- */

/* -------- Typedefs -------- */
typedef struct {
  XtAppContext context;
  Display *disp;
  GC theGC;
  XVisualInfo vi;
  Visual *vis;
  Window root;
  Window canvas;
  Colormap colormap;
  int screenno;
  int depth;
  unsigned long rmask, gmask, bmask;
  int rshift, gshift, bshift;
} XCRUFT;
/* -------- End Typedefs -------- */
  
/* -------- Global Variables ------- */
Widget frame,toplevel,quit_bt,canvas,buttons,stop_bt,loop_bt,swing_bt;
Widget fast_bt,slow_bt;
int shmext,nframes;
XImage *ximg;
byte *imgdata;
XCRUFT xi;
int pfmt=0,rows=0,cols=0;
int running = 0;
int direction = FORWARD;
int curframe=0;
unsigned long interval = 33; /* 1s */
char **fnames;
/* -------- End Global Variables ------- */

/* -------- Static Variables -------- */
static XtActionsRec actions[] = {
  {"Loop", loop_proc},
  {"Swing", swing_proc},
  {"Stop", stop_proc},
  {"Refresh", repaint_proc},
  {"Faster",faster_proc},
  {"Slower",slower_proc},
  {"Quit", quit_proc},
  {"Forward",forward_proc},
  {"Back",back_proc},
  {"Update",update_proc},
  {"Mark",mark_proc}
};

static char *fallback_resources[] = {
  "*Shell.visual:          TrueColor",
  "*Background:            black",
  "*BorderColor:           white",
  "*Foreground:            wheat",
  "*Frame*sensitive:       True",
  "*Font:                  -*-helvetica-bold-r-*-*-*-120-*-*-*-*-*-*",
  "*Buttons.borderWidth:   0",
  "*Buttons.hSpace:        0",
  "*Canvas.fromVert:       Buttons",
  "*Loop.Translations:     #override <Btn1Down>,<Btn1Up>:Loop()",
  "*Swing.fromHoriz:       Loop",
  "*Swing.Translations:    #override <Btn1Down>,<Btn1Up>:Swing()",
  "*Stop.fromHoriz:        Slower",
  "*Stop.Translations:     #override <Btn1Down>,<Btn1Up>:Stop()",
  "*Faster.fromHoriz:      Swing",
  "*Faster.Translations:   #override <Btn1Down>:Faster()",
  "*Slower.fromHoriz:      Faster",
  "*Slower.Translations:   #override <Btn1Down>:Slower()",
  "*Quit.Translations:     #replace <Btn1Down>,<Btn1Up>:Quit() notify()",
  "*Quit.sensitive:        True",
  "*Quit.fromHoriz:        Stop",
  "*Canvas.baseTranslations:   #override <Expose>:Refresh()",
  NULL
};
static char ckeys[] = "#override <Key>Left:Back()\n <Key>Right:Forward()\n <Btn3Down>:Mark()\n <Btn3Motion>:Update()\n <Key>Escape:Quit()";
static XtIntervalId timer;
/* -------- End Static Variables -------- */

void useage(void)
{
  fprintf(stderr,"nmovie <image file> <image file> ...\n");
  exit(1);
}

/* We need this even though it doesn't make sense.  Without this null 
   procedure, update_proc() doesn't get called properly */
void mark_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
}

void update_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  Window root,child;
  int rx,ry,wx,wy;
  unsigned int mask;

  XQueryPointer(xi.disp,xi.canvas,&root,&child,&rx,&ry,&wx,&wy,&mask);

  if (wx>cols)
    wx = cols;
  else if (wx<0)
    wx = 0;

  curframe = (int)(((float)wx/(float)cols)*(nframes-1));

  repaint_proc(canvas,NULL,NULL,NULL);
}

void start_timer(void)
{
  XFlush(xi.disp);
  timer = XtAppAddTimeOut(xi.context, interval, timer_proc, NULL);
}

void forward_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  curframe++;
  if (curframe==nframes)
    curframe = 0;

  repaint_proc(canvas,NULL,NULL,NULL);
}

void back_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  curframe--;
  if (curframe<0)
    curframe = nframes-1;
 
  repaint_proc(canvas,NULL,NULL,NULL);
}

void faster_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  if ((interval<INTERVAL_INC)) 
    interval = 1;
  else
    interval -= INTERVAL_INC;
}

void slower_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  interval += INTERVAL_INC;
}

void timer_proc(XtPointer ptr, XtIntervalId *id)
{
  switch (running)
    {
    case LOOP:
      curframe++;
      if (curframe == nframes)
  curframe = 0;
      break;
    case SWING:
      switch (direction)
  {
  case FORWARD:
    curframe++;
    if (curframe==nframes)
      {
        curframe = nframes-1;
        direction = REVERSE;
      }
    break;
  case REVERSE:
    curframe--;
    if (curframe<0)
      {
        curframe=0;
        direction=FORWARD;
      }
    break;
  }
      break;
    default:
    case NONE:
      break;
    }

  repaint_proc(canvas,NULL,NULL,NULL);
  start_timer();
}

void swing_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  running = SWING;
  start_timer();
}

void loop_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  running = LOOP;
  start_timer();
}

void stop_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  if (timer)
    {
      running = 0;
      XtRemoveTimeOut(timer);
      timer=0;
    }
}

void repaint_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  ximg->data = imgdata + (curframe*rows*ximg->bytes_per_line);

  if (w == canvas)
    {
      XPutImage(xi.disp, xi.canvas, xi.theGC, ximg, 0, 0, 0, 0, cols, rows);
      XStoreName(xi.disp, XtWindow(toplevel), fnames[curframe]);
    }
}

void quit_proc(Widget w, XEvent *event, String *pars, Cardinal *npars)
{
  XtDestroyWidget(toplevel);
  exit(0);
}

static int highbit(unsigned long ul)
{
  /* returns position of highest set bit in 'ul' as an integer (0-31),
   or -1 if none */

  int i;  unsigned long hb;

  hb = 0x80;  hb = hb << 24;   /* hb = 0x80000000UL */
  for (i=31; ((ul & hb) == 0) && i>=0;  i--, ul<<=1);
  return i;
}

static void XInit(int *argc, char ***argv)
{
  XtToolkitInitialize();
  xi.context = XtCreateApplicationContext();
  XtAppSetFallbackResources(xi.context, fallback_resources);
  xi.disp = XtOpenDisplay(xi.context, NULL, "NMovie", "NMovie", NULL, 0, 
        argc, *argv);
  if (!xi.disp)
    ErrorExit(ERROR_BAD_PARM,"Unable to open display");

  shmext = XShmQueryExtension(xi.disp);
  xi.screenno = DefaultScreen(xi.disp);
}

static void XSetupDisplay(int nframes)
{
  XGCValues xgcv;
  XtAccelerators keys;
  
  /* Had to do it this way since embedding the keystrokes in the fallback
     resources failed to work properly -- what a kludge. */

  keys = XtParseAcceleratorTable(ckeys);

  xi.depth = DefaultDepthOfScreen(DefaultScreenOfDisplay(xi.disp));

  if (!XMatchVisualInfo(xi.disp, xi.screenno, xi.depth, TrueColor, &(xi.vi)))
    ErrorExit(ERROR_BADPARM, "Could not find a TrueColor visual");

  xi.vis = xi.vi.visual;
  xi.root = RootWindow(xi.disp, xi.screenno);

  xi.colormap = XCreateColormap(xi.disp, xi.root, xi.vis, AllocNone);

  toplevel = XtVaAppCreateShell("NMovie", "NMovie", 
        applicationShellWidgetClass,
        xi.disp, 
        XtNvisual, xi.vis, 
        XtNcolormap, xi.colormap, 
        NULL);

  XtAppAddActions(xi.context,actions,XtNumber(actions));

  frame = XtVaCreateManagedWidget("Frame", formWidgetClass, toplevel, NULL);
  buttons = XtVaCreateManagedWidget("Buttons", formWidgetClass, frame, NULL );
  loop_bt = XtVaCreateManagedWidget("Loop", commandWidgetClass, 
            buttons, NULL); 
  swing_bt = XtVaCreateManagedWidget("Swing", commandWidgetClass, 
             buttons, NULL); 
  fast_bt = XtVaCreateManagedWidget("Faster", commandWidgetClass, 
            buttons, NULL); 
  slow_bt = XtVaCreateManagedWidget("Slower", commandWidgetClass, 
            buttons, NULL); 
  stop_bt = XtVaCreateManagedWidget("Stop", commandWidgetClass, 
            buttons, NULL); 
  quit_bt = XtVaCreateManagedWidget("Quit", commandWidgetClass, 
            buttons, NULL);
  canvas = XtVaCreateManagedWidget("Canvas", simpleWidgetClass, frame, 
           XtNwidth, cols,
           XtNheight, rows,
           XtNaccelerators, keys, NULL);
  XtInstallAllAccelerators(canvas,toplevel);
  XtRealizeWidget(toplevel);
  xi.canvas = XtWindow(canvas);

  xi.theGC = XCreateGC(xi.disp, xi.canvas, 0L, &xgcv);

  xi.rmask = xi.vis->red_mask;
  xi.gmask = xi.vis->green_mask;
  xi.bmask = xi.vis->blue_mask;

  xi.rshift = 7 - highbit(xi.rmask);
  xi.gshift = 7 - highbit(xi.gmask);
  xi.bshift = 7 - highbit(xi.bmask);

  ximg = XCreateImage(xi.disp,xi.vis,xi.depth,ZPixmap, 0, NULL, 
          cols, rows, 32, 0);

  if ((imgdata = (byte *)calloc((size_t)(rows*ximg->bytes_per_line*nframes),
        sizeof(byte))) 
      ==NULL)
    ErrorExit(ERROR_NO_MEMORY,"Failed to allocate image buffer");

  ximg->data = (char *) imgdata;
}

void rgb2xcol(IMAGE *I, byte *ximgdata, int fnum)
{
  int i,j;
  unsigned long r,g,b,xcol;
  byte *bptr, *xptr, *ip;

  bptr = I->image; 
  xptr = ximgdata+(fnum+1)*rows*ximg->bytes_per_line-ximg->bytes_per_line;
  for(i=0;i<rows;i++,xptr -= ximg->bytes_per_line)
    for(j=0, ip = xptr; j<cols; j++)
      {
        if (nocolor)
        { r = *bptr; g = *bptr; }
        else
        { r = *bptr++; g = *bptr++; }
        b = *bptr++;
  
  if (xi.rshift<0) 
    r = r << (-xi.rshift);
  else 
    r = r >> xi.rshift;
  if (xi.gshift<0) 
    g = g << (-xi.gshift);
  else 
    g = g >> xi.gshift;
  if (xi.bshift<0) 
    b = b << (-xi.bshift);
  else 
    b = b >> xi.bshift;

  r = r & xi.rmask;
  g = g & xi.gmask;
  b = b & xi.bmask; 

  xcol = r | g | b;

  switch(ximg->bits_per_pixel)
    {
    case 32:
      switch(ximg->byte_order)
        {
        case MSBFirst:
    *ip++ = (xcol>>24) & 0xff;
    *ip++ = (xcol>>16) & 0xff;
    *ip++ = (xcol>>8)  & 0xff;
    *ip++ =  xcol      & 0xff;
    break;
        case LSBFirst:
    *ip++ =  xcol      & 0xff;
    *ip++ = (xcol>>8)  & 0xff;
    *ip++ = (xcol>>16) & 0xff;
    *ip++ = (xcol>>24) & 0xff;
    break;
        }
      break;
    case 24:
      switch(ximg->byte_order)
        {
        case MSBFirst:
    *ip++ = (xcol>>16) & 0xff;
    *ip++ = (xcol>>8)  & 0xff;
    *ip++ =  xcol      & 0xff;
    break;
        case LSBFirst:
    *ip++ =  xcol      & 0xff;
    *ip++ = (xcol>>8)  & 0xff;
    *ip++ = (xcol>>16) & 0xff;
    break;
        }
      break;
    case 16:
      switch(ximg->byte_order)
        {
        case MSBFirst:
    *ip++ = (xcol>>8) & 0xff;
    *ip++ =  xcol     & 0xff;
    break;
        case LSBFirst:
    *ip++ =  xcol     & 0xff;
    *ip++ = (xcol>>8) & 0xff;
    break;
        }
      break;
    case 8:
      *ip++ = xcol & 0xff;
      break;
    }
      }
}

void ConvertImages(int nframes, char **argv)
{
  IMAGE *I;
  int i, rows = 0, cols = 0;

  for(i=0;i<nframes;i++)
    {
        
      I = ImageRead(argv[i+1]);
      if (!I)
        ErrorExit(ERROR_NOFILE, 
                  "%s: could not read image file %s\n", Progname,argv[i+1]);
      if (i == 0)
      {
        rows = I->rows ; cols = I->cols ;
      }
      else if (rows != I->rows || cols != I->cols)
      {
#if 0
        ErrorExit(ERROR_BADFILE, "%s: image %s dimensions (%d x %d) don't match first image (%d x %d)",
                  Progname, argv[i+1],I->cols, I->rows, cols, rows) ;
#else
        ErrorPrintf(ERROR_BADFILE, "%s: image %s dimensions (%d x %d) don't match first image (%d x %d)",
                    Progname, argv[i+1],I->cols, I->rows, cols, rows) ;
#endif
        argv++ ; nframes-- ; i-- ;
        continue ;
      }
        
      rgb2xcol(I,imgdata,i);
      ImageFree(&I);
    }
}

void MakeDispNames(int argc, char **argv)
{
  int i;
  char *s;

  fnames = (char **)malloc(sizeof(char *)*nframes);
  if (!fnames)
    ErrorExit(ERROR_NO_MEMORY,"Failed to allocate file name buffer\n");

  for(i=1;i<argc;i++)
    {
      s = strrchr(argv[i],'/');
      if (s!=NULL)
  {
    fnames[i-1] = (char *)malloc(sizeof(char)*strlen(s+1)+1);
    if (!fnames[i-1])
      ErrorExit(ERROR_NO_MEMORY,"Failed to allocate file name buffer\n");
    strcpy(fnames[i-1],s+1);
  }
      else
  {
    fnames[i-1] = (char *)malloc(sizeof(char)*strlen(argv[i])+1);
    if (!fnames[i-1])
      ErrorExit(ERROR_NO_MEMORY,"Failed to allocate file name buffer\n");
    strcpy(fnames[i-1],argv[i]);
  }
    }
}

static int get_option(int argc, char *argv[]) ;
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int
get_option(int argc, char *argv[])
{
  int  nargs = 0 ;
  char *option ;

  option = argv[1] + 1 ;            /* past '-' */
  StrUpper(option) ;
  switch (*option)
  {
  case '?':
  case 'U':
    useage() ;
    exit(1) ;
    break ;
  case '1':
    nocolor = 1 ;
    break ;
  }
  return(nargs) ;
}
int main(int argc, char **argv)
{
  IMAGE   *I;
  int     i;
  int     ac, nargs ;
  char    **av ;

  Progname = argv[0] ;
  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  XInit(&argc,&argv);

  if (argc<2)
    useage();

  for(i=1;i<argc;i++)
  {
    I = ImageReadHeader(argv[i]);
    if (!I)
      ErrorExit(ERROR_NOFILE, 
                "%s: could not read image file %s\n",Progname,argv[i]);
    switch (I->pixel_format)
    {
    case PFBYTE:
    case PFFLOAT:
      nocolor = 1 ;
      break ;
    default:
        break ;
    }
    if ((rows && (I->orows != rows)) || (cols && (I->ocols != cols)))
    {
      ErrorPrintf(ERROR_BADFILE, "%s: image %s dimensions (%d x %d) don't match first image (%d x %d)",
                  Progname, argv[i],I->ocols, I->orows, cols, rows) ;
      ImageFree(&I) ;
      memmove(&argv[i], &argv[i+1], argc-i) ;
      i-- ; argc-- ;
      continue ;
    }
    pfmt = MAX(pfmt,I->pixel_format);
    rows = MAX(rows,I->orows);
    cols = MAX(cols,I->ocols);
    ImageFree(&I);
  }


  nframes = argc-1;

  XSetupDisplay(nframes);

  MakeDispNames(argc,argv);

  ConvertImages(nframes,argv);

  XtMapWidget(toplevel);

  XtAppMainLoop(xi.context);

  return 0; /* Make -Wall happy! */
}



