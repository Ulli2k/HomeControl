
#include <myRadioProtocols.h>

/*****************************************************************************************************/
/****************************** DATA Transform Functions RX & TX *************************************/
/*****************************************************************************************************/


/********************************************************************/
/************************** FSK - LaCrosse **************************/
/********************************************************************/

// RECEIVE
#define ITPlus_MSG_HEADER_MASK 0xF0
#define ITPlus_MSG_START 0x09
bool transformRxData_LaCrosse(myRADIO_DATA *sData) {

	volatile byte *data = sData->DATA;
	volatile byte *len = &(sData->PAYLOADLEN);

//LaCrosse ITPlus
  //check if Technoline Sensor Signal was received. In that case cut message to 4 data bytes (no CRC print foreseen)

	if( (((data[0] & ITPlus_MSG_HEADER_MASK) >> 4) != ITPlus_MSG_START))
		return false;

	sData->CheckSum = _crc8(data,(*len)-1);
	if(sData->CheckSum != data[(*len)-1])
		return false;

	sData->PAYLOADLEN = 4; // remove CRC if correct
	sData->XData_Config = XDATA_NOTHING;
	return true;
}

// TRANSFER


/********************************************************************/
/************************** FSK - HomeMatic **************************/
/********************************************************************/

// RECEIVE
bool transformRxData_HomeMatic(myRADIO_DATA *sData) {

	volatile byte *data = sData->DATA;
	volatile byte *len = &(sData->PAYLOADLEN);
	uint16_t crc;
	byte this_enc, last_enc;
	byte l;

	sData->XData_Config = XDATA_NOTHING;

	*len = (data[0] ^ 0xFF/*PN9*/) + 1 + 2; //+1 LengthByte; +2 CRC16
	if (*len >= RADIO_PROTOCOL_HOMEMATIC_PAYLOADLENGTH) {
		return false;
	}

	// C1101 specific DataWhitening
	xOr_PN9((uint8_t*)data, *len); //needed due to default XOR of C1101

	// CRC check
	*len -= 2; //remove CRC Values
	crc = calcCRC16hm((uint8_t*)data,(uint8_t)*len); //CheckSum without CRC16
	if ( (uint16_t)((data[*len] << 8) | data[*len+1]) != (uint16_t)crc) {
		return false;
	}

	// HomeMatic specific "crypt"
	last_enc = data[1];
	data[1] = (~data[1]) ^ 0x89;
	for (l = 2; l < data[0]; l++) {
		this_enc = data[l];
		data[l] = (last_enc + 0xdc) ^ data[l];
		last_enc = this_enc;
	}
	data[l] = data[l] ^ data[2];

	sData->PAYLOADLEN = *len;
	//sData->XData_Config=XDATA_NOTHING; //currently not used
	return true;
}

// TRANSFER
void transformTxData_HomeMatic	(char *cmd, myRADIO_DATA *sData) {

	volatile byte *data = sData->DATA;
	volatile byte *len = &(sData->PAYLOADLEN);
	uint8_t ctl;
	uint16_t crc;
	byte l;

	*len = (byte)cmd[0];
	*len += 1; //+1 inclusive LengthByte
	if (*len >= RADIO_PROTOCOL_HOMEMATIC_PAYLOADLENGTH) {
		*len = 0;
		return;
	}

	memcpy((void*)data,(const void*)cmd,*len);

	// HomeMatic specific "crypt"
	ctl = data[2];
  data[1] = (~data[1]) ^ 0x89;
  for (l = 2; l < data[0]; l++)
    data[l] = (data[l-1] + 0xdc) ^ data[l];
  data[l] = data[l] ^ ctl;

	// add CRC bytes
	crc = calcCRC16hm((uint8_t*)data,(uint8_t)*len); //CheckSum
	data[*len] = (crc >> 8);
	data[*len+1] = crc & 0xFF;
	*len += 2;

	// C1101 specific DataWhitening
	xOr_PN9((uint8_t*)data, *len); //needed due to default XOR of C1101

	sData->XDATA_Repeats = 0;
	sData->XDATA_BurstTime = 0;
// active Preamble and Sync
	sData->XData_Config = XDATA_PREAMBLE | XDATA_SYNC;

	if (ctl & (1 << 4)) { // BURST-bit set?
		// According to ELV, devices get activated every 300ms, so send burst for 360ms (CULFW copy)
		sData->XData_Config |= XDATA_BURST;
		sData->XDATA_BurstTime 	= 360; //ms
	}
}

/********************************************************************/
/************************** OOK - FS20 ******************************/
/********************************************************************/

// RECEIVE
bool transformRxData_FS20(myRADIO_DATA *sData) {

	volatile byte *data = sData->DATA;
	volatile byte *len = &(sData->PAYLOADLEN);

	byte out[6] = {0,0,0,0,0,0}; //MAX FS20 DATA LEN
	uint8_t pos=0; //pos in out
	uint8_t b=0;

	sData->XData_Config = XDATA_NOTHING;

	reverseBits((byte*)data,(byte)*len);

	for(uint8_t i=0; i<(*len); i++) { //byte weise

		for(b=b; b<=8; b++) {
			uint8_t c = (uint8_t) ( (data[i+1] << 8 | data[i] ) >> b );
			if( (c & 0x3F /*0b00111111*/) == 0x07 /*0b00000111*/)  { // High = 1
				if((int)(pos/8) > 5) {
					reverseBits((byte*)data,(byte)*len); //reverse Data back
					return false; //ERROR
				}
				out[(int)(pos/8)] |= 1 << (pos%8);
				pos++;
				b+=5;
			} else if( (c & 0x0F /*0b00001111*/) == 0x03 /*0b00000011*/) { // Low = 0
				pos++;
				b+=3;
			}
		}
		if(b>8) b-=8;
	}

	reverseBits(out,6);

  //parity check even -> gerade
	//Synchr	HC1		Parity	HC2		Parity	Adresse	Parity	Befehl	Parity	Quersumme	Parity	EOT
	//13 bit	8 bit	1 bit		8 bit	1 bit		8 bit		1 bit		8 bit		1 bit		8 bit			1 bit		1 bit
	//only one (the last) sync bit is stored
	byte HC_high = (byte) (out[0]);
	byte HC_low = (byte) (out[1] << 1) | ((out[2] >> 7) & 0x01);
	byte Adr = (byte) (out[2] << 2) |  ((out[3] >> 6) & 0x03);
	byte Cmd = (byte) (out[3] << 3) |  ((out[4] >> 5) & 0x07);
	byte QSum = (byte) (out[4] << 4) |  ((out[5] >> 4) & 0x0F);
	byte sum= 0x06 + HC_high + HC_low + Adr + Cmd;

	if( ((out[1] >> 7) & 0x01) != parity(HC_high) ||
			((out[2] >> 6) & 0x01) != parity(HC_low) 	||
			((out[3] >> 5) & 0x01) != parity(Adr) 		||
			((out[4] >> 4) & 0x01) != parity(Cmd) 		||
			((out[5] >> 3) & 0x01) != parity(QSum)		||
			QSum != sum
		) {
		reverseBits((byte*)data,(byte)*len); //reverse Data back
		return false; //ERROR
  }

  data[0] = HC_high;
  data[1] = HC_low;
  data[2] = Adr;
  data[3] = Cmd;
	*len = 4;
	sData->CheckSum = sum;
	//sData->XData_Config = XDATA_NOTHING; //currently not used
	/*
	Serial.print(HC_high,HEX);Serial.print(" ");
	Serial.print(HC_low,HEX);Serial.print(" ");
	Serial.print(Adr,HEX);Serial.print(" ");
	Serial.print(Cmd,HEX);Serial.print(" ");
	//Serial.print(QSum,HEX);Serial.print(" ");
	//Serial.print(sum,HEX);					Serial.print(" ");
	Serial.println();
	*/

	return true;
}

// TRANSFER



/********************************************************************/
/************************** FSK - ETH *******************************/
/********************************************************************/

// RECEIVE

// TRANSFER (00R1s501004300 --> Msg Transformed: 6AA965555595555595555555A55955556A5A9656 <320>)
void transformTxData_ETH(char *cmd, myRADIO_DATA *sData) {

	volatile byte *data = sData->DATA;
	volatile byte *len = &(sData->PAYLOADLEN);
  uint16_t crc=0;
//  static unsigned char cnt;                      // Paket counter
  unsigned char buf[10];//,revbuf[10];              // revbuf beinhaltet das Paket mit reverse angeordneten Bits
  unsigned char b,bitcnt,Einscnt,ethPaketi;

	sData->XData_Config = XDATA_NOTHING;

  ethPaketi = 0;                 // Paketzähler
  Einscnt   = 0;                 // Zähler für die aufeinanderfolgenden einsen
  b         = 0;
  bitcnt    = 0;

//  cnt++;
//  if (cnt==0xFF) cnt=1;

  buf[0] = 0x7e;                   // init --> nach Manchester --> Sync 6A A9
  buf[1] = 2; //cnt führt zu problemen wenn das Packet länger wird   // spielt keine Rolle wenn ein Bitstream zeitlich versetzt mit der gleichen PaketID gesendet wird.
  buf[2] = 0x10;                   // Paketlänge inkl CRC16 --> 0x10 = 16
  buf[3] = (uint8_t)( ((uint32_t)HexStr2uint16(cmd)) >>16); //0x00; //(uint8_t)(Adresse>>16);  // Adresse
  buf[4] = (uint8_t)( ((uint32_t)HexStr2uint16(cmd)) >>8);  //0x12; //(uint8_t)(Adresse>>8);
  buf[5] = (uint8_t)( ((uint32_t)HexStr2uint16(cmd)) );     //0x8F; //(uint8_t)(Adresse);
  buf[6] = HexStr2uint8(cmd+4);    //0x40; //msg.cmd;                 // Commando 40 - Temperatur ändern !, 42 - Tagbetrieb, 43 - Nachtbetrieb
  buf[7] = HexStr2uint8(cmd+6);    //31 wäre zwar maximal von 5 -> 29.5 aber ich wollts mal wissen ;)     // Grad der Erhöhung, 01 -> 0.5 Grad, 02 -> 1 Grad, FF -> -0.5 , FE -> -1.0

  crc=0xc11f;
  for(uint16_t i=0;i<8;i++){
    crc=calcCRC16r(buf[i],crc,0x8408);
  };

  buf[8]=(uint8_t)crc;
  buf[9]=(uint8_t)(crc>>8);

  // alle Bits pro Bytes reversen
	reverseBits(buf,10);

  // Manchestercodierung
  // aus 1- mach 2Bytes. 1 => 10 , 0 => 01
  // bei 5 folgenden 1 was bei minus Temperaturen der FAll ist muss eine 0 eingeschoben werden und alle Bits verschieben sich ! Allerdings
  // nicht bei der Initialisierung 7E.

  // Alle 10 Paket durchlaufen !
  for (uint16_t i=0;i<10;i++){
    // und alle 8 Bits vom Höherwertigen an
    for (int j=7;j>-1;j--){
      //und nach der Hälfte haben wir ein Byte voll und machen mit dem nächsten weiter
      if (bitcnt==8){
        data[ethPaketi++] = b;
        b=0;
        bitcnt=0;
      }
      if((buf[i]&(1<<j)) && (Einscnt==4)){
        // 0 einschieben
        b=b<<1 | 0;
        b=b<<1 | 1;
        bitcnt +=2;
        Einscnt = 0;
        if (bitcnt==8){
          data[ethPaketi++] = b;
          b=0;
          bitcnt=0;
        }
      }

      if(buf[i]&(1<<j)){
        b=b<<1 | 1;
        b=b<<1 | 0;
        if(i>0){
          Einscnt++;
        }
      }
      else{
        b=b<<1 | 0;
        b=b<<1 | 1;
        Einscnt = 0;
      }
      bitcnt = bitcnt + 2;
    }
  }
  if(bitcnt>0) data[ethPaketi++] = b;   // letztes angebrochene Byte sichern

	//ethPaket[0] = ethPaketi-1; //-1 only datalen without [0]

	*len = ethPaketi;
	sData->XData_Config = XDATA_BURST | XDATA_BURST_INFINITY | XDATA_CMD_ECHO;
	sData->XDATA_Repeats = RADIO_PROTOCOL_ETH_REPEATS;
	sData->XDATA_BurstTime = 0;
}

/***************************************************************************/
/************************** FSK 868MHz - MyProtocol ************************/
/***************************************************************************/
// Packet: [Len][toAddress][fromAddress][XData_Config][Msg][2xopt.DelayBurst]

// RECEIVE
bool transformRxData_MyProtocol(myRADIO_DATA *sData) {

	volatile byte *data			= sData->DATA;
	volatile byte *len 			= &(sData->PAYLOADLEN);
	sData->XData_Config			=	XDATA_NOTHING;
	sData->XDATA_BurstTime	=	0;
	sData->XDATA_Repeats		= 0;

//Header
	sData->TARGETID			= data[0];
	sData->SENDERID 		= data[1];
	sData->XData_Config = (data[2] & 0xFFFF);

//BurstDelayTime
	if((sData->XData_Config & XDATA_BURST) && (sData->XData_Config & XDATA_BURST_INFO_APPENDIX)) {
		sData->XDATA_BurstTime	=	data[*len-2] | (data[*len-1] << 8);
		*len = (*len)-2; //remove 2 byes from payload
	}
//CRC
	sData->CheckSum = (byte)_crc8(data,(*len)); //needed for debouncing


//check if all bytes are valid characters
	for(uint8_t i=3;i<(*len);i++) {
		if(data[i] < 0x20 || data[i] > 0x7E) {
			Serial.print("invChar"); Serial.println(data[i],HEX);
			return false;
		}
	}

	*len = (*len)-3; //remove [toAddress][fromAddress][XData_Config]
	shiftCharLeft((byte*)data,3,*len,3);

	return (sData->TARGETID==DEVICE_ID || sData->TARGETID==DEVICE_ID_BROADCAST);
}

// TRANSFER
void transformTxData_MyProtocol(char *cmd, myRADIO_DATA *sData) {
//newPaket darf nie 0xAA enthalten. Dies führt zu Problemen im RADIO
	volatile byte *newPaket = sData->DATA;
	volatile byte *len = &(sData->PAYLOADLEN);

	sData->TARGETID = (cmd[0]-'0')*10+(cmd[1]-'0');
	cmd+=2;

	newPaket[1] = sData->TARGETID;
	newPaket[2] = DEVICE_ID;
	*len = strlen(cmd);

	*len = ((RF69_MAX_DATA_LEN > (*len+6)) ? (*len+6) : RF69_MAX_DATA_LEN); //+[Len][toAddress][fromAddress][DataOptions]....[2xopt.DelayBurst]  -Address[2] schon in newPaket drin

//Data
	for(uint8_t i=0; i<(*len)-6; i++ ) { //[Len][toAddress][fromAddress][DataOptions]....[2xopt.DelayBurst]
			newPaket[i+4] = cmd[i];
	}

// active Preamble and Sync
	sData->XData_Config |= XDATA_PREAMBLE | XDATA_SYNC;

// XData_Config: Burst for ListenMode or NormalMode
	if(sData->XData_Config & XDATA_BURST) {
		sData->XData_Config |= XDATA_BURST_INFO_APPENDIX;
		sData->XDATA_BurstTime 	= 264+10;//[ms] transmission time //numRepeatsInListening((*len));
		sData->XDATA_Repeats		=	0; //[-] number of repeats
	} else {
		*len = (*len)-2; //-[2xopt.DelayBurst]
		sData->XDATA_BurstTime 	= 0;
		sData->XDATA_Repeats		=	0; //[-] number of repeats
	}
	newPaket[0] = (*len)-1; //-[Len]
	newPaket[3] = (sData->XData_Config & 0xFF);

//CRC
	//sData->CheckSum = (byte)_crc8(newPaket+1,(*len)-1); //needed for debouncing
	//newPaket[(*len)-1] = (byte)_crc8(newPaket+1,(*len)-2); //without Length
}


/***************************************************************************/
/************************** OOK 433MHz - HX2262 ****************************/
/***************************************************************************/
// Packet: [Len][toAddress][fromAddress][DataOptions][MsgCounter][Msg][CRC]

// RECEIVE
bool transformRxData_HX2262(myRADIO_DATA *sData) {

	volatile byte *data = sData->DATA;
	volatile byte *len = &(sData->PAYLOADLEN);
	char value=0x0;
	sData->XData_Config	=	XDATA_NOTHING;

	for(uint8_t i=0; i<*len; i++) {
		value=data[i];
		switch(data[i]) {
			case 0x88:
				value='0';
				break;
			case 0x8E:
				value='F';
				break;
			case 0xEE:
				value='1';
				break;
			case 0x80: //sync
				break;
			case 0x00: //sync
				break;
			default:
				return false; //invalid data -> skip package
		}
		data[i] = value;
	}
	// sort value 11 out, only 0F and F0 is valid
	if(data[10] == '1' || data[11] == '1') return false;
	*len = 12;
	//sData->XData_Config = XDATA_NOTHING; //currently not used
	return true;
}

// TRANSFER
void transformTxData_HX2262(char *cmd, myRADIO_DATA *sData) {

	volatile byte *newPaket = sData->DATA;
	volatile byte *len = &(sData->PAYLOADLEN);
	byte value=0x0;
	uint8_t i=0;

	for(i=0; i<(strlen(cmd)); i++) {
		switch(cmd[i]) {
			case '0':
				value=0x88;
				break;
			case 'F':
				value=0x8E;
				break;
			case '1':
				value=0xEE;
				break;
			default:
				*len=0;
				return; //invalid data -> skip package
		}
		newPaket[i] = value;
	}

	//Sync muss am Ende des DataFrames sein
	newPaket[i] = 0x80;
	newPaket[i+1] = 0x00;
	newPaket[i+2] = 0x00;
	newPaket[i+3] = 0x00;

	*len = i+4; //data + Sync
	sData->XDATA_Repeats 		= 10;
	sData->XData_Config			= XDATA_NOTHING; //XDATA_SYNC must be disabled for TX!
}

/********************************************************************/
/**************************** HELPER Functions **********************/
/********************************************************************/

// Returns 1 (ODD "ungerade") or 0 (EVEN "gerade") parity
int parity(unsigned char x) {
	x = x ^ x >> 4;
	x = x ^ x >> 2;
	x = x ^ x >> 1;
	return x & 1;
}

void reverseBits(byte *data, uint8_t len) {
	for (byte i = 0; i < len; ++i) {
		byte b = data[i];
		for (byte j = 0; j < 8; ++j) {
			data[i] = (data[i] << 1) | (b & 1);
			b >>= 1;
		}
	}
}

// Thread LaCrosse CRC
#define CRC_POLY 0x31
uint8_t _crc8(volatile uint8_t *data, uint8_t len ) {
  int i,j;
  uint8_t res = 0;
  for (j=0; j<len; j++) {
    uint8_t val = data[j];
    for( i = 0; i < 8; i++ ) {
      uint8_t tmp = (uint8_t)( ( res ^ val ) & 0x80 );
      res <<= 1;
      if( 0 != tmp ) {
        res ^= CRC_POLY;
      }
      val <<= 1;
    }
  }
  return res;
}

// Thread ETH200 CRC
uint16_t calcCRC16r( uint16_t c,uint16_t crc, uint16_t mask)  // reverser crc!!!!!!
{
  unsigned char i;
  for(i=0;i<8;i++)
  {
    if((crc ^ c) & 1) {
      crc=(crc>>1)^mask;
    }
    else crc>>=1;
    c>>=1;
  };
  return(crc);
}


// Thread HomeMatic - PN9 sequence used for data whitening by the CC110x
// initialize with key = 0xff and carry = 0b1
void xOr_PN9(uint8_t *buf, uint8_t len) {

	uint8_t bit_five, bit_zero, carry_new;
	uint8_t key = 0xFF; // initialize value
	uint8_t carry = 1; // initialize value

  for (uint8_t i = 0; i < len; i++) {
		buf[i] ^= key; //xor

		// PN9 Key generation to get next Key for xOr
		for(uint8_t b = 0; b < 8; b++) {
			bit_five = (key & (1 << 5)) >> 5;
			bit_zero = (key & (1 << 0)) >> 0;
			carry_new = bit_five ^ bit_zero;
			key >>= 1;
			key |= (carry << 7);
			carry = carry_new;
		}
  }
}

// Thread HomeMatic - CRC Algorythmus used by the CC110x
// according to TI appnote DN502
uint16_t calcCRC16hm (uint8_t *data, uint8_t length) {
	uint8_t i, i1, bte;
	uint16_t checksum = 0xffff;

	for (i=0; i < length; i++) {
		bte = data[i];
		for (i1=0; i1 < 8; i1++) {
			if (((checksum & 0x8000) >> 8) ^ (bte & 0x80))
				checksum = (checksum << 1) ^ 0x8005;
			else
				checksum = (checksum << 1);
			bte <<= 1;
		}
	}
	return checksum;
}
