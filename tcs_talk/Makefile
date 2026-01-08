.KEEP_STATE:

CC= cc

CFLAGS=	-I ../include -g


# Uncomment the following for real camera operations
LIBS= -lm -lc
#
PROGRAMS= tcs_talk 

SCRIPTS=  make_virtual_port.tcsh  quest_neatsrv.pl  talk.csh  tcs_talk_client.pl  tcs_talk_srv.pl

#

all: $(PROGRAMS) $(SCRIPTS)


	
TCS_TALK_SOURCES= tcs_talk.c    
TCS_TALK_OBJECTS= $(TCS_TALK_SOURCES:.c=.o)


OBJECTS = *.o

tcs_talk: $(TCS_TALK_OBJECTS)   
	$(CC) $(CFLAGS) -o $@ $(TCS_TALK_OBJECTS) $(LIBS) 

.c.o:   
	$(CC) $(CFLAGS) -c $<

clean:
	rm $(OBJECTS) $(PROGRAMS)

INSTALLDIR= ../bin

install: all

	cp $(PROGRAMS) $(SCRIPTS)  $(INSTALLDIR)


