
#include <myBaseModule.h>
//#include <myDisplay.h>

#ifndef QUIETMODE_DEFAULT_VALUE
	#define QUIETMODE_DEFAULT_VALUE		1
#endif
byte myDisplay::QuietMode	=	QUIETMODE_DEFAULT_VALUE;
char myDisplay::lastPrintChar = '\0';


#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
byte myDisplay::DisplayCopy=0;
char myDisplay::cDisplayCopy[MAX_RING_DATA_SIZE] = {0};
#endif
