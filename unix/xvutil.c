/*----------------------------------------------------------------------
           File Name:

           Author:

           Description:

           Conventions:

----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
                           INCLUDE FILES
----------------------------------------------------------------------*/

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "image.h"
#include "utils.h"
#include "macros.h"
#include "xvutil.h"

/*----------------------------------------------------------------------
                           MACROS AND CONSTANTS
----------------------------------------------------------------------*/

#define CHAR_WIDTH        8
#define MAX_TITLE_CHARS   (nint((float)xvf->display_size / (float)CHAR_WIDTH))

#define MAX_DISP_SCALES   4

#define FRAME_X             250
#define FRAME_Y             10

#define DISPLAY_SIZE        128
#define MAX_DISP_VAL        249


#define PANEL_HEIGHT        ((xvf->button_rows+1) * ROW_HEIGHT+4*WINDOW_PAD)

#define CHAR_HEIGHT          20
#define CHAR_PAD             5

#define ROW_HEIGHT           30
#define FIRST_BUTTON_ROW     30
#define SECOND_BUTTON_ROW    60
#define THIRD_BUTTON_ROW     90
#define FOURTH_BUTTON_ROW    120
#define FIFTH_BUTTON_ROW     150
#define SIXTH_BUTTON_ROW     180
#define SEVENTH_BUTTON_ROW   210
#define EIGHTH_BUTTON_ROW    240
#define LAST_BUTTON_ROW      EIGHTH_BUTTON_ROW

#define FIRST_BUTTON_COL     5
#define SECOND_BUTTON_COL    60
#define THIRD_BUTTON_COL     200
#define FOURTH_BUTTON_COL    315
#define FIFTH_BUTTON_COL     450
#define SIXTH_BUTTON_COL     565
#define SEVENTH_BUTTON_COL   665

#define FIRST_FNAME_COL      5
#define SECOND_FNAME_COL     400

#define HIPS_CMD_ROWS       30
#define HIPS_CMD_COLS       400

/*----------------------------------------------------------------------
                              PROTOTYPES
----------------------------------------------------------------------*/

static void  xvCreateFrame(XV_FRAME *xvf, char *name) ;
static void  xvInitColors(XV_FRAME *xvf) ;
static void xvInitImages(XV_FRAME *xvf) ;
static void xv_dimage_repaint(Canvas canvas, Xv_Window window, 
                                       Rectlist *repaint_area) ;
static DIMAGE *xvGetDimage(int which, int alloc) ;
static void xv_dimage_event_handler(Xv_Window window, Event *event) ;
static void xvRepaintImage(XV_FRAME *xvf, int which) ;
static XImage *xvCreateXimage(XV_FRAME *xvf, IMAGE *image) ;
static Panel_setting xvHipsCommand(Panel_item item, Event *event) ;
static void xvHipsCmdFrameInit(void) ;
static void buttonQuit(Panel_item item, Event *event) ;

/*----------------------------------------------------------------------
                              GLOBAL DATA
----------------------------------------------------------------------*/


static XV_FRAME *xvf ;
static IMAGE *GtmpFloatImage = NULL, *GtmpByteImage = NULL,
  *GtmpByteImage2 = NULL ;

static Frame           hips_cmd_frame ;
static Display        *hips_cmd_display;
static Panel_item      hips_cmd_panel_item ;
static Panel           hips_cmd_panel ;
static char            hips_cmd_str[301] ;
static int             hips_cmd_source = 0 ;
static void            (*XVevent_handler)(Event *event, DIMAGE *dimage) = NULL;
static void            (*XVquit_func)(void) = NULL;

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
XV_FRAME *
XValloc(int rows, int cols, int button_rows, int display_size, char *name, 
        Notify_value (*poll)(void))
{
  int               row ;
  static struct itimerval  timer;       

  xvf = (XV_FRAME *)calloc(1, sizeof(XV_FRAME)) ;
  if (!xvf)
    return(NULL) ;

  xvf->rows = rows ;
  xvf->cols = cols ;
  xvf->button_rows = button_rows ;
  if (!display_size)
    display_size = DISPLAY_SIZE ;
  xvf->display_size = display_size ;

  xvf->dimages = (DIMAGE **)calloc(rows, sizeof(DIMAGE *)) ;
  if (!xvf->dimages)
    return(NULL) ;
  for (row = 0 ; row < rows ; row++)
  {
    xvf->dimages[row] = (DIMAGE *)calloc(cols, sizeof(DIMAGE)) ;
    if (!xvf->dimages[row])
      return(NULL) ;
  }

  xvCreateFrame(xvf, name) ;

  window_fit(xvf->frame);

  if (poll)
  {
    timer.it_value.tv_usec = 1;
    timer.it_interval.tv_usec = 1;
    notify_set_itimer_func(xvf->frame, poll, ITIMER_REAL, &timer, NULL);
    if (notify_errno)
    {
      fprintf(stderr, "notifier error %d\n", notify_errno) ;
      notify_perror("notify error installing poll routine:") ;
    }
  }

  xvHipsCmdFrameInit() ;

  xvInitImages(xvf) ;

  return(xvf) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void
xvCreateFrame(XV_FRAME *xvf, char *name)
{
  int width, height ;

  xvInitColors(xvf) ;
  width = xvf->cols * xvf->display_size + (xvf->cols-1)*WINDOW_PAD ;
  height = PANEL_HEIGHT + 
    xvf->rows * xvf->display_size + (xvf->rows-1) * CHAR_HEIGHT ;

  xvf->frame = (Frame)xv_create(NULL, FRAME,
                           FRAME_LABEL, name,
                           XV_X,        FRAME_X,
                           XV_Y,        FRAME_Y,
                           XV_WIDTH,    width,
                           XV_HEIGHT,   height,
                           NULL);
  xvf->display = (Display *)xv_get(xvf->frame,XV_DISPLAY);
  xvf->screen = DefaultScreen(xvf->display);
  xvf->black_pixel = BlackPixel(xvf->display, xvf->screen);
  xvf->white_pixel = WhitePixel(xvf->display, xvf->screen);

  xvf->panel = (Panel)xv_create(xvf->frame, PANEL, XV_X, 0, XV_Y,  0, NULL) ;
  xvf->gc = DefaultGC(xvf->display,DefaultScreen(xvf->display));
  XSetForeground(xvf->display, xvf->gc, xvf->black_pixel);
  XSetBackground(xvf->display, xvf->gc, xvf->white_pixel);

  xv_create(xvf->panel, PANEL_BUTTON,
            XV_X,                  3,
            XV_Y,                  FIRST_BUTTON_ROW,
            PANEL_LABEL_STRING,    "Quit",
            PANEL_NOTIFY_PROC,     buttonQuit,
            NULL);

  /* create a region for messages */
  xvf->msg_item[0] = (Panel_item) 
    xv_create(xvf->panel,        PANEL_MESSAGE,
             PANEL_LABEL_BOLD,   TRUE,
             XV_X,               5,
             XV_Y,               3,
             PANEL_LABEL_STRING, 
             xvf->msg_str[0],
             NULL);
  xvf->msg_item[1] = (Panel_item) 
    xv_create(xvf->panel,        PANEL_MESSAGE,
             PANEL_LABEL_BOLD,   TRUE,
             PANEL_LABEL_STRING, xvf->msg_str[1],
             XV_X,               SEVENTH_BUTTON_COL,
             XV_Y,               5,
             NULL);

}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
XVprintf(XV_FRAME *xvf, int which, ...)
{
  va_list  args ;
  char     *fmt ;
  int      len ;

  va_start(args, which) ;
  fmt = va_arg(args, char *) ;
  vsprintf(xvf->msg_str[which], fmt, args) ;
  len = strlen(xvf->msg_str[which]) ;
  xv_set(xvf->msg_item[which], PANEL_LABEL_STRING, xvf->msg_str[which], NULL);

  va_end(args) ;
  XSync(xvf->display, 0) ;
  return(len) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/

#define MAX_COLORS   256

#define PURPLE_INDEX 250
#define YELLOW_INDEX 251
#define CYAN_INDEX   252
#define GREEN_INDEX  253
#define BLUE_INDEX   254
#define RED_INDEX    255

static int           ncolors = 0 ;
static Xv_singlecolor        xcolors[MAX_COLORS] ;

static void
xvInitColors(XV_FRAME *xvf)
{
  int           i ;
  unsigned long *pix_vals ;

  for (i = 0 ; i < MAX_COLORS ; i++)
  {
    switch (i)
    {
    case PURPLE_INDEX:
      xcolors[i].red = xcolors[i].blue = 255 ;
      xcolors[i].green = 200 ;
      break ;
    case YELLOW_INDEX:
      xcolors[i].green = xcolors[i].red = 255 ;
      xcolors[i].blue = 0 ;
      break ;
    case CYAN_INDEX:
      xcolors[i].green = xcolors[i].blue = 255 ;
      xcolors[i].red = 128 ;
      break ;
    case RED_INDEX:
      xcolors[i].green = xcolors[i].blue = 200 ;
      xcolors[i].red = 255 ;
      break ;
    case BLUE_INDEX:
      xcolors[i].green = xcolors[i].red = 200 ;
      xcolors[i].blue = 255 ;
      break ;
    case GREEN_INDEX:
      xcolors[i].red = xcolors[i].blue = 100 ;
      xcolors[i].green = 255 ;
      break ;
    default:
      xcolors[i].red = i ;
      xcolors[i].green = i ;
      xcolors[i].blue = i ;
      break ;
    }
  }

  xvf->cms = (Cms)xv_create(NULL, CMS,
                       CMS_SIZE,    MAX_COLORS,
                       CMS_COLORS,  xcolors,
                       CMS_TYPE,    XV_STATIC_CMS,
                       NULL) ;
  
  pix_vals = (unsigned long *)xv_get(xvf->cms, CMS_INDEX_TABLE) ;
  xvf->red_pixel = pix_vals[RED_INDEX] ;
  xvf->blue_pixel = pix_vals[BLUE_INDEX] ;
  xvf->green_pixel = pix_vals[GREEN_INDEX] ;
  xvf->cyan_pixel = pix_vals[CYAN_INDEX] ;
  xvf->purple_pixel = pix_vals[PURPLE_INDEX] ;
  xvf->yellow_pixel = pix_vals[YELLOW_INDEX] ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void
xvInitImages(XV_FRAME *xvf)
{
  XGCValues   GCvalues ;
  DIMAGE     *dimage ;
  int        row, col, x, y ;

  y = PANEL_HEIGHT ;
  for (row = 0 ; row < xvf->rows ; row++)
  {
    x = 0 ;
    for (col = 0 ; col < xvf->cols ; col++)
    {
      dimage = &xvf->dimages[row][col] ;

      /* last canvas has its own colormap */
      if (row == xvf->rows-1 && col == xvf->cols-1)
        dimage->canvas =
          (Canvas)xv_create(xvf->frame, CANVAS,
                            XV_X,                  x,
                            XV_Y,                  y,
                            XV_HEIGHT,             xvf->display_size,
                            XV_WIDTH,              xvf->display_size,
                            CANVAS_X_PAINT_WINDOW, TRUE,
                            CANVAS_REPAINT_PROC,   xv_dimage_repaint,
                            CANVAS_RETAINED,       FALSE,
                            WIN_CMS,     xvf->cms,
                            NULL);
      else
        dimage->canvas =
          (Canvas)xv_create(xvf->frame, CANVAS,
                            XV_X,                  x,
                            XV_Y,                  y,
                            XV_HEIGHT,             xvf->display_size,
                            XV_WIDTH,              xvf->display_size,
                            CANVAS_X_PAINT_WINDOW, TRUE,
                            CANVAS_REPAINT_PROC,   xv_dimage_repaint,
                            CANVAS_RETAINED,       FALSE,
                            NULL);

      dimage->title_item = (Panel_item)
        xv_create(xvf->panel, PANEL_MESSAGE,
                  PANEL_LABEL_BOLD, TRUE,
                  XV_X, x,
                  XV_Y, y-CHAR_HEIGHT+CHAR_PAD,
                  PANEL_LABEL_STRING, 
                  dimage->title_string,
                  NULL);

      dimage->dispImage = ImageAlloc(xvf->display_size, xvf->display_size, 
                                    PFBYTE, 1) ;
      dimage->ximage = xvCreateXimage(xvf, dimage->dispImage) ;
      xv_set(canvas_paint_window(dimage->canvas),
             WIN_EVENT_PROC, xv_dimage_event_handler,
             WIN_CONSUME_EVENTS,  MS_LEFT,
             LOC_DRAG,
             MS_RIGHT,
             MS_MIDDLE,
             NULL,
             NULL);
      dimage->window = 
        (Window)xv_get(canvas_paint_window(dimage->canvas),XV_XID);
      dimage->clearGC = 
        XCreateGC(xvf->display, dimage->window, (unsigned long )0, &GCvalues);
      XSetFunction(xvf->display, dimage->clearGC, GXclear);
      dimage->xorGC = 
        XCreateGC(xvf->display, dimage->window, (unsigned long)0, &GCvalues);
      XSetFunction(xvf->display, dimage->xorGC, GXinvert);

      dimage->greenGC = 
        XCreateGC(xvf->display, dimage->window,(unsigned long )0, &GCvalues);
      XSetFunction(xvf->display, dimage->greenGC, GXcopy) ;
      XSetBackground(xvf->display, dimage->greenGC, xvf->green_pixel) ;
      XSetForeground(xvf->display, dimage->greenGC, xvf->green_pixel) ;

      dimage->blueGC = 
        XCreateGC(xvf->display, dimage->window,(unsigned long )0, &GCvalues);
      XSetFunction(xvf->display, dimage->blueGC, GXcopy) ;
      XSetBackground(xvf->display, dimage->blueGC, xvf->blue_pixel) ;
      XSetForeground(xvf->display, dimage->blueGC, xvf->blue_pixel) ;

      dimage->purpleGC = 
        XCreateGC(xvf->display, dimage->window,(unsigned long )0, &GCvalues);
      XSetFunction(xvf->display, dimage->purpleGC, GXcopy) ;
      XSetBackground(xvf->display, dimage->purpleGC, xvf->purple_pixel) ;
      XSetForeground(xvf->display, dimage->purpleGC, xvf->purple_pixel) ;

      dimage->yellowGC = 
        XCreateGC(xvf->display, dimage->window,(unsigned long )0, &GCvalues);
      XSetFunction(xvf->display, dimage->yellowGC, GXcopy) ;
      XSetBackground(xvf->display, dimage->yellowGC, xvf->yellow_pixel) ;
      XSetForeground(xvf->display, dimage->yellowGC, xvf->yellow_pixel) ;

      dimage->cyanGC = 
        XCreateGC(xvf->display, dimage->window,(unsigned long )0, &GCvalues);
      XSetFunction(xvf->display, dimage->cyanGC, GXcopy) ;
      XSetBackground(xvf->display, dimage->cyanGC, xvf->cyan_pixel) ;
      XSetForeground(xvf->display, dimage->cyanGC, xvf->cyan_pixel) ;

      dimage->redGC = 
        XCreateGC(xvf->display, dimage->window,(unsigned long )0, &GCvalues);
      XSetFunction(xvf->display, dimage->redGC, GXcopy) ;
      XSetBackground(xvf->display, dimage->redGC, xvf->red_pixel) ;
      XSetForeground(xvf->display, dimage->redGC, xvf->red_pixel) ;

      dimage->whiteGC = 
        XCreateGC(xvf->display, dimage->window,(unsigned long )0, &GCvalues);
      XSetForeground(xvf->display, dimage->whiteGC, xvf->white_pixel);
      XSetBackground(xvf->display, dimage->whiteGC, xvf->white_pixel);

      x += WINDOW_PAD + xvf->display_size ;
    }
    y += CHAR_HEIGHT + xvf->display_size ;
  }
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void 
XVclearImageTitle(XV_FRAME *xvf, int which) 
{
  XVshowImageTitle(xvf, which, "                                       ") ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void 
XVclearImage(XV_FRAME *xvf, int which, int dotitle)
{
  DIMAGE    *dimage ;

  dimage = xvGetDimage(which, 0) ;
  if (!dimage)
    return ;
  XClearArea(xvf->display, dimage->window, 0, 0, 0, 0, False) ;
  if (dotitle)
    XVclearImageTitle(xvf, which) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
xvRepaintImage(XV_FRAME *xvf, int which)
{
  DIMAGE    *dimage ;
  IMAGE *image ;

  dimage = xvGetDimage(which, 0) ;
  if (!dimage)
    return ;
  image = dimage->dispImage ;
  XPutImage(xvf->display, (Drawable)dimage->window, xvf->gc, dimage->ximage, 
            0, 0, 0, 0, image->cols, image->rows);
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVshowImage(XV_FRAME *xvf, int which, IMAGE *image, int frame)
{
  float     scale, fmin, fmax ;
  DIMAGE    *dimage ;
  IMAGE *dispImage ;

  dimage = xvGetDimage(which, 1) ;
  if (!dimage)
    return ;
  dimage->used = 1 ;
  dimage->frame = frame ;
  scale = 
    (float)xvf->display_size / (float)(MAX(image->cols, image->rows)) ;
  dimage->sourceImage = image ;

  dimage->scale = scale ;

  if (!ImageCheckSize(image, GtmpFloatImage, image->cols, image->rows, 1))
  {
    if (GtmpFloatImage)
      ImageFree(&GtmpFloatImage) ;
    if (GtmpByteImage)
      ImageFree(&GtmpByteImage) ;
    if (GtmpByteImage2)
      ImageFree(&GtmpByteImage2) ;
    GtmpFloatImage = ImageAlloc(image->cols, image->rows, PFFLOAT, 1) ;
    GtmpByteImage = ImageAlloc(image->cols, image->rows, PFBYTE, 1) ;
    GtmpByteImage2 = ImageAlloc(image->cols, image->rows, PFBYTE, 1) ;
  }
  else
  {
    GtmpFloatImage->cols = GtmpByteImage->cols = GtmpByteImage2->cols = 
      image->cols ;
    GtmpFloatImage->rows = GtmpByteImage->rows = GtmpByteImage2->rows = 
      image->rows ;
  }

  ImageCopyFrames(image, GtmpFloatImage, frame, 1, 0) ;
  if (dimage->rescale_range || image->num_frame == 1)
    ImageScale(GtmpFloatImage, GtmpFloatImage, 0, MAX_DISP_VAL) ;
  else   /* use entire sequence to compute display range */
  {
    ImageValRange(image, &fmin, &fmax) ;
    ImageScaleRange(GtmpFloatImage, fmin, fmax, 0, MAX_DISP_VAL) ;
  }

  ImageCopy(GtmpFloatImage, GtmpByteImage) ;
  h_invert(GtmpByteImage, GtmpByteImage2) ;

#if 0
  ImageResize(GtmpByteImage2, dimage->dispImage, 
              dimage->dispImage->rows, dimage->dispImage->cols) ;
#else
  ImageRescale(GtmpByteImage2, dimage->dispImage, scale) ;
#endif

#if 0
ImageWrite(GtmpByteImage, "i1.hipl") ;
ImageWrite(GtmpByteImage2, "i2.hipl") ;
ImageWrite(dimage->dispImage, "i3.hipl") ;
#endif

  xvRepaintImage(xvf, which) ;
  XFlush(xvf->display); 
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void
xv_dimage_repaint(Canvas canvas, Xv_Window window, Rectlist *repaint_area)
{
  int    row, col, which ;
  DIMAGE *dimage ;

  dimage = NULL ;
  for (row = 0 ; row < xvf->rows ; row++)
  {
    for (col = 0 ; col < xvf->cols ; col++)
    {
      if (xvf->dimages[row][col].canvas == canvas)
      {
        dimage = &xvf->dimages[row][col] ;
        which = row * xvf->cols + col ;
      }
    }
  }
  if (dimage && dimage->used)
    xvRepaintImage(xvf, which) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static DIMAGE *
xvGetDimage(int which, int alloc)
{
  DIMAGE *dimage ;
  int    row, col ;

  row = which / xvf->cols ;
  col = which % xvf->cols ;
  if (row >= xvf->rows || col >= xvf->cols)
    return(NULL) ;

  dimage = &xvf->dimages[row][col] ;

  if (!alloc && !dimage->used)
    return(NULL) ;

  return(dimage) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
/*
  the xview coordinate system is (0,0) in the upper left, and increases
  down and to the right.
*/

static void
xv_dimage_event_handler(Xv_Window xv_window, Event *event)
{
  int    x, y ;
  float  val ;
  Window window ;
  int    row, col, which ;
  DIMAGE *dimage ;
  char   *str ;

  window = event_window(event) ;

  dimage = NULL ;
  for (row = 0 ; row < xvf->rows ; row++)
  {
    for (col = 0 ; col < xvf->cols ; col++)
    {
      if (canvas_paint_window(xvf->dimages[row][col].canvas) == xv_window)
      {
        dimage = &xvf->dimages[row][col] ;
        which = row * xvf->cols + col ;
      }
    }
  }
  x = event_x(event) ;
  y = event_y(event) ;  
  
  if (!dimage)
  {
    fprintf(stderr, "(%d, %d) in win %ld\n", x, y, event_window(event)) ;
    fprintf(stderr, "could not find appropriate window in event!\n") ;
    return ;
  }
  if (!dimage->used)
    return ;

  x = (int)((float)x / dimage->scale) ;
  y = (int)((float)y / dimage->scale) ;

  /* convert y to hips coordinate sysem */
  y = (dimage->sourceImage->rows-1) - y ;  

  /* do boundary checking */
  if (y < 0) y = 0 ;
  if (x < 0) x = 0 ;
  if (y >= dimage->sourceImage->rows) y = dimage->sourceImage->rows - 1 ;
  if (x >= dimage->sourceImage->cols) x = dimage->sourceImage->cols - 1 ;

  switch (event_id(event)) 
  {
  case MS_RIGHT:
    xv_set(hips_cmd_frame, XV_SHOW, TRUE, NULL) ;
    hips_cmd_source = which ;
    break ;
  case MS_MIDDLE:
    if (event_is_down(event))
    {
    }
    break ;
  case MS_LEFT:
  case LOC_DRAG:
    switch (dimage->sourceImage->pixel_format)
    {
    case PFFLOAT:
      val = *IMAGEFseq_pix(dimage->sourceImage, x, y, dimage->frame) ;
      break ;
    case PFBYTE:
      val = (float)*IMAGEseq_pix(dimage->sourceImage, x, y, dimage->frame) ;
      break ;
    }
    for (str = dimage->title_string ; *str && isspace(*str) ; str++)
    {}
    XVprintf(xvf, 0, "%10.10s: (%3d, %3d) --> %2.4f\n", str, x, y, val) ;

    /* extract this location as center of new template */
    if (event_id(event) == LOC_DRAG || event_is_down(event))
    {
    }
    else
      if (event_is_up(event))
      {
      }
    break ;
  default:
    return ;
  }
  if (XVevent_handler)
  {
    event_x(event) = x ;
    event_y(event) = y ;
    (*XVevent_handler)(event, dimage) ;
  }
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVsetParms(void (*event_handler)(Event *event, DIMAGE *dimage))
{
  XVevent_handler = event_handler ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVsetQuitFunc(void (*quit_func)(void))
{
  XVquit_func = quit_func ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
XVshowImageTitle(XV_FRAME *xvf, int which, ...)
{
  Panel_item item ;
  va_list args ;
  char    *fmt, *str ;
  int     len, row, col, i, spaces ;

  va_start(args, which) ;
#if 0
  which = va_arg(args, int) ;
#endif
  fmt = va_arg(args, char *) ;

  row = which / xvf->cols ;
  col = which % xvf->cols ;
  if (row >= xvf->rows)
    return(0) ;

  item = xvf->dimages[row][col].title_item ;
  str = xvf->dimages[row][col].title_string ;
  vsprintf(str, fmt, args) ;
  spaces = strlen(str) ;
  spaces = MAX_TITLE_CHARS - strlen(str) ;
  spaces = nint((float)spaces/1.0f) ;
  for (i = 0 ; i < spaces ; i++)
    str[i] = ' ' ;
  vsprintf(str+i, fmt, args) ;
  StrUpper(str) ;

  len = strlen(str) ;

  if (len > (MAX_TITLE_CHARS+spaces/3))
    str[len = MAX_TITLE_CHARS] = 0 ;
  va_end(args) ;
  xv_set(item, PANEL_LABEL_STRING, str, NULL);
  XSync(xvf->display, 0) ;
  return(len) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static XImage *
xvCreateXimage(XV_FRAME *xvf, IMAGE *image)
{
  int    rows, cols ;
  XImage *ximage ;

  if (image->pixel_format != PFBYTE)
  {
    fprintf(stderr,
            "xvCreateXimage: unsupported pixel format %d, must be BYTE\n",
            image->pixel_format) ;
    exit(4) ;
  }
  rows = image->rows ;
  cols = image->cols ;
  ximage = 
    XCreateImage(xvf->display, 
                 DefaultVisual(xvf->display, DefaultScreen(xvf->display)),
                 8,                   /* 8 bits per pixel */
                 ZPixmap,             /* not a bitmap */
                 0, image->image,
                 cols,
                 rows,
                 8,                   /* 8 bits per pixel */
                 cols);
  return(ximage) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static Panel_setting
xvHipsCommand(Panel_item item, Event *event)
{
  int     row, col ;
  DIMAGE  *dimage ;

  strcpy(hips_cmd_str, (char *)xv_get(hips_cmd_panel_item, PANEL_VALUE)) ;
  xv_set(hips_cmd_frame, XV_SHOW, FALSE, NULL) ;

  dimage = xvGetDimage(hips_cmd_source, 0) ;
  if (!dimage)
    return ;
#if 1
  ImageWrite(dimage->sourceImage, "out.hipl") ;
#else
  if (ImageWriteFrames(dimage->sourceImage, "out.hipl", dimage->frame, 1) < 0)
  {
    XVprintf(xvf, 0, "write failed\n") ;
    return(0) ;
  }
#endif

  if (strlen(hips_cmd_str) < 4)
    return(0) ;

  fprintf(stderr, "executing hips command '%s'\n", hips_cmd_str) ;

  system(hips_cmd_str) ;
  if (strstr(hips_cmd_str, "in.hipl"))
  {
    ImageReadInto("in.hipl", dimage->sourceImage, 0) ;
    XVshowImage(xvf, hips_cmd_source, dimage->sourceImage, 0) ;
  }

  XFlush(xvf->display); 
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void
xvHipsCmdFrameInit(void) 
{
  hips_cmd_frame = (Frame)
    xv_create(NULL, FRAME,
              XV_HEIGHT, HIPS_CMD_ROWS,
              XV_WIDTH,  HIPS_CMD_COLS,
              XV_X,      600,
              XV_Y,      310,
              FRAME_LABEL, "HIPS COMMAND",
              XV_SHOW,   FALSE,
              NULL);

  hips_cmd_panel = 
    (Panel)xv_create(hips_cmd_frame, PANEL, XV_X, 0, XV_Y,0,NULL);
  hips_cmd_display = (Display *)xv_get(hips_cmd_frame, XV_DISPLAY);
  hips_cmd_panel_item = (Panel_item)
    xv_create(hips_cmd_panel, PANEL_TEXT,
              PANEL_VALUE_STORED_LENGTH,   300,
              PANEL_VALUE_DISPLAY_LENGTH,  40,
              XV_SHOW,                     TRUE,
              PANEL_NOTIFY_PROC,           xvHipsCommand,
              PANEL_LABEL_STRING,          "Hips Command: ",
              PANEL_VALUE,                 hips_cmd_str,
              NULL) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void 
buttonQuit(Panel_item item, Event *event)
{
  xv_destroy_safe(xvf->frame);
  xv_destroy_safe(hips_cmd_frame) ;
  if (XVquit_func)
    (*XVquit_func)() ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVdrawPoint(XV_FRAME *xvf, int which, int x, int y, int color)
{
  GC      gc ;
  Display *display ;
  Window  window ;
  int     x0, y0, x1, y1 ;
  float   scale ;
  DIMAGE  *dimage ;

  dimage = xvGetDimage(which, 0) ;
  if (!dimage)
    return ;
  
  display = xvf->display ;
  window = dimage->window ;
  switch (color)
  {
  case XWHITE:
    gc = dimage->whiteGC ;
    break ;
  case XBLUE:
    gc = dimage->blueGC ;
    break ;
  case XCYAN:
    gc = dimage->cyanGC ;
    break ;
  case XGREEN:
    gc = dimage->greenGC ;
    break ;
  case XRED:
  default:
    gc = dimage->redGC ;
    break ;
  }

  scale = dimage->scale ;
  x = nint(((float)x+0.5f) * scale) ;
  y = nint((float)(((dimage->sourceImage->rows-1) - y) + 0.5f) * scale) ;
  XSetLineAttributes(display, gc, 0, LineSolid, CapRound, JoinBevel) ;
  
  x0 = x - 4 ;
  y0 = y - 4 ;
  x1 = x + 4 ;
  y1 = y + 4 ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;

  x0 = x + 4 ;
  y0 = y - 4 ;
  x1 = x - 4 ;
  y1 = y + 4 ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;

/*  XFlush(display);*/
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVdrawBox(XV_FRAME *xvf, int which, int x, int y, int dx, int dy, int color)
{
  GC      gc ;
  Display *display ;
  Window  window ;
  int     x0, y0, x1, y1 ;
  DIMAGE  *dimage ;
  float   scale ;

  dimage = xvGetDimage(which, 0) ;
  if (!dimage)
    return ;
  
  display = xvf->display ;
  window = dimage->window ;
  switch (color)
  {
  case XCYAN:
    gc = dimage->cyanGC ;
    break ;
  case XWHITE:
    gc = dimage->whiteGC ;
    break ;
  case XBLUE:
    gc = dimage->blueGC ;
    break ;
  case XGREEN:
    gc = dimage->greenGC ;
    break ;
  case XYELLOW:
    gc = dimage->yellowGC ;
    break ;
  case XPURPLE:
    gc = dimage->purpleGC ;
    break ;
  case XRED:
  default:
    gc = dimage->redGC ;
    break ;
  }

  /* convert to window coordinate system */
  scale = dimage->scale ;

  x = nint(((float)x) * scale) ;
  y = nint((float)(((dimage->sourceImage->rows) - y)) * scale) ;
  dx = nint((float)dx * scale) ;
  dy = nint((float)-dy * scale) ;

  XSetLineAttributes(display, gc, 0, LineSolid, CapRound, JoinBevel) ;

  /* top line */
  x0 = x ;
  y0 = y ;
  x1 = x + dx ;
  y1 = y0 ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;

  /* bottom line */
  x0 = x ;
  y0 = y + dy ;
  x1 = x + dx ;
  y1 = y + dy ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;

  /* draw left line */
  x0 = x ;
  y0 = y ;
  x1 = x ;
  y1 = y0 + dy ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;

  /* draw right line */
  x0 = x + dx ;
  y0 = y ;
  x1 = x + dx ;
  y1 = y0 + dy ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;

/*  XFlush(display);*/
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVdrawLine(XV_FRAME *xvf, int which, int x, int y, int dx, int dy, int color)
{
  GC      gc ;
  Display *display ;
  Window  window ;
  int     x0, y0, x1, y1 ;
  DIMAGE  *dimage ;
  float   scale ;

  dimage = xvGetDimage(which, 0) ;
  if (!dimage)
    return ;

  display = xvf->display ;
  window = dimage->window ;

  /* convert to window coordinate system */
  scale = dimage->scale ;
  x = nint(((float)x +0.5f)* scale) ;
  y = nint((float)(((dimage->sourceImage->rows-1) - y) +0.5f)* scale) ;
  dx = nint((float)dx * scale) ;
  dy = nint((float)-dy * scale) ;

  switch (color)
  {
  case XWHITE:
    gc = dimage->whiteGC ;
    break ;
  case XCYAN:
    gc = dimage->cyanGC ;
    break ;
  case XBLUE:
    gc = dimage->blueGC ;
    break ;
  case XGREEN:
    gc = dimage->greenGC ;
    break ;
  case XRED:
  default:
    gc = dimage->redGC ;
    break ;
  }

  XSetLineAttributes(display, gc, 0, LineSolid, CapRound, JoinBevel) ;

  /* top line */
  x0 = x ;
  y0 = y ;
  x1 = x + dx ;
  y1 = y + dy ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;


/*  XFlush(display);*/
}
#define ARROW_HEAD_SIZE    8.0f
#define ARROW_HEAD_ANGLE   RADIANS(30.0f)

void
XVdrawArrow(XV_FRAME *xvf, int which, int x, int y,float dx,float dy,int color)
{
  GC      gc ;
  Display *display ;
  Window  window ;
  int     x0, y0, x1, y1 ;
  DIMAGE  *dimage ;
  float   scale, theta, theta0 ;

  dimage = xvGetDimage(which, 0) ;
  if (!dimage)
    return ;

  display = xvf->display ;
  window = dimage->window ;

  /* convert to window coordinate system */
  scale = dimage->scale ;
  x = nint(((float)x+0.5f) * scale) ;
  y = nint(((float)((dimage->sourceImage->rows-1) - y) + 0.5f) * scale) ;
  dx = nint(dx * scale) ;
  dy = nint(-dy * scale) ;

  switch (color)
  {
  case XWHITE:
    gc = dimage->whiteGC ;
    break ;
  case XCYAN:
    gc = dimage->cyanGC ;
    break ;
  case XBLUE:
    gc = dimage->blueGC ;
    break ;
  case XGREEN:
    gc = dimage->greenGC ;
    break ;
  case XRED:
  default:
    gc = dimage->redGC ;
    break ;
  }

  XSetLineAttributes(display, gc, 0, LineSolid, CapRound, JoinBevel) ;

  /* top line */
  x0 = x ;
  y0 = y ;
  x1 = nint((float)x + dx) ;
  y1 = nint((float)y + dy) ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;

  /* now draw arrow head */
  if (iszero(dx) && iszero(dy))
    theta0 = 0 ;
  else
    theta0 = atan2(dy, dx) ;

  x0 = x1 ;
  y0 = y1 ;

  /* top portion */
  theta = theta0 + PI-ARROW_HEAD_ANGLE ;
  x1 = x0 + nint(ARROW_HEAD_SIZE * cos(theta)) ;
  y1 = y0 + nint(ARROW_HEAD_SIZE * sin(theta)) ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;

  /* bottom portion */
  theta = theta0 + ARROW_HEAD_ANGLE - PI ;
  x1 = x0 + nint(ARROW_HEAD_SIZE * cos(theta)) ;
  y1 = y0 + nint(ARROW_HEAD_SIZE * sin(theta)) ;
  XDrawLine(display, window, gc, x0, y0, x1, y1) ;
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
              xo,yo is image coordinate of center of area, x1,y1 and
              x2,y2 are left and right side of area in image coords.
----------------------------------------------------------------------*/
void
XVshowVectorImage(XV_FRAME *xvf, int which, int xo, int yo, 
                  int width, int height, int color, IMAGE *image)
{
  int     x, y, rows, cols, x0, y0, x1, y1, half_width, half_height ;
  float   dx, dy, len ;

  rows = image->rows ;
  cols = image->cols ;
  half_width = (width - 1) / 2 ;
  half_height = (height - 1) / 2 ;
  x1 = xo + half_width ;
  y1 = yo + half_height ;
  if (x1 >= cols)
    x1 = cols - 1 ;
  if (y1 >= rows)
    y1 = rows - 1 ;
  x0 = xo - half_width ;
  y0 = yo - half_height ;
  if (x0 < 0) 
    x0 = 0 ;
  if (y0 < 0)
    y0 = 0 ;
  
  for (y = y0 ; y <= y1 ; y++)
  {
    for (x = x0 ; x <= x1 ; x++)
    {
      dx = *IMAGEFseq_pix(image, x, y, 0) ;
      dy = *IMAGEFseq_pix(image, x, y, 1) ;
      len = 2*sqrt (dx * dx + dy * dy) ;
      if (iszero(dx) && iszero(dy))
        XVdrawPoint(xvf, which, x-x0, y-y0, color) ;
      else
        XVdrawArrow(xvf, which, x-x0, y-y0, dx/len, dy/len, color) ;
    }
  }
}
