#!/bin/sh

PASSWD='Chill'
Host='192.168.178.1'
dly=1
extdly=3

killall nc

(
sleep $extdly
echo "$PASSWD"
sleep $dly
echo "killall nc"
sleep $dly
echo "stty sane -F /dev/ttyUSB0"
sleep $dly
echo "stty -F /dev/ttyUSB0 cs8 115200 ignbrk -icrnl -onlcr -icanon -echo -crtscts -clocal; nc -ll -p 2222 -e nc -f /dev/ttyUSB0 &"
sleep $dly
echo "exit"
) | telnet "$Host"

#make clean
#make target=PowerNode freq=16
#make target=PowerNode freq=16 do_upload
avrdude -b 115200 -p m328p -c arduino -P net:192.168.178.1:2222 -Uflash:w:/tmp/build1585287829747622675.tmp/emonTxV3_4_3Phase_Voltage.cpp.hex

(
sleep $extdly
echo "$PASSWD"
sleep $dly
echo "killall nc"
sleep $dly
echo "stty -F /dev/ttyUSB0 cs8 57600 ignbrk -icrnl -onlcr -icanon -echo -crtscts -clocal; nc -ll -p 2222 -e nc -f /dev/ttyUSB0 &"
sleep $dly
echo "exit"
) | telnet "$Host"

nc "$Host" 2222


