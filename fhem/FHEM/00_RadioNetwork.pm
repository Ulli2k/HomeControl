
# $Id: 00_RadioNetwork.pm 2015-08-03 20:10:00Z Ulli $
#TODO: sendpool integrieren für mehrere RadioNetwork. Keine Sendekonflikte (CUL)

package main;

use strict;
use warnings;
use Time::HiRes qw(gettimeofday);

sub RadioNetwork_Attr(@);
sub RadioNetwork_Clear($);
sub RadioNetwork_HandleWriteQueue($);
sub RadioNetwork_Parse($$);
sub RadioNetwork_Read($);
sub RadioNetwork_ReadAnswer($$$$);
sub RadioNetwork_ReadTranslate($);
sub RadioNetwork_Ready($);
sub RadioNetwork_Write($$);
sub RadioNetwork_WriteTranslate($$$);

sub RadioNetwork_SimpleWrite(@);
sub RadioNetwork_ResetDevice($);
sub RadioNetwork_addUpdateRadioModule($$);
sub RadioNetwork_RemoveLaCrossePair($);
sub RadioNetwork_UpdateADCValue($$$);

sub RadioNetwork_PreConfigure($$);
sub RadioNetwork_allSatellitesInitialized($);
sub RadioNetwork_wait4SatelliteInitialization($);
sub RadioNetwork_SendFromQueue($$$);
sub RadioNetwork_AddQueue($$$);
sub RadioNetwork_HandleWriteQueue($);
sub RadioNetwork_LowLevelHandleWriteQueue($$);
sub RadioNetwork_ACKHandling(@);
sub RadioNetwork_MsgACKTrigger($$);


my $clientsRadioNetwork = ":CUL_IR:HX2272:LaCrosse:ETH200comfort:FS20:";

my %matchListRadioNetwork = (
    "1:CUL_IR"  												=> "^[0-9]{2}I............\$",  					#I
    "2:HX2272"  												=> "^[0-9]{2}R[0-9]2[F01]{12}\$", 			#O0112A0
    "3:LaCrosse"                        => "^[0-9]{2}R[0-9]1[A-F0-9]{8}\$", 			#F019205396A	#Transformed LaCross command for 36_LaCrosse.pm
    "4:ETH200comfort"                   => "^[0-9]{2}R[0-9]s50[AC][0-9A-F]{8}\$", 	#F020A01004200
    "5:FS20"                            => "^[0-9]{2}R[0-9]2[A-F0-9]{8}\$", 			#O02D28C0000
);

my %RadioNetworkProtocols = (
	"01" => "myProtocol",
	"02" => "HX2272",
	"03" => "FS20",
	"04" => "LaCrosse",
	"05" => "ETH",
);
my %revRadioNetworkProtocols = reverse %RadioNetworkProtocols;

my %sets = (
  "raw" 								=> "",
  "reset"    						=> "noArg",
  "ResetSatellite"			=> "",
  "flash"       				=> "",
	"beep" 								=> "",
  "led"      					 	=> "on,off",
  "led-on-for-timer"    => "",
  "LaCrossePairForSec"	=> "",
  "setReceiverMode"			=> join(",", sort values %RadioNetworkProtocols),
);

my %gets = (    # Name, Data to send to the CUL, Regexp for the answer
#  "version"  				=> ["v", '^V[0-9]$'],
  "raw"      						=> ["", '.*'],
  "initRadioNetwork" 		=> 1,
  "RFMconfig"      			=> ["f", "^[0-9]{2}f"],
  "firmware"						=> ["w", "^[0-9]{2}w"],
  "AvailRam"							=> ["m", "^[0-9]{2}m"],
);

my $RadioNetworkWaiting4ACKTime		= 5; #seconds to wait for ACK Messages from Satellites
my $RadioNetworkDeactivateACKfail	= 3; #Number of missed ACK messages before deactivating logical device.

my $ForewardCommands							= "[fR]";	#regex for commands which will be forewarded to logical modules

sub
RadioNetwork_Initialize($)
{
  my ($hash) = @_;

  require "$attr{global}{modpath}/FHEM/DevIo.pm";

# Provider
  $hash->{ReadFn}					= "RadioNetwork_Read";
  $hash->{WriteFn} 				= "RadioNetwork_Write";
  $hash->{ReadyFn} 				= "RadioNetwork_Ready";

	$hash->{ParseFn}				= "RadioNetwork_Parse";
	
# Normal devices
  $hash->{DefFn}        	= "RadioNetwork_Define";
  $hash->{FingerprintFn}  = "RadioNetwork_Fingerprint";
  $hash->{UndefFn}      	= "RadioNetwork_Undef";
  $hash->{GetFn}        	= "RadioNetwork_Get";
  $hash->{SetFn}        	= "RadioNetwork_Set";
  $hash->{AttrFn}       	= "RadioNetwork_Attr";
  $hash->{AttrList} 			= "IODev"
  													." lowPower:active,deactivated"
  													." CmdTunneling:Basic,Burst,Satellite"
  													." setADCThreshold"
  													." MsgACK_Trigger"
						                ." hexFile"
						                ." flashCommand"
						                ." BeepLong BeepShort BeepDelay BeepFrequency"
						                ." $readingFnAttributes";
  #$hash->{ShutdownFn} 		= "RadioNetwork_Shutdown";
}

#####################################
sub
RadioNetwork_Fingerprint($$) ## Notwendig da sonst dispatch nicht funktioniert
{
	my ($name, $msg) = @_;
  
  ## Ergänzung der ursprünglichen Nachricht, da sonst nach Dispatch fälschlicherweise CheckDuplicate eine Nachricht als doppelt erkennt.
	return ($name, "RN: $msg");
}
#####################################

sub
RadioNetwork_Define($$)
{
#define <name> RadioNetwork <device/none> <DeviceID> 
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if(@a != 5) {
    my $msg = "wrong syntax: define <name> RadioNetwork { <phyDevice> | devicename[\@baudrate] ".
                        "| devicename\@directio } <DeviceID> <ModuleID>";
    Log3 undef, 2, $msg;
    return $msg;
  }

  DevIo_CloseDev($hash);

  my $name = $a[0];
  my $dev = $a[2];
  return "DeviceID must be [00-99] a value with two digits." if(uc($a[3]) !~ m/^[0-9][1-9]$/);
  return "ModuleID must be [0-9]." if(defined($a[4]) && uc($a[4]) !~ m/^[0-9]$/);
	my $devID = uc($a[3]);
	my $modID = 0; ##broadcast ModID
	$modID = $a[4] if(defined($a[4]));

  #check if DeviceID and ModuleID is already defined in combination
  foreach my $d (keys %defs) {
		next if($d eq $name);
		if($defs{$d}{TYPE} eq "RadioNetwork") {
	    if(uc($defs{$d}{DeviceID}) =~ m/^$devID/ && $defs{$d}{ModuleID} =~ m/^$modID/) {
          my $m = "$name: Cannot define multiple RadioNetworks with identical DeviceID's ($devID) and ModuleID ($modID).";
          Log3 $name, 1, $m;
          return $m;
        }
      }
	}

  $hash->{DeviceID} = $devID;
  $hash->{ModuleID} = $modID;
  $hash->{RadioModules} = "-";
  $hash->{QueryCmdStr} = $hash->{DeviceID};
  $hash->{Clients} = $clientsRadioNetwork;
  $hash->{MatchList} = \%matchListRadioNetwork;

	# check if it is a logical device #
	if( defined($defs{$dev}) && $defs{$dev}{TYPE} eq "RadioNetwork" && !defined($defs{$dev}{IODev}) ) {
		AssignIoPort($hash,$dev);
		Log3 $name, 1, "$name device is linked to $dev as logical device.";
		
		if($hash->{DeviceID} ne $hash->{IODev}{DeviceID}) {
			$hash->{QueryCmdStr} = $hash->{IODev}{DeviceID} . "R" . $hash->{IODev}{ModuleID} . "s" . substr($revRadioNetworkProtocols{"myProtocol"},1,1) . $hash->{DeviceID};
		}
		RadioNetwork_DoInit($hash);
		return undef;
	}
  	
 	$attr{$name}{flashCommand} = "avrdude -p atmega328P -c arduino -P [PORT] -D -U flash:w:[HEXFILE] 2>[LOGFILE]";
	$attr{$name}{CmdTunneling} = "Basic";
	
  $dev .= "\@57600" if( $dev !~ m/\@/ );
	$hash->{DeviceName} = $dev;
  my $ret = DevIo_OpenDev($hash, 0, "RadioNetwork_DoInit");

  return $ret;
}

#####################################
sub
RadioNetwork_Undef($$)
{
  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};

  foreach my $d (sort keys %defs) {
    if(defined($defs{$d}) &&
       defined($defs{$d}{IODev}) &&
       $defs{$d}{IODev} == $hash)
      {
        my $lev = ($reread_active ? 4 : 2);
        Log3 $name, $lev, "deleting port for $d";
        delete $defs{$d}{IODev};
      }
  }
  DevIo_CloseDev($hash);
  RemoveInternalTimer($hash);
  return undef;
}


sub 
RadioNetwork_RemoveLaCrossePair($)
{
  my $hash = shift;
  delete($hash->{LaCrossePair});
}

#####################################

sub
RadioNetwork_Set($@)
{
  my ($hash, @a) = @_;
  
  return "\"set RadioNetwork\" needs at least one parameter" if(@a < 2);
  
  #problematic...does not work with predefined sets values!?
  #return "Unknown argument $a[1], choose one of " . join(" ", sort keys %sets)
  #	if(!defined($sets{$a[1]}));

  my $name = shift @a;
  my $cmd = shift @a;
  my $arg = join(" ", @a);
  
  my $list = join(" ", map { $sets{$_} eq "" ? $_ : "$_:$sets{$_}" } sort keys %sets);
  #my $list = join(" ", map { "$_:$sets{$_}" } keys %sets); #"beep raw led:on,off led-on-for-timer reset LaCrossePairForSec setReceiverMode:LaCrosse,HX2272,FS20 flash";
  #$list =~ s/(: )/ /g;
  return $list if( $cmd eq '?' || $cmd eq '');


	###### set commands ######
  if($cmd eq "raw") {
    Log3 $name, 3, "set $name $cmd $arg";
    RadioNetwork_Write($hash, $arg);
   
  } elsif ($cmd =~ m/ResetSatellite/i) { #phyDevice must be already initialized
  	
  	my $phyDevBurstMode=0;
  	return "Unknown argument $cmd, device not defined." if(!defined($defs{$arg}));
  	return "ResetSatellite can only be set from physical devices" if(defined($hash->{IODev}));
  	
		#foreach my $d (keys %defs) { #find right Module/Satellite for this message
		#	next if($d eq $name);
			#is valid logical Device (Satellite or add-RFM)
			if( $defs{$arg}{TYPE} eq "RadioNetwork" && defined($defs{$arg}{IODev}) && ($defs{$arg}{IODev} eq $hash)) { 
								
				#be sure that the phyDevice will not be reseted again. (just Satellites, no additional RFM Module)
				if(($hash->{DeviceID} ne $defs{$arg}{DeviceID})) {
					Log3 $name, 3, "$defs{$arg}{NAME} reset";
					#RadioNetwork_SimpleWrite($hash, $hash->{DeviceID} . "T" . $hash->{ModuleID} . "b1"); #set to burst mode
					#send reset command to satellite without ACK-Flag
				 	RadioNetwork_SimpleWrite($hash, $hash->{DeviceID} . "R" . $hash->{ModuleID} . "b" . substr($revRadioNetworkProtocols{"myProtocol"},1,1) . (length($arg)>1 ? $defs{$arg}{DeviceID} : "0".$arg) . "o"); 
				 	sleep 3; #wait till satellite is resetted
				 	
				 	#RadioNetwork_SimpleWrite($hash, $hash->{DeviceID} . "T" . $hash->{ModuleID} . "c1"); #set to Basic Mode for pre configuring
					#--> Satellite is resetted and ready (no Listen Mode)
				}
				RadioNetwork_PreConfigure($defs{$arg},"Satellite"); #will set device state to initialized
				
				#Reset Tunneling mode...not for initializing phase!
#				if(($hash->{DeviceID} ne $defs{$arg}{DeviceID}) && (!defined($hash->{IODev}) && $hash->{STATE} eq "Initialized")) {
#					if(AttrVal($name, "CmdTunneling", "-") eq "Burst" ) {#reset to old tunneling mode
#							RadioNetwork_SimpleWrite($hash, $hash->{DeviceID} . "T" . $hash->{ModuleID} . "b1"); #set to burst mode
#							$attr{$name}{CmdTunneling} = "Burst";
#					}
#				}
				#last;
			}
	#	}
  	
  } elsif ($cmd =~ m/reset/i) {
    
    Log3 $name,3, "$name: reset device";
    
    if(defined($hash->{IODev})) {
    	fhem("set " . $hash->{IODev}{NAME} . " ResetSatellite " . $hash->{NAME});
			if(AttrVal($name, "lowPower", "-") ne "-") {
				fhem("attr " . $name ." lowPower " . AttrVal($name, "lowPower", ""));
			}
    } else {
	    return RadioNetwork_ResetDevice($hash);
    }
    
  } elsif( $cmd eq "flash" ) {
    my @args = split(' ', $arg);
    my $log = "";
    my $hexFile = "";
    my @deviceName = split('@', $hash->{DeviceName});
    my $port = $deviceName[0];
    my $firmwareFolder = "./FHEM/firmware/";
    my $logFile = AttrVal("global", "logdir", "./log") . "/RadioNetwork.log";

    my $detectedFirmware = $arg ? $args[0] : "";
    if(!$detectedFirmware) {
      if($hash->{model} =~ /LaCrosse/ ) {
        $detectedFirmware = "LaCrosse";
      }
      elsif($hash->{model} =~ /pcaSerial/ ) {
        $detectedFirmware = "PCA301";
      }
      elsif($hash->{model} =~ /(RadioNetwork|RN)/ ) {
				#my $conf = substr($hash->{model},index($hash->{model}," - ")+3,index($hash->{model},"]"));
        $detectedFirmware = $hash->{model};
        $detectedFirmware =~ s/ |\[|\]//gi;
        $detectedFirmware =~ s/-/_/g;
        $detectedFirmware =~ s/,/-/g;
      }
    }
    $hexFile = $firmwareFolder . "$detectedFirmware.hex";
    
    
    return "No firmware detected. Please use the firmwareName parameter" if(!$detectedFirmware);
    return "The file '$hexFile' does not exist" if(!-e $hexFile);


    $log .= "flashing RadioNetwork $name\n";
    $log .= "detected Firmware: $detectedFirmware\n";
    $log .= "hex file: $hexFile\n";
    $log .= "port: $port\n";
    $log .= "log file: $logFile\n";

    my $flashCommand = AttrVal($name, "flashCommand", "");

    if($flashCommand ne "") {
      if (-e $logFile) {
        unlink $logFile;
      }

      DevIo_CloseDev($hash);
      $hash->{STATE} = "disconnected";
      readingsSingleUpdate($hash, "state", "disconnected", 1);	
      $log .= "$name closed\n";

      my $avrdude = $flashCommand;
      $avrdude =~ s/\Q[PORT]\E/$port/g;
      $avrdude =~ s/\Q[HEXFILE]\E/$hexFile/g;
      $avrdude =~ s/\Q[LOGFILE]\E/$logFile/g;

      $log .= "command: $avrdude\n\n";
      `$avrdude`;

      local $/=undef;
      if (-e $logFile) {
        open FILE, $logFile;
        my $logText = <FILE>;
        close FILE;
        $log .= "--- AVRDUDE ---------------------------------------------------------------------------------\n";
        $log .= $logText;
        $log .= "--- AVRDUDE ---------------------------------------------------------------------------------\n\n";
      }
      else {
        $log .= "WARNING: avrdude created no log file\n\n";
      }

    }
    else {
      $log .= "\n\nNo flashCommand found. Please define this attribute.\n\n";
    }

    DevIo_OpenDev($hash, 0, "RadioNetwork_DoInit");
    $log .= "$name opened\n";

    return $log;

  } elsif( $cmd eq "beep" ) {
    # +    = Langer Piep
    # -    = Kurzer Piep
    # anderes = Pause

    return "Unknown argument $cmd, choose one of \[+- \]" if($arg !~ m/^[\+\-\s]+$/);

    my $longbeep = AttrVal($name, "BeepLong", "250");
    my $shortbeep = AttrVal($name, "BeepShort", "100");
    my $delaybeep = AttrVal($name, "BeepDelay", "0.25");
    #my $freqbeep = sprintf("%04u", substr(AttrVal($name, "BeepFrequency", "2300"),0,4)); #Vierstellig

    for(my $i=0;$i<length($arg);$i++) {
      my $x=substr($arg,$i,1);
      if($x eq "+") {
              # long beep
              RadioNetwork_Write($hash, $hash->{DeviceID} . "b" . $longbeep);
      } elsif($x eq "-") {
              # short beep
              RadioNetwork_Write($hash, $hash->{DeviceID} . "b"  . $shortbeep);
      }
      select(undef, undef, undef, $delaybeep);
    }

  } elsif ($cmd =~ m/^led$/i) {

    return "Unknown argument $cmd, choose one of $list" if($arg !~ m/^(on|off)$/i);

    Log3 $name, 4, "set $name $cmd $arg";
    RadioNetwork_Write($hash, $hash->{DeviceID} . "l" . ($arg eq "on" ? "1" : "0") );
    
  } elsif ($cmd =~ m/led-on-for-timer/i) {
  
    return "Unknown argument $cmd, choose one of $list" if($arg !~ m/^[0-9]+$/);

    #remove timer if there is one active
    if($modules{RadioNetwork}{ldata}{$name}) {
	    CommandDelete(undef, $name . "_timer");
	    delete $modules{RadioNetwork}{ldata}{$name};
    }

	  Log3 $name, 4, "set $name on";
    RadioNetwork_Write($hash, $hash->{DeviceID} . "l" . "1");
  
    my $to = sprintf("%02d:%02d:%02d", $arg/3600, ($arg%3600)/60, $arg%60);
    $modules{RadioNetwork}{ldata}{$name} = $to;
    Log3 $name, 4, "Follow: +$to setstate $name off";
    CommandDefine(undef, $name."_timer at +$to {fhem(\"set $name led" ." off\")}");
        
  } elsif( $cmd eq "LaCrossePairForSec" ) {
    my @args = split(' ', $arg);

    return "Usage: set $name LaCrossePairForSec <seconds_active> [ignore_battery]" 
    	if(!$arg || $args[0] !~ m/^\d+$/ || ($args[1] && $args[1] ne "ignore_battery") );
    $hash->{LaCrossePair} = $args[1]?2:1;
    InternalTimer(gettimeofday()+$args[0], "RadioNetwork_RemoveLaCrossePair", $hash, 0);

  } elsif( $cmd =~ m/^setReceiverMode/ ) {
    return "Usage: set $name setReceiverMode (" . join(", ", sort values %RadioNetworkProtocols) .")"  
    	if(!defined($revRadioNetworkProtocols{$arg}));
		
		Log3 $name, 4, "set $name $cmd $arg";
		    
		#set receiver mode
   	RadioNetwork_Write($hash, $hash->{DeviceID} . "R" . $hash->{ModuleID} . "r" . $revRadioNetworkProtocols{$arg});  

		#update RFM configuration in FHEM (returns e.g. "FSK-868MHz")
    RadioNetwork_Write($hash, $hash->{QueryCmdStr} . "f" . $hash->{ModuleID});
	
		#reset tunneling function in case of "myProtocol"
		if($arg eq "myProtocol" && (AttrVal($name, "CmdTunneling", "-") eq "Burst") ) {#reset to old tunneling mode
			RadioNetwork_Write($hash, $hash->{DeviceID} . "T" . $hash->{ModuleID} . "b1"); #set to burst mode
			$attr{$name}{CmdTunneling} = "Burst";
		}

  	readingsSingleUpdate($hash, $cmd, $arg, 0);  	

  }

  return undef;
}

#####################################

sub
RadioNetwork_Get($@)
{
  my ($hash, $name, $cmd, @msg ) = @_;
  my $arg = join(" ", @msg);
  my $type = $hash->{TYPE};
  
  return "\"get $type\" needs at least one parameter" if(!defined($name) || $name eq "");
  if(!defined($gets{$cmd})) {
    my @list = map { $_ =~ m/^(raw)$/ ? $_ : "$_:noArg" } sort keys %gets;
    return "Unknown argument $cmd, choose one of " . join(" ", @list);
  }

	return "No $cmd for devices which are defined as none." if(IsDummy($hash->{NAME}));
	
	my ($err, $rmsg);
	
  if( $cmd eq "initRadioNetwork" ) {
		$hash->{STATE} = "Opened";
		readingsSingleUpdate($hash, "state", "Opened", 1);
		RadioNetwork_Write($hash, $hash->{DeviceID} . "o");
		return;
		
	} elsif ($cmd eq "RFMconfig" ) {
		RadioNetwork_Write($hash, $hash->{QueryCmdStr} . "f" . $hash->{ModuleID} );
		return;

	} elsif ($cmd eq "raw" && $arg =~ m/^Ir/) {
		return "raw => 01" if($arg =~ m/^Ir$/);
		RadioNetwork_Write($hash, $hash->{DeviceID} . "Ir" . ($arg =~ m/00/ ? "0" : "1"));
		return "raw => " . substr($arg,2,2);

	} elsif (defined($gets{$cmd})) {
		RadioNetwork_SimpleWrite($hash,  $hash->{QueryCmdStr} . $gets{$cmd}[0]);
    ($err, $rmsg) = RadioNetwork_ReadAnswer($hash, $cmd, 1, $gets{$cmd}[1]); #Dispatchs all commands
    $rmsg = "No answer. $err" if(!defined($rmsg));
	
	} else { ## raw
    RadioNetwork_SimpleWrite($hash,  $hash->{QueryCmdStr} . $gets{$arg}[0]);
    ($err, $rmsg) = RadioNetwork_ReadAnswer($hash, $arg, 1, $gets{$arg}[1]); #Dispatchs all commands
    if(!defined($rmsg)) {
      #DevIo_Disconnected($hash);
      $rmsg = "No answer. $err";	
    }
	}

  readingsSingleUpdate($hash, $cmd, $rmsg, 0);  	

  return "$name $cmd => $rmsg";
}

sub
RadioNetwork_Clear($)
{
  my $hash = shift;

  # Clear the pipe
  $hash->{RA_Timeout} = 1;
  for(;;) {
    my ($err, undef) = RadioNetwork_ReadAnswer($hash, "Clear", 0, undef);
    last if($err);
  }
  delete($hash->{RA_Timeout});
}

#####################################
sub
RadioNetwork_DoInit($)
{
  my $hash = shift;
  my $name = $hash->{NAME};
  my $err;
  my $msg = undef;
  my $to=0.1;
	
	if($hash->{STATE} ne "Opened") {
		readingsSingleUpdate($hash, "state", "Opened", 1);
	 # Reset the counter
		delete($hash->{XMIT_TIME});
		delete($hash->{NR_CMD_LAST_H});
		RadioNetwork_ACKHandling($hash,"init","");
		delete($hash->{READINGS}{ACK});
	}
	 
	if(defined($hash->{IODev})) { # logical Device wait for phyDevice initialization
		if($hash->{IODev}{STATE} eq "ready4Satellites") {
			fhem("set " . $hash->{IODev}{NAME} . " ResetSatellite " . $name);
		} else {
			InternalTimer(gettimeofday()+$to, "RadioNetwork_DoInit", $hash, 0);
		}
	} else { #phyDevice only
		RadioNetwork_Clear($hash);
	}
	
  return undef;
}

#####################################
# This is a direct read for commands like get
# Anydata is used by read file to get the filesize
sub RadioNetwork_ReadAnswer($$$$) {

  my ($hash, $arg, $alwaysDispatch, $regexp) = @_;
  my $ohash = $hash;

  while($hash->{TYPE} ne "RadioNetwork" || defined($hash->{IODev})) {   # Look for the first "real" CUL
    $hash = $hash->{IODev};
  }
  return ("No FD", undef)
        if(!$hash || ($^O !~ /Win/ && !defined($hash->{FD})));

  my ($mdata, $rin) = ("", '');
  my $buf;
  my $to = 3;                                         # 3 seconds timeout
  $to = $ohash->{RA_Timeout} if($ohash->{RA_Timeout});  # ...or less
  for(;;) {

    if($^O =~ m/Win/ && $hash->{USBDev}) {
      $hash->{USBDev}->read_const_time($to*1000); # set timeout (ms)
      # Read anstatt input sonst funzt read_const_time nicht.
      $buf = $hash->{USBDev}->read(999);
      return ("Timeout reading answer for get $arg", undef)
        if(length($buf) == 0);

    } else {
      return ("Device lost when reading answer for get $arg", undef)
        if(!$hash->{FD});

      vec($rin, $hash->{FD}, 1) = 1;
      my $nfound = select($rin, undef, undef, $to);
      if($nfound < 0) {
        next if ($! == EAGAIN() || $! == EINTR() || $! == 0);
        my $err = $!;
        DevIo_Disconnected($hash);
        return("RadioNetwork_ReadAnswer $arg: $err", undef);
      }
      return ("Timeout reading answer for get $arg", undef)
        if($nfound == 0);
      $buf = DevIo_SimpleRead($hash);
      return ("No data", undef) if(!defined($buf));

    }

    if(defined($buf)) {
      Log3 $ohash->{NAME}, 5, "RadioNetwork/RAW (ReadAnswer): $buf";
      $mdata .= $buf;
    }

    # Dispatch data in the buffer before the proper answer.
    while(($mdata =~ m/^([^\n]*\n)(.*)/s)) {
      #my $line = ($anydata ? $mdata : $1);
      my $line = $mdata;
      $mdata = $2;
      #(undef, $line) = RadioNetwork_prefix(0, $ohash, $line); # Delete prefix
      if(($regexp && $line !~ m/$regexp/) || $alwaysDispatch) {
        RadioNetwork_Parse($hash, $line);
      }
      return (undef, $line) if($regexp && $line =~ m/$regexp/)
    }
  }
}

#####################################
# Check if the 1% limit is reached and trigger notifies
sub
RadioNetwork_XmitLimitCheck($$$)
{
  my ($hash,$fn,$now) = @_;

  if(!$hash->{XMIT_TIME}) {
    $hash->{XMIT_TIME}[0] = $now;
    $hash->{NR_CMD_LAST_H} = 1;
    return;
  }

  my $nowM1h = $now-3600;
  my @b = grep { $_ > $nowM1h } @{$hash->{XMIT_TIME}};

  if(@b > 163) {          # Maximum nr of transmissions per hour (unconfirmed).
    my $name = $hash->{NAME};
    Log3 $name, 2, "RadioNetwork TRANSMIT LIMIT EXCEEDED";
    DoTrigger($name, "TRANSMIT LIMIT EXCEEDED");
  } else {
    push(@b, $now);
  }
  $hash->{XMIT_TIME} = \@b;
  $hash->{NR_CMD_LAST_H} = int(@b);
}

#####################################
# ACK Message handling
sub RadioNetwork_ACKHandling(@) {
	my ($hash,$handling,$msg) = @_;
	my $HASH;
	
	if(defined($hash->{HASH})) { #executed by InternalTimer?
		$HASH = $hash->{HASH};
		$handling = "remove";
		$msg="";
	} else {
		($hash,$handling,$msg) = @_;
	}	
	
	if($handling =~ m/^init$/) {
		delete($hash->{Waiting4ACK});
		delete($hash->{Waiting4ACK_TIME});
			
	} elsif($handling =~ m/^waiting$/) {
		return defined($hash->{Waiting4ACK});
	
	} elsif($handling =~ m/^add$/) {
			$hash->{Waiting4ACK} = $msg;
			$hash->{Waiting4ACK_TIME} = gettimeofday();
			my %addvals = (Waiting4ACK => $hash->{Waiting4ACK}, Waiting4ACK_TIME => $hash->{Waiting4ACK_TIME}, HASH => $hash);
			InternalTimer(gettimeofday()+$RadioNetworkWaiting4ACKTime, "RadioNetwork_ACKHandling", \%addvals, 0);	
	
	} elsif($handling =~ m/^remove$/) { #only called from InternalTimer to be sure that ACK of specific message was already received
		return if(!defined($HASH->{Waiting4ACK}) || $HASH->{Waiting4ACK} ne $hash->{Waiting4ACK} || $HASH->{Waiting4ACK_TIME} ne $hash->{Waiting4ACK_TIME}); #ACK already received

		Log3 $HASH->{NAME},1, "$HASH->{NAME}: Waiting4ACK expired for command <$hash->{Waiting4ACK}>";

		#$HASH->{Waiting4ACK_Counter} = ((defined($HASH->{Waiting4ACK_Counter}) ? $HASH->{Waiting4ACK_Counter}+1 : 1));  #Auskommentiert sonst wird doppelt gezählt, wegen InternalTimer prüfung
		RadioNetwork_MsgACKTrigger($HASH,$HASH->{Waiting4ACK});
		delete($HASH->{Waiting4ACK});
		delete($HASH->{Waiting4ACK_TIME});
			
	} elsif($handling =~ m/^received$/) {

		my $ACK_CMD = (defined($hash->{Waiting4ACK})) ? $hash->{Waiting4ACK} : "...";
		my $ACKtime = substr($msg,7,length($msg)-7);
		readingsSingleUpdate($hash,"ACK",$ACK_CMD . " " . $ACKtime,1);
		if( $msg =~ m/missed/) {
			$hash->{Waiting4ACK_Counter} = ((defined($hash->{Waiting4ACK_Counter}) ? $hash->{Waiting4ACK_Counter}+1 : 1)); #+1 ACK missed counter
			Log3 $hash->{NAME},1,"$hash->{NAME}: <" .$ACK_CMD . "> failed" ;
		} else {
			Log3 $hash->{NAME},4,"$hash->{NAME}: ACK [" . $ACK_CMD . "] " . $ACKtime;
			delete($hash->{Waiting4ACK_Counter}) if(defined($hash->{Waiting4ACK_Counter})); #reset counter
		}
		
		RadioNetwork_MsgACKTrigger($hash,$hash->{Waiting4ACK});
		delete($hash->{Waiting4ACK}) if(defined($hash->{Waiting4ACK}));
		delete($hash->{Waiting4ACK_TIME}) if(defined($hash->{Waiting4ACK_TIME}));	
		
		if(defined($hash->{Waiting4ACK_Counter}) && $hash->{Waiting4ACK_Counter} >= $RadioNetworkDeactivateACKfail) { #deactivate device due to max failed ACK reached
			Log3 $hash->{NAME}, 1, "$hash->{NAME}: Device will be deactivated due to max number of failed ACK reached. ($RadioNetworkDeactivateACKfail)";
      DevIo_CloseDev($hash);
      $hash->{STATE} = "disconnected";
      readingsSingleUpdate($hash, "state", "disconnected", 1);
		}
	} else {
		Log3 $hash->{NAME},1,"$hash->{NAME}: ACK handling command unknown!" ;
	}
}

sub RadioNetwork_MsgACKTrigger($$) {
	
	my ($hash, $CmdTrigger) = @_;
	my $JustOnce=0;
	
	return if(!defined($attr{$hash->{NAME}}{MsgACK_Trigger}));
	
  foreach my $trigger (split(/;/, $attr{$hash->{NAME}}{MsgACK_Trigger})) {
			my ($cmd, $value) = split(/:/,$trigger,2);
			next if(!length($cmd) || !length($value));
			if($cmd =~ m/^!(.*)/) { #remove from Attribute String
				$cmd = $1;
				$JustOnce=1;
			}
			if($CmdTrigger =~ m/$cmd/ || ($CmdTrigger =~ m/$hash->{DeviceID} . $cmd/)) {
				if($JustOnce) { #remove from Attribute String
					$attr{$hash->{NAME}}{MsgACK_Trigger} =~ s/$trigger//;
					$attr{$hash->{NAME}}{MsgACK_Trigger} =~ s/;;//g;
					delete($attr{$hash->{NAME}}{MsgACK_Trigger}) if(!length($attr{$hash->{NAME}}{MsgACK_Trigger}) || $attr{$hash->{NAME}}{MsgACK_Trigger} eq ";");
				}
				Log3 $hash->{NAME},5,"$hash->{NAME}: ACK <$cmd> trigger";
				fhem($value);
			}
 	}
}

sub
RadioNetwork_SimpleWrite(@)
{
  my ($hash, $msg, $nocr) = @_;
  return if(!$hash);

  my $name = $hash->{NAME};

	$msg =~ s/^\s+|\s+$//g; #löscht Leerzeichen
	$msg =~ s/\v//g; #löscht \r\n

	return if(!length($msg));
	if($hash->{STATE} eq "disconnected") {
		Log3 $name, 1, "$name: SW: <$msg> skipped due to disconnected device.";
		return;
	}
  Log3 $name, 5, "$name: SW: <$msg>" if(!(defined($hash->{IODev})));

	#write from logical Device to phy Device
	if(defined($hash->{IODev})) { # logical Device
		if(!RadioNetwork_ACKHandling($hash,"waiting","")) {
		
			if($hash->{IODev}{STATE} eq "ready4Satellites" && $hash->{STATE} eq "Opened") {
				RadioNetwork_SimpleWrite($hash->{IODev},$msg);
			} else {
				IOWrite($hash,$msg);
			}
			if( ($hash->{DeviceID} ne $hash->{IODev}{DeviceID}) && ((substr($msg,0,2) eq $hash->{DeviceID}) || ($msg =~ m/^$hash->{QueryCmdStr}/)) ) { #just for satellites and their commands "<DevID:2>..."
				RadioNetwork_ACKHandling($hash,"add",$msg);
			}
		} else {
			RadioNetwork_AddQueue($hash, $msg, "SATELLITE_ACKQUEUE");
		}
		return;
	}
	
	$msg .= "\n" unless($nocr);
  $hash->{USBDev}->write($msg)    if($hash->{USBDev});
  syswrite($hash->{TCPDev}, $msg) if($hash->{TCPDev});
  syswrite($hash->{DIODev}, $msg) if($hash->{DIODev});

  # Some linux installations are broken with 0.001, T01 returns no answer
  select(undef, undef, undef, 0.01);
}
 
#####################################
# Translate data prepared for an other syntax, so we can reuse
# the FS20. 
sub
RadioNetwork_WriteTranslate($$$)
{
  my ($hash,$cmd,$msg) = @_;

  ###################
  # Rewrite message from RadioNetwork -> CUL-IR
  $msg =~ s/^Is/$hash->{QueryCmdStr}I/i if(defined($msg));  #SendIR command is "I" not "Is" for RadioNetwork devices

  return ($cmd, $msg);
}

sub
RadioNetwork_Write($$)
{
	my ($hash, $cmd, $msg) = @_;
	my $LogMsg;

  my ($dcmd, $dmsg) = RadioNetwork_WriteTranslate($hash, $cmd, $msg);
  return if(!defined($dcmd));

	my $bstring = $dcmd;
  $bstring .= " " if($bstring ne "" && $dcmd ne " ");
  $bstring .= $dmsg if(defined($dmsg) && $dmsg ne "" && $dmsg ne " ");
  
  Log3 $hash->{NAME}, 5, "$hash->{NAME} sendingQueue $bstring";  
  
  RadioNetwork_AddQueue($hash, $bstring, "QUEUE");
}

sub
RadioNetwork_SendFromQueue($$$)
{
  my ($hash, $bstring, $QueueName) = @_;
  my $name = $hash->{NAME};
  my $to = ($QueueName eq "QUEUE" ? 0.01 : 0.05);#0.01 bei phyDevice; 0.1 bei logicalDevice
  my $now = gettimeofday();

  if($bstring ne "") {
  # ($hash->{STATE} ne "Initialized" && $hash->{STATE} ne "ready4Satellites" )) || 
  	if( ($QueueName eq "QUEUE" && $hash->{STATE} ne "Initialized" ) || 
  			($QueueName eq "SATELLITE_ACKQUEUE" && RadioNetwork_ACKHandling($hash,"waiting","")) ) {
			unshift(@{$hash->{$QueueName}}, ""); #fügt vorne im Array was ein
			InternalTimer($now+$to, "RadioNetwork_HandleWriteQueue", $hash, 0);
			return;
  	}

    RadioNetwork_XmitLimitCheck($hash,$bstring, $now);
    RadioNetwork_SimpleWrite($hash, $bstring);
  }

  InternalTimer($now+$to, "RadioNetwork_HandleWriteQueue", $hash, 1);
}

sub
RadioNetwork_AddQueue($$$)
{
  my ($hash, $bstring, $QueueName) = @_;
  if(!$hash->{$QueueName} || 0 == scalar(@{$hash->{$QueueName}})) { #no other Data available and Device Initialized
    $hash->{$QueueName} = [ $bstring ];
    RadioNetwork_SendFromQueue($hash, $bstring, $QueueName);

  } else { #already Data available
    push(@{$hash->{$QueueName}}, $bstring);
  }
}

sub RadioNetwork_LowLevelHandleWriteQueue($$) {
  my ($hash, $QueueName) = @_;
  my $arr = $hash->{$QueueName};
 
 	#Log3 $hash->{NAME}, 5, "HandleWriteQueue($QueueName): " . join(', ', @{$hash->{$QueueName}}) if(defined($hash->{$QueueName}));
 
  if(defined($arr) && @{$arr} > 0) {
    shift(@{$arr});
    if(@{$arr} == 0) {
      delete($hash->{$QueueName});
      return;
    }
    my $bstring = $arr->[0];
    if($bstring eq "") {
      RadioNetwork_HandleWriteQueue($hash);
    } else {
      RadioNetwork_SendFromQueue($hash, $bstring, $QueueName);
    }
  }
}

sub
RadioNetwork_HandleWriteQueue($)
{
  my $hash = shift;

	RadioNetwork_LowLevelHandleWriteQueue($hash,"QUEUE") 		if($hash->{QUEUE} && scalar(@{$hash->{QUEUE}}) > 0);
	RadioNetwork_LowLevelHandleWriteQueue($hash,"SATELLITE_ACKQUEUE") if($hash->{SATELLITE_ACKQUEUE} && scalar(@{$hash->{SATELLITE_ACKQUEUE}}) > 0);
}

#####################################
# called from the global loop, when the select for hash->{FD} reports data
sub
RadioNetwork_Read($)
{
  my ($hash) = @_;

  my $buf = DevIo_SimpleRead($hash);
  return "" if(!defined($buf));
  my $name = $hash->{NAME};

  my $pdata = $hash->{PARTIAL};
  Log3 $name, 5, "RadioNetwork/RAW: $pdata/$buf";
  $pdata .= $buf;

  while($pdata =~ m/\n/) {
    my $rmsg;
    ($rmsg,$pdata) = split("\n", $pdata, 2);
    $rmsg =~ s/\v//g;
   	$rmsg =~ s/^\s+|\s+$//g; #remove white space
    RadioNetwork_Parse($hash, $rmsg) if($rmsg);
  }
  $hash->{PARTIAL} = $pdata;
}
 
sub RadioNetwork_Parse($$) {

	my ($hash, $rmsg) = @_;

	my $name = $hash->{NAME};
	my $dmsg = $rmsg;
	my $devID = substr($rmsg,0,2);
	my $modID = substr($rmsg,3,1);

  #update RSSI Readings if info is available
	if( $rmsg =~ m/\[RSSI:/) {
		my $RSSI = substr($rmsg,index($rmsg, "\[RSSI:")+6,index($rmsg, "\]\n"));
		readingsSingleUpdate($hash,"RSSI",$RSSI,0);
		$rmsg = substr($rmsg,0,index($rmsg," \[RSSI")); #remove RSSI value
	}

  if(	$rmsg =~ m/^Available commands:/ ||    # ignore Help/Debug messages
  		$rmsg =~ m/^<</ ||
  		$rmsg =~ m/^##/ ||
  		$rmsg =~ m/^\* \[/ ||
  		$rmsg =~ m/^Protocols:</ ||
  		$rmsg =~ m/^\[00-99:DeviceID\]/
  	) {
  	Log3 $name, 3, "$name: $rmsg";
  	return;
  }


	if ( $rmsg =~ m/^[0-9]{2}f[0-9][0-9]{2}[48]/) { #add new RadioModules of same Device
	  #<DeviceID>{2} f <ModuleID>{1} Protocol{2} Frequenz{1}    #05f2018
		RadioNetwork_addUpdateRadioModule($hash, $rmsg);
	}
	
  #check if right DeviceID && ModuleID otherwise forward msg to other define. if successful exit otherwise Dispatch
 	return if(RadioNetwork_forewardMsg($hash, $rmsg));

	# Commands which can only be received from the main physical Module/Device
	Log3 $name, 5, "Parse: <$rmsg>";
  if($rmsg =~ m/^\[RN.+\]$/ && $hash->{STATE} eq "Opened") { #Initialization
    RadioNetwork_PreConfigure($hash,$rmsg);
		return;
	} elsif($rmsg =~ m/^[0-9]{2}w\[RN.+\]$/) {
		$hash->{model} = substr($rmsg,3,length($rmsg)-3);
		return;
	} elsif ( $rmsg =~ m/^[0-9]{2}f[0-9][0-9]{2}[48]/) {
		return; #already handled above
	} elsif ( $rmsg =~ m/^[0-9]{2} ACK /) {
		RadioNetwork_ACKHandling($hash,"received",$rmsg);
		return;
  } elsif ( $rmsg =~ m/^[0-9]{2}m[0-9999]/) {
		readingsSingleUpdate($hash,"freeMemory",substr($rmsg,3,length($rmsg)-3),0);
    return;
  } elsif ( $rmsg =~ m/^[0-9]{2}C[0-9][0-9999]/) {
  	RadioNetwork_UpdateADCValue($hash,substr($rmsg,3,1),substr($rmsg,4,length($rmsg)-4));
 		#readingsSingleUpdate($hash,"ADC" . substr($rmsg,3,1),substr($rmsg,4,length($rmsg)-4),1);
  	return;
  } elsif ( $rmsg =~ m/^[0-9]{2}A[0-9999]/) {
 		readingsSingleUpdate($hash,"VCC",substr($rmsg,3,length($rmsg)-3),1);
  	return;
  }
  	
	#Translate Msg if necessary for other Modules for dispatching
	$dmsg = RadioNetwork_ReadTranslate($rmsg);
	
	
	$hash->{"${name}_MSGCNT"}++;
  $hash->{"${name}_TIME"} =
  #$hash->{READINGS}{state}{TIME} = TimeNow();      # showtime attribute
  # showtime attribute
  readingsSingleUpdate($hash, "state", $hash->{READINGS}{state}{VAL}, 0);
  $hash->{RAWMSG} = $rmsg;
  my %addvals = (RAWMSG => $dmsg);

  Dispatch($hash, $dmsg, \%addvals);
}


sub RadioNetwork_forewardMsg($$) {

	my ($hash, $rmsg) = @_;
	my $name = $hash->{NAME};
	my $devID = substr($rmsg,0,2);
	return 0 if($devID !~ m/^[0-9]{2}/ || length($rmsg)<3);
	my $modID = substr($rmsg,3,1);
	my $isRadioCmd=(substr($rmsg,2,1) =~ m/$ForewardCommands/); #is RFM command with ModuleNo info
	
	return 0 if($devID eq $hash->{DeviceID} && ( ($modID eq $hash->{ModuleID} && $isRadioCmd) || !$isRadioCmd)); #is right Device

	$modID = "1" if(!$isRadioCmd);
	
	foreach my $d (keys %defs) { #find right Module for this message
		next if($d eq $name);

		if( ($defs{$d}{TYPE} eq "RadioNetwork") && (defined($defs{$d}{IODev}) && ($defs{$d}{IODev} eq $hash)) && ($defs{$d}{DeviceID} eq $devID && $defs{$d}{ModuleID} eq $modID) ) {
      Log3 $name, 5, "$name: Forward message <$rmsg> to right define " . $defs{$d}{NAME} .".";#to Device ($devID) with Module ($modID).";   
      RadioNetwork_Parse($defs{$d},$rmsg);
      return 1;
		}
	}
	Log3 $name, 3, "$name: Could not forward message <$rmsg> due to not defined Device ($devID) with Module ($modID).";
	return 1;	
}

#Will be called from phy Devices after receiving InitMsg "[RN-...]"
#Will be called from log Devices when phy is ready4Satellites and set command resetSatellite was called from DoInit()
sub RadioNetwork_PreConfigure($$) {
	
	my ($hash, $rmsg) = @_;
	my $name = $hash->{NAME};
  $hash->{model} = $rmsg;
	
	#Initialize one Device just once. same DeviceID
	if(! (defined($hash->{IODev}) && ($hash->{DeviceID} eq $hash->{IODev}{DeviceID}))) {
	
		if(defined($hash->{IODev})) { #is logical Device
			#set logicalDevice up to tunnel commands to phyDevice
			#Log3 $name,1, "Q0 für Satellite";
			RadioNetwork_SimpleWrite($hash, $hash->{QueryCmdStr} . "q1"); #turn quiet mode off for satellites
			RadioNetwork_SimpleWrite($hash, $hash->{QueryCmdStr} . "T" . $hash->{ModuleID} . "c1"); #CmdTunneling for satellites
			RadioNetwork_SimpleWrite($hash, $hash->{QueryCmdStr} . "w"); #request Firmware
			$attr{$name}{CmdTunneling} = "Satellite";
	
		} else { #is physical Device
			RadioNetwork_SimpleWrite($hash, $hash->{DeviceID} . "q1");  # turn quiet mode off
			RadioNetwork_SimpleWrite($hash, $hash->{DeviceID} . "R" . $hash->{ModuleID} . "r" . $revRadioNetworkProtocols{"myProtocol"});# myProtocol as default Protocol
			RadioNetwork_SimpleWrite($hash, $hash->{DeviceID} . "a0");  # turn activity led off		
		}
	
		RadioNetwork_SimpleWrite($hash, $hash->{QueryCmdStr} . "m");   # show used ram

		#my $DebStr = AttrVal($name, "DebounceTime", undef);					# set debounce time
		#RadioNetwork_SimpleWrite($hash, $hash->{DeviceID} . "R" . $hash->{ModuleID} . "d" . $DebStr) if(defined $DebStr);

		RadioNetwork_SimpleWrite($hash, $hash->{QueryCmdStr} . "f0");   # get all RFM frequence configs
		RadioNetwork_SimpleWrite($hash, $hash->{QueryCmdStr} . "A");   # get supply voltage
	} else { 
		#is logical device but only an other RFM Module
	}
	
	#Check if there are Satellites which are not already initialized
	my $SatellitesInitialized = RadioNetwork_allSatellitesInitialized($hash);
	
	#Initialize phyDevice than all logicalDevices than set phyDebive to "initialized"
	if(!defined($hash->{IODev}) && !$SatellitesInitialized) { #phyDevice and satellites available
		$hash->{STATE} = "ready4Satellites";
		readingsSingleUpdate($hash, "state", "ready4Satellites", 1);
		Log3 $name, 5,"$name device waiting for Satellites";  	    
    return;
	}

	RadioNetwork_wait4SatelliteInitialization($hash);

}

#Function waits till all Commands from Satellites which were sent over SimpleWrite and Queued in "Satellite_ACKQUEUE" were sent and answered with ACK
#Afterwards Satellite Device will be marked as Initialized
sub RadioNetwork_wait4SatelliteInitialization($) {
	
	my ($hash) = @_;
	my $name = $hash->{NAME};
	
	if(defined($hash->{SATELLITE_ACKQUEUE}) && (scalar(@{$hash->{SATELLITE_ACKQUEUE}}) > 0) && RadioNetwork_ACKHandling($hash,"waiting","")) {
		InternalTimer(gettimeofday()+1, "RadioNetwork_wait4SatelliteInitialization", $hash, 0);	
		return 0;	
	}
	
	readingsSingleUpdate($hash, "state", "Initialized", 1);	
	Log3 $name,3,"$name device initialized";
	
#Check if phy Device can be set as initialized
	#Check if there are Satellites which are not already initialized
	my $SatellitesInitialized = RadioNetwork_allSatellitesInitialized($hash);

	#phyDevice initialized?
	if($SatellitesInitialized && defined($hash->{IODev})) { #last Satellite initialized, now intialize phyDevice
		readingsSingleUpdate($hash->{IODev}, "state", "Initialized", 1);
		Log3 $hash->{IODev}{NAME},3,$hash->{IODev}{NAME} . " device initialized";
	}	
	
	return 1;
}

sub RadioNetwork_allSatellitesInitialized($) {

	my ($hash) = @_;
	my $name = $hash->{NAME};
	
	#Check if there are Satellites which are not already initialized
	my $otherSatellites2Initialize=0;
	my $phyhash = ((defined($hash->{IODev})) ? $hash->{IODev} : $hash); #hash of physicalDevice
	
	foreach my $d (keys %defs) { #find if Satellites are available and not initialized
		next if($d eq $name);
		if( ($defs{$d}{TYPE} eq "RadioNetwork") && 
				(defined($defs{$d}{IODev}) && ($defs{$d}{IODev} eq $phyhash)) && 
				#($defs{$d}{DeviceID} ne $hash->{DeviceID}) && 
				$defs{$d}{STATE} ne "Initialized" ) {
#				$defs{$d}{STATE} ne "disconnected" ) {
			$otherSatellites2Initialize=1;
			last;
		}
	}
	
	return !$otherSatellites2Initialize;
}

#####################################
# return 1 ModeID richtig
# return 0 dispatch
sub 
RadioNetwork_addUpdateRadioModule($$)
{ # [0-9]-(OOK|FSK)\-(433|868)MHz
 #  #<DeviceID>{2} f <ModuleID>{1} Protocol{2} Frequenz{1} --> 05f2018
	my ($hash, $a) = @_;
	my $modID = substr($a,3,1);
	my $protocol = substr($a,4,2);
	my $freq = (substr($a,6,1) eq "4" ? "433 MHz" : "868 MHz");

	if($hash->{"RadioModules"} eq "-") {
		$hash->{"RadioModules"} = $modID;
	} else {
		$hash->{"RadioModules"} .= ", $modID" if($hash->{"RadioModules"} !~ m/$modID/);
	}	
	
	if($modID eq $hash->{ModuleID}) {
		#readingsSingleUpdate($hash,"Mode",$mode,0);
		readingsSingleUpdate($hash,"Frequence", $freq,0);
		readingsSingleUpdate($hash,"Protocol", $protocol,0);
		readingsSingleUpdate($hash,"setReceiverMode", $RadioNetworkProtocols{$protocol},0);
		return 1;
	}
	return 0;
}

#####################################
sub getMatchRegExOfString($$) {
    my ($string, $regex) = @_;
    return if not $string =~ /($regex)/;
		return $1;
}

sub RadioNetwork_UpdateADCValue($$$) { #2:2000,100 6:2000,100 7:2000,100

	my ($hash, $id, $value) = @_;
	my $name = $hash->{NAME};
	my $aVal = AttrVal($name, "setADCThreshold", "-");
	
	return if($aVal eq "-");

	my $Threshold = getMatchRegExOfString($aVal, "$id:\\d+,\\d+");
	return if(length($Threshold)==0);
	my @aThreshold = split(/,/,(split(/:/,$Threshold))[1]);
	
	#Log3 $name,1,"$name $aThreshold[0] $aThreshold[1]";
	my $curThreshold = ReadingsVal($name,"ADC" . $id . "_Threshold","-");
	my $newThreshold;
	
	if($curThreshold eq "-" || $curThreshold eq "0") {
		if($value < $aThreshold[0]) {
			$newThreshold = "0";
		} else {
			$newThreshold = "1";
		}
	} else { #High
		if($value < ($aThreshold[0]-$aThreshold[1])) {
			$newThreshold = "0";
		} else {
			$newThreshold = "1";
		}
	}
	
	readingsSingleUpdate($hash,"ADC" . $id,$value,1);
	readingsSingleUpdate($hash,"ADC" . $id . "_Threshold",$newThreshold,1);
}
 		
 		
 		
 		
#####################################
# Translate data prepared for an other syntax, so we can reuse
# the FS20, IR, LaCrosse.

sub
RadioNetwork_ReadTranslate($)
{
  my ($dmsg) = @_;
	my $dmsgMod = $dmsg;
	
	my $LaCrosseMatchRegEx 	= "^[0-9]{2}R[0-9]" . substr($revRadioNetworkProtocols{"LaCrosse"},1,1) 	. "[A-F0-9]{8}\$";
	my $FS20MatchRegEx 			= "^[0-9]{2}R[0-9]" . substr($revRadioNetworkProtocols{"FS20"},1,1)				. "[A-F0-9]{8}\$";  #05R23D28C0000

#Adapt RadioNetwork command (O02D28C0000) to match FS20 command ("^81..(04|0c)..0101a001") from CUL	
  if( $dmsg =~ m/${FS20MatchRegEx}/ ) {  #O02D28C0100 ---> FS20
  	my $dev = substr($dmsg, 5, 4);
    my $btn = substr($dmsg, 9, 2);
    my $cde = substr($dmsg, 11, 2);
    # Msg format:
    # 81 0b 04 f7 0101 a001 HHHH 01 00 11
    $dmsgMod = "810b04f70101a001" . lc($dev) . lc($btn) . "00" . lc($cde);
    #Log 1, "Modified F20 command: " . $dmsgMod;
	}

#Adapt RadioNetwork command (F019204356A) to LaCrosse module standard syntax "OK 9 32 1 4 91 62" ("^\\S+\\s+9 ")
	elsif( $dmsg =~ m/${LaCrosseMatchRegEx}/ ) {
                #
                # Message Format:
                #
                # .- [0] -. .- [1] -. .- [2] -. .- [3] -. .- [4] -.
                # |       | |       | |       | |       | |       |
                # SSSS.DDDD DDN_.TTTT TTTT.TTTT WHHH.HHHH CCCC.CCCC
                # |  | |     ||  |  | |  | |  | ||      | |       |
                # |  | |     ||  |  | |  | |  | ||      | `--------- CRC
                # |  | |     ||  |  | |  | |  | |`-------- Humidity
                # |  | |     ||  |  | |  | |  | |
                # |  | |     ||  |  | |  | |  | `---- weak battery
                # |  | |     ||  |  | |  | |  |
                # |  | |     ||  |  | |  | `----- Temperature T * 0.1
                # |  | |     ||  |  | |  |
                # |  | |     ||  |  | `---------- Temperature T * 1
                # |  | |     ||  |  |
                # |  | |     ||  `--------------- Temperature T * 10
                # |  | |     | `--- new battery
                # |  | `---------- ID
                # `---- START
                #
                #

		my( $addr, $type, $channel, $temperature, $humidity, $batInserted ) = 0.0;

		#$addr = sprintf( "%02X", ((hex(substr($dmsg,5,2)) & 0x0F) << 2) | ((hex(substr($dmsg,7,2)) & 0xC0) >> 6) );
		#$addr = (((hex(substr($dmsg,3,2)) & 0x0F) << 2) | ((hex(substr($dmsg,5,2)) & 0xC0) >> 6));
		$addr = (((hex(substr($dmsg,5,2)) & 0x0F) << 2) | ((hex(substr($dmsg,7,2)) & 0xC0) >> 6));
		$type = ((hex(substr($dmsg,7,2)) & 0xF0) >> 4); # not needed by LaCrosse Module
		#$channel = 1; ## $channel = (hex(substr($dmsg,5,2)) & 0x0F);

		$temperature = ( ( ((hex(substr($dmsg,7,2)) & 0x0F) * 100) + (((hex(substr($dmsg,9,2)) & 0xF0) >> 4) * 10) + (hex(substr($dmsg,9,2)) & 0x0F) ) / 10) - 40;
		return if($temperature >= 60 || $temperature <= -40);

		$humidity = hex(substr($dmsg,11,2));
		$batInserted = ( (hex(substr($dmsg,7,2)) & 0x20) << 2 );

		#build string for 36_LaCrosse.pm   "OK 9 08 1 4 118 106" 
		$dmsgMod = "OK 9 $addr ";
			#bogus check humidity + eval 2 channel TX25IT
		if (($humidity >= 0 && $humidity <= 99) || $humidity == 106 || ($humidity >= 128 && $humidity <= 227) || $humidity == 234) {
			$dmsgMod .= (1 | $batInserted);
		} elsif ($humidity == 125 || $humidity == 253 ) {
			$dmsgMod .= (2 | $batInserted);
		}
		
		$temperature = (($temperature* 10 + 1000) & 0xFFFF);
		$dmsgMod .= " " . (($temperature >> 8) & 0xFF)  . " " . ($temperature & 0xFF) . " $humidity";
	}
				
#Adapt RadioNetwork command (01I02EEEE000200) to CUL-IR module standard syntax  "^I............" I02EEEE000200
	elsif( $dmsg =~ m/^[0-9]{2}I............/ ) {
		$dmsgMod = substr($dmsg,2,length($dmsg)-2);
	}

  return $dmsgMod;
}


#####################################
sub
RadioNetwork_Ready($)
{
  my ($hash) = @_;

  return DevIo_OpenDev($hash, 1, "RadioNetwork_DoInit")
                if($hash->{STATE} eq "disconnected");

  # This is relevant for windows/USB only
  my $po = $hash->{USBDev};
  my ($BlockingFlags, $InBytes, $OutBytes, $ErrorFlags);
  if($po) {
    ($BlockingFlags, $InBytes, $OutBytes, $ErrorFlags) = $po->status;
  }
  return ($InBytes && $InBytes>0);
}

########################

sub
RadioNetwork_ResetDevice($)
{
  my ($hash) = @_;

  DevIo_CloseDev($hash);
  my $ret = DevIo_OpenDev($hash, 0, "RadioNetwork_DoInit");

  return $ret;
}


sub
RadioNetwork_Attr(@)
{
  my ($cmd,$name,$aName,$aVal) = @_;
  my $hash = $defs{$name};

	if($aName =~ m/^IODev/i) {
  
  	delete $hash->{dummy};
  	delete $attr{$name}{dummy};
  	AssignIoPort($hash, $aVal);
  	#return DevIo_OpenDev($hash, 0, "RadioNetwork_DoInit");
  	RadioNetwork_DoInit($hash);
		readingsSingleUpdate($hash, "state", "Initialized", 1);
  	    
  } elsif($aName =~ m/^lowPower/i) {
  	
	  return "Usage: attr $name $aName <active|deactivated> (Satellites only)"
	    if($aVal !~ m/^(active|deactivated)$/ || !defined($hash->{IODev}) || ($hash->{IODev}{DeviceID} eq $hash->{DeviceID}));  	
	  
  	RadioNetwork_Write($hash, $hash->{QueryCmdStr} . "p"); #power all not needed functions off
	 	RadioNetwork_Write($hash, $hash->{QueryCmdStr} . "d" 	. ($aVal eq "active" ? "1" : "0")); #PowerDown Atmega
	 	RadioNetwork_Write($hash, $hash->{QueryCmdStr} . "R1l" . ($aVal eq "active" ? "1" : "0")); #ListenMode of RFM69

	 	#set phyDevice to Burst tunnelling mode
		#RadioNetwork_Write($hash, $hash->{IODev}{DeviceID} . "T" . $hash->{IODev}{ModuleID}	. ($aVal eq "active" ? "b1" : "b0")); #set phyDevice to Burst/Basic mode
		#$attr{$hash->{IODev}{NAME}}{CmdTunneling} = ($aVal eq "active" ? "Burst" : "Basic");
		if($aVal eq "active") {
			$hash->{QueryCmdStr} =~ tr/s/b/;
		} else {
			$hash->{QueryCmdStr} =~ tr/b/s/;
		}
		
	} elsif($aName =~ m/^CmdTunneling/i) {
	
	  return "Usage: attr $name $aName <Basic|Burst|Satellite>"
	    if($aVal !~ m/^(Basic|Burst|Satellite)$/);

	  my $cStr;  
		if($aVal eq "Basic") {
			$cStr = "";
		} elsif($aVal eq "Burst") {
			$cStr = "b";
		} elsif($aVal eq "Satellite") {
			$cStr = "s1";
		}
		RadioNetwork_Write($hash, $hash->{DeviceID} . "T1" . $cStr);

	} elsif($aName =~ m/^setADCThreshold/i) {
		
		 return "Usage: attr $name $aName <ADC-Number:threshold,-tolerance>"
	    if($aVal !~ m/^([0-7]:\d+,\d+\s*)+$/);


  } elsif($aName =~ m/^MsgACK_Trigger/i) {
		  
		#return "Usage: attr $name $aName <!Cmd:Execute;Cmd:Execute>"
	  #  if($aVal !~ m/^(\w+:\w+)$/);	  
	  
  }

  return undef;
}

1;

=pod
=begin html

<a name="RadioNetwork"></a>
<h3>RadioNetwork</h3>
<ul>
  The RadioNetwork is a family of RF devices sold by <a href="http://jeelabs.com">jeelabs.com</a>.

  It is possible to attach more than one device in order to get better
  reception, fhem will filter out duplicate messages.<br><br>

  This module provides the IODevice for:
  <ul>
  <li><a href="#PCA301">PCA301</a> modules that implement the PCA301 protocol.</li>
  <li><a href="#LaCrosse">LaCrosse</a> modules that implement the IT+ protocol (Sensors like TX29DTH, TX35, ...).</li>
  <li>LevelSender for measuring tank levels</li>
  <li>EMT7110 energy meter</li>
  <li>Other Sensors like WT440XH (their protocol gets transformed to IT+)</li>
  </ul>

  <br>
  Note: this module may require the Device::SerialPort or Win32::SerialPort module if you attach the device via USB
  and the OS sets strange default parameters for serial devices.

  <br><br>

  <a name="RadioNetwork_Define"></a>
  <b>Define</b>
  <ul>
    <code>define &lt;name&gt; RadioNetwork &lt;device&gt;</code> <br>
    <br>
    USB-connected devices:<br><ul>
      &lt;device&gt; specifies the serial port to communicate with the RadioNetwork.
      The name of the serial-device depends on your distribution, under
      linux the cdc_acm kernel module is responsible, and usually a
      /dev/ttyACM0 device will be created. If your distribution does not have a
      cdc_acm module, you can force usbserial to handle the RadioNetwork by the
      following command:<ul>modprobe usbserial vendor=0x0403
      product=0x6001</ul>In this case the device is most probably
      /dev/ttyUSB0.<br><br>

      You can also specify a baudrate if the device name contains the @
      character, e.g.: /dev/ttyACM0@57600<br><br>

      If the baudrate is "directio" (e.g.: /dev/ttyACM0@directio), then the
      perl module Device::SerialPort is not needed, and fhem opens the device
      with simple file io. This might work if the operating system uses sane
      defaults for the serial parameters, e.g. some Linux distributions and
      OSX.  <br>

    </ul>
    <br>
  </ul>

  <a name="RadioNetwork_Set"></a>
  <b>Set</b>
  <ul>
    <li>raw &lt;data&gt;<br>
        send &lt;data&gt; to the RadioNetwork. Depending on the sketch running on the RadioNetwork, different commands are available. Most of the sketches support the v command to get the version info and the ? command to get the list of available commands.
    </li><br>

    <li>reset<br>
        force a device reset closing and reopening the device.
    </li><br>

    <li>LaCrossePairForSec &lt;sec&gt; [ignore_battery]<br>
       enable autocreate of new LaCrosse sensors for &lt;sec&gt; seconds. If ignore_battery is not given only sensors
       sending the 'new battery' flag will be created.
    </li><br>

    <li>flash [hexFile]<br>
    The RadioNetwork needs the right firmware to be able to receive and deliver the sensor data to fhem. In addition to the way using the
    arduino IDE to flash the firmware into the RadioNetwork this provides a way to flash it directly from FHEM.

    There are some requirements:
    <ul>
      <li>avrdude must be installed on the host<br>
      On a Raspberry PI this can be done with: sudo apt-get install avrdude</li>
      <li>the flashCommand attribute must be set.<br>
        This attribute defines the command, that gets sent to avrdude to flash the RadioNetwork.<br>
        The default is: avrdude -p atmega328P -c arduino -P [PORT] -D -U flash:w:[HEXFILE] 2>[LOGFILE]<br>
        It contains some place-holders that automatically get filled with the according values:<br>
        <ul>
          <li>[PORT]<br>
            is the port the RadioNetwork is connectd to (e.g. /dev/ttyUSB0)</li>
          <li>[HEXFILE]<br>
            is the .hex file that shall get flashed. There are three options (applied in this order):<br>
            - passed in set flash<br>
            - taken from the hexFile attribute<br>
            - the default value defined in the module<br>
          </li>
          <li>[LOGFILE]<br>
            The logfile that collects information about the flash process. It gets displayed in FHEM after finishing the flash process</li>
        </ul>
      </li>
    </ul>

    </li><br>

    <li>led &lt;on|off&gt;<br>
    Is used to disable the blue activity LED
    </li><br>

    <li>beep<br>
    ...
    </li><br>

    <li>setReceiverMode<br>
    ...
    </li><br>

  </ul>

  <a name="RadioNetwork_Get"></a>
  <b>Get</b>
  <ul>
  </ul>
  <br>

  <a name="RadioNetwork_Attr"></a>
  <b>Attributes</b>
  <ul>
    <li>Clients<br>
      The received data gets distributed to a client (e.g. LaCrosse, EMT7110, ...) that handles the data.
      This attribute tells, which are the clients, that handle the data. If you add a new module to FHEM, that shall handle
      data distributed by the RadioNetwork module, you must add it to the Clients attribute.</li>

    <li>MatchList<br>
      can be set to a perl expression that returns a hash that is used as the MatchList<br>
      <code>attr myRadioNetwork MatchList {'5:AliRF' => '^\\S+\\s+5 '}</code></li>

    <li>initCommands<br>
      Space separated list of commands to send for device initialization.<br>
      This can be used e.g. to bring the LaCrosse Sketch into the data rate toggle mode. In this case initCommands would be: 30t
    </li>

    <li>flashCommand<br>
      See "Set flash"
    </li><br>


  </ul>
  <br>
</ul>

=end html
=cut
