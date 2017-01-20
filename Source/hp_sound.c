/* Hewlett-Packard sound driver.
   Written by J.A.Doggett.
   Would probably work for any unix system that
   uses /dev/audio

   This file is based on i_sound.c and hence
   most of the comments are ID's.
*/

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"

#define MIN_SFX_CHAN		   1
#define MAX_SFX_CHAN		   8

unsigned int current_play_id [MAX_SFX_CHAN + 1];
unsigned int ch_length [MAX_SFX_CHAN + 1];
unsigned int ch_volume [MAX_SFX_CHAN + 1];
unsigned char * play_ptr  [MAX_SFX_CHAN + 1];
FILE * audio_file;

#define AUDIO_DEV "/dev/audioIL"


char * sndserver_filename;   /* Doom seems to want this */
static int last_snd_channel = MIN_SFX_CHAN - 1;

/* ------------------------------------------------------------ */

//
// SFX API
// Note: this was called by S_Init.
// However, whatever they did in the
// old DPMS based DOS version, this
// were simply dummies in the Linux
// version.
// See soundserver initdata().
//
void I_SetChannels()
{
}

/* ------------------------------------------------------------ */


void I_SetSfxVolume(int volume)
{
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  snd_SfxVolume = volume;
}

/* ------------------------------------------------------------ */

// MUSIC API - dummy. Some code from DOS version.
void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  // Now set volume on output device.
  // Whatever( snd_MusciVolume );
}

/* ------------------------------------------------------------ */

static unsigned int read_32 (unsigned char * ptr)
{
  unsigned int rc;

  rc = *ptr++;
  rc |= (*ptr++ << 8);
  rc |= (*ptr++ << 16);
  rc |= (*ptr << 24);
  return (rc);
}

/* ------------------------------------------------------------ */

static void write_32 (unsigned char * ptr, unsigned int data)
{
  *ptr++ = (unsigned char) data;
  data >>= 8;
  *ptr++ = (unsigned char) data;
  data >>= 8;
  *ptr++ = (unsigned char) data;
  data >>= 8;
  *ptr = (unsigned char) data;
}

/* ------------------------------------------------------------ */
//#define SAVE_SOUND_LUMP

#ifdef SAVE_SOUND_LUMP

static void I_SaveSoundLump (char * filename, unsigned char * data, unsigned int size)
{
  FILE * fout;

  fout = fopen (filename, "wb");
  if (fout)
  {
    fwrite (data, 1, size, fout);
    fclose (fout);
  }
}

#endif

/* ------------------------------------------------------------ */
//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
int
I_StartSound
( int           id,
  int           vol,
  int           sep,
  int           pitch)
{
  sfxinfo_t*	sfx;
  unsigned int channel;
  unsigned int cs_id;
  unsigned int qty;

  if (audio_file == 0)
    return (0);

  cs_id = -1;
  sfx = &S_sfx[id];

  // Chainsaw troubles.
  // Play these sound effects only one at a time.
  if ( id == sfx_sawup
	 || id == sfx_sawidl
	 || id == sfx_sawful
	 || id == sfx_sawhit
	 || id == sfx_stnmov
	 || id == sfx_pistol )
  {
    cs_id = id;
  }

  /* Look for an unused sound channel. If there isn't one
  ** then just use the one after last_snd_channel */

  channel = last_snd_channel + 1;
  if (channel > MAX_SFX_CHAN)
    channel = MIN_SFX_CHAN;

  qty = (MAX_SFX_CHAN - MIN_SFX_CHAN) + 1;
  do
  {
    if ((ch_length[channel] == 0)
     || (cs_id == current_play_id [channel]))
      break;
    if (++channel > MAX_SFX_CHAN)
      channel = MIN_SFX_CHAN;
  } while (--qty);			// At exit channel will have incremented
					// back round to the start again.

  // printf ("Playing sound %s\n", sfx->name);
  // printf ("Using sound channel %d\n", channel);

  last_snd_channel = channel;
  current_play_id [channel] = id;


  if (sep < 1) sep = 1;
  if (sep > 255) sep = 255;

  if (vol < 0) vol = 0;
  if (vol > 15) vol = 15;
  if (vol == 0) return (0);

  current_play_id [channel] = id;
  ch_volume [channel] = vol;

#if 1

  if ((sfx->adata == NULL)
   && (S_FindSoundData (sfx) == 0))
    return (channel);

  play_ptr [channel]  = sfx->adata;
  ch_length [channel] = sfx->length;
#else
  ch_length [channel] = W_LumpLength (sfx->lumpnum);
  play_ptr  [channel] = sfx->data;
#endif

  // fprintf(stderr, "%s %X %X %d %d\n", sfx->name, sfx->data, ch_length[channel], vol, sep);
  I_SubmitSound();
  return (channel);
}

/* ------------------------------------------------------------ */

void I_StopSound (int channel)
{
  // You need the handle returned by StartSound.
  // Would be looping all channels,
  //  tracking down the handle,
  //  an setting the channel to zero.

  current_play_id [channel] = 0;
  ch_length [channel] = 0;
  ch_volume [channel] = 0;
  play_ptr  [channel] = 0;
}

/* ------------------------------------------------------------ */

int I_SoundIsPlaying(int handle)
{
// Sounds are worse with this in!!!!
  return (0);
}

/* ------------------------------------------------------------ */

//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
// This function currently supports only 16bit.
//
void I_UpdateSound( void )
{
}

/* ------------------------------------------------------------ */

//
// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime.
// It is called during Timer interrupt with SNDINTR.
// Mixing now done synchronous, and
//  only output be done asynchronous?
//
void
I_SubmitSound(void)
{
  // Write it to DSP device.

  unsigned int channel;
  unsigned int bytes_to_fill;
  unsigned char max_found;
  unsigned char val;
  unsigned char * p;
  unsigned char one_found;

  bytes_to_fill = 2000;

  do
  {
    channel = MIN_SFX_CHAN - 1;
    max_found = 0;
    one_found = 0;
    do
    {
      channel++;
      if (ch_length [channel])
      {
	one_found = 1;
        p = play_ptr [channel];
	val = (*p++ - 128) & 0xFF;
	play_ptr [channel] = p;
        ch_length [channel]--;

        val = (val * ch_volume [channel]) / 15;

	if (val > max_found)
	  max_found = val;
      }
    } while (channel < MAX_SFX_CHAN);


    if (one_found)
    {
      fputc (max_found & 0xFF, audio_file);
      fputc (max_found >> 8, audio_file);
    }
    else
    {
      bytes_to_fill = 1;
    }


  } while (--bytes_to_fill);
}

/* ------------------------------------------------------------ */

void
I_UpdateSoundParams
( channel_t* c,
  int   vol,
  int   sep,
  int   pitch)
{
  // I fail too see that this is used.
  // Would be using the handle to identify
  //  on which channel the sound might be active,
  //  and resetting the channel parameters.

  // UNUSED.
}

/* ------------------------------------------------------------ */

void I_ShutdownSound(void)
{
  fclose (audio_file);
  audio_file = 0;
}

/* ------------------------------------------------------------ */

void
I_InitSound()
{
  unsigned int channel;

  channel = MIN_SFX_CHAN - 1;
  do
  {
    channel++;
    current_play_id [channel] = 0;
    ch_length [channel] = 0;
    ch_volume [channel] = 0;
    play_ptr  [channel] = 0;
  } while (channel <= MAX_SFX_CHAN);

  audio_file = fopen (AUDIO_DEV, "wb");
}

/* ------------------------------------------------------------ */

//
// MUSIC API.
// Still no music done.
// Remains. Dummies.
//
void I_InitMusic(void)          { }
void I_ShutdownMusic(void)      { }


void I_PlaySong(int handle, int looping)
{
}

void I_PauseSong (int handle)
{
}

void I_ResumeSong (int handle)
{
}

void I_StopSong(int handle)
{
}

void I_UnRegisterSong(int handle)
{
}

int I_RegisterSong(musicinfo_t* music, const char * lumpname)
{
  return -1;
}

/* Keep scheduler buffer full */
void I_FillMusBuffer(int handle)
{
  // UNUSED.
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
  return 0;
}



// Interrupt handler.
void I_HandleSoundTimer( int ignore )
{
}

// Get the interrupt. Set duration in millisecs.
int I_SoundSetTimer( int duration_of_tick )
{
  return 0;
}


// Remove the interrupt. Set duration to zero.
void I_SoundDelTimer()
{
}

/* ------------------------------------------------------------ */
