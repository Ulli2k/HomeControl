
#include <myBaseModule.h>

byte myDisplay::QuietMode	=	QUIETMODE_DEFAULT_VALUE;
char myDisplay::lastPrintChar = '\0';

#ifdef HAS_DISPLAY_TUNNELING
bool myDisplay::DisplayCopy=0;
char myDisplay::cDisplayCopy[MAX_RING_DATA_SIZE] = {0};
#endif
