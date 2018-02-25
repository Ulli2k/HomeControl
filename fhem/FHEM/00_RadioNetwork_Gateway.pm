
# $Id: 00_RadioNetwork.pm 2015-08-03 20:10:00Z Ulli $
#TODO: sendpool integrieren für mehrere RadioNetwork. Keine Sendekonflikte (CUL)

package main;

use strict;
use warnings;
use Time::HiRes qw(gettimeofday);

sub RadioNetwork_Gateway_Define($$);
sub RadioNetwork_Gateway_SatelliteReset($);
sub RadioNetwork_Gateway_PreConfigure($$);
sub RadioNetwork_Gateway_UpdateFeaturesByModel($);
sub RadioNetwork_Gateway_SatelliteBurst($$);

sub RadioNetwork_Gateway_Set($@);
sub RadioNetwork_Gateway_Get($@);
sub RadioNetwork_Gateway_Attr(@);
sub RadioNetwork_Gateway_Parse($$);

sub RadioNetwork_Gateway_Prefix($$$);
sub RadioNetwork_Gateway_ReadTranslate($$);
sub RadioNetwork_Gateway_WriteTranslate($$);

sub RadioNetwork_Gateway_getMatchRegExOfString($$);
sub RadioNetwork_Gateway_UpdateReading($$$$);
sub RadioNetwork_Gateway_getAliasName($$);

sub RadioNetwork_Gateway_actCycle($);

my $clientsRadioNetworkGateway = ":CUL_IR:RadioNetwork_RFM:myPowerNode:myBME:myRollo:";

my %matchListRadioNetworkGateway = (
    "1:CUL_IR"  												=> "^I............",
    "2:RadioNetwork_RFM"								=> "^(R[0-9]|f[0-9]{4})",
    "3:myPowerNode"											=> "^P[1-6][0-9A-F]{16}\$",							#Transformed PowerNode command for 36_myPowerNode.pm
    "4:myBME"														=> "^E[0-9]{4}t[0-9]{4}h\$",						#Transformed BME command for 36_myBME.pm    
    "5:myRollo"													=> "^J([0-2]|t[0-9999])\$",
);

my $myProtocol_ProtocolIndex = "1";

my %sets = (
  "raw" 								=> "",
  "reset"    						=> "noArg",
  "flash"       				=> "",
	"beep" 								=> "",
	"switch"							=> "on,off",
  "led"      					 	=> "on,off",
  "led-on-for-timer"    => "",
  "updateAvailRam"			=> "noArg",
  "getModel"						=> "noArg",
);

my %varSetbyModel = (
	"S"										=> "switch",
	"B"										=> "beep",
	"L"										=> "led,led-on-for-timer",
	"R,"									=> "flash", #workaround for wired modules flashability
);

my %gets = (    # Name, Data to send to the CUL, Regexp for the answer
##  "version"  				=> ["v", '^V[0-9]$'],
  "raw"      						=> ["", '.*'],
#  "firmware"						=> ["w", "^w"],
#  "AvailRam"						=> ["m", "^m"],
);

my $SendDelay_Satellite		= 0.05; #[s] delay for nextSend to Satellite
my $RadioNetwork_Gateway_actCycle	=	"00:30"; #[hh]:[mm]
my $RadioNetwork_Gateway_State_Value = "no issues";

sub RadioNetwork_Gateway_Initialize($) {
  my ($hash) = @_;

  require "$attr{global}{modpath}/FHEM/00_RadioNetwork_Base.pm";

	RadioNetwork_Initialize($hash);

	$hash->{ParseFn}							= "RadioNetwork_Gateway_Parse";
	$hash->{WriteTranslateFn}			= "RadioNetwork_Gateway_WriteTranslate";
	#$hash->{ReadTranslateFn}			= "RadioNetwork_Gateway_ReadTranslate";
	$hash->{SatelliteBurstFn}			= "RadioNetwork_Gateway_SatelliteBurst";
	$hash->{LogicalDeviceReset} 	= "RadioNetwork_Gateway_SatelliteReset";
# Normal devices
  $hash->{DefFn}        				= "RadioNetwork_Gateway_Define";
  $hash->{GetFn}        				= "RadioNetwork_Gateway_Get";
  $hash->{SetFn}        				= "RadioNetwork_Gateway_Set";
  $hash->{AttrFn}       				= "RadioNetwork_Gateway_Attr";
  $hash->{AttrList} 						= " lowPower:active,deactivated"
																	." preConfigured"
																	." actCycle"
																	." stateValue"
										           	 	." hexFile"
										           	 	." flashCommand"
										          	  ." BeepLong BeepShort BeepDelay BeepFrequency"
										          	  ." preConfigured"
										          	  ." readingAlias readingValue"
										          	  ." $readingFnAttributes";
  #$hash->{ShutdownFn} 					= "RadioNetwork_Gateway_Shutdown";
}

#####################################

sub RadioNetwork_Gateway_Define($$) {
#define <name> RadioNetwork <device/none> <DeviceID> 
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if(@a < 4 || @a > 5) {
    my $msg = "wrong syntax: define <name> RadioNetwork { <phyDevice> | devicename[\@baudrate] ".
                        "| devicename\@directio } <opt. RFMDevice> <DeviceID>";
    Log3 undef, 2, $msg;
    return $msg;
  }

  DevIo_CloseDev($hash);

  my $name = $a[0];
  my $dev = $a[2];
  my $rdev = (@a == 5 ? $a[3] : undef);
  my $devID = (@a == 5 ? $a[4] : $a[3]);
  return "DeviceID must be [00-99] a value with two digits." if($devID !~ m/^[0-9][1-9]$/);
	return "Invalid RadioNetwork_RFM device ($rdev)." if(defined($rdev) && (!defined($defs{$rdev}) || $defs{$rdev}{TYPE} ne "RadioNetwork_RFM"));
	
  #check if DeviceID and ModuleID is already defined in combination
  foreach my $d (keys %defs) {
		next if($d eq $name);
		if($defs{$d}{TYPE} eq "RadioNetwork_Gateway" && !($defs{$d}{IODev})) {
	    if(uc($defs{$d}{DeviceID}) =~ m/^$devID/) {
          my $m = "$name: Cannot define multiple RadioNetwork-Gateways with identical DeviceID's ($devID).";
          Log3 $name, 1, $m;
          return $m;
        }
      }
	}

	%{$hash->{Sets}} = %sets;
  $hash->{DeviceID} = $devID;
  $hash->{Clients} = $clientsRadioNetworkGateway;
  $hash->{MatchList} = \%matchListRadioNetworkGateway;
	$attr{$name}{stateFormat} = "{RadioNetwork_Gateway_State(\$name,\"\")}";
	
  if(defined($rdev)) { #Satellite Prefix 
	  $hash->{GatewayRFM4Satellite} = \%{$defs{$rdev}};
		$hash->{CmdPrefix} = "R" . $defs{$rdev}{ModuleID} . "s" . $myProtocol_ProtocolIndex . $hash->{DeviceID};  
		$hash->{delaySend} = $SendDelay_Satellite;
		$hash->{DeviceName} = $dev;
		AssignIoPort($hash,$dev);
		Log3 $name, 1, "$name device is linked to $dev and $rdev as satellite.";
		return RadioNetwork_DoInit($hash);
	} else {
		$hash->{CmdPrefix} = $hash->{DeviceID};
	 	$attr{$name}{flashCommand} = "avrdude -p atmega328P -c arduino -P [PORT] -D -U flash:w:[HEXFILE] 2>[LOGFILE]";
	  $dev .= "\@57600" if( $dev !~ m/\@/ );
		$hash->{DeviceName} = $dev;
	  return DevIo_OpenDev($hash, 0, "RadioNetwork_DoInit");
	}
}

#####################################
sub RadioNetwork_Gateway_State($$) {

	my ($name, $sVal) = @_;
	my $state = ReadingsVal($name,"state",undef);
	my $actStatus = ReadingsVal($name,"actStatus",undef);
	
	return "$state" if($state ne "Initialized");
	return "$actStatus" if($actStatus ne "alive" && $actStatus ne "off");
	return "connection problem" if(ReadingsVal($name,"acknowledge_missed",undef));
	return "unexpected reset" if(ReadingsVal($name,"unexpected_reset",undef));
	
	my @a = split(/\|/, $sVal);
	foreach my $val (@a) {
		my @cParam = split(/:/, $val);
		my $cVal = ReadingsVal($name,$cParam[0],undef);
		my $cRegEx = $cParam[1];
		return $cParam[0] if( (defined($cRegEx) && $cVal !~ m/$cRegEx/) || (!defined($cRegEx) && $cVal));
	}
	my $ret = AttrVal($name,"stateValue",$RadioNetwork_Gateway_State_Value);
	if($ret =~ m/^{(.*)}$/s) {
    $ret = eval $1;
    Log3 $name,1,"$name: error evaluating $name stateFormat: $@" if($@);
	}
	return $ret;
}

sub RadioNetwork_Gateway_time2sec($) {
  my ($timeout) = @_;
  my ($h,$m) = split(":",$timeout);
  no warnings 'numeric';
  $h = int($h);
  $m = int($m);
  use warnings 'numeric';
  return ((sprintf("%03s:%02d",$h,$m)),((int($h)*60*60+int($m))*60));
}

sub RadioNetwork_Gateway_actCycle($) {

	my ($hash) = @_;
	my $name = $hash->{NAME};

	RemoveInternalTimer($hash,"RadioNetwork_Gateway_actCycle");
	my $sActCycle = AttrVal($name, "actCycle", $RadioNetwork_Gateway_actCycle);
	
	if($sActCycle =~ m/^off$/) {
		readingsSingleUpdate($hash,"actStatus","off",1);
		return;
	}
	
	my (undef,$tSec) = RadioNetwork_Gateway_time2sec($sActCycle);
	my $waitTime = gettimeofday()+$tSec;	

	if(!defined($hash->{"${name}_MSGCNT_actCycle"}) || $hash->{"${name}_MSGCNT_actCycle"} ne $hash->{"${name}_MSGCNT"}) { #alive
		readingsSingleUpdate($hash,"actStatus","alive",1);
	}	else { #dead
		readingsSingleUpdate($hash,"actStatus","dead",1) if(ReadingsVal($name,"actStatus",undef) ne "dead");
	}

	$hash->{"${name}_MSGCNT_actCycle"} = $hash->{"${name}_MSGCNT"};
	InternalTimer($waitTime, "RadioNetwork_Gateway_actCycle", $hash, 0);
	#Log3 $name , 5, "$name: set actStatus timer to " . $tSec . " seconds (" . $hash->{"${name}_MSGCNT"} . ")";
}

#####################################
sub RadioNetwork_Gateway_SatelliteReset($) {
	my ($hash) = @_;
	my $aVal;
	
	if($aVal = AttrVal($hash->{NAME}, "preConfigured", undef)) { #no reset on preConfigured device
  	RadioNetwork_Gateway_Parse($hash,"w" . $aVal);
   	return;
  }
  
	my $burstOnOff = RadioNetwork_Gateway_SatelliteBurst($hash,undef);
	RadioNetwork_Gateway_SatelliteBurst($hash,0); #turn BurstMode off
	RadioNetwork_Gateway_PreConfigure($hash,"Satellite");	
	RadioNetwork_Gateway_SatelliteBurst($hash,$burstOnOff); #reset BurstMode

}
#####################################

sub RadioNetwork_Gateway_PreConfigure_Satellite($) {
	my ($hash) = @_;
	RadioNetwork_Gateway_PreConfigure($hash,"Satellite");
}

sub RadioNetwork_Gateway_PreConfigure($$) {
	
	my ($hash, $rmsg) = @_;
	my $name = $hash->{NAME};
  my $to = 1;#time to reset satellites; depends on Burst length
  my $now = gettimeofday();
  
  $hash->{model} = $rmsg;
	
	delete($hash->{READINGS}{freeMemory});
	
	if($rmsg eq "Satellite" && ReadingsVal($name,"state","opened") ne "reseted") {
		
		#Reset Satellite in BurstMode
		RadioNetwork_Gateway_SatelliteBurst($hash,1);
		RadioNetwork_SimpleWrite($hash, "o");
		RadioNetwork_Gateway_SatelliteBurst($hash,0);
		readingsSingleUpdate($hash, "state", "reseted", 0);	
		Log3 $hash->{NAME},5,$hash->{NAME} . " device reseted";
		InternalTimer($now+$to, "RadioNetwork_Gateway_PreConfigure_Satellite", $hash, 0);
		return;

	} else {

		readingsSingleUpdate($hash, "state", "opened", 0);
		Log3 $hash->{NAME},4,$hash->{NAME} . " device preConfiguration";
	}

	RadioNetwork_Write($hash, "", "w", "^w",1); 	#request Firmware (must be LAST COMMAND)--> will set Device to be initialized
	
	#RadioNetwork_Write($hash, "", "A", "^A",1);   # get supply voltage	
	RadioNetwork_Write($hash, "", "m", "^m",1);   # show used ram
	#RadioNetwork_Write($hash, "", "a1",undef,1);	# turn activity led off		
	
	RadioNetwork_Write($hash, "", "q1",undef,1) if(AttrVal($hash->{NAME}, "verbose", 0) <= 3); 	# turn quiet mode on
	
	if($rmsg eq "Satellite") {
		RadioNetwork_Write($hash, "", "T" .(defined($hash->{GatewayRFM4Satellite}->{ModuleID}) ? $hash->{GatewayRFM4Satellite}->{ModuleID} : "1") . "c1" . $hash->{IODev}{DeviceID},undef,1); 
	}
	
	readingsSingleUpdate($hash, "state", "preConfiguration", 0); #start preconfiguration (commandQueue complete)	
}



#####################################
sub RadioNetwork_Gateway_SatelliteBurst($$) {
	my ($hash, $on) = @_;
	if(!defined($on)) {
		return (($hash->{CmdPrefix} =~ m/R[0-9]b[0-9]/) ? 1:0);
	}
	$hash->{CmdPrefix} =~ tr/s/b/ if($on);
	$hash->{CmdPrefix} =~ tr/b/s/ if(!$on);
}

sub RadioNetwork_Gateway_Set($@) {
  my ($hash, @a) = @_;
  
  return "\"set RadioNetwork\" needs at least one parameter" if(@a < 2);
  
  #problematic...does not work with predefined sets values!?
  #return "Unknown argument $a[1], choose one of " . join(" ", sort keys %sets)
  #	if(!defined($sets{$a[1]}));

  my $name = shift @a;
  my $cmd = shift @a;
  my $arg = join(" ", @a);
  
  #my $list = join(" ", map { $sets{$_} eq "" ? $_ : "$_:$sets{$_}" } sort keys %sets);
	my $list = join(" ", map { $hash->{Sets}{$_} eq "" ? $_ : "$_:$hash->{Sets}{$_}" } sort keys %{$hash->{Sets}});
  return $list if( $cmd eq '?' || $cmd eq '');


	###### set commands ######
  if($cmd eq "raw") {
    Log3 $name, 3, "set $name $cmd $arg";
    RadioNetwork_Write($hash, $arg);
  	
  } elsif ($cmd =~ m/reset/i) {
  	delete($hash->{READINGS}{unexpected_reset});
    return RadioNetwork_ResetDevice($hash);
    
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
        RadioNetwork_Write($hash, "b" . $longbeep);
      } elsif($x eq "-") {
        # short beep
        RadioNetwork_Write($hash, "b"  . $shortbeep);
      }
      select(undef, undef, undef, $delaybeep);
    }
    
  } elsif( $cmd eq "switch" ) {
  
    return "Unknown argument $cmd, choose one of \[on,off\]" if($arg !~ m/^(on|off|up|down|up10|down10|stop)$/i);
    
    Log3 $name, 4, "set $name $cmd $arg";
    if($arg =~ m/(on|off)/) {
    	RadioNetwork_Write($hash, "S1" . ($arg eq "on" ? "1" : "0") );
    } else {
    	my $val=0;
    	if($arg eq "up") {
    		$val = "1" . "25";
    	} elsif ($arg eq "up10") {
    		$val = "1" . "10";
    	} elsif ($arg eq "down") {
    		$val = "2" . "25";
    	} elsif ($arg eq "down10") {
    		$val = "2" . "10";
    	} else {
    		$val = "0" . "0";
    	}
    	#RadioNetwork_Write($hash, "S" . ($arg eq "up" ? "1" : ($arg eq "down" ? "2" : "0") ) );
    	RadioNetwork_Write($hash, "S" . $val);
    }
    
  } elsif ($cmd =~ m/^led$/i) {

    return "Unknown argument $cmd, choose one of $list" if($arg !~ m/^(on|off)$/i);

    Log3 $name, 4, "set $name $cmd $arg";
    RadioNetwork_Write($hash, "l" . ($arg eq "on" ? "1" : "0") );
    
  } elsif ($cmd =~ m/led-on-for-timer/i) {
  
    return "Unknown argument $cmd, choose one of $list" if($arg !~ m/^[0-9]+$/);

    #remove timer if there is one active
    if($modules{RadioNetwork}{ldata}{$name}) {
	    CommandDelete(undef, $name . "_timer");
	    delete $modules{RadioNetwork}{ldata}{$name};
    }

	  Log3 $name, 4, "set $name on";
    RadioNetwork_Write($hash, "l" . "1");
  
    my $to = sprintf("%02d:%02d:%02d", $arg/3600, ($arg%3600)/60, $arg%60);
    $modules{RadioNetwork}{ldata}{$name} = $to;
    Log3 $name, 4, "Follow: +$to setstate $name off";
    CommandDefine(undef, $name."_timer at +$to {fhem(\"set $name led" ." off\")}");
  
  } elsif ($cmd =~ m/updateAvailRam/i) {
  	RadioNetwork_Write($hash, "m");
  	
  } elsif ($cmd =~ m/getModel/i) {
    RadioNetwork_Write($hash, "w");
  } 
  

  return undef;
}

#####################################

sub RadioNetwork_Gateway_Get($@) {
  my ($hash, $name, $cmd, @msg ) = @_;
  my $arg = join(" ", @msg);
  my $type = $hash->{TYPE};
  

  return "\"get $type\" needs at least one parameter" if(!defined($name) || $name eq "");
#  if(!defined($gets{$cmd})) {
#    my @list = map { $_ =~ m/^(raw)$/ ? $_ : "$_:noArg" } sort keys %gets;
#    return "Unknown argument $cmd, choose one of " . join(" ", @list);
#  }

	return "No $cmd for devices which are defined as none." if(IsDummy($hash->{NAME}));
	
	my ($err, $rmsg);
	
#  if( $cmd eq "initRadioNetwork" ) {
#		$hash->{STATE} = "Opened";
#		readingsSingleUpdate($hash, "state", "Opened", 1);
#		RadioNetwork_Write($hash, "o");
#		return;
#		
#	} elsif ($cmd eq "raw" && $arg =~ m/^Ir/) {
	if ($cmd eq "raw" && $arg =~ m/^Ir/) {
		return "raw => 01" if($arg =~ m/^Ir$/);
		RadioNetwork_Write($hash, "Ir" . ($arg =~ m/00/ ? "0" : "1"));
		return "raw => " . substr($arg,2,2);

#	} elsif (defined($gets{$cmd})) {
#		RadioNetwork_SimpleWrite($hash, $gets{$cmd}[0]);
#    ($err, $rmsg) = RadioNetwork_ReadAnswer($hash, $cmd, 1, $gets{$cmd}[1]); #Dispatchs all commands
#    $rmsg = "No answer. $err" if(!defined($rmsg));
#	
#	} else { ## raw
#    RadioNetwork_SimpleWrite($hash, $gets{$arg}[0]);
#    ($err, $rmsg) = RadioNetwork_ReadAnswer($hash, $arg, 1, $gets{$arg}[1]); #Dispatchs all commands
#    if(!defined($rmsg)) {
#      #DevIo_Disconnected($hash);
#      $rmsg = "No answer. $err";	
#    }
	}
	return;
  #readingsSingleUpdate($hash, $cmd, $rmsg, 0);  	

  #return "$name $cmd => $rmsg";
}
 
#####################################
# Translate data prepared for an other syntax, so we can reuse
# the FS20. 
sub RadioNetwork_Gateway_WriteTranslate($$) {
  my ($hash, $cmd) = @_;

  ###################
  # Rewrite message from RadioNetwork -> CUL-IR
  $cmd =~ s/^Is/I/i if(defined($cmd));  #SendIR command is "I" not "Is" for RadioNetwork devices
	
  return $cmd;
}

#####################################
sub
RadioNetwork_Gateway_Attr(@)
{
  my ($cmd,$name,$aName,$aVal) = @_;
  my $hash = $defs{$name};


  if($aName =~ m/^lowPower/i) {
  	
	  return "Usage: attr $name $aName <active|deactivated>" if($aVal !~ m/^(active|deactivated)$/);  	
	  
  	RadioNetwork_Write($hash, "p"); #power all not needed functions off
	 	RadioNetwork_Write($hash, "d" 	. ($aVal eq "active" ? "1" : "0")); #PowerDown Atmega
	 	
	 	Log3 $hash->{NAME},2,"set $hash->{NAME} " . ($aVal eq "active" ? "in lowPower mode." : "lowPower mode off.");

	} elsif($aName =~ m/^preConfigured/i) {
		return "Usage: attr $name $aName <[RN01s,C,Rphst,Isr,P,L,S,B,E]>" if($aVal !~ m/^\[RN[0-9]{2}s[CNRphstIsrPLSBE,]+\]$/);   #[RN01s,C,Rphs,Isr,P,L,S,B,E]  

		Log3 $hash->{NAME}, 4, "$hash->{NAME} is preConfigured and will not be initialized again.";
		
	} elsif($aName =~ m/^ReadingAlias/i) {
		RadioNetwork_Gateway_updateReadingAlias($hash);
		
	} elsif($aName =~ m/^actCycle/i) {
		return "Usage: attr $name $aName <hh:mm>" if($aVal !~ m/^([0-9]{2}:[0-9]{2}|off)$/);

	  $attr{$hash->{NAME}}{actCycle} = $aVal;
	  RadioNetwork_Gateway_actCycle($hash);
	}
  return undef;
}

#####################################
# Translate data prepared for an other syntax, so we can reuse
# the FS20, IR, LaCrosse.

sub
RadioNetwork_Gateway_ReadTranslate($$)
{
  my ($hash, $dmsg) = @_;
	my $dmsgMod = $dmsg;
	
#Adapt RadioNetwork command (O02D28C0000) to match FS20 command ("^81..(04|0c)..0101a001") from CUL	
	#if( $dmsg =~ m/^I............/ ) {
	#	$dmsgMod = substr($dmsg,2,length($dmsg)-2);
	#}
	
	#my $id = substr($dmsg,0,2);

  return $dmsgMod;
}

sub RadioNetwork_Gateway_Parse($$) {
	my ($hash, $rmsg) = @_;
	my $dmsg = $rmsg;
	my $name = $hash->{NAME};
	my $dispatch = 1;
	
	my $conState = ReadingsVal($hash->{NAME},"state","");
	$hash->{nextSend} = gettimeofday() + $hash->{delaySend} if(defined($hash->{delaySend}));
	
    
	# Commands which can only be received from the main physical Module/Device
  if($rmsg =~ m/^\[RN.+\]/ && $conState eq "opened") { #Initialization of Physical Devices
    RadioNetwork_Gateway_PreConfigure($hash,$rmsg);
		$dispatch = 0;
		
	} elsif($rmsg =~ m/^w\[RN.+\]/) { #initialization done
		$hash->{model} = substr($rmsg,1,length($rmsg)-1);
    RadioNetwork_Gateway_UpdateFeaturesByModel($hash);
		if($conState eq "opened" || $conState eq "preConfiguration") {
			readingsSingleUpdate($hash, "state", "Initialized", 1);
			Log3 $hash->{NAME},3,$hash->{NAME} . " device initialized";
			
			RadioNetwork_Gateway_actCycle($hash);
		}
		$dispatch = 0;
	
	} elsif ($rmsg =~ m/^W/) {
		if($conState eq "Initialized") {
			Log3 $name, 1, "$name: Unexpected device reset detected...reconfiguring";
			RadioNetwork_DoInit($hash);
			readingsSingleUpdate($hash, "unexpected_reset", ReadingsVal($name, "unexpected_reset","0")+1, 1);
		} else {
			Log3 $name, 4, "$name: device reset signal received.";
		}
		$dispatch = 0;
  } elsif ( $rmsg =~ m/^m[0-9999]/) {
		readingsSingleUpdate($hash,"freeMemory",substr($rmsg,1,length($rmsg)-1),0);
    $dispatch = 0;
  } elsif ( $rmsg =~ m/^C[0-9][0-9999]/) { #05C62396
  	RadioNetwork_Gateway_UpdateReading($hash,"ADC",substr($rmsg,1,1),substr($rmsg,2,length($rmsg)-2));
  	$dispatch = 0;
  } elsif ( $rmsg =~ m/^A[0-9999]/) {
 		readingsSingleUpdate($hash,"VCC",substr($rmsg,1,length($rmsg)-1),1);
  	$dispatch = 0;
  } elsif ( $rmsg =~ m/^N[0-9][0-1]/) {
  	RadioNetwork_Gateway_UpdateReading($hash,"Trigger",substr($rmsg,1,1),substr($rmsg,2,1));
  	$dispatch = 0;
  }
  
	#Translate Msg if necessary for other Modules for dispatching
	$dmsg = RadioNetwork_Gateway_ReadTranslate($hash, $rmsg);
	#if ( $dmsg =~ /\[RSSI:/) { #notwendig da sonst LogicDevice wie PowerNode das [RSSI bekommen
	#	$dmsg = substr($dmsg,0,index($dmsg," \[RSSI")); #remove RSSI value
	#}
	
	$hash->{"${name}_MSGCNT"}++;
  $hash->{"${name}_TIME"} = TimeNow();

  return if(!$dispatch);
  $hash->{RAWMSG} = $rmsg;

	my %addvals = (RAWMSG => $dmsg);
	#$addvals{RSSI} = $hash->{helper}{last_RSSI} if($hash->{helper}{last_RSSI});

	#Log3 $hash->{NAME},1, "Dispatch: $rmsg";
  my $found = Dispatch($hash, $dmsg, \%addvals);
}

sub RadioNetwork_Gateway_UpdateFeaturesByModel($) {
	my ($hash) = @_;
	
	return if($hash->{model} !~ m/^\[RN.+\]/);
	
	foreach (keys %varSetbyModel) {
		my $k = $_;
		next if($hash->{model} =~ m/,$k/);

		my @a = split(",", $varSetbyModel{$k});
		foreach my $val (@a) {
			delete  $hash->{Sets}{$val};
		}
	}
}
#####################################
sub RadioNetwork_Gateway_getMatchRegExOfString($$) {
    my ($string, $regex) = @_;
    return if not $string =~ /($regex)/;
		return $1;
}

#Trigger: N<Pin><0:low,1:high/click,2:doubleClick,3:longClick>
#ADC
sub RadioNetwork_Gateway_UpdateReading($$$$) {

	my ($hash, $fkt, $id, $value) = @_;
	my @TriggerValueArray = ("low", "pulse", "doublePulse", "longPulse");
	my $name = $hash->{NAME};
	
	my $readingName = RadioNetwork_Gateway_getAliasName($name, $fkt . $id);
	my $readingValue = RadioNetwork_Gateway_getReadingValue($hash, $readingName, ($fkt eq "Trigger" ? $TriggerValueArray[$value] : $value) );

	readingsSingleUpdate($hash,$readingName, $readingValue,1) if(defined($readingValue));	
}

sub RadioNetwork_Gateway_getReadingValue($$$) {

	my ($hash, $readingName, $value) = @_;
	my $name = $hash->{NAME};
	my $aVal = AttrVal($name, "readingValue", undef);
	my $fktValue = undef;
	
	return $value if(!defined($aVal));
	
	my @a = split(",", $aVal);
	foreach my $val (@a) {
		if($val =~ m/^$readingName:/) {
			$fktValue = (split(/:/, $val))[1];
			last;
		}
	}
	
	if(defined($fktValue)) {
	
#Pulse Counter <readingName>:cnt
		if($fktValue =~ m/^cnt$/) {
			$value = ReadingsVal($name, $readingName,"0");
			$value = 0 if($value !~ m/^[0-9]+/);
			$value++;
			
#Update specific value
		} elsif($fktValue =~ m/^val\|/) {
			my $template = (split(/\|/, $fktValue))[1];

			if($value !~ m/$template/) {
				return undef;
			}
					
#Pulse Counter <readingName>:cnt|pulse
		} elsif($fktValue =~ m/^cnt\|/) {
			my $template = (split(/\|/, $fktValue))[1];

			if($value =~ m/$template/) {
				$value = ReadingsVal($name, $readingName,"0");		
				$value = 0 if($value !~ m/^[0-9]+/);
				$value++;					
			} else {
				return undef;
			}

#Threshold  <readingName>:th|2200|200
		} elsif ($fktValue =~ m/^th\|[0-9]+\|[0-9]+$/) { #Threshold
			my @aThreshold = split(/\|/,$fktValue);
			shift @aThreshold;
			
			my $newValue = $value;
			readingsSingleUpdate($hash,$readingName . "_Value", $newValue,1); #save current Value
			
			my $curThreshold = ReadingsVal($name, $readingName, "-");
			my $newThreshold;
			
			if($curThreshold eq "-" || $curThreshold eq "0") {
				if($newValue < $aThreshold[0]) {
					$newThreshold = "0";
				} else {
					$newThreshold = "1";
				}
			} else { #High
				if($newValue < ($aThreshold[0]-$aThreshold[1])) {
					$newThreshold = "0";
				} else {
					$newThreshold = "1";
				}
			}
			$value = $newThreshold;
		}
	}
	
	return $value;
}

sub RadioNetwork_Gateway_updateReadingAlias($) {

	my ($hash) = @_;
	my $name = $hash->{NAME};
	my $aVal = AttrVal($name, "readingAlias", undef);
	
	if($aVal) {
		my @a = split(",", $aVal);
		foreach my $val (@a) {
			my @names = split(/:/, $val);
			$hash->{READINGS}{$names[1]} = delete $hash->{READINGS}{$names[0]};
			$hash->{READINGS}{$names[1] . "_Value"} = delete $hash->{READINGS}{$names[0]. "_Value"} if(defined($hash->{READINGS}{$names[0]. "_Value"}));
		}
	}	
}

sub RadioNetwork_Gateway_getAliasName($$) {

	my ($name, $value) = @_;
	my $aVal = AttrVal($name, "readingAlias", undef);
	
	if($aVal) {
		my @a = split(",", $aVal);
		foreach my $val (@a) {
			if($val =~ m/^$value:/) {
				$value = (split(/:/, $val))[1];
				last;
			}
		}
	}
	
	return $value;
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

  <a name="RadioNetwork_Gateway_Define"></a>
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

  <a name="RadioNetwork_Gateway_Set"></a>
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

  <a name="RadioNetwork_Gateway_Get"></a>
  <b>Get</b>
  <ul>
  </ul>
  <br>

  <a name="RadioNetwork_Gateway_Attr"></a>
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
