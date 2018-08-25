
#include <myBaseModule.h>

#ifdef QUIETMODE_ACTIVE
byte myDisplay::QuietMode	=	true;
#else
byte myDisplay::QuietMode	=	false;
#endif
char myDisplay::lastPrintChar = '\0';

#ifdef HAS_DISPLAY_TUNNELING
bool myDisplay::DisplayCopy=0;
char myDisplay::cDisplayCopy[MAX_RING_DATA_SIZE] = {0};
#endif
