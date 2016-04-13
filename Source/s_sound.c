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
#define NORM_PRIORITY		64
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
  int		i;

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

  // Note that sounds have not been cached (yet).
  for (i=1 ; i<NUMSFX ; i++)
    S_sfx[i].lumpnum = S_sfx[i].usefulness = -1;
}

/* ------------------------------------------------------------ */

static const unsigned int spmus[]=
{
  // Song - Who? - Where?

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

static void S_StartLevelMusic (void)
{
  int mnum;
  unsigned int gm;
  char * music;
  map_dests_t * map_ptr;

  // start new music for the level
  mus_paused = false;

  map_ptr = G_Access_MapInfoTab (gameepisode, gamemap);
  music = map_ptr -> music;
  if (music)
  {
    S_music[mus_extra].name = music;
    mnum = mus_extra;
  }
  else if (gamemode == commercial)
  {
    gm = gamemap;
    while (gm > 32)
      gm -= 32;
    mnum = mus_runnin + gm - 1;
  }
  else if (gameepisode < 4)
  {
    mnum = mus_e1m1 + (gameepisode-1)*9 + gamemap-1;
  }
  else
  {
    gm = gamemap-1;
    while (gm >= ARRAY_SIZE (spmus))
      gm -= ARRAY_SIZE (spmus);
    mnum = spmus[gm];
  }

  // HACK FOR COMMERCIAL
  //  if (commercial && mnum > mus_e3m9)
  //      mnum -= mus_e3m9;

  S_ChangeMusic (mnum, true);

//nextcleanup = 15;
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

  S_StartLevelMusic ();
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

static void S_StartSoundAtVolume (void* origin_p, int sfx_id, int volume)
{
  int		rc;
  int		sep;
  int		pitch;
  int		priority;
  sfxinfo_t*	sfx;
  channel_t*	c;
  int		cnum;
  int		next;
  int		lumpnum;
  mobj_t*	origin = (mobj_t *) origin_p;


  // Debug.
  /* fprintf( stderr,
  	   "S_StartSoundAtVolume: playing sound %d (%s)\n",
  	   sfx_id, S_sfx[sfx_id].name );*/

  if (nosfx)
    return;

  // check for bogus sound #
  if (sfx_id < 1 || sfx_id > NUMSFX)
  {
    fprintf (stderr, "Bad sfx #: %d\n", sfx_id);
    return;
  }

  sfx = &S_sfx[sfx_id];

  // Initialize sound parameters
  if (sfx->link)
  {
    pitch = sfx->pitch;
    priority = sfx->priority;
    volume += sfx->volume;

    if (volume < 1)
      return;

    if (volume > snd_SfxVolume)
      volume = snd_SfxVolume;
  }
  else
  {
    pitch = NORM_PITCH;
    priority = NORM_PRIORITY;
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

    if ( origin->x == players[consoleplayer].mo->x
	 && origin->y == players[consoleplayer].mo->y)
    {
      sep = NORM_SEP;
    }

    if (!rc)
      return;
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


  //
  // This is supposed to handle the loading/caching.
  // For some odd reason, the caching is done nearly
  //  each time the sound is needed?
  //

  // get lumpnum if necessary
  if ((lumpnum = sfx->lumpnum) < 0)
  {
    lumpnum = S_GetSfxLumpNum (sfx);
    if (lumpnum < 0)
    {
      /* If the wad does not have the DSSECRET lump */
      /* then we use GETPOW instead. */

      lumpnum = S_GetSfxLumpNum (&S_sfx[sfx_getpow]);
      if (lumpnum < 0)
	return;
    }
    sfx->lumpnum = lumpnum;
  }

#ifndef SNDSRV
  // cache data if necessary
  if (!sfx->data)
  {
    // fprintf( stderr,  "S_StartSoundAtVolume: 16bit and not pre-cached - wtf?\n");

    // DOS remains, 8bit handling
    sfx->data = (void *) W_CacheLumpNum (lumpnum, PU_SOUND);
    // fprintf( stderr,
    //	     "S_StartSoundAtVolume: loading %d (lump %d) : 0x%x\n",
    //       sfx_id, lumpnum, (int)sfx->data );

  }
#endif

  // increase the usefulness
  if (sfx->usefulness++ < 0)
    sfx->usefulness = 1;

  // Assigns the handle to one of the channels in the
  //  mix/output buffer.
  c->handle = I_StartSound(sfx_id,
				       /*sfx->data,*/
				       volume,
				       sep,
				       pitch,
				       priority);
}

/* ------------------------------------------------------------ */

void
S_StartSound
( void*		origin,
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

void S_StopSound(void *origin)
{
    int cnum;

    for (cnum=0 ; cnum<numChannels ; cnum++)
    {
	if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
	{
	    S_StopChannel(cnum);
	}
    }
}

/* ------------------------------------------------------------ */

void S_RemoveSoundOrigin (void *origin)
{
    int cnum;
    mobj_t* mobj;
    degenmobj_t * sorigin;

    for (cnum=0 ; cnum<numChannels ; cnum++)
    {
	if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
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



    // Clean up unused data.
    // This is currently not done for 16bit (sounds cached static).
    // DOS 8bit remains.
    /*if (gametic > nextcleanup)
    {
	for (i=1 ; i<NUMSFX ; i++)
	{
	    if (S_sfx[i].usefulness < 1
		&& S_sfx[i].usefulness > -1)
	    {
		if (--S_sfx[i].usefulness == -1)
		{
		    Z_ChangeTag(S_sfx[i].data, PU_CACHE);
		    S_sfx[i].data = 0;
		}
	    }
	}
	nextcleanup = gametic + 15;
    }*/

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
		pitch = NORM_PITCH;
		sep = NORM_SEP;

		if (sfx->link)
		{
		    pitch = sfx->pitch;
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
			I_UpdateSoundParams(c, volume, sep, pitch);
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

//printf ("Starting music %s\n", namebuf);

  lumpnum = W_CheckNumForName (namebuf);
  if (lumpnum == -1)
  {
    // shutdown old music
    S_StopMusic ();
    return;
  }

  if ((lumpnum != music->lumpnum)	// May have changed when using mus_extra
   || (music->data == NULL))
  {
    music->lumpnum = lumpnum;
    music->data = (void *) W_CacheLumpNum (music->lumpnum, PU_MUSIC);
  }
  else if (mus_playing == music)
  {
    return;
  }

  // shutdown old music
  S_StopMusic ();

  // load & register it
  music->handle = I_RegisterSong (music);

  // play it
  I_PlaySong (music->handle, looping);

  mus_playing = music;
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
	Z_ChangeTag(mus_playing->data, PU_CACHE);

	mus_playing->data = 0;
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

	// degrade usefulness of sound data
	c->sfxinfo->usefulness--;

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
    angle_t	angle;

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
    *sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

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

#if 0
  if (thing->subsector->sector == players[displayplayer].mo->subsector->sector)
    printf ("Player has entered sector with music changer %d\n", thing->spawnpoint.type);
#endif

  if ((muschangeinfo.mapthing != thing)
   && (thing->subsector->sector == players[displayplayer].mo->subsector->sector))
  {
    muschangeinfo.mapthing = thing;
    muschangeinfo.tics = 30;
    return;
  }

  if (((tics = muschangeinfo.tics) == 0)
   || ((muschangeinfo.tics = tics - 1) != 0))
    return;

  mnum = thing->spawnpoint.type - 14100;
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
	S_ChangeMusic (mus_extra, true);
	return;
      }

      musc_ptr = musc_ptr -> next;
    } while (musc_ptr);
  }

  if (mnum == 0)			// Return to default?
    S_StartLevelMusic ();
}

/* ------------------------------------------------------------ */
