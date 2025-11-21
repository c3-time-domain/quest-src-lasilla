# Makefile to make libquestcam.a
# $RCSfile: Makefile,v $ $Revision: 1.7 $ $Date: 2010/10/12 14:37:27 $
.KEEP_STATE:
CC=		gcc 
CCC=		gcc 

CFLAGS= 	-g -I../include -O2 -DVERBOSE -Wall 
#CFLAGS= 	-g -I../include -O2 -Wall

CCFLAGS= 	$(CFLAGS)

OBJFILES=	bcd.o dms.o de2h.o riset.o loopio.o \
		neattime.o netio.o quest_sockets.o precess.o \
		siteconf.o telinterp.o telmount.o tm_angles.o \
		telescope_drv.o telescope_controller.o almanac.o 


QUESTCAMFILES= $(OBJFILES) 

INSTALLDIR= ./lib

all: libquestcam.a

install: all

	cp -p libquestcam.a $(INSTALLDIR)

clean:
	rm -f $(QUESTCAMFILES) libquestcam.a *~

libquestcam.a: $(QUESTCAMFILES)
	ar ruv $@ $(QUESTCAMFILES)
	ranlib $@

.cc.o:
	$(CCC) $(CCFLAGS) -c $<
.c.o:
	$(CC) $(CFLAGS) -c $<

