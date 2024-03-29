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

extern keyb_t	keyb;

extern int	viewwidth;
extern int	viewheight;

extern int	weaponrecoil;

extern int	mouseSensitivity;
extern int	showMessages;
extern int	show_discicon;

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
  {"show_discicon",&show_discicon, sizeof(int), (char *) 1},

  {"key_right",&keyb.right_1, sizeof(int), (char *) KEY_RIGHTARROW},
  {"key_left",&keyb.left_1, sizeof(int), (char *) KEY_LEFTARROW},
  {"key_up",&keyb.up_1, sizeof(int), (char *) KEY_UPARROW},
  {"key_down",&keyb.down_1, sizeof(int), (char *) KEY_DOWNARROW},

  {"key_right_alt",&keyb.right_2, sizeof(int), (char *) 'd'},
  {"key_left_alt",&keyb.left_2, sizeof(int), (char *) 'a'},
  {"key_up_alt",&keyb.up_2, sizeof(int), (char *) 'w'},
  {"key_down_alt",&keyb.down_2, sizeof(int), (char *) 's'},

  {"key_strafeleft",&keyb.strafeleft, sizeof(int), (char *) ','},
  {"key_straferight",&keyb.straferight, sizeof(int), (char *) '.'},

  {"key_fire",&keyb.fire, sizeof(int), (char *) KEY_RCTRL},
  {"key_use",&keyb.use, sizeof(int), (char *) ' '},
  {"key_strafe",&keyb.strafe, sizeof(int), (char *) KEY_RALT},
  {"key_speed",&keyb.speed, sizeof(int), (char *) KEY_RSHIFT},


  {"use_mouse",&keyb.usemouse, sizeof(int), (char *) 1},
  {"mouseb_fire",&keyb.mousebfire, sizeof(int), (char *) 0},
  {"mouseb_strafe",&keyb.mousebstrafe, sizeof(int), (char *) 1},
  {"mouseb_forward",&keyb.mousebforward, sizeof(int), (char *) 2},


  {"use_joystick",&keyb.usejoystick, sizeof(int), (char *) 0},
  {"joyb_fire",&keyb.joybfire, sizeof(int), (char *) 0},
  {"joyb_strafe",&keyb.joybstrafe, sizeof(int), (char *) 1},
  {"joyb_use",&keyb.joybuse, sizeof(int), (char *) 3},
  {"joyb_speed",&keyb.joybspeed, sizeof(int), (char *) 2},

  {"novert",&keyb.novert, sizeof(int), (char *) 0},
  {"always_run",&keyb.always_run, sizeof(int), (char *) 0},

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
  char* w;
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
    fputs ("# Doom config file\n"
	   "# Commented out lines indicate the current defaults\n"
	   "# To change an entry uncomment the line and modify the setting\n\n", f);

    for (i=0 ; i<numdefaults ; i++)
    {
      switch (defaults[i].type)
      {
	case 0:
	  w = * (char **) defaults[i].location;
	  if (strcmp (w, defaults[i].defaultvalue) == 0)
	    fputs ("# ", f);
	  fprintf (f,"%s\t\t\"%s\"\n",defaults[i].name, w);
          break;

	default: //case sizeof(int):
	  v = *defaults[i].location;
	  if (v == (intptr_t) defaults[i].defaultvalue)
	    fputs ("# ", f);
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
      if (fscanf (f, "%79s %99[^\n]\n", def, strparm) == 2)
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
#ifndef __riscos
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
boolean
WritePCXfile
( char*		filename,
  byte*		data,
  int		width,
  int		height,
  byte*		palette )
{
  int		i;
  int		length;
  boolean	rc;
  pcx_t*	pcx;
  byte*		pack;

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
  rc = M_WriteFile (filename, pcx, length);

  Z_Free (pcx);
  return (rc);
}
#endif
//-----------------------------------------------------------------------------
//
// M_ScreenShot
//
void M_ScreenShot (void)
{
  int	i;
  byte*	linear;
  boolean status;
  char	pcxname [6];
  char	lbmname [80];
  char * shotdir;

  // munge planar buffer to linear
  linear = screens[2];
  I_ReadScreen (linear);

  // I only want the first four characters as I now use a four digit index...
  memcpy (pcxname, screenshot_messages[SC_FILENAME], 4);
  pcxname [4] = 0;

  // find a file name to save it to

  shotdir = basedefaultdir;		// Assume default directory

  i = M_CheckParm ("-shotdir");
  if (i && i<myargc-1)
  {
    struct stat file_stat;

    if ((stat (myargv[i+1], &file_stat) == 0)
     && (file_stat.st_mode & S_IFDIR))
    {
      shotdir = myargv[i+1];
    }
  }
  else
  {
    char * f;
#ifdef __riscos
    f = getenv ("DoomPicDir");
#else
    f = getenv ("DOOMPICDIR");
#endif
    if (f)
      shotdir = f;
  }

  i = ~0;
  do
  {
    i++;
#ifdef __riscos
    sprintf (lbmname, "%s"DIRSEP"%s%04u", shotdir, pcxname, i);
#else
    sprintf (lbmname, "%s"DIRSEP"%s%04u"EXTSEP"pcx", shotdir, pcxname, i);
#endif
  } while (access(lbmname,0) != -1);	// file doesn't exist

#ifdef __riscos
  status = riscos_screensave (lbmname);
#else
  // save the pcx file
  status = WritePCXfile (lbmname, linear,
		SCREENWIDTH, SCREENHEIGHT,
		W_CacheLumpName ("PLAYPAL",PU_CACHE));
  // set_riscos_filetype (lbmname, 0x697);
#endif

  if (status)
  {
    S_StartSound (NULL, sfx_scrsht);
    players[consoleplayer].message = HU_printf ("%s - %s", screenshot_messages[SC_MESSAGE], lbmname);
#ifdef NORMALUNIX
    printf ("%s\n", players[consoleplayer].message);
#endif
  }
}

//-----------------------------------------------------------------------------
