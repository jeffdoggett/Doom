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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: s_sound.c,v 1.6 1997/02/03 22:45:12 b1 Exp $";
#endif

#include "includes.h"


// Purpose?
const char snd_prefixen[]
= { 'P', 'P', 'A', 'S', 'S', 'S', 'M', 'M', 'M', 'S', 'S', 'S' };

#define S_MAX_VOLUME		127

// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST		(1200*0x10000)

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
#define S_CLOSE_DIST		(160*0x10000)


#define S_ATTENUATOR		((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)

// Adjustable by menu.
#define NORM_VOLUME    		snd_MaxVolume

#define NORM_PITCH     		128
#define NORM_SEP		128

#define S_PITCH_PERTURB		1
#define S_STEREO_SWING		(96*0x10000)

// percent attenuation from front to back
#define S_IFRACVOL		30

#define NA			0
#define S_NUMCHANNELS		2


// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
extern int snd_MusicDevice;
extern int snd_SfxDevice;
// Config file? Same disclaimer as above.
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;

extern void I_FillMusBuffer (int handle);


// the set of channels available
static channel_t*	channels;
static degenmobj_t*	sound_origin;

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int 		snd_SfxVolume = 15;

// Maximum volume of music.
int 		snd_MusicVolume = 15;

// Whether to play music at all
int		snd_AllowMusic  = 1;


// whether songs are mus_paused
static boolean		mus_paused;

// music currently being played
static musicinfo_t*	mus_playing=0;

// following is set
//  by the defaults code in M_misc:
// number of channels available
int			numChannels;
static int		nextChannel = 0;

//static int		nextcleanup;

static int		nomusic;
static int		nosfx;
static int		monosfx;

muschangeinfo_t		muschangeinfo;

/* ------------------------------------------------------------ */
//
// Internals.
//
static int S_getChannel (sfxinfo_t* sfxinfo);


static int
S_AdjustSoundParams
( mobj_t*	listener,
  mobj_t*	source,
  int*		vol,
  int*		sep,
  int*		pitch );

static void S_StopChannel(int cnum);
static void S_ReadMUSINFO (void);

/* ------------------------------------------------------------ */

void S_InitSound (void)
{
  nomusic = !snd_AllowMusic;
  nosfx = false;
  monosfx = false;

  if (M_CheckParm ("-nosound"))
  {
    nomusic = true;
    nosfx = true;
  }
  else
  {
    if (M_CheckParm ("-nomusic"))
      nomusic = true;
    if (M_CheckParm ("-nosfx"))
      nosfx = true;
    if (M_CheckParm ("-monosfx"))
      monosfx = true;
  }

  S_ReadMUSINFO ();

  if (!nomusic)
    I_InitMusic ();

  if ((nosfx == false) || (nomusic == false))
    I_InitSound ();
}

/* ------------------------------------------------------------ */

void S_ShutdownSound (void)
{
  if (!nomusic)
    I_ShutdownMusic();

  if ((nosfx == false) || (nomusic == false))
    I_ShutdownSound();
}

/* ------------------------------------------------------------ */
//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init
( int		sfxVolume,
  int		musicVolume )
{
  // fprintf( stderr, "S_Init: default sfx volume %d\n", sfxVolume);

  // Whatever these did with DMX, these are rather dummies now.
  I_SetChannels();

  S_SetSfxVolume(sfxVolume);
  // No music with Linux - another dummy.
  S_SetMusicVolume(musicVolume);

  // Allocating the internal channels for mixing
  // (the maximum numer of sounds rendered
  // simultaneously) within zone memory.
  channels = (channel_t *) Z_Calloc (numChannels*sizeof(channel_t), PU_STATIC, 0);
  sound_origin = (degenmobj_t *) Z_Calloc (numChannels*sizeof(degenmobj_t), PU_STATIC, 0);

  // Free all channels for use
//for (i=0 ; i<numChannels ; i++)
//  channels[i].sfxinfo = 0;

  // no sounds are playing, and they are not mus_paused
  mus_paused = false;
}

/* ------------------------------------------------------------ */

static const unsigned int spmus[]=
{
  // Song - Who? - Where?
  mus_e3m5,	//		e4m0
  mus_e3m4,	// American	e4m1
  mus_e3m2,	// Romero	e4m2
  mus_e3m3,	// Shawn	e4m3
  mus_e1m5,	// American	e4m4
  mus_e2m7,	// Tim		e4m5
  mus_e2m4,	// Romero	e4m6
  mus_e2m6,	// J.Anderson	e4m7 CHIRON.WAD
  mus_e2m5,	// Shawn	e4m8
  mus_e1m9	// Tim		e4m9
};

/* ------------------------------------------------------------ */

int S_StartLevelMusic (unsigned int ge, unsigned int gm)
{
  int mnum;
  char * music;
  map_dests_t * map_ptr;
  char		namebuf[12];

  if ((ge > 255) || (gm > 255))
    return (1);

  // start new music for the level
  mus_paused = false;

  map_ptr = G_Access_MapInfoTab (ge, gm);
  music = map_ptr -> music;
  if (music)
  {
    S_music[mus_extra].name = music;
    mnum = mus_extra;
  }
  else if (gamemode == commercial)
  {
    while (gm > 32)
      gm -= 32;
    mnum = mus_runnin + gm - 1;
  }
  else if ((ge > 0) && (ge < 4) && (gm > 0) && (gm < 10))
  {
    mnum = mus_e1m1 + ((ge-1)*9) + (gm-1);
  }
  else
  {
    /* See whether we have this music in the wad */
    sprintf (namebuf, "D_E%uM%u", ge, gm);
    if (W_CheckNumForName (namebuf) != -1)
    {
      S_music[mus_extra].name = namebuf;
      mnum = mus_extra;
    }
    else
    {
      while (gm >= ARRAY_SIZE (spmus))
	gm -= ARRAY_SIZE (spmus);
      mnum = spmus[gm];
    }
  }

  // HACK FOR COMMERCIAL
  //  if (commercial && mnum > mus_e3m9)
  //      mnum -= mus_e3m9;

  S_ChangeMusic (mnum, true);

//nextcleanup = 15;
  return (0);
}

/* ------------------------------------------------------------ */
//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
  int cnum;

  // kill all playing sounds at start of level
  //  (trust me - a good idea)
  for (cnum=0 ; cnum<numChannels ; cnum++)
    channels[cnum].origin = NULL;

  muschangeinfo.mapthing = NULL;
  muschangeinfo.musnum = 0;
  muschangeinfo.tics = 0;
  S_StartLevelMusic (gameepisode, gamemap);
}

/* ------------------------------------------------------------ */
//
// Retrieve the raw data lump index
//  for a given SFX name.
//
static int S_GetSfxLumpNum (sfxinfo_t* sfx)
{
  char namebuf[9];

  sprintf (namebuf, "ds%s", sfx->name);
  return W_CheckNumForName (namebuf);
}

/* ------------------------------------------------------------ */

static void S_StartSoundAtVolume (mobj_t* origin, int sfx_id, int volume)
{
  int		rc;
  int		sep;
  int		pitch;
  sfxinfo_t*	sfx;
  channel_t*	c;
  int		cnum;
  int		next;
  int		lumpnum;


  // Debug.
  // fprintf (stderr,"S_StartSoundAtVolume: %d (%s)\n", sfx_id, S_sfx[sfx_id].name );

  if (nosfx)
    return;

  // check for bogus sound #
  if (sfx_id < 1 || sfx_id > NUMSFX)
  {
    fprintf (stderr, "Bad sfx #: %d\n", sfx_id);
    return;
  }

  sfx = &S_sfx[sfx_id];

  // get lumpnum if necessary
  if ((lumpnum = sfx->lumpnum) < 0)
  {
    lumpnum = S_GetSfxLumpNum (sfx);
    if (lumpnum >= 0)
    {
      sfx->link = NULL;			// We have the actual lump, so destroy the link.
    }
    else if (sfx->link == NULL)		// Alternative available?
    {
      return;				// No.
    }
    else
    {
      lumpnum = S_GetSfxLumpNum (sfx->link);
      if (lumpnum < 0)
	return;
      if (sfx->pitch == -1)		// Do we want the different settings?
	sfx->link = NULL;		// No. Destroy the link now that it has been used.
    }
    sfx->lumpnum = lumpnum;
  }

  // Initialize sound parameters

  if (sfx->link)
  {
    volume += sfx->volume;

    if (volume < 1)
      return;

    if (volume > snd_SfxVolume)
      volume = snd_SfxVolume;

    pitch = sfx->pitch;
    if (pitch < 0)
      pitch = NORM_PITCH;
  }
  else
  {
    pitch = NORM_PITCH;
  }

  // Check to see if it is audible,
  //  and if not, modify the params
  if (origin && origin != players[consoleplayer].mo)
  {
    rc = S_AdjustSoundParams(players[consoleplayer].mo,
			     origin,
			     &volume,
			     &sep,
			     &pitch);

    if (!rc)
      return;

    if ( origin->x == players[consoleplayer].mo->x
	 && origin->y == players[consoleplayer].mo->y)
    {
      sep = NORM_SEP;
    }
  }
  else
  {
    sep = NORM_SEP;
  }

  // hacks to vary the sfx pitches
  if (sfx_id >= sfx_sawup
      && sfx_id <= sfx_sawhit)
  {
    pitch += 8 - (M_Random()&15);

    if (pitch<0)
      pitch = 0;
    else if (pitch>255)
      pitch = 255;
  }
  else if (sfx_id != sfx_itemup
	   && sfx_id != sfx_tink)
  {
    pitch += 16 - (M_Random()&31);

    if (pitch<0)
      pitch = 0;
    else if (pitch>255)
      pitch = 255;
  }

  // try to find a channel
  cnum = S_getChannel (sfx);

  next = cnum + 1;
  if (next >= numChannels)
    next = 0;
  nextChannel = next;

  c = &channels[cnum];
  c->sfxinfo = sfx;
  c->origin = origin;

#ifndef SNDSRV
  // cache data if necessary
  if ((sfx->data == NULL)
   || (sfx->data != lumpinfo[lumpnum].cache))
  {
    sfx->data = (void *) W_CacheLumpNum (lumpnum, PU_SOUND);
    sfx->adata = NULL;
    sfx->length = 0;
  }
#endif

  // Assigns the handle to one of the channels in the
  //  mix/output buffer.
  // printf ("I_StartSound (%d, %d, %d, %d, %d\n", sfx_id, volume, sep, pitch);
  c->handle = I_StartSound (sfx_id, volume, sep, pitch);
}

/* ------------------------------------------------------------ */

void
S_StartSound
( mobj_t*	origin,
  int		sfx_id )
{
#ifdef SAWDEBUG
    // if (sfx_id == sfx_sawful)
    // sfx_id = sfx_itemup;
#endif

    S_StartSoundAtVolume(origin, sfx_id, snd_SfxVolume);


    // UNUSED. We had problems, had we not?
#ifdef SAWDEBUG
{
    int i;
    int n;

    static mobj_t*      last_saw_origins[10] = {1,1,1,1,1,1,1,1,1,1};
    static int		first_saw=0;
    static int		next_saw=0;

    if (sfx_id == sfx_sawidl
	|| sfx_id == sfx_sawful
	|| sfx_id == sfx_sawhit)
    {
	for (i=first_saw;i!=next_saw;i=(i+1)%10)
	    if (last_saw_origins[i] != origin)
		fprintf(stderr, "old origin 0x%lx != "
			"origin 0x%lx for sfx %d\n",
			last_saw_origins[i],
			origin,
			sfx_id);

	last_saw_origins[next_saw] = origin;
	next_saw = (next_saw + 1) % 10;
	if (next_saw == first_saw)
	    first_saw = (first_saw + 1) % 10;

	for (n=i=0; i<numChannels ; i++)
	{
	    if (channels[i].sfxinfo == &S_sfx[sfx_sawidl]
		|| channels[i].sfxinfo == &S_sfx[sfx_sawful]
		|| channels[i].sfxinfo == &S_sfx[sfx_sawhit]) n++;
	}

	if (n>1)
	{
	    for (i=0; i<numChannels ; i++)
	    {
		if (channels[i].sfxinfo == &S_sfx[sfx_sawidl]
		    || channels[i].sfxinfo == &S_sfx[sfx_sawful]
		    || channels[i].sfxinfo == &S_sfx[sfx_sawhit])
		{
		    fprintf(stderr,
			    "chn: sfxinfo=0x%lx, origin=0x%lx, "
			    "handle=%d\n",
			    channels[i].sfxinfo,
			    channels[i].origin,
			    channels[i].handle);
		}
	    }
	    fprintf(stderr, "\n");
	}
    }
}
#endif

}

/* ------------------------------------------------------------ */

void
S_StartSoundOnce
( mobj_t*	origin,
  int		sfx_id )
{
    int cnum;
    sfxinfo_t*	sfx;

    sfx = &S_sfx[sfx_id];
    for (cnum=0 ; cnum<numChannels ; cnum++)
    {
	if ((channels[cnum].sfxinfo == sfx) && (channels[cnum].origin == origin))
	{
	  // printf ("Suppressed sound %u\n", sfx_id);
	  return;
	}
    }

    S_StartSoundAtVolume(origin, sfx_id, snd_SfxVolume);
}

/* ------------------------------------------------------------ */

void S_RemoveSoundOrigin (void *origin)
{
    int cnum;
    mobj_t* mobj;
    degenmobj_t * sorigin;

    for (cnum=0 ; cnum<numChannels ; cnum++)
    {
	if ((channels[cnum].sfxinfo) && (channels[cnum].origin == origin))
	{
	    sorigin = &sound_origin [cnum];
	    mobj = (mobj_t*) origin;
	    sorigin -> x = mobj -> x;
	    sorigin -> y = mobj -> y;
	    sorigin -> z = mobj -> z;
	    channels[cnum].origin = sorigin;
	}
    }
}

/* ------------------------------------------------------------ */
//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound(void)
{
    if (mus_playing && !mus_paused)
    {
	I_PauseSong(mus_playing->handle);
	mus_paused = true;
    }
}

/* ------------------------------------------------------------ */

void S_ResumeSound(void)
{
    if (mus_playing && mus_paused)
    {
	I_ResumeSong(mus_playing->handle);
	mus_paused = false;
    }
}

/* ------------------------------------------------------------ */
//
// Updates music & sounds
//
void S_UpdateSounds(void* listener_p)
{
    int		audible;
    int		cnum;
    int		volume;
    int		sep;
    int		pitch;
    sfxinfo_t*	sfx;
    channel_t*	c;

    mobj_t*	listener = (mobj_t*)listener_p;

    for (cnum=0 ; cnum<numChannels ; cnum++)
    {
	c = &channels[cnum];
	sfx = c->sfxinfo;

	if (c->sfxinfo)
	{
	    if (I_SoundIsPlaying(c->handle))
	    {
		// initialize parameters
		volume = snd_SfxVolume;
		sep = NORM_SEP;

		if (sfx->link)
		{
		    volume += sfx->volume;
		    if (volume < 1)
		    {
			S_StopChannel(cnum);
			continue;
		    }
		    else if (volume > snd_SfxVolume)
		    {
			volume = snd_SfxVolume;
		    }
		    pitch = sfx->pitch;
		    if (pitch < 0)
		    {
			pitch = NORM_PITCH;
		    }
		}
		else
		{
		    pitch = NORM_PITCH;
		}

		// check non-local sounds for distance clipping
		//  or modify their params
		if (c->origin && listener_p != c->origin)
		{
		    audible = S_AdjustSoundParams(listener,
						  c->origin,
						  &volume,
						  &sep,
						  &pitch);

		    if (!audible)
		    {
			S_StopChannel(cnum);
		    }
		    else
		    {
			I_UpdateSoundParams(c, volume, sep, pitch);
		    }
		}
	    }
	    else
	    {
		// if channel is allocated but sound has stopped,
		//  free it
		S_StopChannel(cnum);
	    }
	}
    }
    // kill music if it is a single-play && finished
    // if (mus_playing
    //      && !I_QrySongPlaying(mus_playing->handle)
    //      && !mus_paused )
    // S_StopMusic();

    if ((mus_playing)
     && (I_QrySongPlaying (mus_playing->handle))
     && (!mus_paused))
    {
      I_FillMusBuffer (mus_playing->handle);
    }
}

/* ------------------------------------------------------------ */

void S_SetMusicVolume(int volume)
{
    if (volume < 0 || volume > 127)
    {
	I_Error("Attempt to set music volume at %d",
		volume);
    }

    I_SetMusicVolume(volume);
    snd_MusicVolume = volume;
}

/* ------------------------------------------------------------ */

void S_SetSfxVolume(int volume)
{

    if (volume < 0 || volume > 127)
	I_Error("Attempt to set sfx volume at %d", volume);

    snd_SfxVolume = volume;

}

/* ------------------------------------------------------------ */
//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic (int m_id)
{
    S_ChangeMusic (m_id, false);
}

/* ------------------------------------------------------------ */

void S_ChangeMusic (int musicnum, int looping)
{
  int lumpnum;
  musicinfo_t*	music;
  char		namebuf[12];

  if (nomusic)
    return;

  if ((musicnum <= mus_None)
   || (musicnum >= NUMMUSIC)
   || ((music = &S_music[musicnum])->name == NULL))
  {
    fprintf (stderr, "Bad music number %d\n", musicnum);
    if (gamemode == commercial)
      musicnum = mus_doom2;
    else
      musicnum = mus_doom;
  }

  if (musicnum == mus_extra)
  {
    strcpy (namebuf, music->name);
  }
  else
  {
    if (mus_playing == music)
      return;
    sprintf (namebuf, "d_%s", music->name);
  }

  // printf ("Starting music %s\n", namebuf);

  lumpnum = W_CheckNumForName (namebuf);
  if (lumpnum == -1)
  {
    // shutdown old music
    S_StopMusic ();
    return;
  }

  // printf ("Music %d/%d %X %X %X\n", lumpnum, music->lumpnum,music->data,mus_playing, music);

  if ((lumpnum != music->lumpnum)	// May have changed when using mus_extra
   || (music->data == NULL))
  {
    // shutdown old music
    S_StopMusic ();
    music->data = (void *) W_CacheLumpNum (music->lumpnum = lumpnum, PU_MUSIC);
  }
  else if (mus_playing == music)
  {
    return;
  }
  else
  {
    // shutdown old music
    S_StopMusic ();
  }

  // load & register it
  if ((music->handle = I_RegisterSong (music, namebuf)) < 0)
  {
    Z_ChangeTag (music->data, PU_CACHE);
    music->data = NULL;
  }
  else
  {
    // play it
    I_PlaySong (music->handle, looping);
    mus_playing = music;
  }
}

/* ------------------------------------------------------------ */

void S_StopMusic(void)
{
    if (mus_playing)
    {
	if (mus_paused)
	    I_ResumeSong(mus_playing->handle);

	I_StopSong(mus_playing->handle);
	I_UnRegisterSong(mus_playing->handle);
	mus_playing = 0;
    }
}

/* ------------------------------------------------------------ */

static void S_StopChannel(int cnum)
{

    int		i;
    channel_t*	c = &channels[cnum];

    if (c->sfxinfo)
    {
	// stop the sound playing
	if (I_SoundIsPlaying(c->handle))
	{
#ifdef SAWDEBUG
	    if (c->sfxinfo == &S_sfx[sfx_sawful])
		fprintf(stderr, "stopped\n");
#endif
	    I_StopSound(c->handle);
	}

	// check to see
	//  if other channels are playing the sound
	for (i=0 ; i<numChannels ; i++)
	{
	    if (cnum != i
		&& c->sfxinfo == channels[i].sfxinfo)
	    {
		break;
	    }
	}

	c->sfxinfo = 0;
    }
}

/* ------------------------------------------------------------ */
//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
static int
S_AdjustSoundParams
( mobj_t*	listener,
  mobj_t*	source,
  int*		vol,
  int*		sep,
  int*		pitch )
{
    fixed_t	approx_dist;
    fixed_t	adx;
    fixed_t	ady;

    // calculate the distance to sound origin
    //  and clip it if necessary
    adx = abs(listener->x - source->x);
    ady = abs(listener->y - source->y);

    // From _GG1_ p.428. Appox. eucledian distance fast.
    approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

    // I have no idea why ID don't want sound distance on map 8
    // but I feel that it is a bad idea! JAD 1/5/99
    if (approx_dist > S_CLIPPING_DIST)
    {
	// if (gamemap != 8)
	  return 0;
	// if (modifiedgame)
	//  return 0;
    }

    if (monosfx)
    {
      *sep = NORM_SEP;
    }
    else
    {
      angle_t	angle;

      // angle of source to listener
      angle = R_PointToAngle2(listener->x,
			      listener->y,
			      source->x,
			      source->y);

      if (angle > listener->angle)
	  angle = angle - listener->angle;
      else
	  angle = angle + (0xffffffff - listener->angle);

      angle >>= ANGLETOFINESHIFT;

      // stereo separation
      *sep = NORM_SEP - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);
    }

    // volume calculation
    if (approx_dist < S_CLOSE_DIST)
    {
	*vol = snd_SfxVolume;
    }

    // I have no idea why ID don't want sound distance on map 8
    // but I feel that it is a bad idea! JAD 1/5/99

//    else if ((gamemap == 8) && (modifiedgame == 0))
//    {
//	if (approx_dist > S_CLIPPING_DIST)
//	    approx_dist = S_CLIPPING_DIST;
//
//	*vol = 15+ ((snd_SfxVolume-15)
//		    *((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
//	    / S_ATTENUATOR;
//    }
    else
    {
	// distance effect
	*vol = (snd_SfxVolume
		* ((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
	    / S_ATTENUATOR;
    }

    return (*vol > 0);
}

/* ------------------------------------------------------------ */
//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//
static int S_getChannel (sfxinfo_t* sfxinfo)
{
  // channel number to use
  int cnum;

  // Find an open channel
  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (!channels[cnum].sfxinfo)
      return (cnum);

  // Find a channel that we are not tracking
  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (!channels[cnum].origin)
      return (cnum);

  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].origin == players[consoleplayer].mo)
      return (cnum);

  // None available
  // Look for lower priority
  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
      return (cnum);

  return (nextChannel);
}

/* ------------------------------------------------------------ */

static unsigned int read_16 (byte * ptr)
{
  unsigned int rc;

  rc = *ptr++;
  rc |= (*ptr << 8);
  return (rc);
}

/* ------------------------------------------------------------ */

static unsigned int read_32 (byte * ptr)
{
  unsigned int rc;

  rc = *ptr++;
  rc |= (*ptr++ << 8);
  rc |= (*ptr++ << 16);
  rc |= (*ptr << 24);
  return (rc);
}

/* ------------------------------------------------------------ */

static void write_16 (byte * ptr, unsigned int data)
{
  *ptr++ = (byte) data;
  data >>= 8;
  *ptr = (byte) data;
}

/* ------------------------------------------------------------ */

static void write_32 (byte * ptr, unsigned int data)
{
  *ptr++ = (byte) data;
  data >>= 8;
  *ptr++ = (byte) data;
  data >>= 8;
  *ptr++ = (byte) data;
  data >>= 8;
  *ptr = (byte) data;
}

/* ------------------------------------------------------------ */
/*
   Find the location and size of the sound data in the file.
   Some WAD files use RIFF files.
*/

int S_FindSoundData (sfxinfo_t * sfx)
{
  unsigned int chan;
  unsigned int type;
  unsigned int bits;
  unsigned int bytes;
  unsigned int length;
  unsigned int value;
  unsigned int hlength;
  unsigned int channels;
  byte * data;
  byte * sdata;
  byte * ptr_1;
  byte * ptr_2;

  length = 0;

  sdata = sfx->data;
  type = read_32 (sdata);
  if ((type & 0xFFFF) == 0x0003)			// Standard doom lump
  {
    length = read_32 (sdata + 4);
    data = sdata + 8;
    sfx->samplerate = type >> 16;
  }
  else if (type == 0x46464952)				// RIFF
  {
    /* Four bytes 'Riff'				0x00 */
    /* Four bytes filesize not counting this or prev	0x04 */
    /* Four bytes 'WAVE'				0x08 */
    /* Four bytes 'fmt '				0x0C */
    /* Four bytes size of block				0x10 */
    /* Two bytes Wave type format			0x14 */
    /* Two bytes mono/stereo				0x16 */
    /* Four bytes sample rate				0x18 */
    /* Four bytes bytes per second			0x1C */
    /* Two bytes Block alignment			0x20 */
    /* Two bytes bits per sample			0x22 */

    sfx->samplerate = read_32 (sdata + 0x18);

    hlength = read_32 (sdata + 0x10);
    data = sdata + (hlength + 0x14);

    /* We need to find the 'data' section */

    if (read_32 (data) != 0x61746164)			 // data?
    {
      do
      {
	data += read_32 (data + 4) + 8;
	if (data >= sdata + W_LumpLength (sfx->lumpnum))
	  return (length);				// Failed to find data section.
      } while (read_32 (data) != 0x61746164);

      /* Adjust the offset so that we do not */
      /* have to do this again. */
      write_32 (sdata + 0x10, (data - sdata) - 0x14);
    }

    length = read_32 (data + 4);
    data += 8;

    bits = read_16 (sdata + 0x22);
    if (bits == 0x0010)					// 16 bit data?
    {
      write_16 (sdata + 0x22, bits >> 1 /* 0x0008 */);	// Yes. Convert to 8 bit.
      length >>= 1;
      write_32 (data - 4, length);
      bytes = length;
      ptr_1 = data;
      ptr_2 = ptr_1 + 1;
      do
      {							// 16 bit data is -32768 -> +32767
	*ptr_1++ = *ptr_2 + 0x80;			// 8 bit data is 0 -> 255
	ptr_2 += 2;
      } while (--bytes);
    }

    channels = read_16 (sdata + 0x16);
    if (channels > 1)					// More than 1 channel?
    {
      write_16 (sdata + 0x16, 1);			// Yes. Convert to mono.
      length = length / channels;
      write_32 (data - 4, length);

      bytes = length;
      ptr_1 = data;
      ptr_2 = data;
      do
      {
	value = 0;
	chan = channels;
	do
	{
	  value += *ptr_2++;
	} while (--chan);
	*ptr_1++ = value / channels;
      } while (--bytes);
    }
  }

  sfx -> adata = data;
  sfx -> length = length;
  return (length);
}

/* ------------------------------------------------------------ */

static void Parse_Musinfo (char * ptr, char * top)
{
  char cc;
  int map;
  int episode;
  int musnum;
  muschange_t * musc_ptr;

  top = dh_split_lines (ptr, top);
  episode = 255;
  map = 0;

  do
  {
    while (((cc = *ptr) <= ' ') || (cc == '{')) ptr++;

    //printf ("Parse_Mapinfo (%s)\n", ptr);

    if (strncasecmp (ptr, "MAP", 3) == 0)
    {
      ptr += 3;
      while (*ptr == ' ') ptr++;
      map = atoi (ptr);
    }
    else if (((cc = *ptr) == 'E') || (cc == 'e'))
    {
      episode = ptr [1] - '0';
      map = ptr [3] - '0';
    }
    else if ((cc >= '0') && (cc <= '9'))
    {
      musnum = atoi (ptr);
      while (*ptr != ' ') ptr++;
      while (*ptr == ' ') ptr++;
      if (M_CheckParm ("-showmusinfo"))
	printf ("Music %d for episode %d, map %d is '%s'\n", musnum, episode, map, ptr);
      musc_ptr = malloc (sizeof (*musc_ptr));
      if (musc_ptr)
      {
	musc_ptr->episode = episode;
	musc_ptr->map = map;
	musc_ptr->musnum = musnum;
	strncpy (musc_ptr->music, ptr, sizeof (musc_ptr->music));
	musc_ptr->next = muschangeinfo.head;
	muschangeinfo.head = musc_ptr;
      }
    }

    ptr = dh_next_line (ptr,top);
  } while (ptr < top);
}

/* ------------------------------------------------------------ */

static void S_ReadMUSINFO (void)
{
  int lump;
  char * ptr;
  char * top;

  memset (&muschangeinfo, 0, sizeof (muschangeinfo));

  lump = W_CheckNumForName ("MUSINFO");
  if (lump != -1)
  {
    ptr = malloc (W_LumpLength (lump) + 4); 	// Allow extra because some mapinfos lack a trailing CR/LF
    if (ptr)
    {
      W_ReadLump (lump, ptr);
      top = ptr + W_LumpLength (lump);
      *top++ = '\n';
      Parse_Musinfo (ptr, top);
      free (ptr);
    }
  }
}

/* ------------------------------------------------------------ */

void S_MusInfoThinker (mobj_t *thing)
{
  int tics;
  int mnum;
  int episode;
  muschange_t * musc_ptr;

  /* Is this music changer in the players sector? */
  if (thing->subsector->sector == players[displayplayer].mo->subsector->sector)
  {
    muschangeinfo.mapthing = thing;		// Yes.
    mnum = thing->spawnpoint.type;
    // printf ("Player has entered sector with music changer %d\n", mnum);
    if (muschangeinfo.musnum != mnum)		// Different from the last one?
    {
      muschangeinfo.musnum = mnum;
      muschangeinfo.gametic = gametic;
      muschangeinfo.tics = TICRATE;
      // printf ("Start music change to %d at gametic %d\n", mnum, gametic);
      return;
    }
  }

  if (((tics = muschangeinfo.tics) == 0)
   || (muschangeinfo.gametic == gametic))
    return;

  if ((muschangeinfo.tics = tics - 1) != 0)
  {
    muschangeinfo.gametic = gametic;
    return;
  }

  mnum = muschangeinfo.musnum - 14100;
  musc_ptr = muschangeinfo.head;
  if (musc_ptr)
  {
    if (gamemode == commercial)
      episode = 255;
    else
      episode = gameepisode;

    do
    {
      if ((musc_ptr->episode == episode)
       && (musc_ptr->map == gamemap)
       && (musc_ptr->musnum == mnum))
      {
	S_music[mus_extra].name = musc_ptr->music;
	// printf ("Music change to %d at gametic %d\n", mnum, gametic);
	S_ChangeMusic (mus_extra, true);
	return;
      }

      musc_ptr = musc_ptr -> next;
    } while (musc_ptr);
  }

  if (mnum == 0)			// Return to default?
    S_StartLevelMusic (gameepisode, gamemap);
}

/* ------------------------------------------------------------ */
