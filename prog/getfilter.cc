/**************************************************************************
 * getfilter
 * get id of currently mounted filter from TCU. Print string indentifying the
 * filter.
 *
 * DLR 2004 June 8
 *
 * Filterd ID's set up by John Henning, 2004 Jun 2:
 *
 * Hex   Filter
 * ____________
 * 01: no_filter
 * 02: Gunn_rizz
 * 04: Johnson_UBRI
 * 08: RG610
 * 10: not_assigned
 * 20: not_assigned
 * 40: not_assigned
 * 80: Clear
 *
 *
 **************************************************************************/

#include <iostream.h>
#include <stdlib.h>
#include <telescope_controller.h>

#define NUM_FILTER_INDICES 8
#define FILTER_NAME_LENGTH 16
#define NUM_FILTER_INDICES 8
#define FILTER_NAME_LENGTH 16
#define FILTER1 "none"
#define FILTER2 "zzir"
#define FILTER3 "RIBU"
#define FILTER4 "RG610"
#define FILTER5 "unknown"
#define FILTER6 "unknown"
#define FILTER7 "unknown"
#define FILTER8 "clear"
#define COM_PORT 1 /* com_port TCS uses to take remote commands */
#define POINT_TIMEOUT 300 /* timeout in seconds for pointing the telescope */
#define SERVER_NAME "ls4-workstn"

int main(int argc, char *argv[]) {
	telescope_controller *tcu;
        int filter_id,index;
        unsigned short i;
        char filter[NUM_FILTER_INDICES*FILTER_NAME_LENGTH];

        strcpy(filter+0,FILTER1);
        strcpy(filter+(FILTER_NAME_LENGTH*1),FILTER2);
        strcpy(filter+(FILTER_NAME_LENGTH*2),FILTER3);
        strcpy(filter+(FILTER_NAME_LENGTH*3),FILTER4);
        strcpy(filter+(FILTER_NAME_LENGTH*4),FILTER5);
        strcpy(filter+(FILTER_NAME_LENGTH*5),FILTER6);
        strcpy(filter+(FILTER_NAME_LENGTH*6),FILTER7);
        strcpy(filter+(FILTER_NAME_LENGTH*7),FILTER8);

	tcu = new telescope_controller(COM_PORT,POINT_TIMEOUT,SERVER_NAME);
        filter_id=tcu->get_filter();
        if (filter_id==-1){
            printf("%s: couldn't get filter id", "getfocus");
            exit(-1); 
        }

        index=-1;
        i=filter_id;
        while(i>0){
	   index++;
	   i=i>>1;
        }

        if (index<=0||index>=NUM_FILTER_INDICES){
           printf("filter: error\n");
        }
        else{
           printf("filter: %s\n",filter+(FILTER_NAME_LENGTH*index));
        }

	delete tcu;

	exit(0);
}

