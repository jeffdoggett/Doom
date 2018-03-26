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
// DESCRIPTION:
//	The not so system specific sound interface.
//
//-----------------------------------------------------------------------------


#ifndef __S_SOUND__
#define __S_SOUND__


#ifdef __GNUG__
#pragma interface
#endif


void S_InitSound (void);
void S_ShutdownSound (void);

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void
S_Init
( int		sfxVolume,
  int		musicVolume );




//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void);


//
// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
//
void
S_StartSound
( mobj_t*	origin,
  int		sound_id );

void
S_StartSoundOnce
( mobj_t*	origin,
  int		sound_id );


// Stop sound for thing at <origin>
void S_StopSound(void* origin);
void S_RemoveSoundOrigin (void *origin);


int S_StartLevelMusic (unsigned int ge, unsigned int gm);

// Start music using <music_id> from sounds.h
void S_StartMusic(int music_id);

// Start music using <music_id> from sounds.h,
//  and set whether looping
void
S_ChangeMusic
( int		music_id,
  int		looping );

// Stops the music fer sure.
void S_StopMusic(void);

// Stop and resume music, during game PAUSE.
void S_PauseSound(void);
void S_ResumeSound(void);

int S_FindSoundData (sfxinfo_t * sfx);

//
// Updates music & sounds
//
void S_UpdateSounds(void* listener);

void S_SetMusicVolume(int volume);
void S_SetSfxVolume(int volume);

void S_MusInfoThinker (mobj_t *thing);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
