.KEEP_STATE:
CC=		gcc  
CCC=		gcc 
CFLAGS= 	-g -Wall -I../include 
#CFLAGS= 	-g -Wall -I../include -DVERBOSE
#CFLAGS= 	-g -I../include -Wall
CCFLAGS= 	$(CFLAGS)
#LDFLAGS=	-L/opt/local/lib -R/opt/local/lib
LDFLAGS=	-L/usr/lib -lm

QUESTCAMLIB=		../lib/libquestcam.a

#SHELLS  = update_filter_log start_questctl

QUESTPROGRAMS = questctl sunup weather getfocus setfocus \
	mountctl opendome_raw closedome slavedome domestatus \
	slavewindscr getfilter disable_telescope enable_telescope \
	telescope_status stow_telescope get_riset  

QUESTSCRIPTS = start_questctl stop_questctl

INSTALLDIR = ../bin
#INSTALLDIR = /Users/daver/yale/lasilla/quest-src-lasilla/bin

all: $(QUESTPROGRAMS) $(QUESTSCRIPTS)

quest: $(QUESTPROGRAMS)

copy:
	(cd $(INSTALLDIR); rm -f $(QUESTPROGRAMS))
	cp -p $(QUESTPROGRAMS) $(QUESTSCRIPTS) $(INSTALLDIR)

install: all copy

clean:
	rm -f  *~ *.o $(QUESTPROGRAMS)

questctl: questctl.o  $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) $(LDFLAGS) -o $@ \
	    questctl.o $(QUESTCAMLIB) -lstdc++   -lm

# ../lib/telescope_drv.o $(QUESTCAMLIB) -lstdc++   -lm

get_riset: get_riset.o  $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) $(LDFLAGS) -o $@ \
	    get_riset.o \
	     $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++  -lm

sunup: sunup.o  $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) $(LDFLAGS) -o $@ \
	    sunup.o \
	     $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++  -lm

mountctl: mountctl.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) $(LDFLAGS) -o $@ \
	    mountctl.o \
	    $(QUESTCAMLIB)  -lm -lstdc++

setfocus: setfocus.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) $(LDFLAGS) -o $@ \
		setfocus.o  \
		../lib/telescope_drv.o $(QUESTCAMLIB)  -lstdc++ -lm

getfocus: getfocus.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		getfocus.o  \
	     ../lib/telescope_drv.o $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++  -lm

getfilter: getfilter.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		getfilter.o  \
		../lib/telescope_drv.o $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++  -lm

domestatus: domestatus.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) $(LDFLAGS) -o $@ \
		domestatus.o \
		$(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

telescope_status: telescope_status.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		telescope_status.o \
		../lib/telescope_drv.o $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

weather: weather.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		weather.o  \
		../lib/telescope_drv.o  $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++ -lm

opendome_raw: opendome.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		opendome.o\
		../lib/telescope_drv.o  $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

closedome: closedome.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		closedome.o  \
		../lib/telescope_drv.o  $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

stow_telescope: stow_telescope.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		stow_telescope.o  \
		../lib/telescope_drv.o  $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

disable_telescope: disable_telescope.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		disable_telescope.o  \
		../lib/telescope_drv.o  $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

enable_telescope: enable_telescope.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		enable_telescope.o  \
		../lib/telescope_drv.o  $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

slavedome: slavedome.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		slavedome.o  \
		../lib/telescope_drv.o $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

slavewindscr: slavewindscr.o $(QUESTCAMLIB)
	$(CCC) $(CCFLAGS) -o $@ \
		slavewindscr.o  \
		../lib/telescope_drv.o $(QUESTCAMLIB)  $(LDFLAGS) -lstdc++   -lm

test: test.o
	$(CCC) $(CCFLAGS) -o $@ \
		test.o  \
		$(LDFLAGS) -lstdc++   -lm


.cc.o:
	$(CCC) $(CCFLAGS) -c $<
.c.o:
	$(CC) $(CFLAGS) -c $<
