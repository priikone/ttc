#
# Makefile for ttc and ttc daemon.
# You may have to edit this for some systems.
#
# Change INST_DEST if you want to install programs somewhere else.
# Change LIB_INST_DEST if you want to install PAM module somewhere else.
#

INST_DEST = /usr/local/bin
LIB_INST_DEST = /lib/security

srcdir		= .
prefix		= /usr
libdir		= $(prefix)/lib
includedir	= $(prefix)/include
mandir		= /usr/man
man1dir		= $(mandir)/man1
man7dir		= $(mandir)/man7
man8dir		= $(mandir)/man8

CC		= gcc
CFLAGS		= -O2 #-DTEST_DEBUG 
CPPFLAGS	= -I. -I$(includedir)
CCFLAGS		= $(CFLAGS) $(CPPFLAGS)

# Some systems may need here 'LIBS = -L$(libdir) -lsocket'
LIBS		=
PAM_LIBS	= -lpam

TTC_OBJS	= ttc.o confauth.o options.o common.o
TTCD_OBJS	= ttcd.o
TTC_LIB_OBJS	= pam_ttc.o confauth.o options.o common.o

PROGRAMS 	= ttc ttcd
LIB_PROGRAMS 	= pam_ttc

MAN1		= ttc.1
MAN7		= pam_ttc.7
MAN8		= ttcd.8

all: $(PROGRAMS)

modules: $(LIB_PROGRAMS)

ttc:	$(TTC_OBJS)
	-rm -f ttc
	$(CC) -o ttc $(TTC_OBJS) $(LIBS)

ttcd:	$(TTCD_OBJS)
	-rm -f ttcd
	$(CC) -o ttcd $(TTCD_OBJS) $(LIBS)

pam_ttc: $(TTC_LIB_OBJS)
	-rm -f pam_ttc
	$(CC) -shared -fPIC -o pam_ttc.so $(TTC_LIB_OBJS) $(PAM_LIBS) $(LIBS)

install:
	install -o 0 -g 0 -m 0755 ./ttc $(INST_DEST)/ttc
	install -o 0 -g 0 -m 0755 ./ttcd $(INST_DEST)/ttcd
	install -o 0 -g 0 -m 0755 $(MAN1) $(man1dir)/$(MAN1)
	install -o 0 -g 0 -m 0755 $(MAN7) $(man7dir)/$(MAN7)
	install -o 0 -g 0 -m 0755 $(MAN8) $(man8dir)/$(MAN8)

modules_install:
	install -o 0 -g 0 -m 0755 ./pam_ttc.so $(LIB_INST_DEST)/pam_ttc.so
	install -o 0 -g 0 -m 0755 $(MAN7) $(man7dir)/$(MAN7)

modules_uninstall:
	-rm -rf $(LIB_INST_DEST)pam_ttc.so

uninstall:
	-rm -rf $(INST_DEST)/ttc $(INST_DEST)/ttcd 
	-rm -rf $(man1dir)/$(MAN1) $(man8dir)/$(MAN8) $(man7dir)/$(MAN7)

clean:
	-rm -f *.o rm -f *.so rm ttc ttcd

