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
//	Created by a sound utility.
//	Kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: sounds.c,v 1.3 1997/01/29 22:40:44 b1 Exp $";
#endif

#include "includes.h"

//
// Information about all the music
//

musicinfo_t S_music[] =
{
    { 0 },
    { "e1m1", 0 },
    { "e1m2", 0 },
    { "e1m3", 0 },
    { "e1m4", 0 },
    { "e1m5", 0 },
    { "e1m6", 0 },
    { "e1m7", 0 },
    { "e1m8", 0 },
    { "e1m9", 0 },
    { "e2m1", 0 },
    { "e2m2", 0 },
    { "e2m3", 0 },
    { "e2m4", 0 },
    { "e2m5", 0 },
    { "e2m6", 0 },
    { "e2m7", 0 },
    { "e2m8", 0 },
    { "e2m9", 0 },
    { "e3m1", 0 },
    { "e3m2", 0 },
    { "e3m3", 0 },
    { "e3m4", 0 },
    { "e3m5", 0 },
    { "e3m6", 0 },
    { "e3m7", 0 },
    { "e3m8", 0 },
    { "e3m9", 0 },
    { "inter", 0 },
    { "intro", 0 },
    { "bunny", 0 },
    { "victor", 0 },
    { "introa", 0 },
    { "runnin", 0 },
    { "stalks", 0 },
    { "countd", 0 },
    { "betwee", 0 },
    { "doom", 0 },
    { "the_da", 0 },
    { "shawn", 0 },
    { "ddtblu", 0 },
    { "in_cit", 0 },
    { "dead", 0 },
    { "stlks2", 0 },
    { "theda2", 0 },
    { "doom2", 0 },
    { "ddtbl2", 0 },
    { "runni2", 0 },
    { "dead2", 0 },
    { "stlks3", 0 },
    { "romero", 0 },
    { "shawn2", 0 },
    { "messag", 0 },
    { "count2", 0 },
    { "ddtbl3", 0 },
    { "ampie", 0 },
    { "theda3", 0 },
    { "adrian", 0 },
    { "messg2", 0 },
    { "romer2", 0 },
    { "tense", 0 },
    { "shawn3", 0 },
    { "openin", 0 },
    { "evil", 0 },
    { "ultima", 0 },
    { "read_m", 0 },
    { "dm2ttl", 0 },
    { "dm2int", 0 },
    { 0, 0}
};


//
// Information about all the sfx
//

sfxinfo_t S_sfx[] =
{
  // S_sfx[0] needs to be a dummy for odd reasons.
  { "none",   sg_none,     0, sfx_None, -1, -1, -1, 11025 },
  { "pistol", sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "shotgn", sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "sgcock", sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "dshtgn", sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "dbopn",  sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "dbcls",  sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "dbload", sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "plasma", sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "bfg",    sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "sawup",  sg_saw,     64, sfx_None, -1, -1, -1, 11025 },
  { "sawidl", sg_saw,    118, sfx_None, -1, -1, -1, 11025 },
  { "sawful", sg_saw,     64, sfx_None, -1, -1, -1, 11025 },
  { "sawhit", sg_saw,     64, sfx_None, -1, -1, -1, 11025 },
  { "rlaunc", sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "rxplod", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "firsht", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "firxpl", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "pstart", sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "pstop",  sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "doropn", sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "dorcls", sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "stnmov", sg_stnmov, 119, sfx_None, -1, -1, -1, 11025 },
  { "swtchn", sg_none,    78, sfx_None, -1, -1, -1, 11025 },
  { "swtchx", sg_none,    78, sfx_None, -1, -1, -1, 11025 },
  { "plpain", sg_none,    96, sfx_None, -1, -1, -1, 11025 },
  { "dmpain", sg_none,    96, sfx_None, -1, -1, -1, 11025 },
  { "popain", sg_none,    96, sfx_None, -1, -1, -1, 11025 },
  { "vipain", sg_none,    96, sfx_None, -1, -1, -1, 11025 },
  { "mnpain", sg_none,    96, sfx_None, -1, -1, -1, 11025 },
  { "pepain", sg_none,    96, sfx_None, -1, -1, -1, 11025 },
  { "slop",   sg_none,    78, sfx_None, -1, -1, -1, 11025 },
  { "itemup", sg_itemup,  78, sfx_None, -1, -1, -1, 11025 },
  { "wpnup",  sg_wpnup,   78, sfx_None, -1, -1, -1, 11025 },
  { "oof",    sg_oof,     96, sfx_None, -1, -1, -1, 11025 },
  { "telept", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "posit1", sg_none,    98, sfx_None, -1, -1, -1, 11025 },
  { "posit2", sg_none,    98, sfx_None, -1, -1, -1, 11025 },
  { "posit3", sg_none,    98, sfx_None, -1, -1, -1, 11025 },
  { "bgsit1", sg_none,    98, sfx_None, -1, -1, -1, 11025 },
  { "bgsit2", sg_none,    98, sfx_None, -1, -1, -1, 11025 },
  { "sgtsit", sg_none,    98, sfx_None, -1, -1, -1, 11025 },
  { "cacsit", sg_none,    98, sfx_None, -1, -1, -1, 11025 },
  { "brssit", sg_none,    94, sfx_None, -1, -1, -1, 11025 },
  { "cybsit", sg_none,    92, sfx_None, -1, -1, -1, 11025 },
  { "spisit", sg_none,    90, sfx_None, -1, -1, -1, 11025 },
  { "bspsit", sg_none,    90, sfx_None, -1, -1, -1, 11025 },
  { "kntsit", sg_none,    90, sfx_None, -1, -1, -1, 11025 },
  { "vilsit", sg_none,    90, sfx_None, -1, -1, -1, 11025 },
  { "mansit", sg_none,    90, sfx_None, -1, -1, -1, 11025 },
  { "pesit",  sg_none,    90, sfx_None, -1, -1, -1, 11025 },
  { "sklatk", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "sgtatk", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "skepch", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "vilatk", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "claw",   sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "skeswg", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "pldeth", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "pdiehi", sg_none,    32, sfx_pldeth, -1, -1, -1, 11025 },
  { "podth1", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "podth2", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "podth3", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "bgdth1", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "bgdth2", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "sgtdth", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "cacdth", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "skldth", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "brsdth", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "cybdth", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "spidth", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "bspdth", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "vildth", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "kntdth", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "pedth",  sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "skedth", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "posact", sg_none,   120, sfx_None, -1, -1, -1, 11025 },
  { "bgact",  sg_none,   120, sfx_None, -1, -1, -1, 11025 },
  { "dmact",  sg_none,   120, sfx_None, -1, -1, -1, 11025 },
  { "bspact", sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "bspwlk", sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "vilact", sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "noway",  sg_oof,     78, sfx_None, -1, -1, -1, 11025 },
  { "barexp", sg_none,    60, sfx_None, -1, -1, -1, 11025 },
  { "punch",  sg_none,    64, sfx_None, -1, -1, -1, 11025 },
  { "hoof",   sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "metal",  sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "chgun",  sg_none,    64, sfx_pistol, 150, 0, -1, 11025 },
  { "tink",   sg_none,    60, sfx_None, -1, -1, -1, 11025 },
  { "bdopn",  sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "bdcls",  sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "itmbk",  sg_none,   100, sfx_None, -1, -1, -1, 11025 },
  { "flame",  sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "flamst", sg_none,    32, sfx_None, -1, -1, -1, 11025 },
  { "getpow", sg_none,    60, sfx_None, -1, -1, -1, 11025 },
  { "bospit", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "boscub", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "bossit", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "bospn",  sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "bosdth", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "manatk", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "mandth", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "sssit",  sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "ssdth",  sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "keenpn", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "keendt", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "skeact", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "skesit", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "skeatk", sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "radio",  sg_none,    60, sfx_tink, -1, -1, -1, 11025 },
  // killough 11/98: dog sounds
  { "dgsit",  sg_none,    98, sfx_None, -1, -1, -1, 11025 },
  { "dgatk",  sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "dgact",  sg_none,   120, sfx_None, -1, -1, -1, 11025 },
  { "dgdth",  sg_none,    70, sfx_None, -1, -1, -1, 11025 },
  { "dgpain", sg_none,    96, sfx_None, -1, -1, -1, 11025 },
  { "secret", sg_none,    60, sfx_getpow, -1, -1, -1, 11025 },
  { "gibdth", sg_none,    60, sfx_None, -1, -1, -1, 11025 },
  { "scrsht", sg_none,     0, sfx_None, -1, -1, -1, 11025 },
  { NULL,     sg_none,     0, sfx_None, -1, -1, -1, 11025 }
};

char * music_names_copy [ARRAY_SIZE(S_music)];
#if 0
{
  "none",
  "e1m1", "e1m2", "e1m3", "e1m4", "e1m5", "e1m6", "e1m7", "e1m8", "e1m9",
  "e2m1", "e2m2", "e2m3", "e2m4", "e2m5", "e2m6", "e2m7", "e2m8", "e2m9",
  "e3m1", "e3m2", "e3m3", "e3m4", "e3m5", "e3m6", "e3m7", "e3m8", "e3m9",
  "inter", "intro", "bunny", "victor", "introa", "runnin", "stalks", "countd",
  "betwee", "doom", "the_da", "shawn", "ddtblu", "in_cit", "dead", "stlks2",
  "theda2", "doom2", "ddtbl2", "runni2", "dead2", "stlks3", "romero", "shawn2",
  "messag", "count2", "ddtbl3", "ampie", "theda3", "adrian", "messg2", "romer2",
  "tense", "shawn3", "openin", "evil", "ultima", "read_m", "dm2ttl", "dm2int",
  NULL
};
#endif

char * sound_names_copy [ARRAY_SIZE(S_sfx)];
#if 0
{
  "none", "pistol", "shotgn", "sgcock", "dshtgn", "dbopn", "dbcls", "dbload",
  "plasma", "bfg", "sawup", "sawidl", "sawful", "sawhit", "rlaunc", "rxplod",
  "firsht", "firxpl", "pstart", "pstop", "doropn", "dorcls", "stnmov", "swtchn",
  "swtchx", "plpain", "dmpain", "popain", "vipain", "mnpain", "pepain", "slop",
  "itemup", "wpnup", "oof", "telept", "posit1", "posit2", "posit3", "bgsit1",
  "bgsit2", "sgtsit", "cacsit", "brssit", "cybsit", "spisit", "bspsit",
  "kntsit", "vilsit", "mansit", "pesit", "sklatk", "sgtatk", "skepch", "vilatk",
  "claw", "skeswg", "pldeth", "pdiehi", "podth1", "podth2", "podth3", "bgdth1",
  "bgdth2", "sgtdth", "cacdth", "skldth", "brsdth", "cybdth", "spidth", "bspdth",
  "vildth", "kntdth", "pedth", "skedth", "posact", "bgact", "dmact", "bspact",
  "bspwlk", "vilact", "noway", "barexp", "punch", "hoof", "metal", "chgun",
  "tink", "bdopn", "bdcls", "itmbk", "flame", "flamst", "getpow", "bospit",
  "boscub", "bossit", "bospn", "bosdth", "manatk", "mandth", "sssit", "ssdth",
  "keenpn", "keendt", "skeact", "skesit", "skeatk", "radio",
  // killough 11/98: dog sounds
  "dgsit", "dgatk", "dgact", "dgdth", "dgpain",
  "secret",
  NULL
};
#endif
