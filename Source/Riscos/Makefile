################################################################
#
# $Id:$
#
# $Log:$
#
CC= cc
ASM= objasm

CFLAGS=-Wp -JC: -DPADDED_STRUCTS -DMAXSCREENHEIGHT=480 -DMAXSCREENWIDTH=640 -DMORELIGHTLEVELS -DUSE_INTS_FOR_SHORTS
AFLAGS=-CPU ARM7M
LDFLAGS=

# The order of the libraries here is important - almost
# any other sequence causes link errors.

LIBS=-LC:unixlib.o -LC:inetlib.o -LC:socklib.o -LC:stubs.o

HDRS=		acorn.h am_map.h dh_stuff.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h d_englsh.h \
		d_event.h d_french.h d_items.h d_main.h d_net.h d_player.h d_textur.h d_think.h d_ticcmd.h \
		f_finale.h f_wipe.h g_game.h hu_lib.h hu_stuff.h info.h i_net.h i_sound.h i_system.h i_video.h \
		m_argv.h m_bbox.h m_cheat.h m_fixed.h m_menu.h m_misc.h m_random.h m_swap.h p_enemy.h p_inter.h \
		p_local.h p_mobj.h p_pspr.h p_saveg.h p_setup.h p_spec.h p_tick.h r_bsp.h r_data.h r_defs.h \
		r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_sky.h r_state.h r_things.h sounds.h \
 		st_lib.h st_stuff.h s_sound.h tables.h v_text.h v_video.h wi_stuff.h w_wad.h z_zone.h


OBJS=		acorn.o alloca.o am_map.o a_sound.o a_video.o dh_stuff.o doomdef.o doomstat.o \
		d_items.o d_main.o d_net.o f_finale.o f_wipe.o g_game.o hu_lib.o hu_stuff.o info.o i_main.o \
		i_net.o i_system.o maths64.o m_argv.o m_bbox.o m_cheat.o m_fixed.o m_menu.o m_misc.o m_random.o \
		m_swap.o p_ceilng.o p_doors.o p_enemy.o p_floor.o p_inter.o p_lights.o p_map.o p_maputl.o \
		p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o p_switch.o p_telept.o \
		p_tick.o p_user.o r_bsp.o r_data.o r_draw.o r_main.o r_plane.o r_segs.o r_sky.o r_things.o \
		sounds.o st_lib.o st_stuff.o s_sound.o tables.o v_text.o v_video.o wi_stuff.o w_wad.o z_zone.o

all:		^.!RunImage


^.!RunImage:    $(OBJS)
		$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o ^.!RunImage
		squeeze ^.!RunImage
		@echo Finished OK

.SUFFIXES: .o .c
.c.o:;		$(CC) $(CFLAGS) -c -o $@ $*.c

.SUFFIXES: .o .s
.s.o:;		$(ASM) $(AFLAGS) -o $@ $*.s


$(OBJS):	$(HDRS)
