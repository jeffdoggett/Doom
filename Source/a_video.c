/* Acorn Screen drivers
   Written by J.A.Doggett.

   Based on i_video.c so some of the comments herein are ID's
*/

#include "includes.h"
#include <kernel.h>


#define OS_SpriteOp			0x2E
#define OS_ReadVduVariables		0x31
#define OS_SWINumberFromString		0x39
#define OS_SetColour			0x61
#define OS_ScreenMode			0x65
#define ColourTrans_ReturnColourNumber	0x40744
#define ColourTrans_WritePalette	0x4075D


// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int      screen_mode = 1;
static int	vsync;
static unsigned int * screen_addr;
static unsigned int   palette_table [256];
static unsigned int   bgr_palette_table [256];

static unsigned int   rpc_palette;  /* = True when using 8 bit screen mode and */
				    /* RPC palette is available (256 entries) */
static unsigned int   write_via_sprite = 0;

static unsigned char devparm_black;
static unsigned char devparm_white;
static unsigned int  devparm_palette = 0;

unsigned int SCREENWIDTH = 320;
unsigned int SCREENHEIGHT = 200;
unsigned int ORIGSCREENWIDTH = 320;	// in original Doom
unsigned int ORIGSCREENHEIGHT = 200;

/* -------------------------------------------------------------------------- */

typedef struct
{
  unsigned int area_size;
  unsigned int qty_sprites;
  unsigned int first_sprite;
  unsigned int next_free;
  unsigned int sprite_size;
  char sprite_name [12];
  unsigned int width;
  unsigned int height;
  unsigned int first_bit;
  unsigned int last_bit;
  unsigned int offset;
  unsigned int offset2;
} sprite_area_t;

static sprite_area_t * sprite_mem = 0;

/* -------------------------------------------------------------------------- */
// screen_mode
#define MODE_USER_DEFINED	0

#define MODE_320_x_200_8bpp	1
#define MODE_320_x_240_8bpp	2
#define MODE_320_x_256_8bpp	3
#define MODE_320_x_400_8bpp	4
#define MODE_320_x_480_8bpp	5
#define MODE_640_x_400_8bpp	6
#define MODE_640_x_480_8bpp	7

#define MODE_320_x_200_16bpp	8
#define MODE_320_x_240_16bpp	9
#define MODE_320_x_256_16bpp	10
#define MODE_320_x_400_16bpp	11
#define MODE_320_x_480_16bpp	12
#define MODE_640_x_400_16bpp	13
#define MODE_640_x_480_16bpp	14

#define MODE_320_x_200_24bpp	15
#define MODE_320_x_240_24bpp	16
#define MODE_320_x_256_24bpp	17
#define MODE_320_x_400_24bpp	18
#define MODE_320_x_480_24bpp	19
#define MODE_640_x_400_24bpp	20
#define MODE_640_x_480_24bpp	21

#define MODE_800_x_600_8bpp	22
#define MODE_800_x_600_16bpp	23
#define MODE_800_x_600_24bpp	24

#define MODE_1024_x_768_8bpp	25
#define MODE_1024_x_768_16bpp	26
#define MODE_1024_x_768_24bpp	27

#define MODE_1280_x_960_8bpp	28
#define MODE_1280_x_960_16bpp	29
#define MODE_1280_x_960_24bpp	30

#define MODE_1280_x_1024_8bpp	31
#define MODE_1280_x_1024_16bpp	32
#define MODE_1280_x_1024_24bpp	33

/* -------------------------------------------------------------------------- */

static unsigned int screen_mode_block [] =
{
  1,
  640, 480,
  3,		/* 8bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

static unsigned int screen_user_def [] =
{
  1,
  0, 0,		/* X, Y */
  0,		/* bpp */
  -1,		/* framerate - use first found */
  0,0x80,
  3,0xFF,
  -1		/* End of table */
};

/* -------------------------------------------------------------------------- */

static void construct_screen_mode_block (int mode_num)
{
  screen_mode_block [0] = 1;
  screen_mode_block [4] = -1;	// Framerate

  switch (mode_num)
  {
    case MODE_USER_DEFINED:
      memcpy (screen_mode_block, screen_user_def, sizeof (screen_user_def));
      break;

    case MODE_320_x_200_8bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 200;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_320_x_240_8bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 240;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_320_x_256_8bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 256;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_320_x_400_8bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 400;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_320_x_480_8bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 480;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_640_x_400_8bpp:
      screen_mode_block [1] = 640;
      screen_mode_block [2] = 400;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_640_x_480_8bpp:
      screen_mode_block [1] = 640;
      screen_mode_block [2] = 480;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_320_x_200_16bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 200;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_240_16bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 240;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_256_16bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 256;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_400_16bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 400;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_480_16bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 480;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_640_x_400_16bpp:
      screen_mode_block [1] = 640;
      screen_mode_block [2] = 400;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_640_x_480_16bpp:
      screen_mode_block [1] = 640;
      screen_mode_block [2] = 480;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_200_24bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 200;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_240_24bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 240;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_256_24bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 256;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_400_24bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 400;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_320_x_480_24bpp:
      screen_mode_block [1] = 320;
      screen_mode_block [2] = 480;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_640_x_400_24bpp:
      screen_mode_block [1] = 640;
      screen_mode_block [2] = 400;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_640_x_480_24bpp:
      screen_mode_block [1] = 640;
      screen_mode_block [2] = 480;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_800_x_600_8bpp:
      screen_mode_block [1] = 800;
      screen_mode_block [2] = 600;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_800_x_600_16bpp:
      screen_mode_block [1] = 800;
      screen_mode_block [2] = 600;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_800_x_600_24bpp:
      screen_mode_block [1] = 800;
      screen_mode_block [2] = 600;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_1024_x_768_8bpp:
      screen_mode_block [1] = 1024;
      screen_mode_block [2] = 768;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_1024_x_768_16bpp:
      screen_mode_block [1] = 1024;
      screen_mode_block [2] = 768;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_1024_x_768_24bpp:
      screen_mode_block [1] = 1024;
      screen_mode_block [2] = 768;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_1280_x_960_8bpp:
      screen_mode_block [1] = 1280;
      screen_mode_block [2] = 960;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_1280_x_960_16bpp:
      screen_mode_block [1] = 1024;
      screen_mode_block [2] = 960;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_1280_x_960_24bpp:
      screen_mode_block [1] = 1024;
      screen_mode_block [2] = 960;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;


    case MODE_1280_x_1024_8bpp:
      screen_mode_block [1] = 1280;
      screen_mode_block [2] = 1024;
      screen_mode_block [3] = 3;	// 8 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0x80;	// Full palette required
      screen_mode_block [7] = 3;	// Var 3
      screen_mode_block [8] = 0xFF;	// Max colour number
      screen_mode_block [9] = -1;	// End of table.
      break;

    case MODE_1280_x_1024_16bpp:
      screen_mode_block [1] = 1280;
      screen_mode_block [2] = 1024;
      screen_mode_block [3] = 4;	// 16 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;

    case MODE_1280_x_1024_24bpp:
      screen_mode_block [1] = 1280;
      screen_mode_block [2] = 1024;
      screen_mode_block [3] = 5;	// 24 bpp
      screen_mode_block [5] = 0;	// Var 0
      screen_mode_block [6] = 0;	// Nothing required
      screen_mode_block [7] = -1;	// End of table.
      break;
  }
}

/* -------------------------------------------------------------------------- */

void I_ShutdownGraphics(void)
{
  /* Flush keyboard buffer */
  _kernel_osbyte (21, 0, 0);

  /* Re-enable the Escape key */
  _kernel_osbyte (229, 0, 0);

  /* Restore cursor key editing */
  _kernel_osbyte (4, 0, 0);

  /* Restore clip rectangles to full screen */
  _kernel_oswrch (26);
}

/* -------------------------------------------------------------------------- */

//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

/* -------------------------------------------------------------------------- */

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

/* -------------------------------------------------------------------------- */

static void init_sprite_area (void)
{
  unsigned int size;
  _kernel_oserror * rc;
  _kernel_swi_regs regs;

  size = (screen_mode_block [1] * screen_mode_block [2]);
  switch (screen_mode_block [3])
  {
    case 5:
      size <<= 1;
    case 4:
      size <<= 1;
  }

  size += 64;

  sprite_mem = malloc (size);
  if (sprite_mem == 0)
    return;

  sprite_mem -> area_size = size;
  sprite_mem -> qty_sprites = 0;
  sprite_mem -> first_sprite = 16;

  /* initialise the sprite area */
  regs.r[0] = 0x109;
  regs.r[1] = (int) sprite_mem;
  rc = _kernel_swi (OS_SpriteOp, &regs, &regs);
  if (rc)
    I_Error ("Sprite 109 error %s\n", rc -> errmess);

  /* Create a sprite */
  regs.r[0] = 0x10F;
  regs.r[1] = (int) sprite_mem;
  regs.r[2] = (int) "DoomScreen";
  regs.r[3] = 0;			// No Palette required
  regs.r[4] = screen_mode_block [1];
  regs.r[5] = screen_mode_block [2];
  //regs.r[6] = 28;			// Mode 28 is 640 x 480 256 colours
  regs.r[6] = ((screen_mode_block [3]+1)<<27)+(90<<14)+(90<<1)+1;
  rc = _kernel_swi (OS_SpriteOp, &regs, &regs);
  if (rc)
    I_Error ("Sprite 10F error %s\n", rc -> errmess);
}

/* -------------------------------------------------------------------------- */

static void put_sprite (void)
{
  _kernel_oserror * rc;
  _kernel_swi_regs regs;

  regs.r[0] = 0x200+34;			// Put sprite at user coords
  regs.r[1] = (int) sprite_mem;
  regs.r[2] = (int) &sprite_mem->sprite_size;
  regs.r[3] = 0;
  regs.r[4] = 0;
  regs.r[5] = 0;
  //regs.r[6] = 0;
  //regs.r[7] = -1;
  rc = _kernel_swi (OS_SpriteOp, &regs, &regs);
  if (rc)
    I_Error ("Sprite 122 error %s\n", rc -> errmess);
}

/* -------------------------------------------------------------------------- */

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    static int  lasttic;
    int		tics;
    int		i,j,p;
    unsigned char  * screen_copy;
    unsigned char  * screen_8;
    unsigned short * screen_16;
    unsigned int   * screen_24;
    unsigned int   * screen_24s;
    // UNUSED static unsigned char *bigscreen=0;

    // draws little dots on the bottom of the screen
    if (devparm)
    {
	i = I_GetTime();
	tics = i - lasttic;
	lasttic = i;
	if (tics > 20) tics = 20;

	for (i=0 ; i<tics*2 ; i+=2)
	  screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = devparm_white;
	for ( ; i<20*2 ; i+=2)
	  screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = devparm_black;
    }

    // draw the image
    if (write_via_sprite)
    {
      screen_8 = ((unsigned char*) &sprite_mem->sprite_size) + (sprite_mem -> offset);
    }
    else
    {
      if (vsync)
	_kernel_osbyte (19, 0, 0);
      screen_8 = (unsigned char *) screen_addr;
    }

    screen_copy = screens[0];

    /* 320 * 4xx is a special case as it uses pixel doubling */
    if ((screen_mode_block [1] == 320)
     && ((screen_mode_block [2] == 400)
      || (screen_mode_block [2] == 480)))
    {
	switch (screen_mode_block [3])
	{
	  case 3:
	    i = SCREENHEIGHT;
	    if (rpc_palette)
	    {
	      screen_24 = (unsigned int *) screen_8;
	      screen_24s = (unsigned int *) screen_copy;
	      do
	      {
		j = 320 / 4;
		do
		{
		  p = *screen_24s++;
		  screen_24 [320/4] = p;
		  *screen_24++ = p;
		} while (--j);
		screen_24 += (320/4);
	      } while (--i);
	    }
	    else
	    {
	      do
	      {
		j = 320;
		do
		{
		  p = palette_table [*screen_copy++];
		  screen_8 [320] = p;
		  *screen_8++ = p;
		} while (--j);
		screen_8 += 320;
	      } while (--i);
	    }
	    break;

	  case 4:
	    i = SCREENHEIGHT;
	    screen_16 = (unsigned short *) screen_8;
	    do
	    {
	      j = 320;
	      do
	      {
		p = palette_table [*screen_copy++];
		screen_16 [320] = p;
		*screen_16++ = p;
	      } while (--j);
	      screen_16 += 320;
	    } while (--i);
	    break;

	  case 5:
	    i = SCREENHEIGHT;
	    screen_24 = (unsigned int *) screen_8;
	    do
	    {
	      j = 320;
	      do
	      {
		p = palette_table [*screen_copy++];
		screen_24 [320] = p;
		*screen_24++ = p;
	      } while (--j);
	      screen_24 += 320;
	    } while (--i);
	    break;
	}
    }
    else
    {
	switch (screen_mode_block [3])
	{
	  case 3:
	    if (write_via_sprite)
	    {
	      /* No need to do this copy as screens[0] has been set */
	      /* to point to the sprite area in I_InitGraphics */
	    }
	    else if (rpc_palette)
	    {
	      memcpy (screen_8, screen_copy, SCREENHEIGHT * SCREENWIDTH);
	    }
	    else
	    {
	      i = SCREENWIDTH * SCREENHEIGHT;
	      do
	      {
		*screen_8++ = palette_table [*screen_copy++];
	      } while (--i);
	    }
	    break;

	  case 4:
	    screen_16 = (unsigned short *) screen_8;
	    i = SCREENWIDTH * SCREENHEIGHT;
	    do
	    {
	      *screen_16++ = palette_table [*screen_copy++];
	    } while (--i);
	    break;

	  case 5:
	    screen_24 = (unsigned int *) screen_8;
	    i = SCREENWIDTH * SCREENHEIGHT;
	    do
	    {
	      *screen_24++ = palette_table [*screen_copy++];
	    } while (--i);
	    break;
	}
    }

    if (write_via_sprite)
    {
      if (vsync)
	_kernel_osbyte (19, 0, 0);
      put_sprite ();
    }
}

/* -------------------------------------------------------------------------- */

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
  memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

/* -------------------------------------------------------------------------- */

static void set_palette_8_rpc (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;
  _kernel_swi_regs regs;
  unsigned int palette_8_table [256];

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];

    palette_8_table [colour] = (blue << 24) | (green << 16) | (red << 8);
    //palette_table [colour] = colour;
  } while (++colour < 256);

  regs.r[0] = -1;
  regs.r[1] = -1;
  regs.r[2] = (int) palette_8_table;
  regs.r[3] = 0;
  regs.r[4] = 0;
  regs.r[5] = 0;
  regs.r[6] = 0;
  regs.r[7] = 0;
  _kernel_swi (ColourTrans_WritePalette, &regs, &regs);
}

/* -------------------------------------------------------------------------- */

#ifdef ColourTrans_ReturnColourNumber

static void set_palette_xx (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;
  unsigned int bgr;
  _kernel_swi_regs regs;

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];
    bgr = (blue << 24) | (green << 16) | (red << 8);
    if (bgr_palette_table [colour] != bgr)
    {
      bgr_palette_table [colour] = bgr;
      regs.r[0] = bgr;
      _kernel_swi (ColourTrans_ReturnColourNumber, &regs, &regs);
      palette_table [colour] = regs.r[0];
      if (devparm_palette)
      {
	devparm_palette--;
	printf ("Colour %u palette = %08X, ColNum = %08X\n", colour, bgr, regs.r[0]);
      }
    }
  } while (++colour < 256);
}

/* -------------------------------------------------------------------------- */
//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
  if (devparm)
  {
    devparm_black = AM_load_colour (0, 0, 0, palette);
    devparm_white = AM_load_colour (255, 255, 255, palette);
  }

  switch (screen_mode_block [3])
  {
    case 3:
      if (rpc_palette)
      {
	set_palette_8_rpc (palette);
	return;
      }
      break;
  }

  set_palette_xx (palette);
}

#else

/* -------------------------------------------------------------------------- */

static void set_palette_24 (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];
    palette_table [colour] = (blue << 16) | (green << 8) | (red);
  } while (++colour < 256);
}

/* -------------------------------------------------------------------------- */

static void set_palette_16 (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];

    palette_table [colour] = ((blue  & 0xF8) << 7)
			   | ((green & 0xF8) << 2)
			   | ((red   & 0xF8) >> 3);
  } while (++colour < 256);
}

/* -------------------------------------------------------------------------- */

static void set_palette_8_non_rpc (byte * palette)
{
  unsigned int colour;
  unsigned int red;
  unsigned int green;
  unsigned int blue;

  colour = 0;
  do
  {
    red   = gammatable[usegamma][*palette++];
    green = gammatable[usegamma][*palette++];
    blue  = gammatable[usegamma][*palette++];

	// if (red & 0x0F)   red   |= 0x10;
	// if (blue & 0x0F)  blue  |= 0x10;
	// if (green & 0x0F) green |= 0x10;

    palette_table [colour] = (blue & 0x80)
	   		| ((green & 0xC0) >> 1)
			| ((red & 0x80) >> 3)
			| ((blue & 0x40) >> 3)
			| ((red & 0x40) >> 4)
			| (((blue | green | red) & 0x30) >> 4);
  } while (++colour < 256);
}

/* -------------------------------------------------------------------------- */

static void set_palette_8 (byte * palette)
{
  if (rpc_palette)
  {
    set_palette_8_rpc (palette);
  }
  else
  {
    set_palette_8_non_rpc (palette);
  }
}

/* -------------------------------------------------------------------------- */

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
  if (devparm)
  {
    devparm_black = AM_load_colour (0, 0, 0, palette);
    devparm_white = AM_load_colour (255, 255, 255, palette);
  }

  switch (screen_mode_block [3])
  {
     case 3:
      set_palette_8 (palette);
      break;

     case 4:
      set_palette_16 (palette);
      break;

     default:
      set_palette_24 (palette);
      break;
  }
}
#endif
/* -------------------------------------------------------------------------- */

static void show_mode_sel_block (void)
{
  unsigned int pos;

  printf ("Screen mode %u x %u, log2bpp=%u, frame=%d",
	screen_mode_block [1],
	screen_mode_block [2],
	screen_mode_block [3],
	screen_mode_block [4]);

  pos = 5;
  while (screen_mode_block [pos] != -1)
  {
    printf (", var=%d, val=%d",
	screen_mode_block [pos],
	screen_mode_block [pos+1]);
    pos += 2;
  }
}

/* -------------------------------------------------------------------------- */
/*
   Select a screen mode given a mode selector block

   Block [0] = 1;
   Block [1] = X Resolution (Pixels)
   Block [2] = Y Resolution (Pixels)
   Block [3] = Log2BPP value
   Block [4] = Frame rate (-1 = first match)
   Block [5] = Mode Variable numbers followed by value.
   Block [n] = -1 (Terminator)
*/

static _kernel_oserror * os_screen_mode (void)
{
  int attempts;
  _kernel_oserror * rc;
  _kernel_swi_regs regs;

  attempts = 2;
  do
  {
    regs.r[0] = 0;
    regs.r[1] = (int) screen_mode_block;
    rc = _kernel_swi (OS_ScreenMode, &regs, &regs);

    if (devparm)
    {
      show_mode_sel_block ();
      if (rc)
	printf (" - Error %s\n", rc -> errmess);
      else
	printf (" - OK\n");
    }
    if ((rc == NULL)
     || (screen_mode_block [3] < 4))
      break;

    /* Try swapping the trgb value */
    screen_mode_block [5] = 0;
    screen_mode_block [6] ^= (4<<12);
  } while (--attempts);

  return (rc);
}

/* -------------------------------------------------------------------------- */

static _kernel_oserror * sel_os_screen_mode (int number)
{
  _kernel_oserror * rc;

  construct_screen_mode_block (number);
  rc = os_screen_mode ();
  return (rc);
}

/* -------------------------------------------------------------------------- */

static _kernel_oserror * select_screen_mode (int number, int set_text_window)
{
  _kernel_oserror * rc;
  rpc_palette = 0;

  rc = sel_os_screen_mode (number);
  if (rc == 0)
  {
    rpc_palette = 1;

    SCREENWIDTH = screen_mode_block [1];
    SCREENHEIGHT = screen_mode_block [2];

    /* Special case for the 320 x (400/480) as we use */
    /* pixel doubling in the Y plane only */

    if ((SCREENWIDTH == 320)
     && ((SCREENHEIGHT == 400) || (SCREENHEIGHT == 480)))
      SCREENHEIGHT >>= 1;

    if (set_text_window)
    {
      /* Define text window */
      _kernel_oswrch (28);
      _kernel_oswrch (0);				/* Left X */
      _kernel_oswrch ((screen_mode_block [2] / 8) - 1);	/* Bottom Y */
      _kernel_oswrch ((screen_mode_block [1] / 8) - 1);	/* Right X */
      _kernel_oswrch (0);				/* Top Y */
    }
  }

  return (rc);
}

/* -------------------------------------------------------------------------- */

static void decode_screendef (int p)
{
  int i;
  int trgb = 0;

  do
  {
    p++;
    if (p >= myargc)
      break;
    switch (myargv[p][0])
    {
      case 'X':
      case 'x':
	screen_user_def [1] = atoi (&myargv[p][1]);
	break;

      case 'Y':
      case 'y':
	screen_user_def [2] = atoi (&myargv[p][1]);
	break;

      case 'F':
      case 'f':
	screen_user_def [4] = atoi (&myargv[p][1]);
	break;

      case 'C':
      case 'c':
	i = atoi (&myargv[p][1]);
	switch (i)
	{
	  case 256:
	    screen_user_def [3] = 3;
	    break;
	  case 32:
	    screen_user_def [3] = 4;
	    break;
	  case 16:
	    screen_user_def [3] = 5;
	    break;
	  default:
	    if (i > 32768)
	      screen_user_def [3] = 5;
	    else if (i > 256)
	      screen_user_def [3] = 4;
	    else
	      screen_user_def [3] = 3;
	}
	break;

      case 'L':
      case 'l':
	if (strcasecmp (myargv[p], "LTRGB") == 0)
	  trgb = (4<<12);
	break;

      default:
	p = myargc;
    }
  } while (1);

  if (screen_user_def [3] > 3)
  {
    if (trgb)
    {
//    screen_user_def [5] = 0;			// Already = 0
      screen_user_def [6] = trgb;
      screen_user_def [7] = -1;			// End of table
    }
    else
    {
//    screen_user_def [5] = 0;			// Already = 0
      screen_user_def [6] = 0;
      screen_user_def [7] = -1;			// End of table
    }
  }
}

/* -------------------------------------------------------------------------- */
/*
   Find -nn in the arg list.
*/

static int get_screen_mode_arg (void)
{
  int i;
  int j;
  char * p;

  for (i = 1;i<myargc;i++)
  {
    p = myargv[i];
    if (*p == '-')
    {
      p++;
      j = *p;
      if ((j >= '0') && (j <= '9'))
	return (atoi (p));
    }
  }

  return -1;
}

/* -------------------------------------------------------------------------- */

static void select_text_mode (void)
{
  if ((SCREENWIDTH != 640)
   || (SCREENHEIGHT != 480))
  {
    if (sel_os_screen_mode (MODE_640_x_480_8bpp))
      sel_os_screen_mode (MODE_640_x_480_24bpp);
  }
}

/* -------------------------------------------------------------------------- */

static const unsigned char mode_search [] =
{
#if 0
  MODE_800_x_600_8bpp,
  MODE_800_x_600_24bpp,
  MODE_800_x_600_16bpp,
#endif
  MODE_640_x_480_8bpp,
  MODE_640_x_480_24bpp,
  MODE_640_x_480_16bpp,

  MODE_640_x_400_8bpp,
  MODE_640_x_400_24bpp,
  MODE_640_x_400_16bpp,

  MODE_320_x_480_8bpp,
  MODE_320_x_480_24bpp,
  MODE_320_x_480_16bpp,

  MODE_320_x_256_8bpp,
  MODE_320_x_256_24bpp,
  MODE_320_x_256_16bpp
};

/* -------------------------------------------------------------------------- */

void I_SetScreenSize (void)
{
  int count;
  const unsigned char * mode_number;
  _kernel_oserror * rc;
  _kernel_swi_regs regs;

  /* Set VDU driver screen bank to default */
  _kernel_osbyte (112, 0, 0);

  /* Set hardware screen bank to default */
  _kernel_osbyte (113, 0, 0);

  /* Set Non-shadow */
  _kernel_osbyte (114, 1, 0);

  screen_mode = get_screen_mode_arg ();
  if (screen_mode == MODE_USER_DEFINED)
  {
    decode_screendef (M_CheckParm("-0"));
    rc = select_screen_mode (screen_mode,0);
    if (rc)
      I_Error ("Failed to select user def mode (%s)\n", rc -> errmess);
  }

  else if (screen_mode < 0)	/* No screen size/resolution given... */
  {
    regs.r[1] = (int) "OS_ScreenMode";
    rc = _kernel_swi (OS_SWINumberFromString, &regs, &regs);
    if (rc)
    {
      _kernel_oswrch (22);
      _kernel_oswrch (49);
      if (((_kernel_osbyte (135, 0, 0) >> 8) & 0xFF) == 49)
      {
	construct_screen_mode_block (screen_mode = MODE_320_x_480_8bpp);
      }
      else
      {
	I_Error ("Failed to verify the OS_ScreenMode call (%s)\n", rc -> errmess);
      }
    }
    else
    {
      /* So the OS_ScreenMode call exists, which means that 16/24 bit */
      /* colour may be available..... */
      /* I tested this code by preventing my Risc PC from executing it's */
      /* !boot programs, hence the monitor definition file wasn't read and */
      /* hence the 16/24 bit modes were not available...... */

      /* Try first for the best... */
      mode_number = mode_search;
      count = ARRAY_SIZE (mode_search);

      do
      {
	rc = select_screen_mode (screen_mode = *mode_number, 0);
	if (rc == 0)
	  break;

	mode_number++;
	if (--count == 0)
	{
	  I_Error ("Failed to find a suitable screen mode (%s)\n", rc -> errmess);
	}
      } while (1);
    }
  }
  else
  {
    rc = select_screen_mode (screen_mode,0);
    if (rc)
    {
      select_text_mode ();
      I_Error ("Failed to select screen mode (%s)\n", rc -> errmess);
    }
  }

  select_text_mode ();

  memset (bgr_palette_table, 255, sizeof (bgr_palette_table));


  // printf ("Welcome to Doom For Acorn RiscPC.\nFree software built from the Linux source by Jeff Doggett\n\n");
  //k = 0;
  //do
  //{
  //  printf ("Run %s ", myargv[k++]);
  //} while (k < myargc);
  //printf ("\n\n");
}

/* -------------------------------------------------------------------------- */

void I_InitGraphics (void)
{
    _kernel_swi_regs regs;
    unsigned int block [32];

    vsync = M_CheckParm ("-vsync");
    select_screen_mode (screen_mode, 1);

    if (M_CheckParm("--0"))
    {
      write_via_sprite = 1;
      init_sprite_area ();
      if (screen_mode_block [3] == 3)
      {
	/* 320 * 4xx is a special case as it uses pixel doubling */
	if (!((screen_mode_block [1] == 320)
	 && ((screen_mode_block [2] == 400)
	  || (screen_mode_block [2] == 480))))
	{
	  screens[0] = ((unsigned char*) &sprite_mem->sprite_size) + (sprite_mem -> offset);
	  screen_addr = (unsigned int *) screens[0];	// Set this as well in case.
	}
      }
    }
    else
    {
      /* Find out where the operating system has put the screen memory */
      block [0] = 149;
      block [1] = -1;

      regs.r[0] = (int) &block[0];
      regs.r[1] = (int) &block[2];

      _kernel_swi (OS_ReadVduVariables, &regs, &regs);
      screen_addr = (unsigned int *) block[2];
      /* printf ("Screen address is %X\n", screen_addr); */
    }

    if (devparm)
    {
      devparm_palette = 256;
      printf ("Internal mode number is %u, Sprite mode is %u\n", screen_mode, write_via_sprite);
      if (screens[0] == ((unsigned char*) &sprite_mem->sprite_size) + (sprite_mem -> offset))
      {
        printf ("Screens[0] -> Sprite memory\n");
      }
    }

    /* Cursor off */
    _kernel_oswrch (23);
    _kernel_oswrch (1);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
    _kernel_oswrch (0);
}

/* -------------------------------------------------------------------------- */

void I_SetWindowName (const char * title)
{
}

/* -------------------------------------------------------------------------- */
