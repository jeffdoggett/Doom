// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <X11/extensions/XShm.h>
// Had to dig up XShm.c for this one.
// It is in the libXext, but not in the XFree86 headers.
#ifdef LINUX
int XShmGetEventBase( Display* dpy ); // problems with g++?
#endif

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "am_map.h"

#include "doomdef.h"

#define POINTER_WARP_COUNTDOWN	1

static Display*		X_display=0;
static Window		X_mainWindow;
static Colormap		X_cmap;
static Visual*		X_visual;
static GC		X_gc;
static XEvent		X_event;
static int		X_screen;
static XVisualInfo	X_visualinfo;
static XImage*		image;
static int		X_width;
static int		X_height;

// MIT SHared Memory extension.
static boolean		doShm;

static XShmSegmentInfo	X_shminfo;
static int		X_shmeventtype;

// Fake mouse handling.
// This cannot work properly w/o DGA.
// Needs an invisible mouse cursor at least.
static boolean		grabMouse;
static int		doPointerWarp = POINTER_WARP_COUNTDOWN;

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int		multiply=1;

unsigned int SCREENWIDTH = MAXSCREENWIDTH;
unsigned int SCREENHEIGHT = MAXSCREENHEIGHT;
unsigned int ORIGSCREENWIDTH = 320;	// in original Doom
unsigned int ORIGSCREENHEIGHT = 200;

static unsigned int	palette_table [256];
static XColor		colors [256];

static int	lastmousex = 0;
static int	lastmousey = 0;
static boolean	mousemoved = false;
static boolean	shmFinished;
static unsigned char devparm_black;
static unsigned char devparm_white;

extern int	novert;

/***************************************************************************/
//
//  Translates the key currently in X_event
//

int xlatekey(void)
{
  int rc;

  rc = XKeycodeToKeysym (X_display, X_event.xkey.keycode, 0);
  switch (rc)
  {
    case XK_Left:	rc = KEY_LEFTARROW;	break;
    case XK_Right:	rc = KEY_RIGHTARROW;	break;
    case XK_Down:	rc = KEY_DOWNARROW;	break;
    case XK_Up:		rc = KEY_UPARROW;	break;
    case XK_Escape:	rc = KEY_ESCAPE;	break;
    case XK_Return:	rc = KEY_ENTER;		break;
    case XK_Tab:	rc = KEY_TAB;		break;
    case XK_F1:		rc = KEY_F1;		break;
    case XK_F2:		rc = KEY_F2;		break;
    case XK_F3:		rc = KEY_F3;		break;
    case XK_F4:		rc = KEY_F4;		break;
    case XK_F5:		rc = KEY_F5;		break;
    case XK_F6:		rc = KEY_F6;		break;
    case XK_F7:		rc = KEY_F7;		break;
    case XK_F8:		rc = KEY_F8;		break;
    case XK_F9:		rc = KEY_F9;		break;
    case XK_F10:	rc = KEY_F10;		break;
    case XK_F11:	rc = KEY_F11;		break;
    case XK_F12:	rc = KEY_F12;		break;
    case XK_Print:	rc = KEY_PRINTSCRN;	break;
    case XK_BackSpace:
    case XK_Delete:	rc = KEY_BACKSPACE;	break;
    case XK_Pause:	rc = KEY_PAUSE;		break;
    case XK_KP_Add:	rc = '+';		break;
    case XK_KP_Equal:
    case XK_equal:	rc = KEY_EQUALS;	break;
    case XK_KP_Subtract:
    case XK_minus:	rc = KEY_MINUS;		break;
    case XK_Shift_L:	rc = KEY_LSHIFT;	break;
    case XK_Shift_R:	rc = KEY_RSHIFT;	break;
    case XK_Control_L:	rc = KEY_LCTRL;		break;
    case XK_Control_R:	rc = KEY_RCTRL;		break;
    case XK_Alt_L:
    case XK_Meta_L:	rc = KEY_LALT;		break;
    case XK_Alt_R:
    case XK_Meta_R:	rc = KEY_RALT;		break;

    default:
      if (rc >= XK_space && rc <= XK_asciitilde)
	  rc = rc - XK_space + ' ';
      if (rc >= 'A' && rc <= 'Z')
	  rc = rc - 'A' + 'a';
      break;
  }

  return rc;
}

/***************************************************************************/

void I_ShutdownGraphics(void)
{
  if (doShm)
  {
    // Detach from X server
    if (!XShmDetach(X_display, &X_shminfo))
	    printf("XShmDetach() failed in I_ShutdownGraphics()");

    // Release shared memory.
    shmdt(X_shminfo.shmaddr);
    shmctl(X_shminfo.shmid, IPC_RMID, 0);
  }
  // Paranoia.
  image->data = NULL;
}

/***************************************************************************/
//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

/***************************************************************************/

void I_GetEvent(void)
{
    int dy;
    event_t event;

    // put event-grabbing stuff in here
    XNextEvent(X_display, &X_event);
    switch (X_event.type)
    {
      case KeyPress:
	event.type = ev_keydown;
	event.data1 = xlatekey();
	D_PostEvent(&event);
	// fprintf(stderr, "k");
	break;
      case KeyRelease:
	event.type = ev_keyup;
	event.data1 = xlatekey();
	D_PostEvent(&event);
	// fprintf(stderr, "ku");
	break;
      case ButtonPress:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xbutton.state & Button1Mask)
	    | (X_event.xbutton.state & Button2Mask ? 2 : 0)
	    | (X_event.xbutton.state & Button3Mask ? 4 : 0)
	    | (X_event.xbutton.button == Button1)
	    | (X_event.xbutton.button == Button2 ? 2 : 0)
	    | (X_event.xbutton.button == Button3 ? 4 : 0);
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	// fprintf(stderr, "b");
	break;
      case ButtonRelease:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xbutton.state & Button1Mask)
	    | (X_event.xbutton.state & Button2Mask ? 2 : 0)
	    | (X_event.xbutton.state & Button3Mask ? 4 : 0);
	// suggest parentheses around arithmetic in operand of |
	event.data1 =
	    event.data1
	    ^ (X_event.xbutton.button == Button1 ? 1 : 0)
	    ^ (X_event.xbutton.button == Button2 ? 2 : 0)
	    ^ (X_event.xbutton.button == Button3 ? 4 : 0);
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	// fprintf(stderr, "bu");
	break;
      case MotionNotify:
	event.type = ev_mouse;
	if (novert)
	  dy = lastmousey;
	else
	  dy = X_event.xmotion.y;
	event.data1 =
	    (X_event.xmotion.state & Button1Mask)
	    | (X_event.xmotion.state & Button2Mask ? 2 : 0)
	    | (X_event.xmotion.state & Button3Mask ? 4 : 0);
	event.data2 = (X_event.xmotion.x - lastmousex) << 2;
	event.data3 = (lastmousey - dy) << 2;

	if (event.data2 || event.data3)
	{
	    lastmousex = X_event.xmotion.x;
	    lastmousey = dy;
	    if (X_event.xmotion.x != X_width/2 &&
		dy != X_height/2)
	    {
		D_PostEvent(&event);
		// fprintf(stderr, "m");
		mousemoved = false;
	    } else
	    {
		mousemoved = true;
	    }
	}
	break;

      case Expose:
      case ConfigureNotify:
	break;

      default:
	if (doShm && X_event.type == X_shmeventtype) shmFinished = true;
	break;
    }

}

/***************************************************************************/

Cursor
createnullcursor
( Display*	display,
  Window	root )
{
    Pixmap cursormask;
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
				 &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}

/***************************************************************************/
//
// I_StartTic
//
void I_StartTic (void)
{
    if (!X_display)
	return;

    while (XPending(X_display))
	I_GetEvent();

    // Warp the pointer back to the middle of the window
    //  or it will wander off - that is, the game will
    //  lose input focus within X11.
    if (grabMouse)
    {
	if (!--doPointerWarp)
	{
	    XWarpPointer( X_display,
			  None,
			  X_mainWindow,
			  0, 0,
			  0, 0,
			  X_width/2, X_height/2);

	    doPointerWarp = POINTER_WARP_COUNTDOWN;
	}
    }

    mousemoved = false;

}

/***************************************************************************/
//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

/***************************************************************************/

static void I_Do_8 (void)
{
  unsigned int i;
  unsigned int j;
  unsigned char p;
  unsigned int w;
  unsigned char * dest;
  unsigned char * source;

  source = screens [0];
  dest = (unsigned char *) image->data;

  switch (multiply)
  {
    case 1:				// Nothing to do....
      break;

    case 2:
      w = X_width;
      i = SCREENHEIGHT;
      do
      {
      	j = SCREENWIDTH;
      	do
      	{
	  p = *source++;
	  dest [w] = p;
	  *dest++ = p;
	  dest [w] = p;
	  *dest++ = p;
	} while (--j);
	dest += w;
      } while (--i);
      break;

    case 3:
      w = X_width;
      i = SCREENHEIGHT;
      do
      {
      	j = SCREENWIDTH;
      	do
      	{
	  p = *source++;
	  dest [w] = p;
	  dest [w*2] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  *dest++ = p;
	} while (--j);
	dest += (w*2);
      } while (--i);
      break;

    case 4:
      w = X_width;
      i = SCREENHEIGHT;
      do
      {
      	j = SCREENWIDTH;
      	do
      	{
	  p = *source++;
	  dest [w] = p;
	  dest [w*2] = p;
	  dest [w*3] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  dest [w*3] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  dest [w*3] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  dest [w*3] = p;
	  *dest++ = p;
	} while (--j);
	dest += (w*3);
      } while (--i);
      break;

    default:
      memcpy (image->data, screens[0], SCREENHEIGHT * SCREENWIDTH);
      break;
  }
}

/***************************************************************************/

static void I_Do_24 (void)
{
  unsigned int i;
  unsigned int j;
  unsigned int p;
  unsigned int w;
  unsigned int * dest;
  unsigned char * source;

  source = screens [0];
  dest = (unsigned int *) image->data;

  switch (multiply)
  {
    case 1:
      i = SCREENHEIGHT * SCREENWIDTH;
      do
      {
	*dest++ = palette_table [*source++];
      } while (--i);
      break;

    case 2:
      w = X_width;
      i = SCREENHEIGHT;
      do
      {
      	j = SCREENWIDTH;
      	do
      	{
	  p = palette_table [*source++];
	  dest [w] = p;
	  *dest++ = p;
	  dest [w] = p;
	  *dest++ = p;
	} while (--j);
	dest += w;
      } while (--i);
      break;

    case 3:
      w = X_width;
      i = SCREENHEIGHT;
      do
      {
      	j = SCREENWIDTH;
      	do
      	{
	  p = palette_table [*source++];
	  dest [w] = p;
	  dest [w*2] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  *dest++ = p;
	} while (--j);
	dest += (w*2);
      } while (--i);
      break;

    case 4:
      w = X_width;
      i = SCREENHEIGHT;
      do
      {
      	j = SCREENWIDTH;
      	do
      	{
	  p = palette_table [*source++];
	  dest [w] = p;
	  dest [w*2] = p;
	  dest [w*3] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  dest [w*3] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  dest [w*3] = p;
	  *dest++ = p;
	  dest [w] = p;
	  dest [w*2] = p;
	  dest [w*3] = p;
	  *dest++ = p;
	} while (--j);
	dest += (w*3);
      } while (--i);
      break;

    default:
      memcpy (image->data, screens[0], SCREENHEIGHT * SCREENWIDTH);
      break;
  }
}

/***************************************************************************/
//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
  static int	lasttic;
  int		tics;
  int		i;
  // UNUSED static unsigned char *bigscreen=0;

  // draws little dots on the bottom of the screen
  if (devparm)
  {
    i = I_GetTime();
    tics = i - lasttic;
    lasttic = i;
    if (tics > 20) tics = 20;

    for (i=0 ; i<tics*2 ; i+=2)
	screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = devparm_white;
    for ( ; i<20*2 ; i+=2)
	screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = devparm_black;
  }

  switch (X_visualinfo.depth)
  {
    case 8:
      I_Do_8 ();
      break;

    case 24:
      I_Do_24 ();
      break;
  }

  if (doShm)
  {
    if (!XShmPutImage(	X_display,
			X_mainWindow,
			X_gc,
			image,
			0, 0,
			0, 0,
			X_width, X_height,
			True ))
	I_Error("XShmPutImage() failed\n");

    // wait for it to finish and processes all input events
    shmFinished = false;
    do
    {
	I_GetEvent();
    } while (!shmFinished);
  }
  else
  {
    // draw the image
    XPutImage (	X_display,
		X_mainWindow,
		X_gc,
		image,
		0, 0,
		0, 0,
		X_width, X_height );

    // sync up with server
    XSync (X_display, False);
  }
}

/***************************************************************************/
//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

/***************************************************************************/
//
// Palette stuff.
//

static Init_Colour_Map (void)
{
  unsigned int i;

#ifdef __cplusplus
  if (X_visualinfo.c_class == PseudoColor && X_visualinfo.depth == 8)
#else
  if (X_visualinfo.class == PseudoColor && X_visualinfo.depth == 8)
#endif
  {
    i = 0;
    do
    {
      colors[i].pixel = i;
      colors[i].flags = DoRed|DoGreen|DoBlue;
    } while (++i < 256);
  }
}

/***************************************************************************/

//
// I_SetPalette
//

void I_SetPalette (byte* palette)
{
  unsigned int c;
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;

  if (devparm)
  {
    devparm_black = AM_load_colour (0, 0, 0, palette);
    devparm_white = AM_load_colour (255, 255, 255, palette);
  }

#ifdef __cplusplus
  if (X_visualinfo.c_class == PseudoColor && X_visualinfo.depth == 8)
#else
  if (X_visualinfo.class == PseudoColor && X_visualinfo.depth == 8)
#endif
  {
    // set the X colormap entries
    colour = 0;
    do
    {
      c = gammatable[usegamma][*palette++];
      colors[colour].red = (c<<8) + c;
      c = gammatable[usegamma][*palette++];
      colors[colour].green = (c<<8) + c;
      c = gammatable[usegamma][*palette++];
      colors[colour].blue = (c<<8) + c;
    } while (++colour < 256);

    // store the colors to the current colormap
    XStoreColors (X_display, X_cmap, colors, 256);
  }
  else
  {
    colour = 0;
    do
    {
      red   = gammatable[usegamma][*palette++];
      green = gammatable[usegamma][*palette++];
      blue  = gammatable[usegamma][*palette++];

#if 1
      red = ((red << 16) | (red << 8) | (red)) & X_visualinfo.red_mask;
      green = ((green << 16) | (green << 8) | (green)) & X_visualinfo.green_mask;
      blue = ((blue << 16) | (blue << 8) | (blue)) & X_visualinfo.blue_mask;
#else
      red <<= 16;
      green <<= 8;
#endif

      c = red | green | blue;

#ifdef __BIG_ENDIAN__
      if (image->byte_order != MSBFirst)
	c = SwapLONG (c);	// ((c & 0x00FF00) | (c >> 16) | (c << 16)) << 8;
#else
      if (image->byte_order != LSBFirst)
	c = SwapLONG (c);	// ((c & 0x00FF00) | (c >> 16) | (c << 16)) << 8;
#endif
      palette_table [colour] = c;
    } while (++colour < 256);
  }
}

/***************************************************************************/
//
// This function is probably redundant,
//  if XShmDetach works properly.
// ddt never detached the XShm memory,
//  thus there might have been stale
//  handles accumulating.
//
void grabsharedmemory(int size)
{
  int			key = ('d'<<24) | ('o'<<16) | ('o'<<8) | 'm';
  struct shmid_ds	shminfo;
  int			minsize = 320*200;
  int			id;
  int			rc;
  // UNUSED int done=0;
  int			pollution=5;

  // try to use what was here before
  do
  {
    id = shmget((key_t) key, minsize, 0777); // just get the id
    if (id != -1)
    {
      rc=shmctl(id, IPC_STAT, &shminfo); // get stats on it
      if (!rc)
      {
	if (shminfo.shm_nattch)
	{
	  fprintf(stderr, "User %d appears to be running "
		  "DOOM.  Is that wise?\n", shminfo.shm_cpid);
	  key++;
	}
	else
	{
	  if (getuid() == shminfo.shm_perm.cuid)
	  {
	    rc = shmctl(id, IPC_RMID, 0);
	    if (!rc)
	      fprintf(stderr,
		      "Was able to kill my old shared memory\n");
	    else
	      I_Error("Was NOT able to kill my old shared memory");

	    id = shmget((key_t)key, size, IPC_CREAT|0777);
	    if (id==-1)
	      I_Error("Could not get shared memory");

	    rc=shmctl(id, IPC_STAT, &shminfo);

	    break;

	  }
	  if (size >= shminfo.shm_segsz)
	  {
	    fprintf(stderr,
		    "will use %d's stale shared memory\n",
		    shminfo.shm_cpid);
	    break;
	  }
	  else
	  {
	    fprintf(stderr,
		    "warning: can't use stale "
		    "shared memory belonging to id %d, "
		    "key=0x%x\n",
		    shminfo.shm_cpid, key);
	    key++;
	  }
	}
      }
      else
      {
	I_Error("could not get stats on key=%d", key);
      }
    }
    else
    {
      id = shmget((key_t)key, size, IPC_CREAT|0777);
      if (id==-1)
      {
	//extern int errno;
	fprintf(stderr, "errno=%d\n", errno);
	I_Error("Could not get any shared memory");
      }
      break;
    }
  } while (--pollution);

  if (!pollution)
  {
    I_Error("Sorry, system too polluted with stale "
	    "shared memory segments.");
  }

  X_shminfo.shmid = id;

  // attach to the shared memory segment
  image->data = X_shminfo.shmaddr = shmat(id, 0, 0);

  fprintf (stderr, "shared memory id=%d, addr=0x%lx\n", id,
		(pint) (image->data));
}

/***************************************************************************/

static void I_XSetWindowName (Display * display, Window window, char * windowname, char * iconname)
{
   XTextProperty windowName, iconName;

   if (XStringListToTextProperty(&windowname, 1, &windowName) == 0)
   {
      I_Error ("structure allocation for windowName failed.");
   }
   if (XStringListToTextProperty(&iconname, 1, &iconName) == 0)
   {
      I_Error ("structure allocation for iconName failed.");
   }

   XSetWMName(display, window, &windowName);
   XSetWMIconName(display, window, &iconName);
   XFree(windowName.value);
   XFree(iconName.value);
}

/* -------------------------------------------------------------------------- */

void I_SetWindowName (const char * title)
{
  int pos;
  char window_title [50];

  if (netgame)
  {
    pos = sprintf (window_title, "Doom - Player %u", consoleplayer+1);
  }
  else
  {
    pos = sprintf (window_title, "Doom");
  }

  if ((title) && (title[0]))
  {
    sprintf (window_title+pos, " - %s", title);
  }

  I_XSetWindowName (X_display, X_mainWindow, window_title, "Doom");
}

/***************************************************************************/

/* Returns TRUE if this I am displaying on a local machine */
/* Used by the Shared memory stuff to determine whether it can be used */

static unsigned int I_is_display_local (char * display)
{
  int p;
  int q;
  char buffer [30];
  char hostname[30];

  if (display == 0)
  {
    display = (char *) getenv ("DISPLAY");
  }

  if ((display == 0)
   || (display [0] == 0)
   || (display [0] == ':'))
  {
    return (1);
  }

  /* Copy just the hostname part of the display name (host:0.0) to */
  /* a local buffer */
  p = 0;
  do
  {
    q = display [p];
    if ((q == ':')
     || (p >= (sizeof (buffer) - 2)))
    {
      q = 0;
    }
    buffer [p++] = q;
  } while (q);

  /* Now look for some obvious local names */

  if ((strcasecmp (buffer, "unix") == 0)
   || (strcasecmp (buffer, "localhost") == 0))
  {
    return (1);
  }

  // get local address
  p = gethostname (hostname, sizeof(hostname));
  if (p == -1)
  {
    return (1);
  }

  if (strcasecmp (buffer, hostname) == 0)
  {
    return (1);
  }

  return (0);
}

/***************************************************************************/

void I_SetScreenSize (void)
{
  SCREENWIDTH = 320;    /* Assume old style */
  SCREENHEIGHT = 240;

  if (M_CheckParm("-2"))
    multiply = 2;

  if (M_CheckParm("-3"))
    multiply = 3;

  if (M_CheckParm("-4"))
    multiply = 4;

  if (M_CheckParm("+2"))
  {
    SCREENWIDTH = MAXSCREENWIDTH;
    SCREENHEIGHT = MAXSCREENHEIGHT;
  }

  if (M_CheckParm("+4"))
  {
    multiply = 2;
    SCREENWIDTH = MAXSCREENWIDTH;
    SCREENHEIGHT = MAXSCREENHEIGHT;
  }
}

/***************************************************************************/

static void I_FindVisual (void)
{
  unsigned int p;
  unsigned int vis;

  if (M_CheckParm ("-Visual24"))
  {
    if (XMatchVisualInfo(X_display, X_screen, 24, DirectColor, &X_visualinfo))
      return;
    I_Error ("24 bit DirectColor mode is not available");
  }

  if (M_CheckParm ("-Visual8"))
  {
    if (XMatchVisualInfo(X_display, X_screen, 8, PseudoColor, &X_visualinfo))
      return;
    I_Error ("8 bit PseudoColor mode is not available");
  }

//if (XMatchVisualInfo(X_display, X_screen, 8, DirectColor, &X_visualinfo))
//  return;

  if (XMatchVisualInfo(X_display, X_screen, 24, TrueColor, &X_visualinfo))
    return;

  if (XMatchVisualInfo(X_display, X_screen, 24, DirectColor, &X_visualinfo))
    return;

  if (XMatchVisualInfo(X_display, X_screen, 8, PseudoColor, &X_visualinfo))
    return;

  I_Error("xdoom currently only supports 256-color PseudoColor\n"
	  "screens or 24 bit True/DirectColor screens.");
}

/***************************************************************************/

static void I_ShowVisual ()
{
  char * class;

  switch (X_visualinfo.class)
  {
    case StaticGray:	class = "StaticGray";	break;
    case GrayScale:	class = "GrayScale";	break;
    case StaticColor:	class = "StaticColor";	break;
    case PseudoColor:	class = "PseudoColor";	break;
    case TrueColor:	class = "TrueColor";	break;
    case DirectColor:	class = "DirectColor";	break;
    default:		class = "color";
  }

  printf ("Using %u bit %s\n", X_visualinfo.depth, class);
#if 0
  printf ("Red   mask = %08X\n", X_visualinfo.red_mask);
  printf ("Green mask = %08X\n", X_visualinfo.green_mask);
  printf ("Blue  mask = %08X\n", X_visualinfo.blue_mask);
#endif
}

/***************************************************************************/

void I_InitGraphics (void)
{
  char*		displayname;
  int		n;
  int		pnum;
  int		x=0;
  int		y=0;

  // warning: char format, different type arg
  char		xsign=' ';
  char		ysign=' ';

  int		oktodraw;
  unsigned long	attribmask;
  XSetWindowAttributes attribs;
  XGCValues	xgcvalues;
  int		valuemask;
  char *	mem;

  signal(SIGINT, (void (*)(int)) I_Quit);

  X_width = SCREENWIDTH * multiply;
  X_height = SCREENHEIGHT * multiply;

  // check for command-line display name
  pnum = M_CheckParm ("-disp");
  if (pnum == 0)
    pnum = M_CheckParm ("-display");

  if (pnum)
    displayname = myargv[pnum+1];
  else
    displayname = 0;

  // check if the user wants to grab the mouse (quite unnice)
  grabMouse = !!M_CheckParm("-grabmouse");

  // check for command-line geometry
  pnum = M_CheckParm ("-geom");
  if (pnum)
  {
    // warning: char format, different type arg 3,5
    n = sscanf(myargv[pnum+1], "%c%d%c%d", &xsign, &x, &ysign, &y);

    // fprintf (stderr, "sccanf returned %d\n", n);
    switch (n)
    {
      case 2:
	x = y = 0;
	break;

      case 4:
      case 6:
	if (xsign == '-')
	    x = -x;
	if (ysign == '-')
	    y = -y;
	break;

      default:
	I_Error("bad -geom parameter");
    }
  }

  // open the display
  X_display = XOpenDisplay(displayname);
  if (!X_display)
  {
    if (displayname)
      I_Error("Could not open display [%s]", displayname);
    else
      I_Error("Could not open display (DISPLAY=[%s])", getenv("DISPLAY"));
  }

  // use the default visual
  X_screen = DefaultScreen(X_display);
  I_FindVisual ();
  I_ShowVisual ();
  X_visual = X_visualinfo.visual;

  // check for the MITSHM extension
  doShm = XShmQueryExtension(X_display);

  // even if it's available, make sure it's a local connection
  if (doShm)
  {
    if (I_is_display_local (displayname) != 0)
    {
      fprintf (stderr, "Using MITSHM extension\n");
    }
    else
    {
      fprintf (stderr, "Not using MITSHM extension because display is remote\n");
      doShm = false;
    }
  }
  else
  {
    fprintf (stderr, "MITSHM extension is not available\n");
  }


  // create the colormap
  switch (X_visualinfo.depth)
  {
    case 8:
      X_cmap = XCreateColormap(X_display, RootWindow(X_display,
					X_screen), X_visual, AllocAll);
      break;

    case 24:
      X_cmap = XCreateColormap(X_display, RootWindow(X_display,
					X_screen), X_visual, AllocNone);
      break;
  }

  // setup attributes for main window
  attribmask = CWEventMask | CWColormap | CWBorderPixel;
  attribs.event_mask =
      KeyPressMask
      | KeyReleaseMask
      // | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
      | ExposureMask;

  attribs.colormap = X_cmap;
  attribs.border_pixel = 0;

  // create the main window
  X_mainWindow = XCreateWindow(	X_display,
				RootWindow(X_display, X_screen),
				x, y,
				X_width, X_height,
				0, // borderwidth
				X_visualinfo.depth, // depth
				InputOutput,
				X_visual,
				attribmask,
				&attribs );

  XDefineCursor(X_display, X_mainWindow,
		createnullcursor( X_display, X_mainWindow ) );

 // create the GC
  valuemask = GCGraphicsExposures;
  xgcvalues.graphics_exposures = False;
  X_gc = XCreateGC(	X_display,
  			X_mainWindow,
  			valuemask,
  			&xgcvalues );

  I_SetWindowName (NULL);

  // map the window
  XMapWindow(X_display, X_mainWindow);

  // wait until it is OK to draw
  oktodraw = 0;
  while (!oktodraw)
  {
    XNextEvent(X_display, &X_event);
    if (X_event.type == Expose
     && !X_event.xexpose.count)
    {
      oktodraw = 1;
    }
  }

  // grabs the pointer so it is restricted to this window
  if (grabMouse)
      XGrabPointer(X_display, X_mainWindow, True,
		   ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
		   GrabModeAsync, GrabModeAsync,
		   X_mainWindow, None, CurrentTime);

  if (doShm)
  {

      X_shmeventtype = XShmGetEventBase(X_display) + ShmCompletion;

      // create the image
      image = XShmCreateImage(	X_display,
				X_visual,
				X_visualinfo.depth,
				ZPixmap,
				0,
				&X_shminfo,
				X_width,
				X_height );

      grabsharedmemory(image->bytes_per_line * image->height);


      // UNUSED
      // create the shared memory segment
      // X_shminfo.shmid = shmget (IPC_PRIVATE,
      // image->bytes_per_line * image->height, IPC_CREAT | 0777);
      // if (X_shminfo.shmid < 0)
      // {
      // perror("");
      // I_Error("shmget() failed in InitGraphics()");
      // }
      // fprintf(stderr, "shared memory id=%d\n", X_shminfo.shmid);
      // attach to the shared memory segment
      // image->data = X_shminfo.shmaddr = shmat(X_shminfo.shmid, 0, 0);


      if (!image->data)
      {
	perror("");
	I_Error("shmat() failed in InitGraphics()");
      }

      // get the X server to attach to it
      if (!XShmAttach(X_display, &X_shminfo))
	I_Error("XShmAttach() failed in InitGraphics()");

  }
  else
  {
    switch (X_visualinfo.depth)
    {
      case 8:
	mem = malloc (X_width * X_height);
	if (mem == NULL)
	  I_Error ("Failed to claim memory");
	image = XCreateImage(	X_display,
				X_visual,
				X_visualinfo.depth,
				ZPixmap,
				0,
				mem,
				X_width, X_height,
				X_visualinfo.depth,
				X_width);
      	break;

      case 24:
	mem = malloc(X_width * X_height * 4);
	if (mem == NULL)
	  I_Error ("Failed to claim memory");
	image = XCreateImage(	X_display,
				X_visual,
				X_visualinfo.depth,
				ZPixmap,
				0,
				mem,
				X_width, X_height,
				32,//X_visualinfo.depth,
				X_width * 4);
    }
  }

  if ((multiply == 1) && (X_visualinfo.depth == 8))
    screens[0] = (unsigned char *) (image->data);
//else
//  screens[0] = (unsigned char *) malloc (SCREENWIDTH * SCREENHEIGHT);
  Init_Colour_Map ();

  /* Clear the display */
  memset (screens[0], 0, sizeof (screens [0]));
  I_FinishUpdate ();
}

/***************************************************************************/
