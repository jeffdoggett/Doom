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
// DESCRIPTION:  none
//	Implements special effects:
//	Texture animation, height or lighting changes
//	 according to adjacent sectors, respective
//	 utility functions, etc.
//
//-----------------------------------------------------------------------------


#ifndef __P_SPEC__
#define __P_SPEC__


/* p_floor */

#define ELEVATORSPEED		(FRACUNIT*4)
#define FLOORSPEED		FRACUNIT

/* p_ceilng */

#define CEILSPEED		FRACUNIT
#define CEILWAIT		150

/* p_doors */

#define VDOORWAIT		150
#define VDOORSPEED		(FRACUNIT*2)

/* p_plats */

#define PLATWAIT		3
#define PLATSPEED		FRACUNIT


/* p_lights */

#define GLOWSPEED		8
#define STROBEBRIGHT		5
#define FASTDARK		15
#define SLOWDARK		35

/*jff 3/14/98 add bits and shifts for generalized sector types */

#define DAMAGE_MASK		0x60
#define DAMAGE_SHIFT		5
#define SECRET_MASK		0x80
#define SECRET_SHIFT		7
#define FRICTION_MASK		0x100
#define FRICTION_SHIFT		8
#define PUSH_MASK		0x200
#define PUSH_SHIFT		9

/*jff 02/04/98 Define masks, shifts, for fields in */
/* generalized linedef types */

#define GenFloorBase		0x6000
#define GenCeilingBase		0x4000
#define GenDoorBase		0x3c00
#define GenLockedBase		0x3800
#define GenLiftBase		0x3400
#define GenStairsBase		0x3000
#define GenCrusherBase		0x2F80

#define TriggerType		0x0007
#define TriggerTypeShift	0

/* define masks and shifts for the floor type fields */

#define FloorCrush		0x1000
#define FloorChange		0x0c00
#define FloorTarget		0x0380
#define FloorDirection		0x0040
#define FloorModel		0x0020
#define FloorSpeed		0x0018

#define FloorCrushShift		12
#define FloorChangeShift	10
#define FloorTargetShift	7
#define FloorDirectionShift	6
#define FloorModelShift	5
#define FloorSpeedShift	3

/* define masks and shifts for the ceiling type fields */

#define CeilingCrush		0x1000
#define CeilingChange		0x0c00
#define CeilingTarget		0x0380
#define CeilingDirection	0x0040
#define CeilingModel		0x0020
#define CeilingSpeed		0x0018

#define CeilingCrushShift	12
#define CeilingChangeShift	10
#define CeilingTargetShift	7
#define CeilingDirectionShift	6
#define CeilingModelShift	5
#define CeilingSpeedShift	3

/* define masks and shifts for the lift type fields */

#define LiftTarget		0x0300
#define LiftDelay		0x00c0
#define LiftMonster		0x0020
#define LiftSpeed		0x0018

#define LiftTargetShift		8
#define LiftDelayShift		6
#define LiftMonsterShift	5
#define LiftSpeedShift		3

/* define masks and shifts for the stairs type fields */

#define StairIgnore		0x0200
#define StairDirection		0x0100
#define StairStep		0x00c0
#define StairMonster		0x0020
#define StairSpeed		0x0018

#define StairIgnoreShift	9
#define StairDirectionShift	8
#define StairStepShift		6
#define StairMonsterShift	5
#define StairSpeedShift		3

/* define masks and shifts for the crusher type fields */

#define CrusherSilent		0x0040
#define CrusherMonster		0x0020
#define CrusherSpeed		0x0018

#define CrusherSilentShift	6
#define CrusherMonsterShift	5
#define CrusherSpeedShift	3

/* define masks and shifts for the door type fields */

#define DoorDelay		0x0300
#define DoorMonster		0x0080
#define DoorKind		0x0060
#define DoorSpeed		0x0018

#define DoorDelayShift		8
#define DoorMonsterShift	7
#define DoorKindShift		5
#define DoorSpeedShift		3

/* define masks and shifts for the locked door type fields */

#define LockedNKeys		0x0200
#define LockedSkullOrCard	0x0200	// Same bit as above!
#define LockedKey		0x01c0
#define LockedKind		0x0020
#define LockedSpeed		0x0018

#define LockedNKeysShift	9
#define LockedKeyShift		6
#define LockedKindShift		5
#define LockedSpeedShift	3

#define genshift(t,a,s)		((t>>s)&(a>>s))

/* define names for the TriggerType field of the general linedefs */

typedef enum
{
  WalkOnce,
  WalkMany,
  SwitchOnce,
  SwitchMany,
  GunOnce,
  GunMany,
  PushOnce,
  PushMany
} triggertype_e;

typedef enum
{
  WalkedOver,
  Switched,
  Gunned,
  Pressed
} triggered_e;

/* define names for the Speed field of the general linedefs */

typedef enum
{
  SpeedSlow,
  SpeedNormal,
  SpeedFast,
  SpeedTurbo
} motionspeed_e;

/* define names for the Target field of the general floor */

typedef enum
{
  FtoHnF,
  FtoLnF,
  FtoNnF,
  FtoLnC,
  FtoC,
  FbyST,
  Fby24,
  Fby32
} floortarget_e;

/* define names for the Changer Type field of the general floor */

typedef enum
{
  FNoChg,
  FChgZero,
  FChgTxt,
  FChgTyp
} floorchange_e;

/* define names for the Change Model field of the general floor */

typedef enum
{
  FTriggerModel,
  FNumericModel
} floormodel_t;

/* define names for the Target field of the general ceiling */

typedef enum
{
  CtoHnC,
  CtoLnC,
  CtoNnC,
  CtoHnF,
  CtoF,
  CbyST,
  Cby24,
  Cby32
} ceilingtarget_e;

/* define names for the Changer Type field of the general ceiling */

typedef enum
{
  CNoChg,
  CChgZero,
  CChgTxt,
  CChgTyp
} ceilingchange_e;

/* define names for the Change Model field of the general ceiling */

typedef enum
{
  CTriggerModel,
  CNumericModel
} ceilingmodel_t;

/* define names for the Target field of the general lift */

typedef enum
{
  F2LnF,
  F2NnF,
  F2LnC,
  LnF2HnF
} lifttarget_e;

/* define names for the door Kind field of the general ceiling */

typedef enum
{
  OdCDoor,
  ODoor,
  CdODoor,
  CDoor
} doorkind_e;

/* define names for the locked door Kind field of the general ceiling */

typedef enum
{
  AnyKey,
  RCard,
  BCard,
  YCard,
  RSkull,
  BSkull,
  YSkull,
  AllKeys
} keykind_e;


/* enums for classes of linedef triggers */

/*jff 2/23/98 identify the special classes that can share sectors */
#if 0
typedef enum
{
  floor_special,
  ceiling_special,
  lighting_special
} special_e;
#endif

/*jff 3/15/98 pure texture/type change for better generalized support */
typedef enum
{
  trigChangeOnly,
  numChangeOnly
} change_e;

/* p_floor */

typedef enum
{
  elevateUp,
  elevateDown,
  elevateCurrent
} elevator_e;


/* general enums */


/* linedef and sector special data types */

/* p_floor */

typedef struct
{
  thinker_t thinker;
  elevator_e type;
  sector_t* sector;
  int direction;
  fixed_t floordestheight;
  fixed_t ceilingdestheight;
  fixed_t speed;
} elevator_t;

/* p_spec */

/* killough 3/7/98: Add generalized scroll effects */

typedef enum
{
  sc_side,
  sc_floor,
  sc_ceiling,
  sc_carry,
  sc_carry_ceiling	/* killough 4/11/98: carry objects hanging on ceilings */
} scrolltype_e;


typedef struct
{
  thinker_t thinker;	/* Thinker structure for scrolling */
  fixed_t dx, dy;	/* (dx,dy) scroll speeds */
  int affectee;		/* Number of affected sidedef, sector, tag, or whatever */
  int control;		/* Control sector (-1 if none) used to control scrolling */
  fixed_t last_height;	/* Last known height of control sector */
  fixed_t vdx, vdy;	/* Accumulated velocity if accelerative */
  int accel;		/* Whether it's accelerative */
  scrolltype_e type;	/* Type of scroll effect */
} scroll_t;

/* phares 3/12/98: added new model of friction for ice/sludge effects */

typedef struct
{
  thinker_t thinker;	/* Thinker structure for friction */
  int friction;		/* friction value (E800 = normal) */
  int movefactor;	/* inertia factor when adding to momentum */
  int affectee;		/* Number of affected sector */
} friction_t;

/* phares 3/20/98: added new model of Pushers for push/pull effects */

typedef struct
{
  thinker_t thinker;	/* Thinker structure for Pusher */
  enum
  {
    p_push,
    p_pull,
    p_wind,
    p_current
  } type;
  mobj_t* source;	/* Point source if point pusher */
  int x_mag;		/* X Strength */
  int y_mag;		/* Y Strength */
  int magnitude;	/* Vector strength for point pusher */
  int radius;		/* Effective radius for point pusher */
  int x;		/* X of point source if point pusher */
  int y;		/* Y of point source if point pusher */
  int affectee;		/* Number of affected sector */
} pusher_t;

void P_CalcHeight
( player_t *player );

fixed_t P_FindNextLowestFloor
( const sector_t* sec,
  int currentheight );

fixed_t P_FindNextLowestCeiling
( const sector_t *sec,
  int currentheight );	/* jff 2/04/98 */

fixed_t P_FindNextHighestCeiling
( const sector_t *sec,
  int currentheight );	/* jff 2/04/98 */

fixed_t P_FindShortestTextureAround
( int secnum );		/* jff 2/04/98 */

fixed_t P_FindShortestUpperAround
( int secnum );		/* jff 2/04/98 */

sector_t* P_FindModelFloorSector
( fixed_t floordestheight,
  int secnum );		/*jff 02/04/98 */

sector_t* P_FindModelCeilingSector
( fixed_t ceildestheight,
  int secnum );		/*jff 02/04/98 */

int P_FindLineFromTag
( int tag,
  int start );		/* killough 4/17/98 */

int P_CheckTag
(const line_t *line);	/* jff 2/27/98 */

boolean P_MonsterCanOperate (unsigned int special, triggered_e trigtype);

boolean P_CanUnlockGenDoor
( const line_t* line,
  player_t* player);

#if 0
int P_SectorActive
( special_e t,
  const sector_t* s );

boolean P_IsSecret
( const sector_t *sec );

boolean P_WasSecret
( const sector_t *sec );

#endif

/* killough 2/14/98: Add silent teleporter */
int EV_SilentTeleport
( const line_t* line,
  int side,
  mobj_t* thing );

/* killough 1/31/98: Add silent line teleporter */
int EV_SilentLineTeleport
( const line_t* line,
  int side,
  mobj_t* thing,
  boolean reverse);

mobj_t* P_GetPushThing(int);                                /* phares 3/23/98 */


int EV_DoGenFloor
( const line_t* line );

int EV_DoGenCeiling
( const line_t* line );

int EV_DoGenLift
( const line_t* line );

/* No const line_t* */
int EV_DoGenStairs
( line_t* line );

int EV_DoGenCrusher
( const line_t* line );

int EV_DoChange
( line_t* line,
  change_e changetype );


//
// End-level timer (-TIMER option)
//
extern	boolean levelTimer;
extern	int	levelTimeCount;


//      Define values for map objects
#define MO_TELEPORTMAN          14


// at game start
void    P_InitPicAnims (void);

// at map load
void    P_SpawnSpecials (void);

// every tic
void    P_UpdateSpecials (void);

// when needed
boolean
P_UseSpecialLine
( mobj_t*	thing,
  line_t*	line,
  int		side );

boolean
P_ShootSpecialLine
( mobj_t*	thing,
  line_t*	line );

boolean
P_CrossSpecialLine
(
  line_t*	line,
  int		side,
  mobj_t*	thing );

boolean
P_BoomSpecialLine
( mobj_t*	thing,
  line_t*	line,
  int		side,
  triggered_e	trigtype);


void    P_PlayerInSpecialSector (player_t* player);

int
twoSided
( int		sector,
  int		line );

sector_t*
getSector
( int		currentSector,
  int		line,
  int		side );

side_t*
getSide
( int		currentSector,
  int		line,
  int		side );

fixed_t P_FindLowestFloorSurrounding(sector_t* sec);
fixed_t P_FindHighestFloorSurrounding(sector_t* sec);

fixed_t
P_FindNextHighestFloor
( sector_t*	sec,
  int		currentheight );

fixed_t P_FindLowestCeilingSurrounding(sector_t* sec);
fixed_t P_FindHighestCeilingSurrounding(sector_t* sec);

int
P_FindSectorFromLineTag
( line_t*	line,
  int		start );

int
P_FindMinSurroundingLight
( sector_t*	sector,
  int		max );

int
P_FindMaxSurroundingLight
( sector_t*	sector,
  int		min );

sector_t*
getNextSector
( line_t*	line,
  sector_t*	sec );


//
// SPECIAL
//
int EV_DoDonut(line_t* line);



//
// P_LIGHTS
//
typedef struct
{
    thinker_t	thinker;
    sector_t*	sector;
    int		count;
    int		maxlight;
    int		minlight;

} fireflicker_t;



typedef struct
{
    thinker_t	thinker;
    sector_t*	sector;
    int		count;
    int		maxlight;
    int		minlight;
    int		maxtime;
    int		mintime;

} lightflash_t;



typedef struct
{
    thinker_t	thinker;
    sector_t*	sector;
    int		count;
    int		minlight;
    int		maxlight;
    int		darktime;
    int		brighttime;

} strobe_t;




typedef struct
{
    thinker_t	thinker;
    sector_t*	sector;
    int		minlight;
    int		maxlight;
    int		direction;

} glow_t;


void    P_SpawnFireFlicker (sector_t* sector);
void    T_LightFlash (lightflash_t* flash);
void    P_SpawnLightFlash (sector_t* sector);
void    T_StrobeFlash (strobe_t* flash);
void    P_SpawnLightChase (sector_t* sector);
void    P_SpawnLightPhased (sector_t* sector);
void	P_SpawnStrobeFlash (sector_t* sector, int fastOrSlow, int inSync, int RemoveSpecial);
void    EV_StartLightStrobing(line_t* line);
void    EV_TurnTagLightsOff(line_t* line);
void	EV_LightTurnOn (line_t* line, int bright);
void	P_AdjustDoorLight (line_t* line, fixed_t fraction);
void    T_Glow(glow_t* g);
void    P_SpawnGlowingLight(sector_t* sector);



//
// P_SWITCH
//


typedef enum
{
    nowhere = -1,
    top,
    middle,
    bottom
} bwhere_e;


typedef struct
{
    thinker_t	thinker;
    line_t*	line;
    bwhere_e	where;
    int		btexture;
    int		btimer;
    mobj_t*	soundorg;
} button_t;


extern void P_StartButton (line_t* line, bwhere_e w, int texture, int time);
extern void T_Button (button_t* button);

 // 1 second, in ticks.
#define BUTTONTIME      35


void
P_ChangeSwitchTexture
( line_t*	line,
  int		useAgain );

void P_InitSwitchList(void);
void P_PatchSwitchList (const char * patchline);

//
// P_PLATS
//
typedef enum
{
    up,
    down,
    waiting,
    in_stasis
} plat_e;



typedef enum
{
    perpetualRaise,
    downWaitUpStay,
    raiseAndChange,
    raiseToNearestAndChange,
    blazeDWUS,
    genLift,
    genPerpetual,
    toggleUpDn,
    downToLowestCeiling,
    nextLowestNeighbourFloor,
    raiseAndChange24,
    raiseAndChange32
} plattype_e;



typedef struct plat_s
{
    thinker_t	thinker;
    struct plat_s* next;
    struct plat_s* prev;
    sector_t*	sector;
    fixed_t	speed;
    fixed_t	low;
    fixed_t	high;
    int		wait;
    int		count;
    plat_e	status;
    plat_e	oldstatus;
    boolean	crush;
    int		tag;
    plattype_e	type;
#ifdef BOOM
    int		activeidx;
#endif
} plat_t;




extern plat_t*	activeplatshead;

void    T_PlatRaise(plat_t*	plat);

int
EV_DoPlat
( line_t*	line,
  plattype_e	type);

void    P_AddActivePlat(plat_t* plat);
void    P_RemoveActivePlat(plat_t* plat);
void    EV_StopPlat(line_t* line);


//
// P_DOORS
//
typedef enum
{
    normal,
    close30ThenOpen,
    normalClose,
    normalOpen,
    raiseIn5Mins,
    blazeRaise,
    blazeOpen,
    blazeClose,
    closeThenOpen
} vldoor_e;



typedef struct
{
    thinker_t	thinker;
    vldoor_e	type;
    sector_t*	sector;
    fixed_t	topheight;
    fixed_t	speed;

    // 1 = up, 0 = waiting at top, -1 = down
    int         direction;

    // tics to wait at the top
    int         topwait;
    // (keep in case a door going down is reset)
    // when it reaches 0, start going down
    int         topcountdown;

    line_t*	line;
} vldoor_t;


void
EV_VerticalDoor
( line_t*	line,
  mobj_t*	thing );

int
EV_DoDoor
( line_t*	line,
  vldoor_e	type );

int
EV_DoDoorR
( line_t*	line,
  vldoor_e	type,
  mobj_t*	thing );

int
EV_DoLockedDoor
( line_t*	line,
  int		keynum,
  vldoor_e	type,
  mobj_t*	thing );

int EV_DoGenLockedDoor
( line_t*	line,
  int		keynum,
  vldoor_e	type,
  mobj_t*	thing );

int
EV_DoGenDoor
( line_t*	line,
  vldoor_e	type,
  mobj_t*	thing );

void    T_VerticalDoor (vldoor_t* door);
void    P_SpawnDoorCloseIn30 (sector_t* sec);
int	EV_Check_Lock (player_t * p, int keynum);

void
P_SpawnDoorRaiseIn5Mins
( sector_t*	sec,
  int		secnum );



#if 0 // UNUSED
//
//      Sliding doors...
//
typedef enum
{
    sd_opening,
    sd_waiting,
    sd_closing

} sd_e;



typedef enum
{
    sdt_openOnly,
    sdt_closeOnly,
    sdt_openAndClose

} sdt_e;




typedef struct
{
    thinker_t	thinker;
    sdt_e	type;
    line_t*	line;
    int		frame;
    int		whichDoorIndex;
    int		timer;
    sector_t*	frontsector;
    sector_t*	backsector;
    sd_e	 status;

} slidedoor_t;



typedef struct
{
    char	frontFrame1[9];
    char	frontFrame2[9];
    char	frontFrame3[9];
    char	frontFrame4[9];
    char	backFrame1[9];
    char	backFrame2[9];
    char	backFrame3[9];
    char	backFrame4[9];

} slidename_t;



typedef struct
{
    int             frontFrames[4];
    int             backFrames[4];

} slideframe_t;



// how many frames of animation
#define SNUMFRAMES		4

#define SDOORWAIT		35*3
#define SWAITTICS		4

// how many diff. types of anims
#define MAXSLIDEDOORS	5

void P_InitSlidingDoorFrames(void);

void
EV_SlidingDoor
( line_t*	line,
  mobj_t*	thing );
#endif



//
// P_CEILNG
//
typedef enum
{
    lowerToFloor,
    raiseToHighest,
    lowerAndCrush,
    crushAndRaise,
    fastCrushAndRaise,
    silentCrushAndRaise,
    lowerToLowest,
    lowerToMaxFloor
} ceiling_e;



typedef struct ceiling_s
{
    thinker_t	thinker;
    struct ceiling_s* next;
    struct ceiling_s* prev;
    ceiling_e	type;
    sector_t*	sector;
    fixed_t	bottomheight;
    fixed_t	topheight;
    fixed_t	speed;
    boolean	crush;

    // 1 = up, 0 = waiting, -1 = down
    int		direction;

    // ID
    int		tag;
//  int		olddirection;

    sector_t *	secspecial;
#ifdef BOOM
    fixed_t     oldspeed;
    int         newspecial;
    int         oldspecial;
    int		texture;
    int		activeidx;
#endif
} ceiling_t;


extern ceiling_t*	activeceilingshead;

int
EV_DoCeiling
( line_t*	line,
  ceiling_e	type );

void    T_MoveCeiling (ceiling_t* ceiling);
void    P_AddActiveCeiling(ceiling_t* c);
void    P_RemoveActiveCeiling(ceiling_t* c);
int	EV_CeilingCrushStop(line_t* line);
ceiling_t* P_NewCeilingAction (sector_t* sec, ceiling_e type);


//
// P_FLOOR
//
typedef enum
{
    // lower floor to highest surrounding floor
    lowerFloor,

    // lower floor to lowest surrounding floor
    lowerFloorToLowest,

    // raise floor to highest surrounding floor
    raiseFloorToHighest,

    // lower floor to highest surrounding floor VERY FAST
    turboLower,

    // raise floor to lowest surrounding CEILING
    raiseFloor,

    // raise floor to next highest surrounding floor
    raiseFloorToNearest,

    // raise floor to shortest height texture around it
    raiseToTexture,

    // lower floor to lowest surrounding floor
    //  and change floorpic
    lowerAndChange,

    raiseFloor24,
    changeAndRaiseFloor24,
    raiseFloorCrush,

     // raise to next highest floor, turbo-speed
    raiseFloorTurbo,
    donutRaise,
    raiseFloor512,
    lowerFloorToNearest,
    lowerFloor24,
    lowerFloor32Turbo,
    raiseFloor32Turbo,
    raiseToCeiling
} floor_e;




typedef enum
{
  stairslowup4 = ((0<<StairSpeedShift) | (0<<StairStepShift) | StairDirection),
  stairnormalup4 = ((1<<StairSpeedShift) | (0<<StairStepShift) | StairDirection),
  stairfastup4 = ((2<<StairSpeedShift) | (0<<StairStepShift) | StairDirection),
  stairturboup4 = ((3<<StairSpeedShift) | (0<<StairStepShift) | StairDirection),
  stairslowup8 = ((0<<StairSpeedShift) | (1<<StairStepShift) | StairDirection),
  stairnormalup8 = ((1<<StairSpeedShift) | (1<<StairStepShift) | StairDirection),
  stairfastup8 = ((2<<StairSpeedShift) | (1<<StairStepShift) | StairDirection),
  stairturboup8 = ((3<<StairSpeedShift) | (1<<StairStepShift) | StairDirection),
  stairslowup16 = ((0<<StairSpeedShift) | (2<<StairStepShift) | StairDirection),
  stairnormalup16 = ((1<<StairSpeedShift) | (2<<StairStepShift) | StairDirection),
  stairfastup16 = ((2<<StairSpeedShift) | (2<<StairStepShift) | StairDirection),
  stairturboup16 = ((3<<StairSpeedShift) | (2<<StairStepShift) | StairDirection),
  stairslowup24 = ((0<<StairSpeedShift) | (3<<StairStepShift) | StairDirection),
  stairnormalup24 = ((1<<StairSpeedShift) | (3<<StairStepShift) | StairDirection),
  stairfastup24 = ((2<<StairSpeedShift) | (3<<StairStepShift) | StairDirection),
  stairturboup24 = ((3<<StairSpeedShift) | (3<<StairStepShift) | StairDirection)
} stair_e;



typedef struct
{
    thinker_t	thinker;
//  floor_e	type;			// No longer required
    boolean	crush;
    sector_t*	sector;
    int		direction;
    int		newspecial;
    int		newtexture;
    fixed_t	floordestheight;
    fixed_t	speed;
} floormove_t;



typedef enum
{
    ok,
    crushed,
    pastdest

} result_e;

result_e T_MoveFloorPlane (sector_t* sector, fixed_t speed, fixed_t dest, boolean crush, int direction);
result_e T_MoveCeilingPlane (sector_t* sector, fixed_t speed, fixed_t dest, boolean crush,int direction);

int EV_BuildStairs (line_t* line, stair_e buildtype);
int EV_DoFloor (line_t* line, floor_e floortype);
void T_MoveFloor(floormove_t* floor);
void EV_DoGoobers (void);


//
// P_TELEPT
//
int EV_Teleport (line_t* line, int side, mobj_t* thing);

/* special action function prototypes */

void T_LightFlash (lightflash_t* flash);
void T_StrobeFlash (strobe_t* flash);
void T_FireFlicker (fireflicker_t* flick);
void T_Glow (glow_t* g);
void T_PlatRaise (plat_t* plat);
void T_VerticalDoor (vldoor_t* door);
void T_MoveCeiling (ceiling_t* ceiling);
void T_MoveFloor (floormove_t* floor);
void T_MoveElevator (elevator_t* elevator);
void T_Scroll (scroll_t *);
void T_Friction (friction_t *);
void T_Pusher (pusher_t *);

int EV_DoElevator ( line_t* line, elevator_e type );


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
