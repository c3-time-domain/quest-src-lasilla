/*
 * $RCSfile: get_riset.c,v $
 * $Revision: 1.1 $
 * $Date: 2010/10/12 14:37:28 $
 * Code for the calculation of object rise/set times.
 *
 * This code borrowed (and modified slightly) from "XEphem", which included
 * the copyright notice shown below.
 * Steve Groom, JPL
 * 11/21/95
 */
/* static char *sccsid="@(#)riset.c	1.2 21 Nov 1995"; */
//#define MAIN
/*******************************************************************
Xephem Version 2.7.
Copyright (c) 1990,1991,1992,1993,1994,1995 by Elwood Charles Downey

Permission is granted to make and distribute copies of this program free of
charge, provided the copyright notice and this permission notice are
preserved on all copies. All work developed as a consequence of the use of
this program should duly acknowledge such use. All other rights reserved.  No
representation is made regarding the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.

	THE AUTHOR DISCLAIMS ALL  WARRANTIES  WITH  REGARD  TO  THIS
	SOFTWARE,  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABIL-
	ITY AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
	ANY  SPECIAL,  INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAM-
	AGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
	WHETHER  IN  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TOR-
	TIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH  THE  USE
	OR PERFORMANCE OF THIS SOFTWARE.


*******************************************************************/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <mathconst.h>
#include <neattime.h>
#define MAX_HA 5.0 /*hours*/
/*void new_riset (double ra, double dec, double lat, double dis,
	double *lstr, double *lsts, double *azr, double *azs, 
	double *har, double *has, int *status);
*/

/* given the true geocentric ra (hrs), and dec (deg) of an object,
 * the observer's latitude,  lat (deg), and a horizon displacement
 * correction, dis (degrees), find the
 *   local sidereal times and azimuths of rising and setting, lstr/s (hours)
 *   and azr/s (degrees), respectively.
 * dis is the vertical displacement from the true position of the horizon. it
 *   is positive if the apparent position is lower than the true position.
 *   said another way, it is positive if the shift causes the object to spend
 *   longer above the horizon. for example, atmospheric refraction is typically
 *   assumed to produce a vertical shift of 34 arc minutes at the horizon; dis
 *   would then take on the value +9.89e-3 (radians). On the other hand, if
 *   your horizon has hills such that your apparent horizon is, say, 1 degree
 *   above sea level, you would allow for this by setting dis to -1.75e-2
 *   (radians).
 *
 * algorithm:
 *   the situation is described by two spherical triangles with two equal angles
 *    (the right horizon intercepts, and the common horizon transverse):
 *   given lat, d(=d1+d2), and dis find z(=z1+z2) and rho, where      /| eq pole
 *     lat = latitude,                                              /  |
 *     dis = horizon displacement (>0 is below ideal)             / rho|
 *     d = angle from pole = PI/2 - declination                /       |
 *     z = azimuth east of north                            /          |
 *     rho = polar rotation from down = PI - hour angle    /           | 
 *   solve simultaneous equations for d1 and d2:         /             |
 *     1) cos(d) = cos(d1+d2)                           / d2           | lat
 *            = cos(d1)cos(d2) - sin(d1)sin(d2)        /               |
 *     2) sin(d2) = sin(lat)sin(d1)/sin(dis)          /                |
 *   then can solve for z1, z2 and rho, taking       /                 |
 *     care to preserve quadrant information.       /                 -|
 *                                              z1 /        z2       | |
 *                      ideal horizon ------------/--------------------| 
 *                                         | |   /                     N
 *                                          -|  / d1
 *                                       dis | /
 *                                           |/
 *                  apparent horizon  ---------------------------------
 *
 * note that when lat=0 this all breaks down (because d2 and z2 degenerate to 0)
 *   but fortunately then we can solve for z and rho directly.
 *
 * status: 0: normal; 1: never rises; -1: circumpolar; 2: trouble.
 */
void
new_riset (double ra, double dec, double lat, double dis,
	double *lstr, double *lsts, double *azr, double *azs,
	double *har, double *has, int *status) 
{
#define	EPS	(1e-6)	/* math rounding fudge - always the way, eh? */
	double d;	/* angle from pole */
	double h;	/* hour angle */
	double crho;	/* cos hour-angle complement */
	int shemi;	/* flag for southern hemisphere reflection */
        double max_ha;
	ra = ra * 15.0 * RPD;	/* hours to radians */
	dec = dec * RPD;	/* degrees to radians */
	lat = lat * RPD;	/* degrees to radians */
	dis = dis * RPD;	/* degrees to radians */

	d = PI/2 - dec;
	*har=0.0;
	*has=0.0;

	/* reflect if in southern hemisphere.
	 * (then reflect azimuth back after computation.)
	 */
	if ((shemi = (lat < 0)) != 0) {
	    lat = -lat;
	    d = PI - d;
	}

	/* do the easy ones (and avoid violated assumptions) if d arc never
	 * meets horizon. 
	 */
	if (d <= lat + dis + EPS) {
	    *status = -1; /* never sets */
	    return;
	}
	if (d >= PI - lat + dis - EPS) {
	    *status = 1; /* never rises */
	    return;
	}

	/* find rising azimuth and cosine of hour-angle complement */
	if (lat > EPS) {
	    double d2, d1; /* polr arc to ideal hzn, and corrctn for apparent */
	    double z2, z1; /* azimuth to ideal horizon, and " */
	    double a;	   /* intermediate temp */
	    double sdis, slat, clat, cz2, cd2;	/* trig temps */
	    sdis = sin(dis);
	    slat = sin(lat);
	    a = sdis*sdis + slat*slat + 2*cos(d)*sdis*slat;
	    if (a <= 0) {
		*status = 2; /* can't happen - hah! */
		return;
	    }
	    d1 = asin (sin(d) * sdis / sqrt(a));
	    d2 = d - d1;
	    cd2 = cos(d2);
	    clat = cos(lat);
	    cz2 = cd2/clat;
	    z2 = acos (cz2);
	    z1 = acos (cos(d1)/cos(dis));
	    if (dis < 0)
		z1 = -z1;
	    *azr = z1 + z2;
	    *azr = fmod(*azr, PI);
	    crho = (cz2 - cd2*clat)/(sin(d2)*slat);
	} else {
	    *azr = acos (cos(d)/cos(dis));
	    crho = sin(dis)/sin(d);
	}

	if (shemi)
	    *azr = PI - *azr;
        *azs = 2*PI - *azr;
	
	/* find hour angle */
	h = PI - acos (crho);
/**
        *lstr = radhr(ra-h);
	*lsts = radhr(ra+h);
**/
	max_ha=MAX_HA*RPD*15.0;
fprintf(stderr,"%lf %lf\n",max_ha,h);
        if(h>max_ha)h=max_ha;
fprintf(stderr,"%lf %lf\n",max_ha,h);
     
        *lstr = (ra-h)/RPD/15.0;
	*lsts = (ra+h)/RPD/15.0;


	*lstr = fmod(*lstr+24.0, 24.0);
	*lsts = fmod(*lsts+24.0, 24.0);

	*azr /= RPD;
	*azs /= RPD;

        *har = -h/RPD/15.;
        *has = h/RPD/15.;

	*status = 0;
}
/*******************************/

main(int argc, char *argv[])
{
/*
SITE_NAME       "ESO_SCHMIDT/LA_SILLA"   # site name
*/
#define SITE_LAT        -29.257        /* site latitude (deg north) */
#define SITE_LON        289.2705       /* site longitude (deg east) */
#define SITE_ELEV       2347.0         /* site elevation, meters */
#define SITE_MIN_ELEVATION 30.1        /* no pointings allowed below this elevation (except stow)*/
/*
SITE_SAFE_ELEVATION 30.0       # point to zenith first if pointing this close to the lowest dec of the horizon
*/

    double ra;
    double dec;
    double dis = -SITE_MIN_ELEVATION;
    double lat = SITE_LAT;		
    double lon = SITE_LON;
    double lstr;
    double lsts;
    double azr;
    double azs;
    double har, has;

    double lst;
    double uxt;
    double uxts;
    double uxtr;
    int status;
    time_t clock;

    char str[128];
    double fake_ut_offset = 0.0;

    if (argc != 3 && argc != 4)
    {
	fprintf(stderr,"Usage: %s ra dec [fake_ut_offset (hours)] \n",argv[0]);
	exit(1);
    }

    sscanf(argv[1],"%lf",&ra);
    sscanf(argv[2],"%lf",&dec);
    if (argc == 4) sscanf(argv[3],"%lf", &fake_ut_offset);

    new_riset (ra, dec, lat, dis, &lstr, &lsts, &azr, &azs, &har, &has, &status);

    uxt = neat_gettime_utc(fake_ut_offset);
    lst = uxt_lst(uxt,lon);
    uxts = (lsts - lst)*3600.0 + uxt;
    uxtr = (lstr - lst)*3600.0 + uxt;
    printf("uxt %lf\n",uxt);

    hrhmsstr(lst,str);
    clock = (time_t) uxt;
    printf("lst %lf (%s) %s\n",lst,str,ctime(&clock));

    hrhmsstr(lstr,str);
    clock = (time_t) uxtr;
    printf("lstr %f (%s) %s\n",lstr,str,ctime(&clock));

    hrhmsstr(lsts,str);
    clock = (time_t) uxts;
    printf("lsts %f (%s) %s\n",lsts,str,ctime(&clock));

    printf("azr %f\n",azr);
    printf("azs %f\n",azs);
    printf("har %f\n",har);
    printf("has %f\n",has);
    printf("status %d\n",status);
}
/**************************/
