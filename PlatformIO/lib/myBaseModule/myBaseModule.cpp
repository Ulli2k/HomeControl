
#include <myBaseModule.h>

/******************************* Global Helper Functions ******************************/
uint8_t HexChar2uint8(char data) {
  data -= '0';
  if(data>42) data-= 39;  		//a-f
  else if (data>9) data-= 7;	//A-F
  return data;
}

void shiftCharRight(byte *data, uint8_t len, uint8_t bytes) {
	for(uint8_t i=len; i>0 ;i--) {
		data[i+bytes-1] = data[i-1];
	}
}

void shiftCharLeft(byte *data, uint8_t start, uint8_t len, uint8_t bytes) {
	for(uint8_t i=start; i<=start+len ;i++) {
		data[i-bytes] = data[i];
	}
}
