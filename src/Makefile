#
# $Id: Makefile.dist 948 2006-12-04 00:55:16Z zsuzsu $
#
CC		= gcc
LD		= gcc
INCLUDES	= -I .
#WFLAGS		= -Wall -Werror -Wshadow -Wformat-security -Wpointer-arith \
#		-Wcast-align -Wredundant-decls
#WFLAGS		= -Wall 
WFLAGS		= -Wall -Wformat-security -Wpointer-arith -Wcast-align

CFLAGS		= -g $(WFLAGS) -O -pipe $(INCLUDES)
LFLAGS		= -g
LIBS		=
INSTALL_PATH	= ..

#
# for FreeBSD
#CFLAGS	+= -DBSD44 
#LIBS	+= -lcrypt

#
# for Linux
CFLAGS	+= -DLINUX 
LIBS	+= -lcrypt

#
# for SunOS 5 (Solaris)
#CFLAGS	+= -DSUNOS
#LIBS	+= -lsocket -lnsl
#CFILES	+= compat/strsep.c compat/snprintf.c
#OFILES	+= strsep.o snprintf.o

#
# for Unixware 2.x
#CFLAGS	+= -DSVR4 -DOLD_RAND
#LIBS	+= -lsocket -lnsl -lcrypt
#CFILES	+= compat/strsep.c
#OFILES	+= strsep.o

#
# Generally Makefile should not be edited below this line
#
COMM_CFILES = comm/ban.c comm/charset.c comm/comm.c comm/comm_act.c\
	      comm/comm_colors.c comm/comm_info.c comm/resolver.c \
	      #comm/comm_login.c

DB_CFILES = db/db.c db/db_area.c db/db_clan.c db/db_skilltree.c  \
	    db/db_class.c db/db_hometown.c db/db_lang.c db/db_material.c \
	    db/db_race.c db/db_skills.c db/db_socials.c db/db_system.c\
	    db/cmd.c db/gsn.c db/hometown.c db/lang.c db/msg.c db/spellfn.c\
	    db/socials.c

OLC_CFILES = olc/olc.c olc/olc_area.c olc/olc_clan.c olc/olc_help.c\
	     olc/olc_lang.c \
	     olc/olc_material.c olc/olc_mob.c olc/olc_mpcode.c olc/olc_msg.c\
	     olc/olc_room.c olc/olc_rule.c olc/olc_obj.c olc/olc_race.c \
	     olc/olc_save.c olc/olc_social.c

CFILES = act_comm.c act_info.c act_move.c act_obj.c act_wiz.c auction.c\
         augment.c buffer.c clan.c skilltree.c channels.c class.c config.c \
	 debug.c\
	 effects.c fight.c fixes.c flag.c gamble.c handler.c healer.c help.c\
	 hunt.c interp.c log.c lookup.c magic.c magic2.c magic3.c \
	 martial_art.c material.c mem.c\
	 mlstring.c mob_cmds.c mob_prog.c namedp.c note.c pdata.o obj_prog.c \
	 quest.c race.c raffects.c rating.c recycle.c religion.c repair.c\
	 save.c skills.c skills2.c skill_info.c special.c stats.c \
	 str.c string_edit.c\
	 system.c tables.c update.c usage.c util.c waffects.c varr.c\
	 $(COMM_CFILES) $(DB_CFILES) $(OLC_CFILES)

OFILES := $(patsubst %.c,o/%.o,$(CFILES))

zim: $(OFILES)
	$(LD) $(LFLAGS) -o zim $(OFILES) $(LIBS)

run: zim 
	./startup

mem: zim 
	./startup_mem

o/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

.c.o:
	$(CC) -c $(CFLAGS) $<

dep:
	mkdep $(CFLAGS) $(CFILES)

clean:
	rm -f zim
	find ./o -name "*.o" -exec rm -f {} \;

ctags:
	ctags -dtw $(CFILES) *.h comm/*.h compat/*.h db/*.h olc/*.h

gtags:
	gtags -co .

install:
	install -c -m 770 zim $(INSTALL_PATH)/bin

