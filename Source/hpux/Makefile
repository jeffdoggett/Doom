################################################################
#
# $Id:$
#
# $Log:$
#
CC=  cc  # gcc or g++

CFLAGS= -Ae -O -I/usr/include/X11R5 -DNORMALUNIX -D__BIG_ENDIAN__ -DMAXSCREENHEIGHT=480 -DMAXSCREENWIDTH=640 -DMORELIGHTLEVELS -DHAVE_ALLOCA
LDFLAGS=-L/usr/lib/X11R5
LIBS=-lXext -lX11 -lm

HDRS=		am_map.h dh_stuff.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h d_englsh.h \
		d_event.h d_french.h d_items.h d_main.h d_net.h d_player.h d_textur.h d_think.h d_ticcmd.h \
		f_finale.h f_wipe.h g_game.h hu_lib.h hu_stuff.h info.h i_net.h i_sound.h i_system.h i_video.h \
		m_argv.h m_bbox.h m_cheat.h m_fixed.h m_menu.h m_misc.h m_random.h m_swap.h p_enemy.h p_inter.h \
		p_local.h p_mobj.h p_pspr.h p_saveg.h p_setup.h p_spec.h p_tick.h r_bsp.h r_data.h r_defs.h \
		r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_sky.h r_state.h r_things.h sounds.h \
 		st_lib.h st_stuff.h s_sound.h tables.h v_text.h v_video.h wi_stuff.h w_wad.h z_zone.h

OBJS=		doomdef.o doomstat.o i_main.o i_system.o i_sound.o i_video.o i_net.o \
		tables.o f_finale.o f_wipe.o dh_stuff.o d_main.o d_net.o d_items.o \
		g_game.o m_menu.o m_misc.o m_argv.o m_bbox.o m_fixed.o m_swap.o	\
		m_cheat.o m_random.o am_map.o p_ceilng.o p_doors.o p_enemy.o	\
		p_floor.o p_inter.o p_lights.o p_map.o p_maputl.o p_plats.o p_pspr.o \
		p_setup.o p_sight.o p_spec.o p_switch.o p_mobj.o p_telept.o p_tick.o \
		p_saveg.o p_user.o r_bsp.o r_data.o r_draw.o r_main.o r_plane.o \
		r_segs.o r_sky.o r_things.o w_wad.o wi_stuff.o v_text.o v_video.o st_lib.o \
		st_stuff.o hu_stuff.o hu_lib.o s_sound.o z_zone.o info.o sounds.o \
		usleep.o

all:		xdoom

clean:
	rm -f *.o *~ *.flc
	rm -f linux/*

xdoom:	$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS):	$(HDRS)
