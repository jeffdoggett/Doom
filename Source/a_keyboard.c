/* Acorn Kerboard drivers
   Written by J.A.Doggett.

   Based on i_video.c so some of the comments herein are ID's
*/
#include "includes.h"
#include <kernel.h>

#define OS_Word				0x07
#define OS_Mouse			0x1C


#define MOUSE_SELECT 1
#define MOUSE_MENU   3
#define MOUSE_ADJUST 2

static unsigned char  key_down_table [128];
static unsigned char  num_keys_down [10];
static unsigned char  key_to_scan = 3;

static unsigned int   lastmousex;
static unsigned int   lastmousey;
static unsigned int   lastmousebut;
static unsigned char  menu_in_use = 0;
static boolean	      constrain_mouse = false;

extern boolean	menuactive;
extern keyb_t	keyb;

/* -------------------------------------------------------------------------- */
/* Table to convert Acorn inkey() values to Doom key strokes */
/* These tables are overwritten by the contents of file "Inkeys" if it exists */

static unsigned char key_xlate_table [] =
{
  0, 0, 0,	/* 1st three are duplicates */
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

/* -------------------------------------------------------------------------- */

/* Table to swap mouse buttons round */
static int mouse_button_conv[] = { 0, 4, 2, 6, 1, 5, 3, 7 };


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

static void mouse_to (unsigned int x, unsigned int y)
{
    _kernel_swi_regs regs;
    unsigned char osword_data [12];

    regs.r[0] = 21;
    regs.r[1] = (int) &osword_data[0];
    osword_data[0] = 3;
    osword_data[1] = x; x >>= 8;
    osword_data[2] = x;
    osword_data[3] = y; y >>= 8;
    osword_data[4] = y;
    _kernel_swi (OS_Word, &regs, &regs);
}

/* -------------------------------------------------------------------------- */

static void poll_mouse (event_t * event)
{
  _kernel_swi_regs regs;

  _kernel_swi (OS_Mouse, &regs, &regs);

  if (keyb.novert)
    regs.r[1] = lastmousey;

  if ((regs.r[0] != lastmousex)
   || (regs.r[1] != lastmousey)
   || (regs.r[2] != lastmousebut))
  {
#if 0
    _kernel_oswrch (4);
    _kernel_oswrch (31);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    printf ("%d,%d  %d,%d\n", regs.r[0], regs.r[1], lastmousex, lastmousey);
    _kernel_oswrch (5);
#endif
    event->type = ev_mouse;
    event->data1 = 0;
    lastmousebut = regs.r[2];
    event->data1 = mouse_button_conv [lastmousebut & 7];
    event->data2 = determine_amount_mouse_has_moved (regs.r[0], lastmousex);
    event->data3 = determine_amount_mouse_has_moved (regs.r[1], lastmousey);
    D_PostEvent (event);
    if (constrain_mouse)
    {
      mouse_to (lastmousex = SCREENWIDTH / 2, lastmousey = SCREENHEIGHT / 2);
    }
    else
    {
      lastmousex = regs.r[0];
      lastmousey = regs.r[1];
    }
  }
}

/* -------------------------------------------------------------------------- */

/* When the menu is in use, we use inkey(0) to read the keyboard */

static void poll_kbd_menu_mode (void)
{
  event_t event;
  unsigned int key;

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
  poll_mouse (&event);
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
    if (key_to_scan > 127) key_to_scan = 3;
  }
  else
  {
    key_to_scan = 3;
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
  poll_mouse (&event);
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

void I_InitKeyboard (void)
{
    _kernel_swi_regs regs;
    unsigned char osword_data [12];

    memset (key_down_table, 0, sizeof (key_down_table));

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

    constrain_mouse = (boolean) M_CheckParm ("-constrain_mouse");

    /* Read inkey() table file */
    init_key_tables ();
}

/* -------------------------------------------------------------------------- */
