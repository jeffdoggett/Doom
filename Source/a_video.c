/* Acorn Screen drivers
   Written by J.A.Doggett.

   Based on i_video.c so some of the comments herein are ID's
*/

#include "includes.h"
#include <kernel.h>

extern boolean	menuactive;
extern int 	novert;

#define OS_Word			 0x07
#define OS_Mouse		 0x1C
#define OS_SpriteOp		 0x2E
#define OS_ReadVduVariables	 0x31
#define OS_SWINumberFromString	 0x39
#define OS_ScreenMode		 0x65
#define ColourTrans_WritePalette 0x4075D

#define MOUSE_SELECT 1
#define MOUSE_MENU   3
#define MOUSE_ADJUST 2

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int      screen_mode = 1;
static unsigned int * screen_addr;
static unsigned int   palette_table   [256];
static unsigned char  key_down_table  [128];
static unsigned char  num_keys_down [10];
static unsigned char  key_to_scan;

static unsigned int   lastmousex;
static unsigned int   lastmousey;
static unsigned int   lastmousebut;

static unsigned int   rpc_palette;  /* = True when using 8 bit screen mode and */
				    /* RPC palette is available (256 entries) */
static unsigned char menu_in_use = 0;

static unsigned char devparm_black;
static unsigned char devparm_white;

typedef struct
{
  unsigned int area_size;
  unsigned int qty_sprites;
  unsigned int first_sprite;
  unsigned int next_free;
  unsigned int sprite_size;
  char sprite_name [12];
  unsigned int width;
  unsigned int height;
  unsigned int first_bit;
  unsigned int last_bit;
  unsigned int offset;
  unsigned int offset2;
} sprite_area_t;

static sprite_area_t * sprite_mem = 0;

// screen_mode
#define MODE_USER_DEF		0

#define MODE_320_x_200_8bpp	1
#define MODE_320_x_240_8bpp	2
#define MODE_320_x_256_8bpp	3
#define MODE_320_x_400_8bpp	4
#define MODE_320_x_480_8bpp	5
#define MODE_640_x_400_8bpp	6
#define MODE_640_x_480_8bpp	7

#define MODE_320_x_200_16bpp	8
#define MODE_320_x_240_16bpp	9
#define MODE_320_x_256_16bpp	10
#define MODE_320_x_400_16bpp	11
#define MODE_320_x_480_16bpp	12
#define MODE_640_x_400_16bpp	13
#define MODE_640_x_480_16bpp	14

#define MODE_320_x_200_24bpp	15
#define MODE_320_x_240_24bpp	16
#define MODE_320_x_256_24bpp	17
#define MODE_320_x_400_24bpp	18
#define MODE_320_x_480_24bpp	19
#define MODE_640_x_400_24bpp	20
#define MODE_640_x_480_24bpp	21



static const unsigned int screen_320x200x8 [] =
{
  1,
  320, 200,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

static const unsigned int screen_320x240x8 [] =
{
  1,
  320, 240,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

static const unsigned int screen_320x256x8 [] =
{
  1,
  320, 256,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

static const unsigned int screen_320x480x8 [] =
{
  1,
  320, 480,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

static const unsigned int screen_320x400x8 [] =
{
  1,
  320, 400,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

static const unsigned int screen_320x200x16 [] =
{
  1,
  320, 200,
  4,		/* 16bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x240x16 [] =
{
  1,
  320, 240,
  4,		/* 16bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x256x16 [] =
{
  1,
  320, 256,
  4,		/* 16bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x400x16 [] =
{
  1,
  320, 400,
  4,		/* 16bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x480x16 [] =
{
  1,
  320, 480,
  4,		/* 16bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x200x24 [] =
{
  1,
  320, 200,
  5,		/* 24bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x240x24 [] =
{
  1,
  320, 240,
  5,		/* 24bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x256x24 [] =
{
  1,
  320, 256,
  5,		/* 24bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x400x24 [] =
{
  1,
  320, 400,
  5,		/* 24bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_320x480x24 [] =
{
  1,
  320, 480,
  5,		/* 24bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_640x400x8 [] =
{
  1,
  640, 400,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

static const unsigned int screen_640x400x16 [] =
{
  1,
  640, 400,
  4,		/* 16bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_640x400x24 [] =
{
  1,
  640, 400,
  5,		/* 24bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_640x480x8 [] =
{
  1,
  640, 480,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

static const unsigned int screen_640x480x16 [] =
{
  1,
  640, 480,
  4,		/* 16bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

static const unsigned int screen_640x480x24 [] =
{
  1,
  640, 480,
  5,		/* 24bpp */
  -1,		/* framerate - use first found */
  -1		/* End of table */
};

unsigned int screen_user_def [] =
{
  1,
  640, 480,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

/* Table to convert Acorn inkey() values to Doom key strokes */
/* These tables are overwritten by the contents of file "Inkeys" if it exists */

static unsigned char key_xlate_table [] =
{
  0, 0, 0,
  KEY_LSHIFT,
  KEY_LCTRL,
  KEY_LALT,
  KEY_RSHIFT,
  KEY_RCTRL,
  KEY_RALT,
  MOUSE_SELECT,	/* Left mouse button */
  MOUSE_MENU,	/* Centre mouse button */
  MOUSE_ADJUST,	/* Right mouse button */
  0,0,0,0,
  'q',
  '3',
  '4',
  '5',
  KEY_F4,
  '8',
  KEY_F7,
  '-',
  0,		/* ^ */
  KEY_LEFTARROW,
  '6',		/* Keypad */
  '7',		/* Keypad */
  KEY_F11,
  KEY_F12,
  KEY_F10,
  0,		/* Scroll lock */
  KEY_PRINTSCRN,/* Print (F0) */
  'w',
  'e',
  't',
  '7',
  'i',
  '9',
  '0',
  0,		/* _ */
  KEY_DOWNARROW,
  '8',		/* Keypad */
  '9',		/* Keypad */
  KEY_PAUSE,	/* Break */
  '`',
  '#',
  KEY_BACKSPACE,
  '1',
  '2',
  'd',
  'r',
  '6',
  'u',
  'o',
  'p',
  '[',
  KEY_UPARROW,
  '+',		/* Keypad */
  '-',		/* Keypad */
  KEY_ENTER,	/* Keypad */
  0,		/* Insert */
  0,		/* Home */
  0,		/* Page up */
  0,		/* Caps lock */
  'a',
  'x',
  'f',
  'y',
  'j',
  'k',
  0,		/* @ */
  0,		/* : */
  KEY_ENTER,
  '/',		/* Keypad */
  0,
  '.',		/* Keypad */
  0,		/* Num lock */
  0,		/* Page down */
  '\'',		/* Single quote */
  0,
  's',
  'c',
  'g',
  'h',
  'n',
  'l',
  ';',
  ']',
  KEY_BACKSPACE,	/* Delete */
  '#',		/* Keypad */
  '*',		/* Keypad */
  0,
  '=',
  '\\',
  0,
  KEY_TAB,
  'z',
  ' ',
  'v',
  'b',
  'm',
  ',',
  '.',
  '/',
  0,		/* Copy / End */
  '0',		/* Keypad */
  '1',		/* Keypad */
  '3',		/* Keypad */
  0,0,0,
  KEY_ESCAPE,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F5,
  KEY_F6,
  KEY_F8,
  KEY_F9,
  '\\',
  KEY_RIGHTARROW,
  '4',		/* Keypad */
  '5',		/* Keypad */
  '2',		/* Keypad */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0
};

/* Table to swap mouse buttons round */
static int mouse_button_conv[] = { 0, 4, 2, 6, 1, 5, 3, 7 };


unsigned int SCREENWIDTH = MAXSCREENWIDTH;
unsigned int SCREENHEIGHT = MAXSCREENHEIGHT;

/* -------------------------------------------------------------------------- */

void I_ShutdownGraphics(void)
{
  /* Flush keyboard buffer */
  _kernel_osbyte (21, 0, 0);

  /* Re-enable the Escape key */
  _kernel_osbyte (229, 0, 0);

  /* Restore cursor key editing */
  _kernel_osbyte (4, 0, 0);

  /* Restore clip rectangles to full screen */
  _kernel_oswrch (26);
}

/* -------------------------------------------------------------------------- */

//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

/* -------------------------------------------------------------------------- */

/* The mouse has a 16 bit field of motion, so need to allow for overflow */

static int determine_amount_mouse_has_moved (unsigned int new_pos, unsigned int old_pos)
{
  unsigned int a;
  a = new_pos - old_pos;
  if (a & 0x8000)
    a |= ~0xFFFF;
  else
    a &= 0xFFFF;
  return (a << 2);
}

/* -------------------------------------------------------------------------- */

/* When the menu is in use, we use inkey(0) to read the keyboard */

static void poll_kbd_menu_mode (void)
{
  event_t event;
  unsigned int key;
  _kernel_swi_regs regs;

  if (menu_in_use == 0)	     /* Menu just entered? */
  {
    menu_in_use = 1;
    _kernel_osbyte (21, 0, 0);     /* Flush keyboard buffer */
    do
    {
      key = _kernel_osbyte (129, 10, 0); /* Doesn't always seem to */
    }
    while ((key & 0xFF00) == 0);
    return;			/* Do no more this time */
  }

  key = _kernel_osbyte (129, 0, 0);
  if ((key & 0xFF00) == 0)
  {
    key &= 0xFF;
    switch (key)
    {
      case 0x7F:
      case 8:
	key = KEY_BACKSPACE;
	break;

      case 136:
	key = KEY_LEFTARROW;
	break;

      case 137:
	key = KEY_RIGHTARROW;
	break;

      case 138:
	key = KEY_DOWNARROW;
	break;

      case 139:
	key = KEY_UPARROW;
	break;
    }
    event.data1 = key;
    event.type = ev_keydown;
    D_PostEvent (&event);
    event.type = ev_keyup;
    D_PostEvent (&event);
  }


  /* See whether the mouse has moved */

  _kernel_swi (OS_Mouse, &regs, &regs);

  if (novert)
    regs.r[1] = lastmousey;

  if ((regs.r[0] != lastmousex)
   || (regs.r[1] != lastmousey)
   || (regs.r[2] != lastmousebut))
  {
    /* printf ("%d,%d  %d,%d\n", regs.r[0], regs.r[1], lastmousex, lastmousey); */
    event.type = ev_mouse;
    event.data1 = 0;
    lastmousebut = regs.r[2];
    event.data1 = mouse_button_conv [lastmousebut & 7];
    event.data2 = determine_amount_mouse_has_moved (regs.r[0], lastmousex);
    event.data3 = determine_amount_mouse_has_moved (regs.r[1], lastmousey);
    D_PostEvent (&event);
    lastmousex = regs.r[0];
    lastmousey = regs.r[1];
  }
}

/* -------------------------------------------------------------------------- */

static unsigned int allow_key (unsigned int key)
{
  if ((key >= '0') && (key <= '9'))
    return (key);
  else if ((key >= 'A') && (key <= 'Z'))
    return (key + 0x20);
  else if ((key >= 'a') && (key <= 'z'))
    return (key);

  return (0);
}

/* -------------------------------------------------------------------------- */

/* During game play we use osbyte 121 to poll the keyboard */

static void poll_kbd (void)
{
  event_t event;
  unsigned int key;
  unsigned int rkey;
  unsigned int dokey;
  unsigned int qtydown;
  _kernel_swi_regs regs;

  menu_in_use = 0;

  key = _kernel_osbyte (129, 0, 0);
  if ((key & 0xFF00) == 0)		/* Key waiting in the buffer? */
  {
    key = allow_key (key & 0xFF);
    if (key)
    {
      dokey = 1;
      rkey = 0;				/* The number keys appear twice in */
      do				/* this table just to cause problems. */
      {
	if (key_xlate_table [rkey] == key)
	{
	  if (key_down_table [rkey])
	    dokey = 0;
	  else
	    key_down_table [rkey] = 1;
	}
      } while (++rkey < sizeof (key_xlate_table));

      if (dokey)
      {
	if ((key >= '0') && (key <= '9'))
	  num_keys_down [key-'0'] = 2;
	event.data1 = key;
	event.type = ev_keydown;
	D_PostEvent (&event);
      }
    }
  }

  key = (_kernel_osbyte (121, key_to_scan, 0)) & 0xFF;
  if (key != 255)
  {
    if (key_down_table [key] == 0)	/* Yes. Newly down? */
    {
      key_down_table [key] = 1;
      event.data1 = key_xlate_table [key];
      switch (event.data1)
      {
	case 0:
	case MOUSE_SELECT:		/* Mouse buttons */
	case MOUSE_MENU:
	case MOUSE_ADJUST:
	  break;

	default:
	  event.type = ev_keydown;
	  D_PostEvent (&event);
      }
    }
    key_to_scan = key + 1;
    if (key_to_scan > 127) key_to_scan = 0;
  }
  else
  {
    key_to_scan = 0;
  }


  /* Run through the key down table and see if they're still down */
  key = 0;
  qtydown = 0;
  do
  {
    if (key_down_table [key])
    {
      qtydown++;
      if (_kernel_osbyte (129, 255 - key, 0xFF) == 0)	/* This key now up? */
      {
 	key_down_table [key] = 0;
	rkey = key_xlate_table [key];
	switch (rkey)
	{
	  case 0:
	  case MOUSE_SELECT:				/* Mouse buttons */
	  case MOUSE_MENU:
	  case MOUSE_ADJUST:
	    break;

	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	    if ((num_keys_down [rkey-'0'])
	     && (--num_keys_down [rkey-'0']))
	      break;

	  default:
	    event.data1 = rkey;
	    event.type = ev_keyup;
	    D_PostEvent (&event);
	}
      }
    }
    key++;
  } while (key< 128);

  if (qtydown == 0)					// Try to keep in step!
  {
    key = 0;
    do
    {
      if (num_keys_down [key])
      {
        num_keys_down [key] = 0;
	event.data1 = key + '0';
	event.type = ev_keyup;
	D_PostEvent (&event);
      }
    } while (++key < 10);
  }

  /* See whether the mouse has moved */

  _kernel_swi (OS_Mouse, &regs, &regs);

  if (novert)
    regs.r[1] = lastmousey;

  if ((regs.r[0] != lastmousex)
   || (regs.r[1] != lastmousey)
   || (regs.r[2] != lastmousebut))
  {
    /* printf ("%d,%d  %d,%d\n", regs.r[0], regs.r[1], lastmousex, lastmousey); */
    event.type = ev_mouse;
    event.data1 = 0;
    lastmousebut = regs.r[2];
    event.data1 = mouse_button_conv [lastmousebut & 7];
    event.data2 = determine_amount_mouse_has_moved (regs.r[0], lastmousex);
    event.data3 = determine_amount_mouse_has_moved (regs.r[1], lastmousey);
    D_PostEvent (&event);
    lastmousex = regs.r[0];
    lastmousey = regs.r[1];
  }
}

/* -------------------------------------------------------------------------- */
//
// I_StartTic
//
void I_StartTic (void)
{
  //I_GetEvent ();

  if (menuactive)
  {
    poll_kbd_menu_mode ();
  }
  else
  {
    poll_kbd ();
  }
}

/* -------------------------------------------------------------------------- */

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

/* -------------------------------------------------------------------------- */

static void init_sprite_area (void)
{
  unsigned int size;
  _kernel_oserror * rc;
  _kernel_swi_regs regs;

  size = (SCREENWIDTH * SCREENHEIGHT);
  switch (screen_user_def [3])
  {
    case 5:
      size <<= 1;
    case 4:
      size <<= 1;
  }

  size += 64;

  sprite_mem = malloc (size);
  if (sprite_mem == 0)
    return;

  sprite_mem -> area_size = size;
  sprite_mem -> qty_sprites = 0;
  sprite_mem -> first_sprite = 16;

  /* initialise the sprite area */
  regs.r[0] = 0x109;
  regs.r[1] = (int) sprite_mem;
  rc = _kernel_swi (OS_SpriteOp, &regs, &regs);
  if (rc)
    I_Error ("Sprite 109 error %s\n", rc -> errmess);

  /* Create a sprite */
  regs.r[0] = 0x10F;
  regs.r[1] = (int) sprite_mem;
  regs.r[2] = (int) "DoomScreen";
  regs.r[3] = 0;			// No Palette required
  regs.r[4] = SCREENWIDTH;
  regs.r[5] = SCREENHEIGHT;
  //regs.r[6] = 28;			// Mode 28 is 640 x 480 256 colours
  regs.r[6] = ((screen_user_def [3]+1)<<27)+(90<<14)+(90<<1)+1;
  rc = _kernel_swi (OS_SpriteOp, &regs, &regs);
  if (rc)
    I_Error ("Sprite 10F error %s\n", rc -> errmess);
}

/* -------------------------------------------------------------------------- */

static void put_sprite (void)
{
  _kernel_oserror * rc;
  _kernel_swi_regs regs;

  regs.r[0] = 0x200+34;			// Put sprite at user coords
  regs.r[1] = (int) sprite_mem;
  regs.r[2] = (int) &sprite_mem->sprite_size;
  regs.r[3] = 0;
  regs.r[4] = 0;
  regs.r[5] = 0;
  //regs.r[6] = 0;
  //regs.r[7] = -1;
  rc = _kernel_swi (OS_SpriteOp, &regs, &regs);
  if (rc)
    I_Error ("Sprite 122 error %s\n", rc -> errmess);
}

/* -------------------------------------------------------------------------- */

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{

    static int  lasttic;
    int		tics;
    int		i,j,p;
    unsigned char  * screen_copy;
    unsigned char  * screen_8;
    unsigned short * screen_16;
    unsigned int   * screen_24;
    unsigned int   * screen_24s;
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


	// draw the image
    screen_copy = screens[0];

    switch (screen_mode)
    {
      case MODE_USER_DEF:			// user defined
	switch (screen_user_def [3])
	{
#if 0
	  /* No need to do this copy as screens[0] has been set */
	  /* to point to the sprite area in I_InitGraphics */
	  case 3:
	    screen_8 = ((unsigned char*) &sprite_mem->sprite_size) + (sprite_mem -> offset);
	    memcpy (screen_8, screen_copy, SCREENHEIGHT * SCREENWIDTH);
	    break;
#endif
	  case 4:
	    screen_16 = (unsigned short *)(((unsigned char*) &sprite_mem->sprite_size) + (sprite_mem -> offset));
	    i = SCREENWIDTH * SCREENHEIGHT;
	    do
	    {
	      *screen_16++ = palette_table [*screen_copy++];
	    } while (--i);
	    break;

	  case 5:
	    screen_24 = (unsigned int *)(((unsigned char*) &sprite_mem->sprite_size) + (sprite_mem -> offset));
	    i = SCREENWIDTH * SCREENHEIGHT;
	    do
	    {
	      *screen_24++ = palette_table [*screen_copy++];
	    } while (--i);
	    break;
	}

	put_sprite ();
	break;

      case MODE_320_x_200_8bpp:
      case MODE_320_x_240_8bpp:
      case MODE_320_x_256_8bpp:
	if (rpc_palette)
	{
	  memcpy (screen_addr, screen_copy, SCREENHEIGHT * 320);
	}
	else
	{
	  screen_8 = (unsigned char *) screen_addr;
	  i = 320 * SCREENHEIGHT;
	  do
	  {
	    *screen_8++ = palette_table [*screen_copy++];
	  } while (--i);
	}
	break;

      case MODE_320_x_400_8bpp:
      case MODE_320_x_480_8bpp:
	i = SCREENHEIGHT;
	screen_8 = (unsigned char *) screen_addr;
	if (rpc_palette)
	{
	  screen_24 = (unsigned int *) screen_8;
	  screen_24s = (unsigned int *) screen_copy;
	  do
	  {
	    j = 320 / 4;
	    do
	    {
	      p = *screen_24s++;
	      screen_24 [320/4] = p;
	      *screen_24++ = p;
	    } while (--j);
	    screen_24 += (320/4);
	  } while (--i);
	}
	else
	{
	  do
	  {
	    j = 320;
	    do
	    {
	      p = palette_table [*screen_copy++];
	      screen_8 [320] = p;
	      *screen_8++ = p;
	    } while (--j);
	    screen_8 += 320;
	  } while (--i);
	}
	break;

      case MODE_320_x_200_16bpp:
      case MODE_320_x_240_16bpp:
      case MODE_320_x_256_16bpp:
	screen_16 = (unsigned short *) screen_addr;
	i = 320 * SCREENHEIGHT;
	do
	{
	  *screen_16++ = palette_table [*screen_copy++];
	} while (--i);
	break;

      case MODE_320_x_400_16bpp:
      case MODE_320_x_480_16bpp:
	i = SCREENHEIGHT;
	screen_16 = (unsigned short *) screen_addr;
	do
	{
	  j = 320;
	  do
	  {
	    p = palette_table [*screen_copy++];
	    screen_16 [320] = p;
	    *screen_16++ = p;
	  } while (--j);
	  screen_16 += 320;
	} while (--i);
	break;

      case MODE_320_x_200_24bpp:
      case MODE_320_x_240_24bpp:
      case MODE_320_x_256_24bpp:
	i = 320 * SCREENHEIGHT;
	screen_24 = screen_addr;
	do
	{
	  *screen_24++ = palette_table [*screen_copy++];
	} while (--i);
	break;

      case MODE_320_x_400_24bpp:
      case MODE_320_x_480_24bpp:
	i = SCREENHEIGHT;
	screen_24 = screen_addr;
	do
	{
	  j = 320;
	  do
	  {
	    p = palette_table [*screen_copy++];
	    screen_24 [320] = p;
	    *screen_24++ = p;
	  } while (--j);
	  screen_24 += 320;
	} while (--i);
	break;

      case MODE_640_x_400_8bpp:
      case MODE_640_x_480_8bpp:
	if (rpc_palette)
	{
	  memcpy (screen_addr, screen_copy, SCREENHEIGHT * 640);
	}
	else
	{
	  screen_8 = (unsigned char *) screen_addr;
	  i = 640 * SCREENHEIGHT;
	  do
	  {
	    *screen_8++ = palette_table [*screen_copy++];
	  } while (--i);
	}
	break;

      case MODE_640_x_400_16bpp:
      case MODE_640_x_480_16bpp:
	screen_16 = (unsigned short *) screen_addr;
	i = 640 * SCREENHEIGHT;
	do
	{
	  *screen_16++ = palette_table [*screen_copy++];
	} while (--i);
	break;

      case MODE_640_x_400_24bpp:
      case MODE_640_x_480_24bpp:
	i = 640 * SCREENHEIGHT;
	screen_24 = screen_addr;
	do
	{
	  *screen_24++ = palette_table [*screen_copy++];
	} while (--i);
	break;
    }
}

/* -------------------------------------------------------------------------- */

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
  memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

/* -------------------------------------------------------------------------- */

static void set_palette_24 (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];

    palette_table [colour] = (blue << 16) | (green << 8) | (red);
  } while (++colour < 256);
}

/* -------------------------------------------------------------------------- */

static void set_palette_16 (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];


    palette_table [colour] = ((blue  & 0xF8) << 7)
			   | ((green & 0xF8) << 2)
			   | ((red   & 0xF8) >> 3);
  } while (++colour < 256);
}

/* -------------------------------------------------------------------------- */

static void set_palette_8_non_rpc (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];


	// if (red & 0x0F)   red   |= 0x10;
	// if (blue & 0x0F)  blue  |= 0x10;
	// if (green & 0x0F) green |= 0x10;

    palette_table [colour] = (blue & 0x80)
	   		| ((green & 0xC0) >> 1)
			| ((red & 0x80) >> 3)
			| ((blue & 0x40) >> 3)
			| ((red & 0x40) >> 4)
			| (((blue | green | red) & 0x30) >> 4);

  } while (++colour < 256);
}

/* -------------------------------------------------------------------------- */

static void set_palette_8_rpc (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;
  _kernel_swi_regs regs;
  unsigned int palette_8_table [256];

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];

    palette_8_table [colour] = (blue << 24) | (green << 16) | (red << 8);
    //palette_table [colour] = colour;
  } while (++colour < 256);

  regs.r[0] = -1;
  regs.r[1] = -1;
  regs.r[2] = (int) palette_8_table;
  regs.r[3] = 0;
  regs.r[4] = 0;
  regs.r[5] = 0;
  regs.r[6] = 0;
  regs.r[7] = 0;
  _kernel_swi (ColourTrans_WritePalette, &regs, &regs);
}

/* -------------------------------------------------------------------------- */

static void set_palette_8 (byte * palette)
{
  if (rpc_palette)
  {
    set_palette_8_rpc (palette);
  }
  else
  {
    set_palette_8_non_rpc (palette);
  }
}

/* -------------------------------------------------------------------------- */

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
  if (devparm)
  {
    devparm_black = AM_load_colour (0, 0, 0, palette);
    devparm_white = AM_load_colour (255, 255, 255, palette);
  }

  switch (screen_mode)
  {
    case MODE_USER_DEF:
      switch (screen_user_def [3])
      {
	 case 3:
	  set_palette_8 (palette);
	  break;

	 case 4:
	  set_palette_16 (palette);
	  break;

	 default:
	  set_palette_24 (palette);
	  break;
      }
      break;

    case MODE_320_x_200_8bpp: /* 8bpp */
    case MODE_320_x_240_8bpp:
    case MODE_320_x_256_8bpp:
    case MODE_320_x_400_8bpp:
    case MODE_320_x_480_8bpp:
    case MODE_640_x_400_8bpp:
    case MODE_640_x_480_8bpp:
      set_palette_8 (palette);
      break;

    case MODE_320_x_200_16bpp: /* 16bpp */
    case MODE_320_x_240_16bpp:
    case MODE_320_x_256_16bpp:
    case MODE_320_x_400_16bpp:
    case MODE_320_x_480_16bpp:
    case MODE_640_x_400_16bpp:
    case MODE_640_x_480_16bpp:
      set_palette_16 (palette);
      break;

    default: /* 24bpp */
      set_palette_24 (palette);
  }
}

/* -------------------------------------------------------------------------- */

static int read_k_var (char * line, int * ptr)
{
  int n;
  char k;
  char *l;

  n = 0;
  l = &line[*ptr];
  switch (*l)
  {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
      n = atoi (l);
      break;

    case '\'':
      n = l[1];
      break;

    default:
      if      (strncasecmp (l,"KEY_RIGHTARROW", 14) == 0)   n = KEY_RIGHTARROW;
      else if (strncasecmp (l,"KEY_LEFTARROW",  13) == 0)   n = KEY_LEFTARROW;
      else if (strncasecmp (l,"KEY_UPARROW",    11) == 0)   n = KEY_UPARROW;
      else if (strncasecmp (l,"KEY_DOWNARROW",  13) == 0)   n = KEY_DOWNARROW;
      else if (strncasecmp (l,"KEY_ESCAPE",     10) == 0)   n = KEY_ESCAPE;
      else if (strncasecmp (l,"KEY_ENTER",	 9) == 0)   n = KEY_ENTER;
      else if (strncasecmp (l,"KEY_TAB",	 7) == 0)   n = KEY_TAB;
      else if (strncasecmp (l,"KEY_F1",		 6) == 0)   n = KEY_F1;
      else if (strncasecmp (l,"KEY_F2",		 6) == 0)   n = KEY_F2;
      else if (strncasecmp (l,"KEY_F3",		 6) == 0)   n = KEY_F3;
      else if (strncasecmp (l,"KEY_F4",		 6) == 0)   n = KEY_F4;
      else if (strncasecmp (l,"KEY_F5",		 6) == 0)   n = KEY_F5;
      else if (strncasecmp (l,"KEY_F6",		 6) == 0)   n = KEY_F6;
      else if (strncasecmp (l,"KEY_F7",	 	 6) == 0)   n = KEY_F7;
      else if (strncasecmp (l,"KEY_F8",		 6) == 0)   n = KEY_F8;
      else if (strncasecmp (l,"KEY_F9",		 6) == 0)   n = KEY_F9;
      else if (strncasecmp (l,"KEY_F10",	 7) == 0)   n = KEY_F10;
      else if (strncasecmp (l,"KEY_F11",	 7) == 0)   n = KEY_F11;
      else if (strncasecmp (l,"KEY_F12", 	 7) == 0)   n = KEY_F12;
      else if (strncasecmp (l,"KEY_PRINTSCRN",	13) == 0)   n = KEY_PRINTSCRN;
      else if (strncasecmp (l,"KEY_BACKSPACE",	13) == 0)   n = KEY_BACKSPACE;
      else if (strncasecmp (l,"KEY_PAUSE",	 9) == 0)   n = KEY_PAUSE;
      else if (strncasecmp (l,"KEY_EQUALS",	10) == 0)   n = KEY_EQUALS;
      else if (strncasecmp (l,"KEY_MINUS",	 9) == 0)   n = KEY_MINUS;
      else if (strncasecmp (l,"KEY_LSHIFT",	10) == 0)   n = KEY_LSHIFT;
      else if (strncasecmp (l,"KEY_LCTRL",	 9) == 0)   n = KEY_LCTRL;
      else if (strncasecmp (l,"KEY_LALT",	 8) == 0)   n = KEY_LALT;
      else if (strncasecmp (l,"KEY_RSHIFT",	10) == 0)   n = KEY_RSHIFT;
      else if (strncasecmp (l,"KEY_RCTRL",	 9) == 0)   n = KEY_RCTRL;
      else if (strncasecmp (l,"KEY_RALT",	 8) == 0)   n = KEY_RALT;
      else if (strncasecmp (l,"MOUSE_SELECT",	12) == 0)   n = MOUSE_SELECT;
      else if (strncasecmp (l,"MOUSE_MENU",	10) == 0)   n = MOUSE_MENU;
      else if (strncasecmp (l,"MOUSE_ADJUST",	12) == 0)   n = MOUSE_ADJUST;
  }

  do
  {
    k = line[*ptr];
    (*ptr)++;
  } while ((k) && (k != ','));

  return (n);
}

/* -------------------------------------------------------------------------- */

static void read_key_table (char * file_name)
{
  char inkey_line [256];
  int i,j;
  FILE * fin;

  fin = fopen (file_name, "r");
  if (fin)
  {
    do
    {
      if (fgets (inkey_line, 250, fin))
      {
	switch (inkey_line[0])
	{
	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	  case '-':
	    i = 0;
	    j = read_k_var (inkey_line, &i);
	    if (j < 0) j = j + 255;
	    if ((j >= 0) && (j < 128))
	    {
	      key_xlate_table [j] = read_k_var (inkey_line, &i);
	    }
	}
      }
    } while (!feof(fin));
    fclose (fin);
  }
}

/* -------------------------------------------------------------------------- */

static void init_key_tables (void)
{
  char inkey_file [256];
  int i,j,k;

  i = 0;
  j = 0;
  do
  {
    k = myargv[0][i];
    inkey_file[i] = k;
    i++;
    if (k == '.')  j = i;
  }
  while (k);

  /* Read the main copy first */
  strcpy (&inkey_file[j], "Inkeys");
  read_key_table (inkey_file);

  /* Then read any local overrides */
  read_key_table ("<DoomSavDir>.Inkeys");
}

/* -------------------------------------------------------------------------- */

static _kernel_oserror * select_screen_mode (int number)
{
  _kernel_oserror * rc;
  _kernel_swi_regs regs;

  rpc_palette = 0;

  switch (number)
  {
    case MODE_USER_DEF:
      regs.r[0] = 0;
      regs.r[1] = (int) screen_user_def;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      if (rc == 0)
	rpc_palette = 1;

      if (screen_user_def [1] >= 640)
	SCREENWIDTH = 640;
      else
	SCREENWIDTH = 320;

      if (screen_user_def [2] >= 480)
	SCREENHEIGHT = 480;
      else if (screen_user_def [2] >= 400)
	SCREENHEIGHT = 400;
      else if (screen_user_def [2] >= 256)
	SCREENHEIGHT = 256;
      else if (screen_user_def [2] >= 240)
	SCREENHEIGHT = 240;
      else
	SCREENHEIGHT = 200;

#if 0
      if (SCREENWIDTH > MAXSCREENWIDTH)
	SCREENWIDTH = MAXSCREENWIDTH;
      if (SCREENHEIGHT > MAXSCREENHEIGHT)
	SCREENHEIGHT = MAXSCREENHEIGHT;
#endif
	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((screen_user_def [2] / 8) - 1); /* Bottom Y */
      _kernel_oswrch ((screen_user_def [1] / 8) - 1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;



    case MODE_320_x_200_8bpp:
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x200x8;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      if (rc == 0)
	rpc_palette = 1;

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 200;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((200/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;


    case MODE_320_x_240_8bpp:
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x240x8;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      if (rc == 0)
	rpc_palette = 1;

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 240;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((240/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;


    case MODE_320_x_256_8bpp:	/* Select screen mode 13, 320x256 256 colours */
	    			/* We carefully avoid using Risc PC SWI here :) */

      /* Try RPC mode with programmable palette first */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x256x8;
      if (_kernel_swi (OS_ScreenMode, &regs, &regs) == 0)
      {
	rpc_palette = 1;
      }
      else
      {
	_kernel_oswrch (22);
	_kernel_oswrch (13);
	// rpc_palette = 0;
      }
      rc = 0;			/* This always succeeds! */

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 256;

	    /* Define text window */
	    /* This prevents the screen scrolling! */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((256/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_320_x_400_8bpp: /* Select screen mode 320x400 8 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x400x8;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      if (rc == 0)
	rpc_palette = 1;

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 200;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((400/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_320_x_480_8bpp: /* Select screen mode 320x480 8 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x480x8;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      if (rc == 0)
      {
	rpc_palette = 1;
      }
      else
      {
	_kernel_oswrch (22);
	_kernel_oswrch (49);
	if (((_kernel_osbyte (135, 0, 0) >> 8) & 0xFF) == 49)
	  rc = 0;
	// rpc_palette = 0;
      }

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 240;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((480/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;


    case MODE_320_x_200_16bpp:
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x200x16;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 200;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((200/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;


    case MODE_320_x_240_16bpp:
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x240x16;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 240;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((240/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;


    case MODE_320_x_256_16bpp: /* Select screen mode 320x256 16 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x256x16;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 256;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((256/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_320_x_400_16bpp: /* Select screen mode 320x400 16 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x400x16;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 200;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((400/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_320_x_480_16bpp: /* Select screen mode 320x480 16 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x480x16;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 240;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((480/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_320_x_200_24bpp:
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x200x24;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 200;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((200/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;


    case MODE_320_x_240_24bpp:
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x240x24;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 240;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((240/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_320_x_256_24bpp: /* Select screen mode 320x256 24 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x256x24;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 256;

	    /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((256/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_320_x_400_24bpp: /* Select screen mode 320x400 24 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x400x24;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 200;

	  /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((320/8)-1); /* Bottom Y */
      _kernel_oswrch ((400/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_320_x_480_24bpp: /* Select screen mode 320x480 24 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_320x480x24;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 320;
      SCREENHEIGHT = 240;

	  /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((480/8)-1); /* Bottom Y */
      _kernel_oswrch ((320/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_640_x_400_8bpp: /* Select screen mode 640x400 8 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_640x400x8;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      rpc_palette = 1;

      SCREENWIDTH  = 640;
      SCREENHEIGHT = 400;

	  /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((400/8)-1); /* Bottom Y */
      _kernel_oswrch ((640/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_640_x_480_8bpp: /* Select screen mode 640x480 8 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_640x480x8;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      rpc_palette = 1;

      SCREENWIDTH  = 640;
      SCREENHEIGHT = 480;

	  /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((480/8)-1); /* Bottom Y */
      _kernel_oswrch ((640/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_640_x_400_16bpp: /* Select screen mode 640x400 16 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_640x400x16;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      rpc_palette = 1;

      SCREENWIDTH  = 640;
      SCREENHEIGHT = 400;

	  /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((400/8)-1); /* Bottom Y */
      _kernel_oswrch ((640/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_640_x_480_16bpp: /* Select screen mode 640x480 16 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_640x480x16;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 640;
      SCREENHEIGHT = 480;

	  /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((480/8)-1); /* Bottom Y */
      _kernel_oswrch ((640/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

    case MODE_640_x_400_24bpp: /* Select screen mode 640x400 24 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_640x400x24;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);
      rpc_palette = 1;

      SCREENWIDTH  = 640;
      SCREENHEIGHT = 400;

	  /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((400/8)-1); /* Bottom Y */
      _kernel_oswrch ((640/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;

     case MODE_640_x_480_24bpp: /* Select screen mode 640x480 24 bit colours */
      regs.r[0] = 0;
      regs.r[1] = (int) screen_640x480x24;
      rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

      SCREENWIDTH  = 640;
      SCREENHEIGHT = 480;

      /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);  /* Left X */
      _kernel_oswrch ((480/8)-1); /* Bottom Y */
      _kernel_oswrch ((640/8)-1); /* Right X */
      _kernel_oswrch (00); /* Top Y */
      break;
  }
  return (rc);
}

/* -------------------------------------------------------------------------- */

static void decode_screendef (int p)
{
  int i;

  do
  {
    p++;
    if (p >= myargc)
      break;
    switch (myargv[p][0])
    {
      case 'X':
      case 'x':
	screen_user_def [1] = atoi (&myargv[p][1]);
	break;
      case 'Y':
      case 'y':
	screen_user_def [2] = atoi (&myargv[p][1]);
	break;
      case 'C':
      case 'c':
	i = atoi (&myargv[p][1]);
	switch (i)
	{
	  case 256:
	    screen_user_def [3] = 3;
	    break;
	  case 32:
	    screen_user_def [3] = 4;
	    break;
	  case 16:
	    screen_user_def [3] = 5;
	    break;
	  default:
	    if (i > 32768)
	      screen_user_def [3] = 5;
	    else if (i > 256)
	      screen_user_def [3] = 4;
	    else
	      screen_user_def [3] = 3;
	}
	break;
      default:
	p = myargc;
    }
  } while (1);

  if (screen_user_def [3] > 3)
    screen_user_def [5] = -1;			// End of table
}

/* -------------------------------------------------------------------------- */
/*
   Find -nn in the arg list.
*/

static int get_screen_mode_arg (void)
{
  int i;
  int j;
  char * p;

  for (i = 1;i<myargc;i++)
  {
    p = myargv[i];
    if (*p == '-')
    {
      p++;
      j = *p;
      if ((j >= '0') && (j <= '9'))
	return (atoi (p));
    }
  }

  return -1;
}

/* -------------------------------------------------------------------------- */

static void select_text_mode (void)
{
  _kernel_oswrch (22);
  _kernel_oswrch (28);
  if (((_kernel_osbyte (135, 0, 0) >> 8) & 0xFF) == 28)
  {
    printf ("\n");
  }
  else
  {
    _kernel_oswrch (22);
    _kernel_oswrch (12);
  }
}

/* -------------------------------------------------------------------------- */

static unsigned char mode_search [] =
{
  MODE_640_x_480_8bpp,
  MODE_640_x_480_24bpp,
  MODE_640_x_480_16bpp,
  MODE_640_x_400_8bpp,
  MODE_640_x_400_24bpp,
  MODE_640_x_400_16bpp,
  MODE_320_x_480_24bpp,
  MODE_320_x_480_16bpp,
  MODE_320_x_480_8bpp,
  MODE_320_x_256_24bpp,
  MODE_320_x_256_16bpp,
  MODE_320_x_256_8bpp
};

/* -------------------------------------------------------------------------- */

void I_SetScreenSize (void)
{
    unsigned char * mode_number;
    _kernel_oserror * rc;
    _kernel_swi_regs regs;

    /* Set VDU driver screen bank to default */
    _kernel_osbyte (112, 0, 0);

    /* Set hardware screen bank to default */
    _kernel_osbyte (113, 0, 0);

    /* Set Non-shadow */
    _kernel_osbyte (114, 1, 0);

    screen_mode = get_screen_mode_arg ();
    if (screen_mode == MODE_USER_DEF)
    {
      decode_screendef (M_CheckParm("-0"));
      select_screen_mode (0);
      init_sprite_area ();
    }

    else if (screen_mode < 0)	/* No screen size/resolution given... */
    {
      regs.r[1] = (int) "OS_ScreenMode";
      if ((_kernel_swi (OS_SWINumberFromString, &regs, &regs)) == 0)
      {  /* So the OS_ScreenMode call exists, which means that 16/24 bit */
	 /* colour may be available..... */
	 /* I tested this code by preventing my Risc PC from executing it's */
	 /* !boot programs, hence the monitor definition file wasn't read and */
	 /* hence the 16/24 bit modes were not available...... */

	 /* Try first for the best... */
	mode_number = mode_search;

	do
	{
	  rc = select_screen_mode (*mode_number);
	  if (rc)
	    mode_number++;
	  else
	    screen_mode = *mode_number;
	} while (rc);
      }
      else  /* Not an RPC, so try mode 49 */
      {
	_kernel_oswrch (22);
	_kernel_oswrch (49);
	if (((_kernel_osbyte (135, 0, 0) >> 8) & 0xFF) == 49)
	  screen_mode = MODE_320_x_480_8bpp;
      }
    }

    rc = select_screen_mode (screen_mode);
    if (rc)
    {
      select_text_mode ();
      I_Error ("Failed to select screen mode (%s)\n", rc -> errmess);
    }

    select_text_mode ();

    // printf ("Welcome to Doom For Acorn RiscPC.\nFree software built from the Linux source by Jeff Doggett\n\n");
    //k = 0;
    //do
    //{
    //  printf ("Run %s ", myargv[k++]);
    //} while (k < myargc);
    //printf ("\n\n");
}

/* -------------------------------------------------------------------------- */

void I_InitGraphics (void)
{
    _kernel_swi_regs regs;
    unsigned int block [32];
    unsigned char osword_data [12];

    memset (key_down_table, 0, sizeof (key_down_table));
    select_screen_mode (screen_mode);

    if (screen_mode == MODE_USER_DEF)
    {
      if (screen_user_def [3] == 3)
	screens[0] = ((unsigned char*) &sprite_mem->sprite_size) + (sprite_mem -> offset);
    }
    else
    {
      /* Find out where the operating system has put the screen memory */
      block [0] = 149;
      block [1] = -1;

      regs.r[0] = (int) &block[0];
      regs.r[1] = (int) &block[2];

      _kernel_swi (OS_ReadVduVariables, &regs, &regs);
      screen_addr = (unsigned int *) block[2];
      /* printf ("Screen address is %X\n", screen_addr); */
    }

    /* Cursor off */
    _kernel_oswrch (23);
    _kernel_oswrch (1);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);

    /* Stop the Escape key aborting the program */
    _kernel_osbyte (229, 1, 0);

    /* Turn off cursor key editing */
    _kernel_osbyte (4, 1, 0);


    /* Set an infinite mouse bounding box */
    regs.r[0] = 21;
    regs.r[1] = (int) &osword_data[0];
    osword_data[0] = 1;
    osword_data[1] = 0x00;
    osword_data[2] = 0x80;
    osword_data[3] = 0x00;
    osword_data[4] = 0x80;
    osword_data[5] = 0xFF;
    osword_data[6] = 0x7F;
    osword_data[7] = 0xFF;
    osword_data[8] = 0x7F;
    _kernel_swi (OS_Word, &regs, &regs);


    /* Find out where the mouse currently is */
    _kernel_swi (OS_Mouse, &regs, &regs);
    lastmousex = regs.r[0];
    lastmousey = regs.r[1];

    /* Read inkey() table file */
    init_key_tables ();
}

/* -------------------------------------------------------------------------- */

void I_SetWindowName (const char * title)
{
}

/* -------------------------------------------------------------------------- */
