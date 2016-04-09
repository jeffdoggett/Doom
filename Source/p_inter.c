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
//	Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: p_inter.c,v 1.4 1997/02/03 22:45:11 b1 Exp $";
#endif

#include "includes.h"
extern item_to_drop_t * item_drop_head;
extern castinfo_t castorder[];
extern char* player_names[];
extern unsigned char player_genders[];
extern int gamecompletedtimer;
extern boolean crushchange;

#define BONUSADD	6

/* ------------------------------------------------------- */

char * got_messages_orig [] =
{
  GOTARMOUR, GOTMEGA, GOTHTHBONUS, GOTARMBONUS, GOTSTIM,
  GOTMEDINEED, GOTMEDIKIT, GOTSUPER, GOTBLUECARD,
  GOTYELWCARD, GOTREDCARD, GOTBLUESKUL, GOTYELWSKUL,
  GOTREDSKULL, GOTINVUL, GOTBERSERK, GOTINVIS,
  GOTSUIT, GOTMAP, GOTVISOR, GOTMSPHERE,
  GOTCLIP, GOTCLIPBOX, GOTROCKET, GOTROCKBOX,
  GOTCELL, GOTCELLBOX, GOTSHELLS, GOTSHELLBOX,
  GOTBACKPACK, GOTBFG9000, GOTCHAINGUN, GOTCHAINSAW,
  GOTLAUNCHER, GOTPLASMA, GOTSHOTGUN, GOTSHOTGUN2,
  GOTNSHELLS, GOT1SHELL,				// Extra text, not DEHACKable
  NULL
};

char * got_messages [ARRAY_SIZE(got_messages_orig)];

typedef enum
{
  P_GOTARMOUR, P_GOTMEGA, P_GOTHTHBONUS, P_GOTARMBONUS, P_GOTSTIM,
  P_GOTMEDINEED, P_GOTMEDIKIT, P_GOTSUPER, P_GOTBLUECARD,
  P_GOTYELWCARD, P_GOTREDCARD, P_GOTBLUESKUL, P_GOTYELWSKUL,
  P_GOTREDSKULL, P_GOTINVUL, P_GOTBERSERK, P_GOTINVIS,
  P_GOTSUIT, P_GOTMAP, P_GOTVISOR, P_GOTMSPHERE,
  P_GOTCLIP, P_GOTCLIPBOX, P_GOTROCKET, P_GOTROCKBOX,
  P_GOTCELL, P_GOTCELLBOX, P_GOTSHELLS, P_GOTSHELLBOX,
  P_GOTBACKPACK, P_GOTBFG9000, P_GOTCHAINGUN, P_GOTCHAINSAW,
  P_GOTLAUNCHER, P_GOTPLASMA, P_GOTSHOTGUN, P_GOTSHOTGUN2,
  P_GOTNSHELLS, P_GOT1SHELL
} got_kit_texts_t;

/* ------------------------------------------------------- */

char * obituary_messages [] =
{
  "%o got too close to %k.",
  "%o was squished.",
  "%o was melted.",
  CC_OBIT_SELF,
  NULL
};

// Lookup table in dh_stuff.c is in the same order...
typedef enum
{
  OB_BARREL,
  OB_CRUSH,
  OB_LAVA,
  OB_KILLEDSELF,
} obits_t;

typedef struct
{
  mobjtype_t mt;
  obits_t ob;
  char name [12];
} obitinfo_t;

static obitinfo_t obit_lookup [] =
{
  MT_BARREL, OB_BARREL, "barrel"
};

/* ------------------------------------------------------- */

// a weapon is found with two clip loads,
// a big item has five clip loads
int	maxammo[NUMAMMO] = {200, 50, 300, 50};
int	clipammo[NUMAMMO] = {10, 4, 20, 1};

unsigned int Max_Health_100 = 100;
unsigned int Max_Health_200 = 200;
unsigned int Max_Armour = 200;
unsigned int Max_Soulsphere_Health = 200;
unsigned int Soulsphere_Health = 100;
unsigned int Megasphere_Health = 200;
armour_class_t Green_Armour_Class = GREENARMOUR;
armour_class_t Blue_Armour_Class = BLUEARMOUR;
boolean Give_Max_Damage = false;

extern boolean Monsters_Infight1;
extern boolean Monsters_Infight2;

//
// GET STUFF
//

//-----------------------------------------------------------------------------
//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns qty of ammo given.
//

static unsigned int
P_GiveAmmo
( player_t*	player,
  ammotype_t	ammo,
  int		num )
{
  int		oldammo;
  int		maxammo;

  if (ammo == am_noammo)
      return (0);

  if (ammo < 0 || ammo > NUMAMMO)
      I_Error ("P_GiveAmmo: bad type %i", ammo);

  if (num)
      num *= clipammo[ammo];
  else
      num = (clipammo[ammo]+1)/2;

  if ((gameskill == sk_baby)
      || (gameskill == sk_nightmare))
  {
      // give double ammo in trainer mode,
      // you'll need in nightmare
      num <<= 1;
  }

  oldammo = player->ammo[ammo];
  maxammo = player->maxammo[ammo];

  if ((oldammo + num) > maxammo)
    num = maxammo - oldammo;

  if (num < 1)		// Defensive coding here in case the
    return (0);		// maths above produced a negative number!

  player->ammo[ammo] = oldammo + num;

  // If non zero ammo,
  // don't change up weapons,
  // player was lower on purpose.
  if (oldammo)
      return (num);

  // We were down to zero,
  // so select a new weapon.
  // Preferences are not user selectable.
  switch (ammo)
  {
    case am_clip:
      if (player->readyweapon == wp_fist)
      {
	  if (player->weaponowned[wp_chaingun])
	      player->pendingweapon = wp_chaingun;
	  else
	      player->pendingweapon = wp_pistol;
      }
      break;

    case am_shell:
      if (player->readyweapon == wp_fist
	  || player->readyweapon == wp_pistol)
      {
	  if (player->weaponowned[wp_shotgun])
	      player->pendingweapon = wp_shotgun;
      }
      break;

    case am_cell:
      if (player->readyweapon == wp_fist
	  || player->readyweapon == wp_pistol)
      {
	  if (player->weaponowned[wp_plasma])
	      player->pendingweapon = wp_plasma;
      }
      break;

    case am_misl:
      if (player->readyweapon == wp_fist)
      {
	  if (player->weaponowned[wp_missile])
	      player->pendingweapon = wp_missile;
      }
    default:
      break;
  }

  return (num);
}

//-----------------------------------------------------------------------------
//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
static boolean
P_GiveWeapon
( player_t*	player,
  weapontype_t	weapon,
  boolean	dropped )
{
  unsigned int	qtyammogiven;
  boolean	gaveweapon;

  if (netgame
      && (deathmatch!=2)
       && !dropped )
  {
      // leave placed weapons forever on net games
      if (player->weaponowned[weapon])
	  return false;

      player->bonuscount += BONUSADD;
      player->weaponowned[weapon] = true;

      if (deathmatch)
	  P_GiveAmmo (player, weaponinfo[weapon].ammo, 5);
      else
	  P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);

      if ((netgame)
       || (M_CheckParm ("-noautoweaponchange") == 0))
	player->pendingweapon = weapon;

      if (player == &players[consoleplayer])
	  S_StartSound (NULL, sfx_wpnup);
      return false;
  }

  if (weaponinfo[weapon].ammo != am_noammo)
  {
      // give one clip with a dropped weapon,
      // two clips with a found weapon
      if (dropped)
	  qtyammogiven = P_GiveAmmo (player, weaponinfo[weapon].ammo, 1);
      else
	  qtyammogiven = P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
  }
  else
      qtyammogiven = 0;

  if (player->weaponowned[weapon])
      gaveweapon = false;
  else
  {
      gaveweapon = true;
      player->weaponowned[weapon] = true;
      if ((netgame)
       || (M_CheckParm ("-noautoweaponchange") == 0))
	player->pendingweapon = weapon;
  }

  return (boolean)(gaveweapon || qtyammogiven);
}

//-----------------------------------------------------------------------------
//
// P_GiveBody
// Returns false if the body isn't needed at all
//
boolean
P_GiveBody
( player_t*	player,
  int		num )
{
  if (player->health >= Max_Health_100)
      return false;

  player->health += num;
  if (player->health > Max_Health_100)
      player->health = Max_Health_100;
  player->mo->health = player->health;

  return true;
}

//-----------------------------------------------------------------------------
//
// P_GiveArmour
// Returns false if the armour is worse
// than the current armour.
//
static boolean
P_GiveArmour
( player_t*	 player,
  armour_class_t armourtype )
{
  int		hits;

  hits = armourtype*100;
  if (player->armourpoints >= hits)
      return false;	// don't pick up

  player->armourtype = armourtype;
  player->armourpoints = hits;

  return true;
}

//-----------------------------------------------------------------------------
//
// P_GiveCard
//
char * P_GiveCard
( player_t*	player,
  card_t	card )
{
  if (player->cards[card])
    return (NULL);

  player->bonuscount = BONUSADD;
  player->cards[card] = true;
  return (got_messages [(int)P_GOTBLUECARD+card]);
}

//-----------------------------------------------------------------------------
//
// P_GivePower
//
boolean
P_GivePower
( player_t*	player,
  int /*powertype_t*/	power )
{
  if (power == pw_invulnerability)
  {
      player->powers[power] = INVULNTICS;
      return true;
  }

  if (power == pw_invisibility)
  {
      player->powers[power] = INVISTICS;
      player->mo->flags |= MF_SHADOW;
      return true;
  }

  if (power == pw_infrared)
  {
      player->powers[power] = INFRATICS;
      return true;
  }

  if (power == pw_ironfeet)
  {
    if ((!netgame && gamekeydown['`'])
     || (M_CheckParm ("-permradsuit")))
      P_MakeSuitPerm ();

    player->powers[power] = IRONTICS;
    return true;
  }

  if (power == pw_strength)
  {
      P_GiveBody (player, 100);
      player->powers[power] = 1;
      return true;
  }

  if (player->powers[power])
      return false;	// already got it

  player->powers[power] = 1;
  return true;
}

//-----------------------------------------------------------------------------

void P_NoMoreSectorDamage (sector_t* sector)
{
  int sp;

  sp = sector->special;
  switch (sp & 0x1F)
  {
    case 4:
    case 5:
    case 7:
    case 16:
      sp &= ~0x1F;
  }
  sector->special = sp & ~0x60;
}

//-----------------------------------------------------------------------------
//
// P_MakeSuitPerm
//
void P_MakeSuitPerm (void)
{
  int i;
  sector_t*	sector;

  i = numsectors;
  sector = &sectors [0];
  do
  {
    P_NoMoreSectorDamage (sector);
    sector++;
  } while (--i);
}


//-----------------------------------------------------------------------------
//
// P_TouchSpecialThing
//
void
P_TouchSpecialThing
( mobj_t*	special,
  mobj_t*	toucher )
{
  player_t*	player;
  int		i;
  fixed_t	delta;
  int		sound;
  unsigned int	qtyammogiven;
  char *	msg;

  delta = special->z - toucher->z;

  if (delta > toucher->height
      || delta < -8*FRACUNIT)
  {
      // out of reach
      return;
  }


  sound = sfx_itemup;
  player = toucher->player;

  // Dead thing touching.
  // Can happen with a sliding player corpse.
  if (toucher->health <= 0)
      return;

  // Identify by sprite.
  switch (special->sprite)
  {
      // armour
    case SPR_ARM1:
      if (!P_GiveArmour (player, Green_Armour_Class))
	  return;
      player->message = got_messages [P_GOTARMOUR];
      break;

    case SPR_ARM2:
      if (!P_GiveArmour (player, Blue_Armour_Class))
	  return;
      player->message = got_messages [P_GOTMEGA];
      break;

      // bonus items
    case SPR_BON1:
      player->health++;		// can go over 100%
      if (player->health > Max_Health_200)
	  player->health = Max_Health_200;
      player->mo->health = player->health;
      player->message = got_messages [P_GOTHTHBONUS];
      break;

    case SPR_BON2:
      player->armourpoints++;		// can go over 100%
      if (player->armourpoints > Max_Armour)
	  player->armourpoints = Max_Armour;
      if (player->armourtype == NOARMOUR)
	  player->armourtype = GREENARMOUR;
      player->message = got_messages [P_GOTARMBONUS];
      break;

    case SPR_SOUL:
      if (player -> health >= Max_Soulsphere_Health)
	return;
      player->health += Soulsphere_Health;
      if (player->health > Max_Soulsphere_Health)
	  player->health = Max_Soulsphere_Health;
      player->mo->health = player->health;
      player->message = got_messages [P_GOTSUPER];
      sound = sfx_getpow;
      break;

    case SPR_MEGA:
      {
	boolean mega_given;

	if (gamemode != commercial)
	  return;

	mega_given = P_GiveArmour (player, BLUEARMOUR);

	if (player->health < Megasphere_Health)
	{
	  player->health = Megasphere_Health;
	  player->mo->health = player->health;
	  mega_given = true;
	}

	if (mega_given == false)
	  return;

	player->message = got_messages [P_GOTMSPHERE];
	sound = sfx_getpow;
      }
      break;

      // cards
      // leave cards for everyone
    case SPR_BKEY:
      msg = P_GiveCard (player, it_bluecard);
      if (msg) player->message = msg;
      if (!netgame)
	  break;
      return;

    case SPR_YKEY:
      msg = P_GiveCard (player, it_yellowcard);
      if (msg) player->message = msg;
      if (!netgame)
	  break;
      return;

    case SPR_RKEY:
      msg = P_GiveCard (player, it_redcard);
      if (msg) player->message = msg;
      if (!netgame)
	  break;
      return;

    case SPR_BSKU:
      msg = P_GiveCard (player, it_blueskull);
      if (msg) player->message = msg;
      if (!netgame)
	  break;
      return;

    case SPR_YSKU:
      msg = P_GiveCard (player, it_yellowskull);
      if (msg) player->message = msg;
      if (!netgame)
	  break;
      return;

    case SPR_RSKU:
      msg = P_GiveCard (player, it_redskull);
      if (msg) player->message = msg;
      if (!netgame)
	  break;
      return;

      // medikits, heals
    case SPR_STIM:
      if (!P_GiveBody (player, 10))
	  return;
      player->message = got_messages [P_GOTSTIM];
      break;

    case SPR_MEDI:
      if (!P_GiveBody (player, 25))
	  return;

      if (player->health < 50)
	  player->message = got_messages [P_GOTMEDINEED];
      else
	  player->message = got_messages [P_GOTMEDIKIT];
      break;


      // power ups
    case SPR_PINV:
      if (!P_GivePower (player, pw_invulnerability))
	  return;
      player->message = got_messages [P_GOTINVUL];
      sound = sfx_getpow;
      break;

    case SPR_PSTR:
      if (!P_GivePower (player, pw_strength))
	  return;
      player->message = got_messages [P_GOTBERSERK];
      if (player->readyweapon != wp_fist)
      {
	if ((netgame)
	 || (M_CheckParm ("-noautoweaponchange") == 0))
	  player->pendingweapon = wp_fist;
      }
      sound = sfx_getpow;
      break;

    case SPR_PINS:
      if (!P_GivePower (player, pw_invisibility))
	  return;
      player->message = got_messages [P_GOTINVIS];
      sound = sfx_getpow;
      break;

    case SPR_SUIT:
      if (!P_GivePower (player, pw_ironfeet))
	  return;
      player->message = got_messages [P_GOTSUIT];
      sound = sfx_getpow;
      break;

    case SPR_PMAP:
      if (!P_GivePower (player, pw_allmap))
	  return;
      player->message = got_messages [P_GOTMAP];
      sound = sfx_getpow;
      break;

    case SPR_PVIS:
      if (!P_GivePower (player, pw_infrared))
	  return;
      player->message = got_messages [P_GOTVISOR];
      sound = sfx_getpow;
      break;

      // ammo
    case SPR_CLIP:
      if (special->flags & MF_DROPPED)
      {
	  if (!P_GiveAmmo (player,am_clip,0))
	      return;
      }
      else
      {
	  if (!P_GiveAmmo (player,am_clip,1))
	      return;
      }
      player->message = got_messages [P_GOTCLIP];
      break;

    case SPR_AMMO:
      if (!P_GiveAmmo (player, am_clip,5))
	  return;
      player->message = got_messages [P_GOTCLIPBOX];
      break;

    case SPR_ROCK:
      if (!P_GiveAmmo (player, am_misl,1))
	  return;
      player->message = got_messages [P_GOTROCKET];
      break;

    case SPR_BROK:
      if (!P_GiveAmmo (player, am_misl,5))
	  return;
      player->message = got_messages [P_GOTROCKBOX];
      break;

    case SPR_CELL:
      if (!P_GiveAmmo (player, am_cell,1))
	  return;
      player->message = got_messages [P_GOTCELL];
      break;

    case SPR_CELP:
      if (!P_GiveAmmo (player, am_cell,5))
	  return;
      player->message = got_messages [P_GOTCELLBOX];
      break;

    case SPR_SHEL:
      if ((qtyammogiven = P_GiveAmmo (player, am_shell,1)) == 0)
	  return;

      /* If we haven't changed the P_GOTSHELLS message then we */
      /* can use the new one with the correct quantity in. */

      msg = got_messages [P_GOTSHELLS];
      if (msg == got_messages_orig [P_GOTSHELLS])
      {
	if (qtyammogiven == 1)
	  player->message = got_messages [P_GOT1SHELL];
	else
	  player->message = HU_printf (got_messages [P_GOTNSHELLS], qtyammogiven);
      }
      else
      {
	player->message = msg;
      }
      break;

    case SPR_SBOX:
      if (!P_GiveAmmo (player, am_shell,5))
	  return;
      player->message = got_messages [P_GOTSHELLBOX];
      break;

    case SPR_BPAK:
      if (!player->backpack)
      {
	  for (i=0 ; i<NUMAMMO ; i++)
	      player->maxammo[i] *= 2;
	  player->backpack = true;
      }
      qtyammogiven = 0;
      for (i=0 ; i<NUMAMMO ; i++)
	  qtyammogiven += P_GiveAmmo (player, (ammotype_t) i, 1);
      if (qtyammogiven == 0)
	return;
      player->message = got_messages [P_GOTBACKPACK];
      break;

      // weapons
    case SPR_BFUG:
      if (!P_GiveWeapon (player, wp_bfg, false) )
	  return;
      player->message = got_messages [P_GOTBFG9000];
      sound = sfx_wpnup;
      break;

    case SPR_MGUN:
      if (!P_GiveWeapon (player, wp_chaingun, (boolean)(special->flags&MF_DROPPED)))
	  return;
      player->message = got_messages [P_GOTCHAINGUN];
      sound = sfx_wpnup;
      break;

    case SPR_CSAW:
      if (!P_GiveWeapon (player, wp_chainsaw, false) )
	  return;
      player->message = got_messages [P_GOTCHAINSAW];
      sound = sfx_wpnup;
      break;

    case SPR_LAUN:
      if (!P_GiveWeapon (player, wp_missile, false) )
	  return;
      player->message = got_messages [P_GOTLAUNCHER];
      sound = sfx_wpnup;
      break;

    case SPR_PLAS:
      if (!P_GiveWeapon (player, wp_plasma, false) )
	  return;
      player->message = got_messages [P_GOTPLASMA];
      sound = sfx_wpnup;
      break;

    case SPR_SHOT:
      if (!P_GiveWeapon (player, wp_shotgun, (boolean)(special->flags&MF_DROPPED)))
	  return;
      player->message = got_messages [P_GOTSHOTGUN];
      sound = sfx_wpnup;
      break;

    case SPR_SGN2:
      if (!P_GiveWeapon (player, wp_supershotgun, (boolean)(special->flags&MF_DROPPED)))
	  return;
      player->message = got_messages [P_GOTSHOTGUN2];
      sound = sfx_wpnup;
      break;

    default:
      if (M_CheckParm ("-showunknown"))
	fprintf (stderr, "P_SpecialThing: Unknown gettable thing %u\n", special->sprite);
      return;
  }

  if (special->flags & MF_COUNTITEM)
      player->itemcount++;
  P_RemoveMobj (special);
  player->bonuscount += BONUSADD;
  if (player == &players[consoleplayer])
      S_StartSound (NULL, sound);
}

//-----------------------------------------------------------------------------
/*
  The player names end in ": " which we don't want
*/

static int copy_player_name (char * dest, const char * name)
{
  int len;
  char cc;

  len = 0;

  do
  {
    cc = name [len];
    dest [len] = cc;
    len++;
  } while (cc);

  while ((--len) && (((cc = dest [len-1]) == ' ') || (cc == ':')));

  return (len);
}

//-----------------------------------------------------------------------------
/*
  %o: victim's name
  %k: killer's name
  %g: he/she/it, depending on the gender of the victim
  %h: him/her/it
  %p: his/her/its
*/

static char * write_obit (const char * s_msg, const char * monstername, mobj_t* killer, mobj_t* victim)
{
  char cc;
  int youshown;
  int gender;
  char * d_msg;
  char * obit_msg;

  gender = 0;
  if (victim->player)			// We already checked this!
    gender = player_genders [victim->player - players];

  obit_msg = d_msg = HU_printf (NULL);	// Get the static storage ptr
  youshown = 0;
  do
  {
    cc = *s_msg++;
    if (cc == '%')
    {
      switch (tolower(*s_msg++))
      {
	case 'g':
	  switch (gender)
	  {
	    case 0:
	      strcpy (d_msg, "he");
	      d_msg += 2;
	      break;
	    case 1:
	      strcpy (d_msg, "she");
	      d_msg += 3;
	      break;
	    default:
	      strcpy (d_msg, "it");
	      d_msg += 2;
	      break;
	  }
	  break;

	case 'h':
	  if (youshown)
	  {
	    strcpy (d_msg, "your");
	    d_msg += 4;
	  }
	  else
	  {
	    switch (gender)
	    {
	      case 0:
		strcpy (d_msg, "him");
		d_msg += 3;
		break;
	      case 1:
		strcpy (d_msg, "her");
		d_msg += 3;
		break;
	      default:
		strcpy (d_msg, "it");
		d_msg += 2;
		break;
	    }
	  }
	  break;

	case 'p':
	  switch (gender)
	  {
	    case 0:
	      strcpy (d_msg, "his");
	      break;
	    case 1:
	      strcpy (d_msg, "her");
	      break;
	    default:
	      strcpy (d_msg, "its");
	      break;
	  }
	  d_msg += 3;
	  break;

	case 'o':				// Victim's name
	  if (victim->player)			// We already checked this!
	  {
	    if (victim->player != &players[consoleplayer])
	    {
	      d_msg += copy_player_name (d_msg, player_names[victim->player - players]);
	    }
	    else
	    {
	      strcpy (d_msg, "You");
	      d_msg += 3;
	      if (strncasecmp (s_msg, " was", 4) == 0)
	      {
		s_msg += 4;
		strcpy (d_msg, " were");
		d_msg += 5;
	      }
	      youshown = 1;
	    }
	  }
	  break;

	case 'k':				// Killer's name
	  if ((killer)
	   && (killer->type == MT_PLAYER))
	  {
	    if (killer->player)			// Just in case!
	    {
	      if (killer->player != &players[consoleplayer])
	      {
		d_msg += copy_player_name (d_msg, player_names [killer->player - players]);
	      }
	      else if ((killer == victim) && (youshown))
	      {
		strcpy (d_msg, "yourself");
		d_msg += 8;
	      }
	      else
	      {
		strcpy (d_msg, "you");
		d_msg += 3;
	      }
	    }
	  }
	  else
	  {
	    *d_msg++ = 'a';
	    switch (tolower (monstername [0]))
	    {
	      case 0:
		return (0);

	      case 'a':
	      case 'e':
	      case 'i':
	      case 'o':
	      case 'u':
		*d_msg++ = 'n';
		break;
	    }
	    *d_msg++ = ' ';
	    strcpy (d_msg, monstername);
	    d_msg += strlen (monstername);
	  }
	  break;
      }
    }
    else
    {
      *d_msg++ = cc;
    }
  } while (cc);
#ifdef NORMALUNIX
  printf ("%s\n", obit_msg);
#endif
  return (obit_msg);
}

//-----------------------------------------------------------------------------

static char * find_obit_msg (mobj_t* killer, mobj_t* victim)
{
  unsigned int count;
  castinfo_t * cptr;
  obitinfo_t * obit_ptr;

  if (killer == victim)
    return (write_obit (obituary_messages[OB_KILLEDSELF], "self", killer, victim));

  cptr = castorder;
  do
  {
    if (killer -> type == cptr -> type)
      return (write_obit (cptr -> obit, cptr -> name, killer, victim));
    cptr++;
  } while (cptr -> obit);

  obit_ptr = obit_lookup;
  count = ARRAY_SIZE (obit_lookup);
  do
  {
    if (killer -> type == obit_ptr -> mt)
      return (write_obit (obituary_messages[obit_ptr -> ob], obit_ptr -> name, killer, victim));
    obit_ptr++;
  } while (--count);

  if (M_CheckParm ("-showunknown"))
    printf ("find_obit_msg: type = %u\n", killer -> type);
  return (0);
}

//-----------------------------------------------------------------------------

void P_AdjustKillCount (mobj_t* target, mobj_t* source, int amount)
{
  if (source && source->player)
  {
    // count for intermission
    // killough 7/20/98: don't count friends
    if ((target->flags & (MF_COUNTKILL|MF_FRIEND)) == MF_COUNTKILL)
    {
      if ((amount > 0)
       || (source->player->killcount))
	source->player->killcount += amount;
    }

    if (target->player)
    {
      if ((amount > 0)
       || (source->player->frags[target->player-players]))
	source->player->frags[target->player-players] += amount;
    }
  }
  else if (!netgame && ((target->flags & (MF_COUNTKILL|MF_FRIEND)) == MF_COUNTKILL))
  {
    // count all monster deaths,
    // even those caused by other monsters
    if ((amount > 0)
     || (players[0].killcount))
      players[0].killcount += amount;
  }
}

//-----------------------------------------------------------------------------
//
// KillMobj
//
static void
P_KillMobj
( mobj_t*	target,
  mobj_t*	inflictor,
  mobj_t*	source)
{
  int		th;
  int		flags;
  int		special;
  int		gibhealth;
  fixed_t	height;
  fixed_t	dropheight;
  char *	msg;
  mobj_t*	mo;
  mobjinfo_t*	info;
  item_to_drop_t * p;

  target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

  if (target->type != MT_SKULL)
    target->flags &= ~MF_NOGRAVITY;

  target->flags |= MF_CORPSE|MF_DROPOFF;
  height = target->height;
  target->height = height >> 2;

  flags = 0;  /* Assume nothing to be dropped */

  // Drop stuff.
  // This determines the kind of object(s) spawned
  // during the death frame of a thing.

  p = item_drop_head;
  while (p)
  {
    if (p -> just_died == target->type)
    {
      if (mobjinfo [p -> mt_spawn].flags & MF_COUNTKILL)
      {
	dropheight = ONFLOORZ;
      }
      else
      {
	/* Spawn the new object off the floor so */
	/* that it looks like it is falling. JAD 23/07/12. */
	dropheight = target -> z + (height>>1);
      }

      mo = P_SpawnMobj (target -> x, target -> y, dropheight, p -> mt_spawn);

      // special versions of items
      mo->flags = ((mo->flags & ~MF_FRIEND) | (target->flags & MF_FRIEND)) | MF_DROPPED;
      flags |= mo->flags;

      /* If we are in massacre mode then immediately kill the new */
      /* monster. Done this way so that Hitler without his armour in */
      /* the Wolfenstein original.wad will be killed and end the level. */

      if ((mo->flags & MF_COUNTKILL)
       && (target->flags2 & MF2_MASSACRE))
      {
	mo->flags2 |= MF2_MASSACRE;		// Infinite recursion hell here!!
	P_DamageMobj (mo, NULL, NULL, mo -> health);
      }
    }
    p = p -> next;
  }



  /* Assume that if the new object is killable then it's
     replacing an existing object, e.g. hitler without his
     armour in Wolfendoom */

  if ((flags & MF_COUNTKILL) == 0)
    P_AdjustKillCount (target, source, 1);

  if (target->player)
  {
    msg = NULL;

    if (!source)
    {
      // count environment kills against you
      target->player->frags[target->player-players]++;
      if (inflictor)
      {
	msg = find_obit_msg (inflictor, target);
      }
      /* No source and no inflictor. */
      /* Only happens on crushing and slime sectors. */
      else if (crushchange)
      {
	msg = write_obit (obituary_messages[OB_CRUSH], "ceiling", NULL, target);
      }
      else
      {
	special = target->subsector->sector->special;
	switch (special & 0x1F)
	{
	  case 4:
	  case 5:
	  case 7:
	  case 16:
	    msg = write_obit (obituary_messages[OB_LAVA], "lava", NULL, target);
	    break;

	  default:
	    if (special & 0x60)
	      msg = write_obit (obituary_messages[OB_LAVA], "lava", NULL, target);
	}
      }
    }
    else
    {
      msg = find_obit_msg (source, target);
    }

    if (msg)
    {
      /* We write to the console player here rather */
      /* than target->player->message so that in a  */
      /* netgame you will see if you shot another player */
      players[consoleplayer].message = msg;
    }

    target->flags &= ~MF_SOLID;
    target->player->playerstate = PST_DEAD;
    P_DropWeapon (target->player);

    if (target->player == &players[consoleplayer])
    {
      // don't die in auto map,
      // switch view prior to dying
      if (automapactive)
	AM_Stop ();
    }
  }

  th = target->health;
  target->health = -1;
  info = target->info;
  gibhealth = info->gibhealth;

  if ((gibhealth < 0)
   && (th < gibhealth)
   && (info->xdeathstate))
  {
    if (P_SetMobjState (target, (statenum_t)info->xdeathstate) == false)
      return;
  }
  else
  {
    if (P_SetMobjState (target, (statenum_t)info->deathstate) == false)
      return;
  }

  target->tics -= P_Random()&3;

  if (target->tics < 1)
      target->tics = 1;

  // I_StartSound (&actor->r, actor->info->deathsound);
}

//-----------------------------------------------------------------------------
//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
void
P_DamageMobj
( mobj_t*	target,
  mobj_t*	inflictor,
  mobj_t*	source,
  int 		damage )
{
  unsigned	ang;
  int		saved;
  player_t*	player;
  fixed_t	thrust;
  int		temp;
  int		damagecount;
  boolean	justhit;	  // killough 11/98

  if (gamecompletedtimer)
    return;

  if ( !(target->flags & MF_SHOOTABLE) )
      return;	// shouldn't happen...

  if (target->health <= 0)
      return;

  if ( target->flags & MF_SKULLFLY )
  {
      target->momx = target->momy = target->momz = 0;
  }

  player = target->player;
  if (player)
  {
    if (gameskill == sk_baby)
      damage >>= 1;		// take half damage in trainer mode
  }
  else if ((Give_Max_Damage)
	&& (source)
	&& ((source->player) || (source->flags & MF_FRIEND))
	&& ((target->flags & MF_FRIEND) == 0)
	&& (damage < target->health))
  {
    damage = target->health;
  }

  if (damage == 0)		// It's possible for the /2 above to
    return;			// yeild zero if there was only 1% damage
				// but we'll do the test down here to be safe.

  // Some close combat weapons should not
  // inflict thrust and push the victim out of reach,
  // thus kick away unless using the chainsaw.
  if (inflictor
      && !(target->flags & MF_NOCLIP)
      && (!source
	  || !source->player
	  || source->player->readyweapon != wp_chainsaw))
  {
      ang = R_PointToAngle2 ( inflictor->x,
			      inflictor->y,
			      target->x,
			      target->y);

      thrust = damage*(FRACUNIT>>3)*100/target->info->mass;

      // make fall forwards sometimes
      if ( damage < 40
	   && damage > target->health
	   && target->z - inflictor->z > 64*FRACUNIT
	   && (P_Random ()&1) )
      {
	  ang += ANG180;
	  thrust *= 4;
      }

      ang >>= ANGLETOFINESHIFT;
      target->momx += FixedMul (thrust, finecosine[ang]);
      target->momy += FixedMul (thrust, finesine[ang]);
  }

  // player specific
  if (player)
  {
      // end of game hell hack
      if ((target->subsector->sector->special & 0x1F) == 11
	  && damage >= target->health)
      {
	  damage = target->health - 1;
      }

      // Below certain threshold,
      // ignore damage in GOD mode, or with INVUL power.
      if ( /*damage < 1000
	   && */ ( (player->cheats&CF_GODMODE)
		|| player->powers[pw_invulnerability] ) )
      {
	  return;
      }

      if (player->armourtype != NOARMOUR)
      {
	  if (player->armourtype == GREENARMOUR)
	      saved = damage/3;
	  else
	      saved = damage/2;

	  if (player->armourpoints <= saved)
	  {
	      // armour is used up
	      saved = player->armourpoints;
	      player->armourtype = NOARMOUR;
	  }
	  player->armourpoints -= saved;
	  damage -= saved;
      }
      player->health -= damage; 	// mirror mobj health here for Dave
      if (player->health < 0)
	  player->health = 0;

      player->attacker = source;
      damagecount = player->damagecount + damage; // add damage after armour / invuln

      if ((damage > 0) && (damagecount < 2))	// damagecount gets decremented before
	damagecount = 2;			// being used so needs to be at least 2.
      if (damagecount > 100)
	damagecount = 100;			// teleport stomp does 10k points...

      player->damagecount = damagecount;

      temp = damage < 100 ? damage : 100;

      if (player == &players[consoleplayer])
	  I_Tactile (40,10,40+temp*2);
  }

  // do the damage
  if ((target->health -= damage) <= 0)
  {
      P_KillMobj (target, inflictor, source);
      return;
  }

  justhit = false;

  if ( (P_Random () < target->info->painchance)
       && !(target->flags&MF_SKULLFLY) )
  {
      justhit = true;

      if (P_SetMobjState (target, (statenum_t)target->info->painstate) == false)
	return;
  }

  target->reactiontime = 0;		// we're awake now...

  // killough 9/9/98: cleaned up, made more consistent:

  if (source && source != target && source->type != MT_VILE &&
      (!target->threshold || target->type == MT_VILE) &&
      ((source->flags ^ target->flags) & MF_FRIEND ||
       Monsters_Infight1 || Monsters_Infight2))
    {
      // if not intent on another player, chase after this one
      //
      // killough 2/15/98: remember last enemy, to prevent
      // sleeping early; 2/21/98: Place priority on players
      // killough 9/9/98: cleaned up, made more consistent:
#if 0
      if (!target->lastenemy || target->lastenemy->health <= 0 ||
	  (demo_version < 203 ? !target->lastenemy->player :
	   !((target->flags ^ target->lastenemy->flags) & MF_FRIEND) &&
	   target->target != source)) // remember last enemy - killough
	P_SetTarget(&target->lastenemy, target->target);
#endif

      target->target = source;		// killough 11/98
      target->threshold = BASETHRESHOLD;
      if (target->state == &states[target->info->spawnstate]
	  && target->info->seestate != S_NULL)
	P_SetMobjState (target, (statenum_t) target->info->seestate);
    }

  // killough 11/98: Don't attack a friend, unless hit by that friend.
  if (justhit && (target->target == source || !target->target ||
		  !(target->flags & target->target->flags & MF_FRIEND)))
    target->flags |= MF_JUSTHIT;	// fight back!
}

//-----------------------------------------------------------------------------
//
// P_Massacre : Bump off all the baddies....
//

void P_Massacre (void)
{
  thinker_t*		th;
  mobj_t*		mobj;

  for (th = thinker_head ; th != NULL ; th=th->next)
  {
    if (th->function.acp1 == (actionf_p1)P_MobjThinker)
    {
      mobj = (mobj_t *) th;
      if ((mobj->player == 0)
       && ((mobj->flags & (MF_SHOOTABLE|MF_FRIEND)) == MF_SHOOTABLE))
      {
	mobj->flags2 |= MF2_MASSACRE;
	P_DamageMobj (mobj, NULL, NULL, mobj -> health);
      }
    }
  }
}

//-----------------------------------------------------------------------------
