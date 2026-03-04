.KEEP_STATE:

CC= cc

CFLAGS=	-I ../include -g


# Uncomment the following for real camera operations
LIBS= -lm -lc
#
PROGRAMS= date_to_time  parse_meteo

#
SCRIPTS = ntt_dome_status  start_weather_server  weather_client.pl  weather_srv.pl


all: $(PROGRAMS) $(SCRIPTS)

	
DATE_TO_TIME_SOURCES= date_to_time.c    
DATE_TO_TIME_OBJECTS= $(DATE_TO_TIME_SOURCES:.c=.o)

PARSE_METEO_SOURCES= parse_meteo.c
PARSE_METEO_OBJECTS= $(PARSE_METEO_SOURCES:.c=.o)


OBJECTS = *.o


date_to_time: $(DATE_TO_TIME_OBJECTS)   
	$(CC) $(CFLAGS) -o $@ $(DATE_TO_TIME_OBJECTS) $(LIBS) 

parse_meteo: $(PARSE_METEO_OBJECTS)   
	$(CC) $(CFLAGS) -o $@ $(PARSE_METEO_OBJECTS) $(LIBS) 

.c.o:   
	$(CC) $(CFLAGS) -c $<

clean:
	rm $(OBJECTS) $(PROGRAMS)

INSTALLDIR= ../bin

install: all

	cp $(PROGRAMS) $(SCRIPTS)  $(INSTALLDIR)


