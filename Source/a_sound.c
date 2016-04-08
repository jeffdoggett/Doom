/* Acorn sound driver.
   Written by J.A.Doggett.
   Uses the DataVox sound player module which
   plays the sound under interrupt control and
   requires hardly any effort from me!

   This file is based on i_sound.c and hence
   most of the comments are ID's.
*/

#include "includes.h"
#include <kernel.h>

// #define DEBUG_MUSIC

#define OS_Module			0x1E
#define OS_SWINumberFromString		0x39
#define OS_ReadMonotonicTime		0x42


#define Sound_Configure			0x40140
#define Sound_Stereo			0x40142
#define Sound_AttachVoice		0x40185
#define Sound_Control			0x40189
#define Sound_AttachNamedVoice		0x4018A


#define DataVox_Type			0x44380
#define DataVox_Timed			0x44381
#define DataVox_Pitch			0x44382
#define DataVox_ReadPitch		0x44383
#define DataVox_ReadTimer		0x44384
#define DataVox_ReadType		0x44385
#define DataVox_SetMemory		0x44386
#define DataVox_ReadMemory		0x44387
#define DataVox_ReadAddress		0x44388
#define DataVox_SetRepeat		0x44389
#define DataVox_ReadRepeat		0x4438A
#define DataVox_SetReverse		0x4438B
#define DataVox_ReadReverse		0x4438C
#define DataVox_PitchToSample		0x4438D
#define DataVox_SampleToPitch		0x4438E
#define DataVox_Duplicate		0x4438F
#define DataVox_Unset			0x44390
#define DataVox_ConvertByte		0x44391
#define DataVox_ReadBufferFlag		0x44392
#define DataVox_AllocateChannel		0x44393
#define DataVox_DeAllocateChannel	0x44394
#define DataVox_RequestChannel		0x44395
#define DataVox_ChannelsFree		0x44396
#define DataVox_ConvertArea		0x44397
#define DataVox_ChannelFreeMap		0x44398
#define DataVox_UpCallHandler		0x44399
#define DataVox_FlushKeys		0x4439A
#define DataVox_VoiceActive		0x4439B
#define DataVox_SystemSpeed		0x4439C
#define DataVox_Version			0x4439D
#define DataVox_SlaveChannel		0x4439E
#define DataVox_ReadMaster		0x4439F
#define DataVox_ReadUpCallStatus	0x443A0
#define DataVox_SetUpCallStatus		0x443A1
#define DataVox_AdjustMemory		0x443A2
#define DataVox_SetInternalPitch	0x443A3


#define MIDI_SoundEnable		0x404C0
#define MIDI_SetMode			0x404C1
#define MIDI_SetTxChannel		0x404C2
#define MIDI_SetTxActiveSensing		0x404C3
#define MIDI_InqSongPositionPointer	0x404C4
#define MIDI_InqBufferSize		0x404C5
#define MIDI_InqError			0x404C6
#define MIDI_RxByte			0x404C7
#define MIDI_RxCommand			0x404C8
#define MIDI_TxByte			0x404C9
#define MIDI_TxCommand			0x404CA
#define MIDI_TxNoteOff			0x404CB
#define MIDI_TxNoteOn			0x404CC
#define MIDI_TxPolyKeyPressure		0x404CD
#define MIDI_TxControlChange		0x404CE
#define MIDI_TxLocalControl		0x404CF
#define MIDI_TxAllNotesOff		0x404D0
#define MIDI_TxOmniModeOff		0x404D1
#define MIDI_TxOmniModeOn		0x404D2
#define MIDI_TxMonoModeOn		0x404D3
#define MIDI_TxPolyModeOn		0x404D4
#define MIDI_TxProgramChange		0x404D5
#define MIDI_TxChannelPressure		0x404D6
#define MIDI_TxPitchWheel		0x404D7
#define MIDI_TxSongPositionPointer	0x404D8
#define MIDI_TxSongSelect		0x404D9
#define MIDI_TxTuneRequest		0x404DA
#define MIDI_TxStart			0x404DB
#define MIDI_TxContinue			0x404DC
#define MIDI_TxStop			0x404DD
#define MIDI_TxSystemReset		0x404DE
#define MIDI_IgnoreTiming		0x404DF
#define MIDI_SynchSoundScheduler	0x404E0
#define MIDI_FastClock			0x404E1
#define MIDI_Init			0x404E2
#define MIDI_SetBufferSize		0x404E3
#define MIDI_Interface			0x404E4
#define MIDI_InstallDriver		0x404E5
#define MIDI_PortMapRx			0x404E6
#define MIDI_PortMapTx			0x404E7
#define MIDI_DriverInfo			0x404E8
#define MIDI_ClockStamp			0x404E9
#define MIDI_Thru			0x404EA


#define QTM_Load			0x47E40
#define QTM_Start			0x47E41
#define QTM_Stop			0x47E42
#define QTM_Pause			0x47E43
#define QTM_Clear			0x47E44
#define QTM_Info			0x47E45
#define QTM_Pos				0x47E46
#define QTM_EffectControl		0x47E47
#define QTM_Volume			0x47E48
#define QTM_SetSampleSpeed		0x47E49
#define QTM_DMABuffer			0x47E4A
#define QTM_RemoveChannel		0x47E4B
#define QTM_RestoreChannel		0x47E4C
#define QTM_Stereo			0x47E4D
#define QTM_ReadSongLength		0x47E4E
#define QTM_ReadSequenceTable		0x47E4F
#define QTM_VUBarControl		0x47E50
#define QTM_ReadVULevels		0x47E51
#define QTM_ReadSampleTable		0x47E52
#define QTM_ReadSpeed			0x47E53
#define QTM_PlaySample			0x47E54
#define QTM_SongStatus			0x47E55
#define QTM_ReadPlayingTime		0x47E56
#define QTM_PlayRawSample		0x47E57
#define QTM_SoundControl		0x47E58
#define QTM_SWITableAddress		0x47E59
#define QTM_RegisterSample		0x47E5A
#define QTM_SetSpeed			0x47E5B
#define QTM_MusicVolume			0x47E5C
#define QTM_SampleVolume		0x47E5D
#define QTM_MusicOptions		0x47E5E
#define QTM_MusicInterrupt		0x47E5F
#define QTM_ReadChannelData		0x47E60
#define QTM_ReadNoteWord		0x47E61


#define TimPlayer_Version		0x51380
#define TimPlayer_Configure		0x51381
#define TimPlayer_SongLoad		0x51382
#define TimPlayer_SongUnload		0x51383
#define TimPlayer_SongNew		0x51384
#define TimPlayer_SongLoad2		0x51385
#define TimPlayer_SongDecompress	0x51386
#define TimPlayer_SongPlay		0x51388
#define TimPlayer_SongPause		0x51389
#define TimPlayer_SongStop		0x5138A
#define TimPlayer_SongPosition		0x5138B
#define TimPlayer_SongVolume		0x5138C
#define TimPlayer_SongStatus		0x5138D
#define TimPlayer_SongConfigure		0x5138E
#define TimPlayer_SongInfo		0x51390
#define TimPlayer_SongTexts		0x51391
#define TimPlayer_SongInitialSettings	0x51392
#define TimPlayer_ChannelInitialSettings 0x51393


#define AMPlayer_Play			0x52E00
#define AMPlayer_Stop			0x52E01
#define AMPlayer_Pause			0x52E02
#define AMPlayer_Locate			0x52E03
#define AMPlayer_Info			0x52E04
#define AMPlayer_Control		0x52E05
#define AMPlayer_Plugin			0x52E06
#define AMPlayer_FileInfo		0x52E07
#define AMPlayer_StreamOpen		0x52E08
#define AMPlayer_StreamClose		0x52E09
#define AMPlayer_StreamGiveData		0x52E0A
#define AMPlayer_StreamInfo		0x52E0B
#define AMPlayer_MetaDataPollChange	0x52E0C
#define AMPlayer_MetaDataLookup		0x52E0D
#define AMPlayer_SoundSystem		0x52E0E
#define AMPlayer_StreamReadData		0x52E0F
#define AMPlayer_Instance		0x52E10


#define MIN_SFX_CHAN		   1
#define MAX_SFX_CHAN		   8

#define MUS_TEMP_FILE	"<Wimp$Scrap>"

/* ------------------------------------------------------------ */

static unsigned int current_play_id [MAX_SFX_CHAN + 1];
static int prev_snd_config [6];
static int prev_snd_voice [MAX_SFX_CHAN + 1];
static int last_snd_channel = MIN_SFX_CHAN - 1;

extern char * music_names_copy [];

/* ------------------------------------------------------------ */

/* Info about music playing */
static const char midimap[16]={0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,9};
static unsigned int music_available = 0;/* true if MIDI initialised ok			*/
static byte*	music_data=NULL;	/* Pointer to score if registered, else 0	*/
static byte*	music_pos=NULL;		/* Current position if playing, else 0	 	*/
static int	music_loop;		/* Loop flag if playing				*/
static int 	music_time;		/* Current music time if playing		*/
static int 	music_vel[16];		/* Previous velocity on midichannel		*/
static int	music_pause=0;		/* Time that music was paused, or 0 if it isn't	*/
static int 	music_midivol;		/* Main volume (-127..0)			*/
static int	music_qtmvol;
static int	music_timvol;
static int	music_ampvol;
static int	qtm_channels_swiped = 0;/* Number of sound channels used by qtm		*/
static int	timplayer_handle = 0;
int		timplayer_vol_tab [] =
{
  0x00,0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,
  0x20,0x24,0x28,0x2C,0x30,0x34,0x38,0x3C
};

#define MIDI_AVAILABLE	0x01
#define TIM_LOADED	0x02
#define TIM_NOT_AVAIL	0x04
#define TIM_PLAYING	0x08
#define QTM_PLAYING	0x10
#define AMP_PLAYING	0x20

/* ------------------------------------------------------------ */

int mp3priority = 1;

typedef struct mus_dir_s
{
  char musname [12];
  char filename [100];
  struct mus_dir_s * next;
} mus_dir_t;

static mus_dir_t * music_directory_head;
static mus_dir_t * amp_current;

static const mus_dir_t wimp_temp =
{
  "fred",
  MUS_TEMP_FILE,
  NULL
};

/* ------------------------------------------------------------ */

static _kernel_oserror * RmLoad_Module (const char * filepath)
{
  _kernel_oserror * rc;
  _kernel_swi_regs regs;

  regs.r[0] = 1;
  regs.r[1] = (int) filepath;
  rc = _kernel_swi (OS_Module, &regs, &regs);
  if (rc)
    printf ("RmLoad: %s Error (%s)\n", filepath, rc -> errmess);
  return (rc);
}

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
void I_SetChannels(void)
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
/*
   Find the location and size of the sound data in the file.
   Some WAD files use RIFF files.
*/

static void I_FindData (unsigned int * start, unsigned int * end, unsigned int * data, unsigned int length)
{
  unsigned int type;
  unsigned int newlength;
  unsigned char * ptr_1;
  unsigned char * ptr_2;

  type = read_32 ((unsigned char *) data);
  if ((type & 0xFFFF) == 0x0003)			// Standard doom lump
  {
    data++;
    length -= 8;
    newlength = read_32 ((unsigned char *) data);
    data++;
    if (newlength < length)
      length = newlength;
  }
  else if (type == 0x46464952)				// RIFF
  {
    data += 8;
    type = read_32 ((unsigned char *) data);
    if (type == 0x00100002)				// 16 bit data?
    {
      write_32 ((unsigned char *) data, 0x00080001);	// Yes. Convert to 8 bit.
      data += 2;
      newlength = read_32 ((unsigned char *) data);
      newlength >>= 1;
      write_32 ((unsigned char *) data, newlength);
      ptr_1 = ((unsigned char *) data) + 4;
      ptr_2 = ptr_1 + 1;
      do
      {
	*ptr_1++ = *ptr_2 + 0x80;
	ptr_2 += 2;
      } while (--newlength);
    }
    else
    {
      data += 2;
      length -= (11 * 4);
    }
    newlength = read_32 ((unsigned char *) data);
    data++;
    if (newlength < length)
      length = newlength;
  }

  *start = (int)data;
  *end = (int)(((unsigned char *) data) + length);
}

/* ------------------------------------------------------------ */

static int I_Save_MusFile (const char * filename, void * start, unsigned int length)
{
  int rc;
  _kernel_osfile_block block;

  block.load = 0x1AD;	// MP3 file
  block.exec = 0;
  block.start = (int) start;
  block.end = (int) start + length;
  rc = _kernel_osfile (10, filename, &block);
  return (rc - 10);		// Returns with r0 preserved if no error.
}

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
( int	   id,
  int	   vol,
  int	   sep,
  int	   pitch,
  int	   priority )
{
  sfxinfo_t*	sfx;
  unsigned int length;
  unsigned int channel;
  unsigned int qty;
  unsigned int max_sfx_chan;
  _kernel_swi_regs regs;

  /* Look for an unused sound channel. If there isn't one
  ** then just use the one after last_snd_channel */

  max_sfx_chan = MAX_SFX_CHAN;
  if (music_available & QTM_PLAYING)		// Qtm swipes 4 channels!
    max_sfx_chan = MAX_SFX_CHAN - qtm_channels_swiped;

  channel = ~0;

  // Chainsaw troubles.
  // Play these sound effects only one at a time.
  if ((id == sfx_sawup)
   || (id == sfx_sawidl)
   || (id == sfx_sawful)
   || (id == sfx_sawhit)
   || (id == sfx_stnmov)
   || (id == sfx_pistol))
  {
    channel = MIN_SFX_CHAN;
    while ((id != current_play_id [channel]) && (++channel <= max_sfx_chan));
  }

  if (channel > max_sfx_chan)
  {
    channel = last_snd_channel + 1;
    if (channel > max_sfx_chan)
      channel = MIN_SFX_CHAN;
    qty = (max_sfx_chan - MIN_SFX_CHAN) + 1;
    do
    {
      regs.r[0] = channel;
      _kernel_swi (DataVox_ReadAddress, &regs, &regs);
      // printf ("Channel %u = %X\n", channel, regs.r[1]);
      if (regs.r[1] == 0)
	break;
      if (++channel > max_sfx_chan)
	channel = MIN_SFX_CHAN;
    } while (--qty);			// At exit channel will have incremented
  }					// back round to the start again.

  last_snd_channel = channel;
  current_play_id [channel] = id;
  sfx = &S_sfx[id];

  // printf ("Playing sound %s\n", sfx->name);
  // printf ("Using sound channel %d\n", channel);

  regs.r[0] = channel;
  length = W_LumpLength (sfx->lumpnum);
  I_FindData ((unsigned int*)&regs.r[1], (unsigned int*)&regs.r[2], sfx->data, length);
  // R1 = Start of data
  // R2 = End of data
  _kernel_swi (DataVox_SetMemory, &regs, &regs);

#if 0
  I_Save_MusFile (sfx->name, sfx->data, regs.r[2] - (unsigned int) sfx->data);
#endif

  regs.r[0] = channel;
  regs.r[1] = 1;
  _kernel_swi (DataVox_Type, &regs, &regs);

  regs.r[0] = channel;
  regs.r[1] = 0;
  _kernel_swi (DataVox_Timed, &regs, &regs);

  if (pitch > 255) pitch = 255;
  if (pitch < 0)   pitch = 0;

  regs.r[0] = channel;
  regs.r[1] = pitch + 0x1580;  /* Seems about right to me.... */
  _kernel_swi (DataVox_Pitch, &regs, &regs);

  if (sep < 1) sep = 1;
  if (sep > 255) sep = 255;

  regs.r[0] = channel;
  regs.r[1] = sep - 128;
  _kernel_swi (Sound_Stereo, &regs, &regs);

  if (vol < 0) vol = 0;
  if (vol > 15) vol = 15;

  regs.r[0] = channel;
  regs.r[1] = -vol;
  regs.r[2] = 1;
  regs.r[3] = 1;
  _kernel_swi (Sound_Control, &regs, &regs);	// Start sound playing

  // fprintf(stderr, "%s %X %X %d %d\n", sfx->name, sfx->data, length, vol, sep);
  return (channel);
}

/* ------------------------------------------------------------ */

void I_StopSound (int handle)
{
  // You need the handle returned by StartSound.
  // Would be looping all channels,
  //  tracking down the handle,
  //  an setting the channel to zero.
 _kernel_swi_regs regs;

 regs.r[0] = handle;
 _kernel_swi (DataVox_Unset, &regs, &regs);

}

/* ------------------------------------------------------------ */

int I_SoundIsPlaying (int handle)
{
  _kernel_swi_regs regs;

  regs.r[0] = handle;
  _kernel_swi (DataVox_ReadAddress, &regs, &regs);
  return (regs.r[1]);
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
}

/* ------------------------------------------------------------ */

void
I_UpdateSoundParams
( int   channel,
  int   vol,
  int   sep,
  int   pitch)
{
  _kernel_swi_regs regs;

  if (pitch > 255) pitch = 255;
  if (pitch < 0)   pitch = 0;

  regs.r[0] = channel;
  regs.r[1] = pitch + 0x1580;  /* Seems about right to me.... */
  _kernel_swi (DataVox_Pitch, &regs, &regs);

  if (sep < 1) sep = 1;
  if (sep > 255) sep = 255;

  regs.r[0] = channel;
  regs.r[1] = sep - 128;
  _kernel_swi (Sound_Stereo, &regs, &regs);

  /* Cannot find a call in RiscOS to update the volume of */
  /* an already playing sound.... */
}

/* ------------------------------------------------------------ */

void I_ShutdownSound(void)
{
  _kernel_swi_regs regs;
  unsigned int channel;
  unsigned int time_started;
  unsigned int time_now;


  _kernel_swi (OS_ReadMonotonicTime, &regs, &regs);
  time_started = regs.r [0];

  channel = MIN_SFX_CHAN;

  do
  {
    do
    {
      /* Let's wait a while until this	  */
      /* channel has stopped playing..... */

      _kernel_swi (OS_ReadMonotonicTime, &regs, &regs);
      time_now = regs.r [0];

      regs.r[0] = channel;
      _kernel_swi (DataVox_ReadAddress, &regs, &regs);
    } while ((regs.r[1]) && (time_now - time_started < 900));


    regs.r[0] = channel;
    _kernel_swi (DataVox_Unset, &regs, &regs);

    regs.r[0] = channel;
    regs.r[1] = 128;
    _kernel_swi (Sound_Stereo, &regs, &regs);

    regs.r[0] = channel;
    regs.r[1] = prev_snd_voice [channel];
    _kernel_swi (Sound_AttachVoice, &regs, &regs);

    channel++;
  } while (channel <= MAX_SFX_CHAN);

  regs.r[0] = prev_snd_config [0];
  regs.r[1] = prev_snd_config [1];
  regs.r[2] = prev_snd_config [2];
  regs.r[3] = prev_snd_config [3];
  regs.r[4] = prev_snd_config [4];
  regs.r[5] = prev_snd_config [5];
  _kernel_swi (Sound_Configure, &regs, &regs);
}

/* ------------------------------------------------------------ */

void
I_InitSound(void)
{
  _kernel_swi_regs regs;
  unsigned int channel;

  regs.r[0] = MAX_SFX_CHAN;
  regs.r[1] = 208;
  regs.r[2] = 48;
  regs.r[3] = 0;
  regs.r[4] = 0;
  regs.r[5] = 0;
  _kernel_swi (Sound_Configure, &regs, &regs);

  prev_snd_config [0] = regs.r[0];
  prev_snd_config [1] = regs.r[1];
  prev_snd_config [2] = regs.r[2];
  prev_snd_config [3] = regs.r[3];
  prev_snd_config [4] = regs.r[4];
  prev_snd_config [5] = regs.r[5];

  system ("tuning 0");			// Set to default

  channel = MIN_SFX_CHAN;
  do
  {
    /* Attach Named Voice doesn't return the old voice so */
    /* Must attach a dummy (or null) channel first */

    regs.r[0] = channel;
    regs.r[1] = 0;
    _kernel_swi (Sound_AttachVoice, &regs, &regs);
    prev_snd_voice [channel] = regs.r[1];

    regs.r[0] = channel;
    regs.r[1] = (int) "DataVox-Voice";
    _kernel_swi (Sound_AttachNamedVoice, &regs, &regs);
    channel++;
  } while (channel <= MAX_SFX_CHAN);
}

/* ------------------------------------------------------------ */
/* Music handling code from DIY doom */

static unsigned int Mus_CheckAndInit (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = 0;
  if (_kernel_swi (MIDI_Init, &regs, &regs))
    return (0);

  if (regs.r[1] == 0)
    return (0);

  regs.r[0] = 15;
  if (_kernel_swi (MIDI_Init, &regs, &regs))
    return (0);

  regs.r[0] = 1;
  if (_kernel_swi (MIDI_IgnoreTiming, &regs, &regs))
    return (0);

  return (1);
}


static void Mus_ClearQueue (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = 15;
  _kernel_swi (MIDI_Init, &regs, &regs);
}


static void Mus_StopSound (void)
{
  unsigned int channel;
  _kernel_swi_regs regs;

  channel = 1;
  do
  {
    regs.r[0] = channel;
    _kernel_swi (MIDI_SetTxChannel, &regs, &regs);
    do {} while (_kernel_swi (MIDI_TxAllNotesOff, &regs, &regs));
    channel++;
  } while (channel < 17);
}


static unsigned int Mus_StartClock (unsigned int a, unsigned int b)
{
  _kernel_swi_regs regs;

  regs.r[0] = a;
  regs.r[1] = b;
  _kernel_swi (MIDI_FastClock, &regs, &regs);
  return (regs.r[1]);
}

static unsigned int Mus_TxCommand (unsigned int a, unsigned int b)
{
  _kernel_swi_regs regs;

  regs.r[0] = a;
  regs.r[1] = b * 7;
  if (_kernel_swi (MIDI_TxCommand, &regs, &regs))
  {
    return (-3);	// Return -3 if tx buffer full
  }
  else
  {
    return (regs.r[0]);
  }
}

/* ------------------------------------------------------------ */

static unsigned int Mus_QTM_load (byte * addr)
{
  _kernel_swi_regs regs;
  _kernel_oserror * rc;

  regs.r[0] = -1; //0;		// Must copy to QTM private memory otherwise crashes doom.
  regs.r[1] = (int) addr;	// A competant programmer would find out why and fix it!
  rc = _kernel_swi (QTM_Load, &regs, &regs);
#ifdef DEBUG_MUSIC
  if (rc)
    printf ("QTM Load:%s (%X,%s)\n", rc -> errmess, addr, addr);
#endif
  return ((unsigned int) rc);
}

static unsigned int Mus_QTM_start (void)
{
  _kernel_swi_regs regs;

  if (_kernel_swi (QTM_Start, &regs, &regs))
    return (1);

  regs.r[0] = -1;
  regs.r[1] = -1;
  regs.r[2] = -1;
  _kernel_swi (QTM_SoundControl, &regs, &regs);
#ifdef DEBUG_MUSIC
  printf ("QTM_SoundControl %d %d %d\n", regs.r[0], regs.r[1], regs.r[2]);
#endif
  qtm_channels_swiped = regs.r[0];
  return (0);
}

static unsigned int Mus_QTM_stop (void)
{
  _kernel_swi_regs regs;

  return ((int)_kernel_swi (QTM_Stop, &regs, &regs));
}

static unsigned int Mus_QTM_pause (void)
{
  _kernel_swi_regs regs;

  return ((int)_kernel_swi (QTM_Pause, &regs, &regs));
}

static unsigned int Mus_QTM_clear (void)
{
  _kernel_swi_regs regs;

  return ((int)_kernel_swi (QTM_Clear, &regs, &regs));
}

static unsigned int Mus_QTM_set_volume (unsigned int vol)
{
  _kernel_swi_regs regs;

  regs.r[0] = vol;
  return ((int)_kernel_swi (QTM_Volume, &regs, &regs));
}

/* ------------------------------------------------------------ */

static unsigned int Mus_TIM_load (byte * addr, unsigned int size)
{
  _kernel_swi_regs regs;
  _kernel_oserror * rc;

  regs.r[0] = 0;
  regs.r[1] = 0;
  regs.r[2] = (int) addr;
  regs.r[3] = size;
  rc = _kernel_swi (TimPlayer_SongLoad, &regs, &regs);
  if (rc)
  {
#ifdef DEBUG_MUSIC
    printf ("TIM Load:%s (%X,%s)\n", rc -> errmess, addr, addr);
#endif
  }
  else
  {
    timplayer_handle = regs.r[0];
#ifdef DEBUG_MUSIC
    printf ("Tim handle = %X\n", timplayer_handle);
#endif
  }
  return ((unsigned int) rc);
}

static unsigned int Mus_TIM_start (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = timplayer_handle;
  return ((int)_kernel_swi (TimPlayer_SongPlay, &regs, &regs));
}

static unsigned int Mus_TIM_loop (int loop)
{
  _kernel_swi_regs regs;

  regs.r[0] = 257;
  regs.r[1] = !loop;
  return ((int) _kernel_swi (TimPlayer_Configure, &regs, &regs));
}

static unsigned int Mus_TIM_stop (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = timplayer_handle;
  return ((int)_kernel_swi (TimPlayer_SongStop, &regs, &regs));
}

static unsigned int Mus_TIM_pause (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = timplayer_handle;
  return ((int)_kernel_swi (TimPlayer_SongPause, &regs, &regs));
}

static unsigned int Mus_TIM_clear (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = timplayer_handle;
  return ((int)_kernel_swi (TimPlayer_SongUnload, &regs, &regs));
}

static unsigned int Mus_TIM_set_volume (unsigned int vol)
{
  _kernel_swi_regs regs;

  if (music_available & TIM_PLAYING)
  {
    regs.r[0] = timplayer_handle;
    regs.r[1] = vol;
    return ((int)_kernel_swi (TimPlayer_SongVolume, &regs, &regs));
  }
  return (0);
}

static unsigned int Mus_Load_TimPlayer (void)
{
  _kernel_swi_regs regs;

  if ((music_available & (TIM_LOADED | TIM_NOT_AVAIL)) == 0)	// Been here before?
  {
    regs.r[1] = (int) "TimPlayer_SongVolume";
    if (_kernel_swi (OS_SWINumberFromString, &regs, &regs) == 0)
    {
      music_available |= TIM_LOADED;		// Was already loaded
    }
    else if (RmLoad_Module ("System:Modules.Audio.Trackers.TimPlayer"))
    {
      music_available |= TIM_NOT_AVAIL;
    }
    else
    {
      music_available |= TIM_LOADED;
      return (0);
    }
  }

  /* Was already loaded, so we cannot try again */
  return (1);
}

/* ------------------------------------------------------------ */

static unsigned int Mus_AMP_load (const char * filename)
{
  _kernel_swi_regs regs;
  _kernel_oserror * rc;

  regs.r[0] = 0;
  regs.r[1] = (int) filename;
  rc = _kernel_swi (AMPlayer_Play, &regs, &regs);
#ifdef DEBUG_MUSIC
  if (rc)
    printf ("AMP Load:%s (%s)\n", rc -> errmess, filename);
#endif
  return ((unsigned int) rc);
}

static unsigned int Mus_AMP_start (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = 1;
  _kernel_swi (AMPlayer_Pause, &regs, &regs);
  return (0);
}

static unsigned int Mus_AMP_stop (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = 0;
  return ((int)_kernel_swi (AMPlayer_Stop, &regs, &regs));
}

static unsigned int Mus_AMP_pause (void)
{
  _kernel_swi_regs regs;

  regs.r[0] = 0;
  return ((int)_kernel_swi (AMPlayer_Pause, &regs, &regs));
}

static unsigned int Mus_AMP_clear (void)
{
  return (0);
}

static unsigned int Mus_AMP_set_volume (unsigned int vol)
{
  _kernel_swi_regs regs;

  regs.r[0] = 0;
//regs.r[1] = 0;
  regs.r[1] = vol; // Docs say R2, Actually R1
  return ((int)_kernel_swi (AMPlayer_Control, &regs, &regs));
}

static void Mus_AMP_restart_song (const char * filename)
{
  _kernel_swi_regs regs;

  regs.r[0] = 0;
  if ((_kernel_swi (AMPlayer_Info, &regs, &regs) == 0)
   && (regs.r[0] == 0))
    Mus_AMP_load (filename);
}

static int Mus_AMP_info (void)
{
  _kernel_swi_regs regs;
  regs.r[0] = 0;
  _kernel_swi (AMPlayer_Info, &regs, &regs);
  return (regs.r[0]);
}

/* ------------------------------------------------------------ */

static int I_Validate_MusName (const char * musname)
{
  unsigned int i;
  musicinfo_t * sptr;
  map_dests_t * map_ptr;

  if (strncasecmp (musname, "D_", 2))
    return (1);

  sptr = S_music;
  sptr++;		// 1st one is empty.
  musname += 2;
  do
  {
    if (strcasecmp (sptr -> name, musname) == 0)
      return (0);
    sptr++;
  } while (sptr -> name);

#if 0
  /* Failed to find it in the standard tables - try the original in case it has been dehacked */
  {
    char ** mcopy;
    mcopy = music_names_copy;
    mcopy++;		// 1st one is empty.
    do
    {
      if (strcasecmp (*mcopy, musname) == 0)
        return (0);
      mcopy++;
    } while (*mcopy);
  }
#endif

  /* Failed to find it in the standard tables - try the map table */

  i = 0;
  map_ptr = G_Access_MapInfoTab_E (1,0);
  do
  {
    if ((map_ptr -> music)
     && (strcasecmp (map_ptr -> music, musname) == 0))
      return (0);
    map_ptr++;
  } while (++i < (9*10));

  i = 0;
  map_ptr = G_Access_MapInfoTab_E (255,0);
  do
  {
    if ((map_ptr -> music)
     && (strcasecmp (map_ptr -> music, musname) == 0))
      return (0);
    map_ptr++;
  } while (++i < 100);

  return (1);
}

/* ------------------------------------------------------------ */

static void I_InitMusicDirectory (void)
{
  unsigned int pos;
  unsigned int line;
  FILE * fin;
  mus_dir_t * mptr;
  char * prefix;
  char * bptr;
  char buffer [200];
  char filename [100];

  if ((Mus_AMP_set_volume (127))
   && ((RmLoad_Module ("System:Modules.Audio.MP3.AMPlayer"))
    || (Mus_AMP_set_volume (127))))
  {
    printf ("Failed to initialise AMPlayer\n");
    return;
  }

  fin = fopen ("<DoomMusDir>.Directory", "rb");
  if (fin == NULL)
  {
    fprintf (stderr, "Failed to open music directory (%s)\n", "<DoomMusDir>.Directory");
    return;
  }

  /* Work out which version we are */
  prefix = "Doom1";
  if (gamemode == commercial)
  {
    prefix = "Doom2";
    if (gamemission == pack_plut)
      prefix = "Plutonia";
    else if (gamemission == pack_tnt)
      prefix = "Tnt";
  }

  line = 0;
  do
  {
    line++;
    dh_fgets (buffer, sizeof (buffer), fin);
    if ((buffer [0]) && (buffer [0] != '#'))
    {
      bptr = buffer;
      pos = dh_inchar (bptr, ':');
      if (pos)
      {
	buffer [pos-1] = 0;
	if (dh_instr (buffer, prefix) == 0)
	  continue;
	bptr = buffer + pos;
      }
      pos = dh_inchar (bptr,',');
      if (pos)
      {
	bptr [pos-1] = 0;
	if (I_Validate_MusName (bptr))
	{
	  if (M_CheckParm ("-showunknown"))
	    fprintf (stderr, "Directory line %u: %s is not a valid sound name\n", line, bptr);
	}
	else
	{
	  if ((bptr [pos] == '<')		// Assume full path if starts with '<'
	   || (dh_inchar (bptr+pos,'$')))	// or if $ present
	    strcpy (filename, bptr+pos);
	  else
	    sprintf (filename,"<DoomMusDir>.%s", bptr+pos);

	  if (access (filename, R_OK))
	  {
	    if (M_CheckParm ("-showunknown"))
	      fprintf (stderr, "Directory line %u: Sound file %s not found\n", line, filename);
	  }
	  else
	  {
	    mptr = malloc (sizeof (mus_dir_t));
	    if (mptr == NULL)
	      break;

	    mptr -> next = music_directory_head;
	    music_directory_head = mptr;

	    strcpy (mptr -> musname, bptr);
	    strcpy (mptr -> filename, filename);

  	    if (M_CheckParm ("-showmusdir"))
              printf ("Mus dir '%s' -> '%s'\n", mptr -> musname, mptr -> filename);
          }
        }
      }
    }
  } while (!feof (fin));

  fclose (fin);
}

/* ------------------------------------------------------------ */

/* This is called once at startup */
void I_InitMusic (void)
{
  int p;
  music_available = 0;
  music_pause = 0;
  music_data = NULL;
  music_pos = NULL;
  amp_current = NULL;
  music_directory_head = NULL;

  p = M_CheckParm ("-mp3");
  if (p)
  {
    p++;
    if ((p < myargc) && (isdigit(myargv[p][0])))
    {
      mp3priority = atoi (myargv[p]);
    }
    I_InitMusicDirectory();
  }

  if (music_pause==0)
  {
    if (Mus_CheckAndInit ())
    {
      music_available |= MIDI_AVAILABLE;
      fprintf(stderr,"MIDI initialised\n");
    }
    else
    {
      fprintf(stderr,"Could not initialise MIDI\n");
    }
  }
  else
  {
    I_ResumeSong (1);
  }
}

/* ------------------------------------------------------------ */
/* This will restore system resources to their state
 * before I_ClaimMusic was called
 */
void I_ReleaseMusic(void)
{
  if (!music_available)
    return;
  I_PauseSong(1);
}

/* ------------------------------------------------------------ */

static int I_PlayMusicFile (const char * lumpname)
{
  mus_dir_t * ptr;

  ptr = music_directory_head;
  if (ptr == NULL)
    return (1);

  do
  {
//  printf ("Comparing (%s) with (%s)\n", ptr -> musname, lumpname);
    if (strcasecmp (ptr -> musname, lumpname) == 0)
    {
      if (Mus_AMP_load (ptr -> filename) == 0)
      {
	amp_current = ptr;
//	printf ("Playing %s\n", ptr -> filename);
	music_available |= AMP_PLAYING;
	return (0);
      }
      break;
    }
    ptr = ptr -> next;
  } while (ptr);

  amp_current = NULL;
  return (1);
}

/* ------------------------------------------------------------ */
/* Song has been loaded, and placed at 'data'.
   If music is ready, get ready to play.
   Return a dummy handle */
int I_RegisterSong (musicinfo_t * music)
{
  unsigned int size;
  unsigned int offset;
  byte * data;
  void * vdata;
  char namebuf[9];

  I_UnRegisterSong (1);

  vdata = music->data;
  data = (byte*) vdata;
  size = W_LumpLength (music->lumpnum);

  if ((mp3priority == 0)
   && (M_CheckParm ("-mp3")))
  {
    sprintf (namebuf, "d_%s", music->name);
    if (I_PlayMusicFile (namebuf) == 0)
      return 1;
  }


  if (Mus_QTM_load (data) == 0)		// Did QTM recognise it?
  {
    music_available |= QTM_PLAYING;
    return (1);
  }

  if (Mus_TIM_load (data,size) == 0)	// Did TIM player recognise it?
  {
    music_available |= TIM_PLAYING;
    return (timplayer_handle);
  }

  if (((int*)data)[0]==0x1a53554d)
  {
    if ((mp3priority)
     && (M_CheckParm ("-mp3")))
    {
      sprintf (namebuf, "d_%s", music->name);
      if (I_PlayMusicFile (namebuf) == 0)
	return 1;
    }

    if (music_available & MIDI_AVAILABLE)
    {
      offset=*(data+6)+((*(data+7))<<8);
      music_data=data+offset;
      return 1;
    }
    return (0);
  }

  if ((Mus_Load_TimPlayer () == 0)	// Have we now loaded TimPlayer?
   && (Mus_TIM_load (data,size) == 0))	// Did TIM player recognise it?
  {
    music_available |= TIM_PLAYING;
    return (timplayer_handle);
  }


  if (((Mus_AMP_set_volume (127) == 0)	// Is Amplayer loaded?
   || ((RmLoad_Module ("System:Modules.Audio.MP3.AMPlayer") == 0)
    && (Mus_AMP_set_volume (127) == 0)))
   && (I_Save_MusFile (MUS_TEMP_FILE, vdata, size) == 0))
  {
    if (Mus_AMP_load (MUS_TEMP_FILE) == 0)
    {
      amp_current = (mus_dir_t *) &wimp_temp;
//    printf ("Playing %s\n", ptr -> filename);
      music_available |= AMP_PLAYING;
	return 1;
    }
    else
    {
      remove (MUS_TEMP_FILE);
    }
  }

  if ((mp3priority)
   && (M_CheckParm ("-mp3")))
  {
    sprintf (namebuf, "d_%s", music->name);
    if (I_PlayMusicFile (namebuf) == 0)
      return 1;
  }

  return 0;
}

/* ------------------------------------------------------------ */
/* Keep scheduler buffer full */
void I_FillMusBuffer(int handle)
{
  int free,cmd,delay,ch,par1,par2;

  if ((music_available & AMP_PLAYING)
   && (amp_current)
   && (music_loop))
  {
    Mus_AMP_restart_song (amp_current->filename);
    return;
  }

  if (music_pos==NULL) return;
  free=Mus_TxCommand(0xFE,music_time);
  while ((free>0) && (music_pos!=NULL))
  {
    cmd=*music_pos++;
    ch=midimap[cmd &15];
    switch ((cmd>>4) &7)
    {
      case 0: /* NoteOff */
	free=Mus_TxCommand(0x400080|ch|(*music_pos++<<8),music_time);
	break;
      case 1: /* NoteOn */
	par1=*music_pos++;
	if (par1 &128)
	  par2=(music_vel[ch]=*music_pos++)+music_midivol;
	else
	  par2=music_vel[ch]+music_midivol;
	par1&=127;
	if (par2>0) free=Mus_TxCommand(0x90|ch|(par1<<8)|(par2<<16),music_time);
	break;
      case 2: /* Bend */
	par1=*music_pos++;
	free=Mus_TxCommand(0xE0|ch|((par1 &127)<<8)|((par1 &128)<<1),music_time);
	break;
      case 3: /* System */
	switch (*music_pos++)
	{
	  case 10:
	  case 11: free=Mus_TxCommand(0x7BB0|ch,music_time); break;
	  case 12: free=Mus_TxCommand(0x7EB0|ch,music_time); break;
	  case 13: free=Mus_TxCommand(0x7FB0|ch,music_time); break;
	  case 14: free=Mus_TxCommand(0x79B0|ch,music_time); break;
	}
	break;
      case 4: /* ControlChange */
	par1=*music_pos++;
	par2=*music_pos++;
	switch (par1)
	{
	  case 0: free=Mus_TxCommand(0x00C0|ch|(par2<<8),music_time); break;
	  case 2: free=Mus_TxCommand(0x01B0|ch|(par2<<16),music_time); break;
	  case 3: free=Mus_TxCommand(0x07B0|ch|(par2<<16),music_time); break;
	  case 4: free=Mus_TxCommand(0x0AB0|ch|(par2<<16),music_time); break;
	  case 5: free=Mus_TxCommand(0x0BB0|ch|(par2<<16),music_time); break;
	  case 6: free=Mus_TxCommand(0x5BB0|ch|(par2<<16),music_time); break;
	  case 7: free=Mus_TxCommand(0x5DB0|ch|(par2<<16),music_time); break;
	  case 8: free=Mus_TxCommand(0x40B0|ch|(par2<<16),music_time); break;
	  case 9: free=Mus_TxCommand(0x43B0|ch|(par2<<16),music_time); break;
	}
	break;
      case 6: /* End */
	if (music_loop)
	  music_pos=music_data;
	else
	  music_pos=NULL;
	break;
    }
    if (cmd>127)
    {
      delay=0;
      while (1)
      {
	cmd=*music_pos++;
	delay=(cmd & 127)|(delay<<7);
	if (cmd<128) break;
      }
      music_time+=delay;
    }
  }
}

/* ------------------------------------------------------------ */
/* If a song is registered, start playing it */
void I_PlaySong (int handle, int loop)
{
  int i;

  // I_StopSong(handle);

  if (music_available & QTM_PLAYING)
  {
    Mus_QTM_start ();
    return;
  }

  if (music_available & TIM_PLAYING)
  {
    Mus_TIM_start ();
    Mus_TIM_set_volume (music_timvol);
    Mus_TIM_loop (loop);
    return;
  }

  if (music_available & AMP_PLAYING)
  {
    Mus_AMP_start ();
    Mus_AMP_set_volume (music_ampvol);
    music_loop=loop;
    return;
  }

  if (((music_available & MIDI_AVAILABLE) == 0)
   || (music_data == NULL))
    return;

  music_loop=loop;
  music_pos=music_data;
  music_time=music_pause=0;
  for (i=0; i<16; i++) music_vel[i]=64;
  Mus_StartClock(100,0);
  I_FillMusBuffer(handle);
}


/* ------------------------------------------------------------ */
/* Is the song playing, even if paused? */
int I_QrySongPlaying (int handle)
{
  if ((music_available & (QTM_PLAYING|TIM_PLAYING|AMP_PLAYING))
   || (music_pos != NULL))
    return (1);

  return (0);
}

/* ------------------------------------------------------------ */

void I_SetMusicVolume (int volume) /* 0..15 */
{
  if (volume > 15) volume = 15;
  music_midivol = (volume-15)*127/15;
  music_qtmvol  = volume << 2;	/* Convert volume 0-15 to 0-63 */
  music_timvol  = timplayer_vol_tab [volume];	/* Convert volume 0-15 to 0-256 */
  music_ampvol  = volume << 3;	/* Convert volume 0-15 to 0-127 */
  Mus_QTM_set_volume (music_qtmvol);
  Mus_TIM_set_volume (music_timvol);
  Mus_AMP_set_volume (music_ampvol);
}

/* ------------------------------------------------------------ */
/* Pause song, be silent */
void I_PauseSong (int handle)
{
  if (music_available & QTM_PLAYING)
  {
    Mus_QTM_pause ();
    return;
  }

  if (music_available & TIM_PLAYING)
  {
    Mus_TIM_pause ();
    return;
  }

  if (music_available & AMP_PLAYING)
  {
    Mus_AMP_pause ();
    return;
  }

  if (((music_available & MIDI_AVAILABLE) == 0)
   || (music_pause != 0))
    return;				/* Already paused */

  music_pause=Mus_StartClock(0,0);	/* Stop clock and note time */
  Mus_StopSound();
}

/* ------------------------------------------------------------ */
/* Resume song, obviously */
void I_ResumeSong (int handle)
{
  if (music_available & QTM_PLAYING)
  {
    Mus_QTM_start ();
    return;
  }

  if (music_available & TIM_PLAYING)
  {
    Mus_TIM_start ();
    return;
  }

  if (music_available & AMP_PLAYING)
  {
    Mus_AMP_start ();
    return;
  }

  if (((music_available & MIDI_AVAILABLE) == 0)
   || (music_pause == 0))
    return;				/* Wasn't paused */

  Mus_StartClock(100,music_pause);	/* Restart clock */
  music_pause=0;
}

/* ------------------------------------------------------------ */
/* Stop song and sounds */
void I_StopSong (int handle)
{
  int i,j;

  if (music_available & QTM_PLAYING)
  {
    Mus_QTM_stop ();
    Mus_QTM_clear ();
    music_available &= ~QTM_PLAYING;
    return;
  }

  if (music_available & TIM_PLAYING)
  {
    Mus_TIM_stop ();
    Mus_TIM_clear ();
    music_available &= ~TIM_PLAYING;
    return;
  }

  if (music_available & AMP_PLAYING)
  {
    Mus_AMP_stop ();
    Mus_AMP_clear ();
    if (amp_current == (mus_dir_t *) &wimp_temp)
    {
      i = I_GetTime ();
      do
      {
        j = Mus_AMP_info ();
      } while ((j) && ((I_GetTime () - i) < 100));
      remove (amp_current -> filename);
    }
    amp_current = NULL;
    music_available &= ~AMP_PLAYING;
    return;
  }

  if (((music_available & MIDI_AVAILABLE) == 0)
   || (music_data == NULL))		/* Not registered */
    return;

  Mus_ClearQueue();			/* Might be stopped already if not looping */
  Mus_StopSound();
  music_pos=NULL;
  music_pause=0;
}

/* ------------------------------------------------------------ */
/* Stop and forget song */
void I_UnRegisterSong (int handle)
{
  I_StopSong (handle);
  music_data=NULL;
}

/* ------------------------------------------------------------ */
/* This should kill playing music, and tidy up */
void I_ShutdownMusic (void)
{
  I_UnRegisterSong(1);
  music_available = 0;
}

/* ------------------------------------------------------------ */
