/* Lazy way to include all of the files */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#ifdef NORMALUNIX
#include <unistd.h>
#include <fcntl.h>
#endif
#ifdef __riscos
#include "acorn.h"
#endif


#include "doomtype.h"
#include "doomdef.h"
#include "doomdata.h"
#include "doomstat.h"

#include "d_event.h"

#include "am_map.h"
#include "dh_stuff.h"
#include "dstrings.h"
#include "d_englsh.h"
//#include "d_french.h"
#include "d_items.h"
#include "d_main.h"
#include "d_net.h"
#include "d_player.h"
#include "d_textur.h"
#include "d_think.h"
#include "d_ticcmd.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "g_game.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "info.h"
#include "i_net.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_cheat.h"
#include "m_fixed.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_random.h"
#include "m_swap.h"
#include "p_enemy.h"
#include "p_inter.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_pspr.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "p_spec.h"
#include "p_tick.h"
#include "r_bsp.h"
#include "r_data.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_local.h"
#include "r_main.h"
#include "r_patch.h"
#include "r_plane.h"
#include "r_segs.h"
#include "r_sky.h"
#include "r_state.h"
#include "r_things.h"
#include "sounds.h"
#include "st_lib.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "tables.h"
#include "v_text.h"
#include "v_video.h"
#include "wi_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
