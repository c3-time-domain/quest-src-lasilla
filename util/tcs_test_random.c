/* test program for tcs

   DLR 2009 02 04
 
   feed tcs random pointings between RA1 and RA2 and DEC1 and DEC2 every DELTA seconds

*/
#include <stdio.h>
#include <math.h>

#define RA1 4.0
#define RA2 16.0
#define DEC1 -90
#define DEC2 +30
#define NUM_POINTINGS 240
extern double drand48();
main()
 {
   double ra,dec;
   int i;
   
   for(i=1;i<=NUM_POINTINGS;i++){
     ra = RA1 + (RA2-RA1)*drand48();
     dec = DEC1 + (DEC2-DEC1)*drand48();
     printf("%7.3f %7.3f\n",ra,dec);
   }  

}

