/* test program for tcs

   DLR 2009 02 04
 
   feed tcs random pointings between RA1 and RA2 and DEC1 and DEC2 every DELTA seconds

*/
#include <stdio.h>
#include <math.h>

#define RA1 4.0
#define RA2 12.0
#define DEC1 +30
#define DEC2 -80
#define NUM_RA_INCR  16
#define NUM_DEC_INCR  15

extern double drand48();
main()
 {
   double ra,dec;
   int i,j;
   
   for(i=1;i<=NUM_RA_INCR;i++){
     ra = RA1 + (RA2-RA1)*i/NUM_RA_INCR;
     for (j=1;j<=NUM_DEC_INCR;j++){
     dec = DEC1 + (DEC2-DEC1)*j/NUM_DEC_INCR;
     printf("%7.3f %7.3f\n",ra,dec);
     }
   }  

}

