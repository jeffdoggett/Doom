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
//
// $Log:$
//
// DESCRIPTION:
//	Main loop menu stuff.
//	Default Config File.
//	PCX Screenshots.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: m_misc.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";
#endif

#include "includes.h"

char * screenshot_messages_orig [] =
{
  "screen shot",
  "DOOM00.pcx",
  NULL
};

char * screenshot_messages [ARRAY_SIZE(screenshot_messages_orig)];

typedef enum
{
  SC_MESSAGE,
  SC_FILENAME,
} screenshot_texts_t;

//-----------------------------------------------------------------------------
//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//
extern patch_t*		hu_font[HU_FONTSIZE];

int
M_DrawText
( int		x,
  int		y,
  boolean	direct,
  char*		string )
{
    int 	c;
    int		w;

    while (*string)
    {
	c = toupper(*string) - HU_FONTSTART;
	string++;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    x += 4;
	    continue;
	}

	w = SHORT (hu_font[c]->width);
	if (x+w > SCREENWIDTH)
	    break;
	if (direct)
	    V_DrawPatchDirect(x, y, 0, hu_font[c]);
	else
	    V_DrawPatchScaled(x, y, 0, hu_font[c]);
	x+=w;
    }

    return x;
}

//-----------------------------------------------------------------------------
//
// M_WriteFile
//

boolean M_WriteFile (char const* name, void*source, int length)
{
  int count;
  FILE * handle;

  handle = fopen (name, "wb");
  if (handle == NULL)
    return (false);

  count = fwrite (source, 1, length, handle);
  fclose (handle);

  if (count < length)
    return false;

  return true;
}

//-----------------------------------------------------------------------------
//
// M_ReadFile
//
unsigned int M_ReadFile (char const*name, byte**buffer)
{
  unsigned int length;
  byte *buf;
  FILE * handle;

  length = filesize (name);
  if (length)
  {
    handle = fopen (name, "rb");
    if (handle == NULL)
    {
      length = 0;
    }
    else
    {
      buf = Z_Malloc (length, PU_STATIC, NULL);
      length = fread (buf, 1, length, handle);
      fclose (handle);
      *buffer = buf;
    }
  }

  return (length);
}

//-----------------------------------------------------------------------------
//
// DEFAULTS
//
int		usemouse;
int		novert;
int		usejoystick;

extern int	key_right;
extern int	key_left;
extern int	key_up;
extern int	key_down;

extern int	key_strafeleft;
extern int	key_straferight;

extern int	key_fire;
extern int	key_use;
extern int	key_strafe;
extern int	key_speed;
extern int	always_run;

extern int	mousebfire;
extern int	mousebstrafe;
extern int	mousebforward;


extern int	joybfire;
extern int	joybstrafe;
extern int	joybuse;
extern int	joybspeed;

extern int	viewwidth;
extern int	viewheight;

extern int	weaponrecoil;

extern int	mouseSensitivity;
extern int	showMessages;

extern int	detailLevel;
extern int	screenblocks;

extern int	showMessages;

// machine-independent sound params
extern int	numChannels;

extern unsigned int max_vissprites;

// UNIX hack, to be removed.
#ifdef NORMALUNIX
#ifdef SNDSERV
extern char*	sndserver_filename;
#endif
#endif

#ifdef LINUX
char*		mousetype;
char*		mousedev;
#endif

extern int	translucency;

extern char*	chat_macros[];

//-----------------------------------------------------------------------------
#ifdef __riscos
extern int	mp3priority;
extern int	timplayer_vol_tab [16];
#endif

typedef struct
{
  char*		name;
  int*		location;
  int		type;
  char *	defaultvalue;
  int		scantranslate;		// PC scan code hack
  int		untranslated;		// lousy hack
} default_t;

default_t	defaults[] =
{
  {"mouse_sensitivity",&mouseSensitivity, sizeof(int), (char *) 5},
  {"sfx_volume",&snd_SfxVolume, sizeof(int), (char *) 8},
  {"music_volume",&snd_MusicVolume, sizeof(int), (char *) 8},
  {"show_messages",&showMessages, sizeof(int), (char *) 1},

  {"key_right",&key_right, sizeof(int), (char *) KEY_RIGHTARROW},
  {"key_left",&key_left, sizeof(int), (char *) KEY_LEFTARROW},
  {"key_up",&key_up, sizeof(int), (char *) KEY_UPARROW},
  {"key_down",&key_down, sizeof(int), (char *) KEY_DOWNARROW},
  {"key_strafeleft",&key_strafeleft, sizeof(int), (char *) ','},
  {"key_straferight",&key_straferight, sizeof(int), (char *) '.'},

  {"key_fire",&key_fire, sizeof(int), (char *) KEY_RCTRL},
  {"key_use",&key_use, sizeof(int), (char *) ' '},
  {"key_strafe",&key_strafe, sizeof(int), (char *) KEY_RALT},
  {"key_speed",&key_speed, sizeof(int), (char *) KEY_RSHIFT},

// UNIX hack, to be removed.
#ifdef NORMALUNIX
#ifdef SNDSERV
  {"sndserver", (int *) &sndserver_filename, 0, "sndserver"},
#endif
#endif


#ifdef LINUX
  {"mousedev", (int*)&mousedev, 0, "/dev/ttyS0"},
  {"mousetype", (int*)&mousetype, 0, "microsoft"},
#endif

  {"use_mouse",&usemouse, sizeof(int), (char *) 1},
  {"mouseb_fire",&mousebfire, sizeof(int), (char *) 0},
  {"mouseb_strafe",&mousebstrafe, sizeof(int), (char *) 1},
  {"mouseb_forward",&mousebforward, sizeof(int), (char *) 2},
  {"novert",&novert, sizeof(int), (char *) 0},


  {"use_joystick",&usejoystick, sizeof(int), (char *) 0},
  {"joyb_fire",&joybfire, sizeof(int), (char *) 0},
  {"joyb_strafe",&joybstrafe, sizeof(int), (char *) 1},
  {"joyb_use",&joybuse, sizeof(int), (char *) 3},
  {"joyb_speed",&joybspeed, sizeof(int), (char *) 2},

  {"always_run",&always_run, sizeof(int), (char *) 0},

  {"screenblocks",&screenblocks, sizeof(int), (char *) 10},
  {"detaillevel",&detailLevel, sizeof(int), (char *) 0},
  {"stbar_scale",&stbar_scale, sizeof(int), (char *) 1},
  {"hutext_scale",&hutext_scale, sizeof(int), (char *) 1},
  {"weaponscale",&weaponscale, sizeof(int), (char *) 1},

  {"weaponrecoil",&weaponrecoil, sizeof(int), (char *) 0},

  {"snd_channels",&numChannels, sizeof(int), (char *) 3},
  {"play_music",&snd_AllowMusic, sizeof(int), (char *) 1},

  {"usegamma",&usegamma, sizeof(int), 0},
  {"lightscaleshift",&lightScaleShift, sizeof(int), (char *) LIGHTSCALESHIFT},
  {"translucency",&translucency, sizeof(int), (char *) 50},

#ifdef __riscos
#ifdef RCOMP
  {"mp3priority", (int *) &mp3priority,  sizeof(int), (char *) 0},
#else
  {"mp3priority", (int *) &mp3priority,  sizeof(int), (char *) 1},
#endif
  {"timplayer_vol0", (int *) &timplayer_vol_tab[0],  sizeof(int), (char *) 0x00},
  {"timplayer_vol1", (int *) &timplayer_vol_tab[1],  sizeof(int), (char *) 0x04},
  {"timplayer_vol2", (int *) &timplayer_vol_tab[2],  sizeof(int), (char *) 0x08},
  {"timplayer_vol3", (int *) &timplayer_vol_tab[3],  sizeof(int), (char *) 0x0C},
  {"timplayer_vol4", (int *) &timplayer_vol_tab[4],  sizeof(int), (char *) 0x10},
  {"timplayer_vol5", (int *) &timplayer_vol_tab[5],  sizeof(int), (char *) 0x14},
  {"timplayer_vol6", (int *) &timplayer_vol_tab[6],  sizeof(int), (char *) 0x18},
  {"timplayer_vol7", (int *) &timplayer_vol_tab[7],  sizeof(int), (char *) 0x1C},
  {"timplayer_vol8", (int *) &timplayer_vol_tab[8],  sizeof(int), (char *) 0x20},
  {"timplayer_vol9", (int *) &timplayer_vol_tab[9],  sizeof(int), (char *) 0x24},
  {"timplayer_vol10", (int *) &timplayer_vol_tab[10],  sizeof(int), (char *) 0x28},
  {"timplayer_vol11", (int *) &timplayer_vol_tab[11],  sizeof(int), (char *) 0x2C},
  {"timplayer_vol12", (int *) &timplayer_vol_tab[12],  sizeof(int), (char *) 0x30},
  {"timplayer_vol13", (int *) &timplayer_vol_tab[13],  sizeof(int), (char *) 0x34},
  {"timplayer_vol14", (int *) &timplayer_vol_tab[14],  sizeof(int), (char *) 0x38},
  {"timplayer_vol15", (int *) &timplayer_vol_tab[15],  sizeof(int), (char *) 0x3C},
#endif

  {"max_vissprites", (int *) &max_vissprites,  sizeof(int), (char *) 0},

  {"chatmacro0", (int *) &chat_macros[0], 0, HUSTR_CHATMACRO0 },
  {"chatmacro1", (int *) &chat_macros[1], 0, HUSTR_CHATMACRO1 },
  {"chatmacro2", (int *) &chat_macros[2], 0, HUSTR_CHATMACRO2 },
  {"chatmacro3", (int *) &chat_macros[3], 0, HUSTR_CHATMACRO3 },
  {"chatmacro4", (int *) &chat_macros[4], 0, HUSTR_CHATMACRO4 },
  {"chatmacro5", (int *) &chat_macros[5], 0, HUSTR_CHATMACRO5 },
  {"chatmacro6", (int *) &chat_macros[6], 0, HUSTR_CHATMACRO6 },
  {"chatmacro7", (int *) &chat_macros[7], 0, HUSTR_CHATMACRO7 },
  {"chatmacro8", (int *) &chat_macros[8], 0, HUSTR_CHATMACRO8 },
  {"chatmacro9", (int *) &chat_macros[9], 0, HUSTR_CHATMACRO9 }
};

#define	numdefaults	(ARRAY_SIZE(defaults))

//-----------------------------------------------------------------------------

static char * get_doomrc_filename (char * buffer)
{
  unsigned int i;

  i = M_CheckParm ("-config");
  if (i && i<myargc-1)
  {
    return (myargv[i+1]);
  }

#ifdef __riscos
  sprintf (buffer, "%s.doomrc", basedefaultdir);
#else
  sprintf (buffer, "%s/.doomrc", basedefaultdir);
#endif
  return (buffer);
}

//-----------------------------------------------------------------------------
/* If we are running under Rox, then there might be a default */
/* config file in the system area, so need to search through */
/* the CHOICESPATH to find it. */
#ifdef NORMALUNIX

char * search_choices_path (char * buffer)
{
  char cc;
  unsigned int p;
  char * basedir;
  char *choicespath;

  choicespath = getenv ("CHOICESPATH");			// Are we running under ROX?
  if (choicespath)
  {							// Yes. Search for the first
    p = 0;						// writable directory.

    do
    {
      cc = *choicespath++;
      buffer [p++] = cc;

      if ((cc == 0) || (cc == ':'))
      {
	buffer [--p] = 0;
	strcat (buffer, "/.doomrc");
	if (access (buffer, R_OK) == 0)
	  return (buffer);
	buffer [p] = 0;
	strcat (buffer, "/dsg/.doomrc");
	if (access (buffer, R_OK) == 0)
	  return (buffer);
	buffer [p] = 0;
	strcat (buffer, "/doomrc");
	if (access (buffer, R_OK) == 0)
	  return (buffer);
	buffer [p] = 0;
	strcat (buffer, "/dsg/doomrc");
	if (access (buffer, R_OK) == 0)
	  return (buffer);
	p = 0;
      }
    } while (cc);
  }

  return (NULL);
}
#endif
//-----------------------------------------------------------------------------
//
// M_SaveDefaults
//
void M_SaveDefaults (void)
{
  int	i,v;
  FILE*	f;
  char*	defaultfile;
  char	buffer [1024];

  defaultfile = get_doomrc_filename (buffer);

  /* If the file has been set to read-only then don't bother */
#if 1
  if ((access (defaultfile, F_OK) == 0)
   && (access (defaultfile, W_OK) != 0))
    return;
#else
  rc = stat (defaultfile, &buf);
  if ((rc == 0)
   && ((buf.st_mode & S_IWRITE) == 0))
    return;
#endif

  if ((f = fopen (defaultfile, "w")) != NULL)
  {
    for (i=0 ; i<numdefaults ; i++)
    {
      switch (defaults[i].type)
      {
	case 0:
	  fprintf (f,"%s\t\t\"%s\"\n",defaults[i].name,
		 * (char **) (defaults[i].location));
          break;

	default: //case sizeof(int):
	  v = *defaults[i].location;
	  fprintf (f,"%s\t\t%i\n",defaults[i].name,v);
          break;
      }
    }

    fclose (f);
  }
}

//-----------------------------------------------------------------------------
//
// M_LoadDefaults
//
extern byte	scantokey[128];

void M_LoadDefaults (void)
{
  unsigned int	i;
  int		len;
  FILE*		f;
  char		def[80];
  char		strparm[100];
  char*		newstring;
  int		parm;
  boolean	isstring;
  char*		defaultfile;
  char		buffer [1024];

  // set everything to base values
  for (i=0 ; i<numdefaults ; i++)
      *defaults[i].location = (uintptr_t) defaults[i].defaultvalue;

  defaultfile = get_doomrc_filename (buffer);

#ifdef NORMALUNIX
  if (access (defaultfile, R_OK) != 0)
    defaultfile = search_choices_path (buffer);
#endif

  // read the file in, overriding any set defaults
  if ((defaultfile) && (defaultfile [0])		// Just in case...
   && ((f = fopen (defaultfile, "r")) != NULL))
  {
    while (!feof(f))
    {
      isstring = false;
      if (fscanf (f, "%79s %[^\n]\n", def, strparm) == 2)
      {
	if (strparm[0] == '"')
	{
	  // get a string default
	  isstring = true;
	  len = strlen(strparm);
	  newstring = (char *) malloc(len);
	  strparm[len-1] = 0;
	  strcpy(newstring, strparm+1);
	}
	else if (strparm[0] == '0' && strparm[1] == 'x')
	  sscanf(strparm+2, "%x", &parm);
	else
	  sscanf(strparm, "%i", &parm);
	for (i=0 ; i<numdefaults ; i++)
	  if (!strcasecmp(def, defaults[i].name))
	  {
	    if (!isstring)
	    {
		*defaults[i].location = parm;
	    }
	    else
	    {
	      char * p;
	      p = (char*)defaults[i].location;
	      p = newstring;
	    }
	    break;
	  }
      }
    }

    fclose (f);
  }
}


//-----------------------------------------------------------------------------
//
// SCREEN SHOTS
//


typedef struct
{
    char		manufacturer;
    char		version;
    char		encoding;
    char		bits_per_pixel;

    unsigned short	xmin;
    unsigned short	ymin;
    unsigned short	xmax;
    unsigned short	ymax;

    unsigned short	hres;
    unsigned short	vres;

    unsigned char	palette[48];

    char		reserved;
    char		color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;

    char		filler[58];
    unsigned char	data;		// unbounded
} pcx_t;


//-----------------------------------------------------------------------------
//
// WritePCXfile
//
void
WritePCXfile
( char*		filename,
  byte*		data,
  int		width,
  int		height,
  byte*		palette )
{
  int		i;
  int		length;
  pcx_t*	pcx;
  byte*	pack;

  pcx = Z_Malloc (width*height*2+1000, PU_STATIC, NULL);

  pcx->manufacturer = 0x0a;		// PCX id
  pcx->version = 5;			// 256 colour
  pcx->encoding = 1;			// uncompressed
  pcx->bits_per_pixel = 8;		// 256 colour
  pcx->xmin = 0;
  pcx->ymin = 0;
  i = width - 1;
  pcx->xmax = SHORT(i);
  i = height - 1;
  pcx->ymax = SHORT(i);
  pcx->hres = SHORT(width);
  pcx->vres = SHORT(height);
  memset (pcx->palette,0,sizeof(pcx->palette));
  pcx->color_planes = 1;		// chunky image
  pcx->bytes_per_line = SHORT(width);
  i = 2;				// not a grey scale
  pcx->palette_type = SHORT(i);
  memset (pcx->filler,0,sizeof(pcx->filler));


  // pack the image
  pack = &pcx->data;

  for (i=0 ; i<width*height ; i++)
  {
      if ( (*data & 0xc0) != 0xc0)
	  *pack++ = *data++;
      else
      {
	  *pack++ = 0xc1;
	  *pack++ = *data++;
      }
  }

  // write the palette
  *pack++ = 0x0c;	// palette ID byte
  for (i=0 ; i<768 ; i++)
      *pack++ = *palette++;

  // write output file
  length = pack - (byte *)pcx;
  M_WriteFile (filename, pcx, length);

  Z_Free (pcx);
}

//-----------------------------------------------------------------------------
//
// M_ScreenShot
//
void M_ScreenShot (void)
{
  int	i;
  byte*	linear;
  char	pcxname [6];
  char	lbmname [80];

  // munge planar buffer to linear
  linear = screens[2];
  I_ReadScreen (linear);

  // I only want the first four characters as I now use a four digit index...
  memcpy (pcxname, screenshot_messages[SC_FILENAME], 4);
  pcxname [4] = 0;

  // find a file name to save it to
  i=~0;
  do
  {
    i++;
    sprintf (lbmname, "%s"DIRSEP"%s%04u"EXTSEP"pcx", basedefaultdir, pcxname, i);
  } while (access(lbmname,0) != -1);	// file doesn't exist

  // save the pcx file
  WritePCXfile (lbmname, linear,
		SCREENWIDTH, SCREENHEIGHT,
		W_CacheLumpName ("PLAYPAL",PU_CACHE));
#ifdef __riscos
  set_riscos_filetype (lbmname, 0x697);
#endif

  players[consoleplayer].message = HU_printf ("%s - %s", screenshot_messages[SC_MESSAGE], lbmname);
#ifdef NORMALUNIX
  printf ("%s\n", players[consoleplayer].message);
#endif
}

//-----------------------------------------------------------------------------
