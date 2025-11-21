#ifndef _NEATTIME_H_
#define _NEATTIME_H_
/* neattime.h
 * @(#)neattime.h	1.4 21 Nov 1995
 *
 * Stuff for manipulating various representations of time.
 *
 * Steve Groom, JPL
 * 7/16/95
 */

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

//#define UT_OFFSET 0.0
/* AA95 is Astronomical Almanac 1995 */
#define UXT0	2440587.5 /* JD of Unix ref time, 1/1/1970 00:00:00, AA95, K2 */
#define MJD0	2400000.5 /* JD of MJD ref time, AA95, B4 */
#define J2000	2451545.0 /* JD for epoch J2000, AA95, B4 */
#define MJ2000	(J2000-MJD0)	/* MJD for epoch J2000 */

EXTERN double get_date_time(struct tm *tm, double ut_offset);/* return UT date from system clock */
EXTERN double neat_gettime_utc(double ut_offset);	/* read system clock */
EXTERN double uxt_mjd(double uxt);	/* convert Unix time to MJD */
EXTERN double uxt_lst(double uxt, double longitude); /* Unix time to LST */
EXTERN double mjd_uxt(double mjd);	/* convert MJD to Unix time */
EXTERN double mjd_lst(double mjd, double longitude);	/* MJD to LST */
EXTERN int advance_tm_day(struct tm *tm);
EXTERN int leap_year_check(int year);


#endif /* _NEATTIME_H_ */
