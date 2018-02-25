
# $Id: 00_RadioNetwork.pm 2015-08-03 20:10:00Z Ulli $
#TODO: sendpool integrieren für mehrere RadioNetwork. Keine Sendekonflikte (CUL)

package main;

use strict;
use warnings;
use Time::HiRes qw(gettimeofday);

my $RadioNetwork_ACKRegEx						= "^ ACK \\d+ms"; #used for Write commands comming from Satellite without a specific RegEx
my $RadioNetwork_Waiting4ACK_Time		= 5; #sec

sub RadioNetwork_Parse($$);
sub RadioNetwork_Ready($);
sub RadioNetwork_Read($);
sub RadioNetwork_ReadAnswer($$$$);

sub RadioNetwork_QueueAnswer($$$$);
sub RadioNetwork_SendFromQueue($$$$);
sub RadioNetwork_AddQueue($$$$);
sub RadioNetwork_HandleWriteQueue($);
sub RadioNetwork_QueueSucessfullyDone($);
sub RadioNetwork_Write(@);
sub RadioNetwork_SimpleWrite(@);

sub RadioNetwork_Prefix($$$);

sub RadioNetwork_Clear($);
sub RadioNetwork_ResetDevice($);

sub RadioNetwork_getMyHash($$$$);
sub RadioNetwork_getMyHashByModuleID($$);

sub RadioNetwork_Initialize($) {
  my ($hash) = @_;

  require "$attr{global}{modpath}/FHEM/DevIo.pm";

# Provider
  $hash->{ReadFn}								= "RadioNetwork_Read";
  $hash->{WriteFn} 							= "RadioNetwork_Write";
  $hash->{ReadyFn} 							= "RadioNetwork_Ready";
  $hash->{getRightQueueHashFn}	= "RadioNetwork_getRightQueueHash";
	
# Normal devices
  $hash->{FingerprintFn} 				= "RadioNetwork_Fingerprint";
  $hash->{UndefFn}							= "RadioNetwork_Undef";
  #$hash->{ShutdownFn} 					= "RadioNetwork_Shutdown";
 
}

sub RadioNetwork_getRightQueueHash($$) {

	my ($hash, $dmsg) = @_;
	my $id;
	my $devHash;

	#return if($hash->{IODev}); #only for phyDevices
	#Log3 $hash->{NAME},1,"$hash->{NAME}: RadioNetwork_getRightQueueHash <$dmsg>";
		
	if(length($dmsg)>=8) {
		$id = substr($dmsg,6,2); #01R2s109T1c101
	} else {
		$id = substr($dmsg,0,2); #01R
	}
	
	if($id ne $hash->{DeviceID}) {
	 	foreach my $name (keys %{ $modules{RadioNetwork_Gateway}{defptr} }) {
			if(defined($modules{RadioNetwork_Gateway}{defptr}{$name}->{DeviceID}) && defined($id) && $modules{RadioNetwork_Gateway}{defptr}{$name}->{DeviceID} eq $id) {
				$devHash = $modules{RadioNetwork_Gateway}{defptr}{$name};
			}
		}
	}	else {
		$devHash = $hash;
	}
	return if(!defined($devHash));
 
  if(defined($devHash->{nextSend})) {
		$hash->{helper}{$id}{nextSend} = \$devHash->{nextSend};
		$hash->{helper}{$id}{delaySend} = $devHash->{delaySend};
		$hash->{helper}{$id}{NAME} = $devHash->{NAME};
		#Log3 $hash->{NAME},1,"$hash->{NAME}: RadioNetwork_getRightQueueHash Id=$id and " . $hash->{helper}{$id}{nextSend};
		return $id;
	}
	return;
}

#####################################
sub RadioNetwork_Fingerprint($$) { ## Notwendig da sonst dispatch nicht funktioniert
	my ($name, $msg) = @_;
  ## Ergänzung der ursprünglichen Nachricht, da sonst nach Dispatch fälschlicherweise CheckDuplicate eine Nachricht als doppelt erkennt.
	return ($name, "RN: $msg");
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
  delete( $modules{$hash->{TYPE}}{defptr}{$hash->{NAME}} );
  DevIo_CloseDev($hash);
  RemoveInternalTimer($hash);
  return undef;
}

#####################################

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
  
  RemoveInternalTimer($hash);
}

#####################################
sub RadioNetwork_DoInit($) {

  my $hash = shift;
  my $name = $hash->{NAME};
  my $to = 0.1; #weit time for next try to initialize/reset the satellite
	
	if(ReadingsVal($hash->{NAME},"state","") ne "opened") { 
		readingsSingleUpdate($hash, "state", "opened", 1);
	 # Reset the counter
		delete($hash->{XMIT_TIME});
		delete($hash->{NR_CMD_LAST_H});
	}
	#delete($hash->{QUEUE}); #nicht löschen da sonst sets in der config (in der Queue) gelöscht werden! Initialisierungsprozess!
	delete($hash->{QUEUE_COMMAND});
	delete($hash->{QUEUE_ANSWER});
	delete($hash->{QUEUE_ANSWER_TIME});
	delete($hash->{QUEUE_ERROR});
	delete($hash->{READINGS}{acknowledge_responseTime});
	delete($hash->{READINGS}{acknowledge_retries});
	delete($hash->{READINGS}{acknowledge_missed});	
	delete($hash->{READINGS}{RSSI});
	#delete($hash->{READINGS});
	
	if(defined($hash->{IODev})) { # logical Device wait for phyDevice initialization
		if(ReadingsVal($hash->{IODev}{NAME},"state","") eq "Initialized" && 
				(!defined($hash->{GatewayRFM4Satellite}) || ReadingsVal($hash->{GatewayRFM4Satellite}->{NAME},"state","") eq "Initialized") # Satellite: waiting for IDDev and RFM to be initialized
			) {
			
			Log3 $hash->{NAME}, 5, $hash->{NAME} . ": ready to initialize due to initialized IODev <" . $hash->{IODev}{NAME} . ">" . 
																								(defined($hash->{GatewayRFM4Satellite})? " and RFM <" . $hash->{GatewayRFM4Satellite}->{NAME} . ">" :"");
		} else {
			InternalTimer(gettimeofday()+$to, "RadioNetwork_DoInit", $hash, 0);
			return undef;
		}
		
	} else { #phyDevice initialization
		RadioNetwork_Clear($hash);
	}
	
	$modules{$hash->{TYPE}}{defptr}{$name} = $hash; #needed for RadioNetwork_getMyHash
	CallFn($hash->{NAME}, "LogicalDeviceReset", $hash, undef) if(defined($hash->{IODev})); #reset Satellite, but after adding hash to {defptr} 
  return undef;
}

#####################################
sub RadioNetwork_getMyHash($$$$) {
	my ($iohash, $devType, $devID, $modID) = @_;
	my $phyhash = $iohash;
	
	return if((defined($modID) && $modID !~ m/^[0-9]/) || (defined($devID) && $devID !~ m/^[0-9]{2}/));
# Look for the first "real" RadioGateway and compare DeviceID
	while(defined($phyhash->{IODev}) && !defined($phyhash->{DeviceID})) {   
    $phyhash = $phyhash->{IODev};
  }
  #Log3 $iohash, 1,"getMyHash: valid phyDevice $phyhash->{NAME} of $devID";
	return if(defined($devID) && defined($phyhash) && $phyhash->{DeviceID} ne $devID);
	
#check ModuleID	
  foreach my $name (keys %{ $modules{$devType}{defptr} }) {
		if((!defined($iohash) || $modules{$devType}{defptr}{$name}->{IODev} == $iohash) && 
			 (!defined($modID) || $modules{$devType}{defptr}{$name}->{ModuleID} eq $modID)
			) {
     return ($modules{$devType}{defptr}{$name});
    }
  }
  return undef;
}

sub RadioNetwork_getMyHashByModuleID($$) {
	my ($iohash, $modID) = @_;

  foreach my $name (keys %{ $modules{RadioNetwork_RFM}{defptr} }) {
		if((defined($modules{RadioNetwork_RFM}{defptr}{$name}->{IODev}) && $modules{RadioNetwork_RFM}{defptr}{$name}->{IODev} == $iohash) && 
			 (defined($modules{RadioNetwork_RFM}{defptr}{$name}->{ModuleID}) && $modules{RadioNetwork_RFM}{defptr}{$name}->{ModuleID} eq $modID)
			) {
     	return ($modules{RadioNetwork_RFM}{defptr}{$name});
    }
  }
  return undef;	
}

#####################################
# This is a direct read for commands like get
# IMPORTANT: ReadAnswer beachtet DeviceID!! Fügt diese dem Regex hinzu
sub RadioNetwork_ReadAnswer($$$$) {

  my ($hash, $arg, $alwaysDispatch, $regexp) = @_;
  my $ohash = $hash;
  my $devID=undef; $devID=$hash->{DeviceID} if(defined($hash->{DeviceID}) && $hash->{TYPE} eq "RadioNetwork_Gateway");

	while(defined($hash->{IODev})) { #look for the physical Device
		$hash = $hash->{IODev};
		$devID=$hash->{DeviceID} if(defined($hash->{DeviceID}) && $hash->{TYPE} eq "RadioNetwork_Gateway" && !defined($devID)); #save first valid DeviceID
  }
 	return ("physical Device not found. Starting from $hash->{NAME}", undef) if(!$devID);
  return ("No FD", undef)
        if(!$hash || ($^O !~ /Win/ && !defined($hash->{FD})));

  my ($mdata, $rin) = ("", '');
  my $buf;
  my $to = 3;		                                       # 3 seconds timeout
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
	    my $line = $mdata;
	    $mdata = $2;
    	$hash->{PARTIAL} = $mdata; # for recursive calls
      
      #$line = RadioNetwork_Prefix(0, $ohash, $line); # Delete prefix
			my $regexLine = $line;
			$regexLine =~ s/^$devID//;
      if(($regexp && $regexLine !~ m/$regexp/)) {
      	 $line =~ s/[\n\r]+//g;
      	 RadioNetwork_Parse($hash,$line);
      	 #CallFn($hash->{NAME}, "ParseFn", $hash, $line);
      	 $mdata = $hash->{PARTIAL};
      } else {
        return (undef, $line);
      }
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

sub RadioNetwork_Prefix($$$) {
	my ($isadd, $hash, $msg) = @_;
	return if(!defined($hash->{CmdPrefix}));
	my $prefix  = $hash->{CmdPrefix};
	my $noPrefix = (defined($hash->{NoPrefix}) ? $hash->{NoPrefix} : undef);
	
	#Log3 $hash,1,"$hash->{NAME}: Prefix($isadd) $prefix - $msg - $noPrefix";
	
	if($isadd) {
		foreach my $regex (@{$noPrefix}) {
			return $msg if($msg =~ m/$regex/);
		}
		my $sub=0;
		while($msg =~ m/^-/) {
			$msg = substr($msg,1,length($msg)-1);
			$sub+=1;
		}
		$sub = length($prefix) if($sub gt length($prefix));
		$msg = substr($prefix,0,length($prefix)-$sub) . $msg;
	} else {
		$msg =~ s/^$prefix//i;
	}

	return $msg;
}

sub RadioNetwork_SimpleWrite(@) {
  my ($hash, $msg, $noPrefix ,$nocr) = @_;
  return if(!$hash);
  my $name = $hash->{NAME};
  
	$msg =~ s/^\s+|\s+$//g; #löscht Leerzeichen
	$msg =~ s/\v//g; #löscht \r\n
	return if(!length($msg));
	$msg = RadioNetwork_Prefix(1, $hash, $msg)	if(!$noPrefix);

	if(ReadingsVal($hash->{NAME},"state","") eq "disconnected") {
		Log3 $name, 1, "$name: SW: <$msg> skipped due to disconnected device.";
		return;
	}
  
	if(defined($hash->{IODev})) { # logical Device
		IOWrite($hash,$msg);
  } else {
		Log3 $name, 4, "$name: SW: <$msg>";
		$msg .= "\n" unless($nocr);
		$hash->{USBDev}->write($msg)    if($hash->{USBDev});
		syswrite($hash->{TCPDev}, $msg) if($hash->{TCPDev});
		syswrite($hash->{DIODev}, $msg) if($hash->{DIODev});
	}
  # Some linux installations are broken with 0.001, T01 returns no answer
  select(undef, undef, undef, 0.01);
}
 
sub RadioNetwork_Write(@) { #initCmd=1 --> Needed for PreConfiguration; adds command in front of the MessageQueue
	my ($hash, $fn, $cmd, $regexAnswer, $initCmd) = @_;
	$cmd = $fn if(!$cmd); #needed for CUL_IR ->CUL_Write($hash,$fn,$msg)

	$regexAnswer = $RadioNetwork_ACKRegEx if(defined($hash->{GatewayRFM4Satellite}) && !$regexAnswer); #ACK default Regex only at Satellites
	
	#Log3 $hash->{NAME}, 5, "$hash->{NAME} Write CMD:<$cmd> - MSG:<$msg> - ASW:<$regexAnswer>";
	
  my $dcmd = CallFn($hash->{NAME}, "WriteTranslateFn", $hash, $cmd);
  $dcmd = $cmd if(!length($dcmd) || !defined($dcmd));
  
  return if(!defined($dcmd));

	my $bstring = $dcmd;
  $bstring .= " " if($bstring ne "" && $dcmd ne " ");
	$bstring =~ s/^\s+|\s+$//g; #löscht Leerzeichen
	$bstring =~ s/\v//g; #löscht \r\n  
  $bstring = RadioNetwork_Prefix(1, $hash, $bstring); #addPrefix
  
  Log3 $hash->{NAME}, 4, "$hash->{NAME} sendingQueue $bstring" . (defined($hash->{GatewayRFM4Satellite}) ? " with request $regexAnswer.":"") . ($initCmd?" (preConf)":"");  
  
  RadioNetwork_AddQueue($hash, $bstring, $regexAnswer, $initCmd);
}

sub RadioNetwork_QueueAnswer($$$$) {

	my ($hash, $cmd, $transmit, $regexAnswer) = @_;
	my $now = gettimeofday();
	
	#return if($hash->{STATE} ne "Initialized");

	if($cmd eq "set") { #add regexAgnswer or HomeMatic TransmitDelay
		return 1 if(!$regexAnswer);
		$hash->{QUEUE_COMMAND} = $transmit;
		$hash->{QUEUE_ANSWER} = $regexAnswer;
		$hash->{QUEUE_ANSWER_TIME} = $now;

	} elsif($cmd eq "check") { #checking: if regexAnswer already received or HomeMatic TransmitDelay is expired
		
		return 0 if(!defined($hash->{QUEUE_ANSWER}) && !defined($hash->{sendLogDevice})); #nothing to check
		
		if(defined($hash->{QUEUE_ANSWER})) {
			my $diff = $now - $hash->{QUEUE_ANSWER_TIME};
			if($diff > $RadioNetwork_Waiting4ACK_Time) {
				#readingsSingleUpdate($hash, "state", "not responding", 1);
				Log3 $hash->{NAME},1,"$hash->{NAME}: QUEUE_ANSWER_TIME time out (".int($diff)."s) of " . $hash->{QUEUE_COMMAND} . " - missed answer " . $hash->{QUEUE_ANSWER};
				$hash->{QUEUE_ERROR} = (defined($hash->{QUEUE_ERROR}) ? $hash->{QUEUE_ERROR}+1 : 1); 
				delete($hash->{QUEUE_COMMAND});
				delete($hash->{QUEUE_ANSWER});
				delete($hash->{QUEUE_ANSWER_TIME});
				return 0; #send next TX
			}
			return 1; #wait with next TX
		}
		
		if(defined($hash->{sendLogDevice})) {
			my $qID = $hash->{sendLogDevice};
			my $dDly = ${$hash->{helper}{$qID}{nextSend}} - $now;
			#$dDly -= 0.04 if ($mTy eq "02");# while HM devices need a rest there are
			                                 # still some devices that need faster
			                                 # reactionfor ack.
			                                 # Mode needs to be determined
			if($dDly > 0.01) { # wait less then 10 ms will not work
				#Log3 $hash->{NAME}, 4, "$hash->{NAME}: QueueDelay of $hash->{helper}{$qID}{NAME}:<$transmit> with ".int($dDly*1000)."ms";
			  return 1; #wait with next TX
			}
			Log3 $hash->{NAME}, 5, "$hash->{NAME}: sendingQueue $hash->{helper}{$qID}{NAME}:<$transmit> delayed with ".int($hash->{helper}{$qID}{delaySend}*1000)."ms";
			${$hash->{helper}{$qID}{nextSend}} = gettimeofday() + $hash->{helper}{$qID}{delaySend};
			return 0; #send next TX
		}
		
	} elsif($cmd eq "recv") {
		return 0 if(!defined($hash->{QUEUE_ANSWER}));
		my $regex = $hash->{QUEUE_ANSWER};
		if($regexAnswer =~ m/$regex/) {
			Log3 $hash->{NAME},4,"$hash->{NAME}: QUEUE_ANSWER $regexAnswer of " . $hash->{QUEUE_COMMAND};
			#delete($hash->{QUEUE_ERROR}); #reset only in case of initialization
			delete($hash->{QUEUE_COMMAND});		
			delete($hash->{QUEUE_ANSWER});
			delete($hash->{QUEUE_ANSWER_TIME});				
			return 1;
		}
		#Log3 $hash->{NAME},1,"$hash->{NAME}: Answer <$hash->{QUEUE_ANSWER}> ne <$regexAnswer> received";
	}
}

sub
RadioNetwork_SendFromQueue($$$$)
{
  my ($hash, $bstring, $regexAnswer, $initCmd) = @_;
  my $name = $hash->{NAME};
  my $to = 0.01;#0.01 bei phyDevice; 0.1 bei logicalDevice
  my $now = gettimeofday();

  if($bstring ne "") {
  	my $conState = ReadingsVal($hash->{NAME},"state","");
  	if( ($conState ne "Initialized" && $conState ne "preConfiguration") ||
  			($conState eq "preConfiguration" && !$initCmd) ||
  			RadioNetwork_QueueAnswer($hash, "check", $bstring, undef)
#  			( $hash->{STATE} ne "Initialized" && ($hash->{STATE} ne "PreConfiguration" && $initCmd) ) || 
  		) {
			unshift(@{$hash->{QUEUE}}, ["", "", ""]); #fügt vorne im Array was ein
			InternalTimer($now+$to, "RadioNetwork_HandleWriteQueue", $hash, 0);
			return;
  	}

    RadioNetwork_XmitLimitCheck($hash,$bstring, $now);
    RadioNetwork_SimpleWrite($hash, $bstring, 1); #noPrefix, already handeled by Write()
    RadioNetwork_QueueAnswer($hash,"set", $bstring, $regexAnswer);
  }

  InternalTimer($now+$to, "RadioNetwork_HandleWriteQueue", $hash, 0); #war vorher 1
}

sub RadioNetwork_AddQueue($$$$) {
  my ($hash, $bstring, $regexAnswer, $initCmd) = @_;
  my $startQueueLoop=0;
  
  #HomeMatic: Find right Device, QUEUE and update DelayValues. Values are saved by ID in Gateway Hash
  my $qID = CallFn($hash->{NAME}, "getRightQueueHashFn", $hash, $bstring);
 	if(defined($qID) && $qID) { #copy nextSend, delaySend information
 		$hash->{sendLogDevice} = $qID;
	} elsif(defined($hash->{sendLogDevice})) {
		delete($hash->{sendLogDevice});
	}
	
	
	if(!$hash->{QUEUE} || 0 == scalar(@{$hash->{QUEUE}})) {
		$hash->{QUEUE}[0] = [ "", "", "" ];
		$hash->{QUEUE}[1] = [ $bstring, $regexAnswer, $initCmd ];
		RadioNetwork_SendFromQueue($hash, "", "", 0); #start QueueLoop
		#RadioNetwork_SendFromQueue($hash, $bstring, $regexAnswer, $initCmd);
	} else {
		if($initCmd) {
			splice(@{$hash->{QUEUE}},1,0,[$bstring, $regexAnswer, $initCmd]); #add on 2 position of array. first will be shifted by HandleWriteQueue
		} else { # add at the end of the queue
			push(@{$hash->{QUEUE}}, [$bstring, $regexAnswer, $initCmd] );
		}
	}
	
#  if(!$hash->{QUEUE} || 0 == scalar(@{$hash->{QUEUE}})) { #no other Data available and Device Initialized
#    $hash->{QUEUE}[0] = [ $bstring, $regexAnswer, $initCmd ];
##    RadioNetwork_SendFromQueue($hash, $bstring, $regexAnswer, $initCmd);
#		RadioNetwork_SendFromQueue($hash, "", "", 0);
#	}
##  } else { #already Data available
#  	if($initCmd) { #Needed for SatelliteInitialization
#  		if(!$hash->{QUEUE} || 0 == scalar(@{$hash->{QUEUE}})) { #add in front of the
#  			unshift(@{$hash->{QUEUE}}, ["", "", ""]);
#  			unshift(@{$hash->{QUEUE}}, [$bstring, $regexAnswer, $initCmd] );
#  		} else {
#  			splice(@{$hash->{QUEUE}},1,0,[$bstring, $regexAnswer, $initCmd]); #add on 2 position of array. first is ["",""]
#  		}
#  	} else {
#  		if(!$hash->{QUEUE} || 0 == scalar(@{$hash->{QUEUE}})) {
#  			$hash->{QUEUE}[0] = [ $bstring, $regexAnswer, $initCmd ];
#  		} else {
#				push(@{$hash->{QUEUE}}, [$bstring, $regexAnswer, $initCmd] );
#			}
#		}
##  }

}

#sub RadioNetwork_wait4PreConfigureComplete($) {
#	my ($hash) = @_;
#	my $to = 0.2; #wait till PreConfiguration done
#	
#	my $ret = RadioNetwork_QueueSucessfullyDone( (defined($hash->{IODev}) ? $hash->{IODev} : $hash));
#	if(!$ret) {
#		readingsSingleUpdate($hash, "state", "Initialized", 1);
#		Log3 $hash->{NAME},3,$hash->{NAME} . " device initialized";
#		
#	} elsif($ret==-1) {
#		readingsSingleUpdate($hash, "state", "disconnected", 1);
#		Log3 $hash->{NAME},3,$hash->{NAME} . " device initialization failed";	

#	} else {
#		Log3 $hash->{NAME},5,$hash->{NAME} . " wait till device is preconfigured <$ret>";	
#		InternalTimer(gettimeofday()+$to, "RadioNetwork_wait4PreConfigureComplete", $hash, 0);
#	}
#}

#sub RadioNetwork_QueueSucessfullyDone($) { #error=-1; done=0; ongoing=1 
#	my ($hash) = @_;
#	
#	if( (!$hash->{QUEUE} || scalar(@{$hash->{QUEUE}}) == 0) ) { 
#		return 0 if(!$hash->{QUEUE_ERROR}); #done
#		return -1; #error
#	}	
#	return 1; #ongoing
#}

sub RadioNetwork_HandleWriteQueue($) {
  my ($hash) = @_;
  my $arr = $hash->{QUEUE};
 
  if(defined($arr) && scalar(@{$arr}) > 0) {
#  	Log3 $hash->{NAME}, 1, $hash->{NAME} . ": HandleWriteQueue";
    shift(@{$arr});
    if(scalar(@{$arr}) == 0) {
    	if(RadioNetwork_QueueAnswer($hash, "check", "", undef)) { #Queue ist leet aber wir warten noch auf eine Antwort
    		unshift(@{$hash->{QUEUE}}, ["", "", ""]); #fügt vorne im Array was ein
				InternalTimer(gettimeofday()+0.01, "RadioNetwork_HandleWriteQueue", $hash, 0);
				return;
    	}
      delete($hash->{QUEUE});
      #Log3 $hash->{NAME}, 1, $hash->{NAME} . ": Queue is empty.";
      return;
    }
    my $bstring = $arr->[0][0];
    if($bstring eq "") {
#    	Log3 $hash->{NAME}, 1, $hash->{NAME} . ": HandleWriteQueue <$bstring> <" . $arr->[0][1] . "> <". $arr->[0][2] . ">";
      RadioNetwork_HandleWriteQueue($hash);
      #InternalTimer(gettimeofday()+0.01, "RadioNetwork_HandleWriteQueue", $hash, 0); #war vorher 1
    } else {
#    	Log3 $hash->{NAME}, 1, $hash->{NAME} . ": HandleWriteQueue <$bstring>";
      RadioNetwork_SendFromQueue($hash, $bstring, $arr->[0][1], $arr->[0][2]);
    }
  }
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
  Log3 $name, 5, "$name/RAW: $pdata/$buf";
  $pdata .= $buf;

  while($pdata =~ m/\n/) {
    my $rmsg;
    ($rmsg,$pdata) = split("\n", $pdata, 2);
    $rmsg =~ s/\v//g;
   	$rmsg =~ s/^\s+|\s+$//g; #remove white space
    if($rmsg) {
	    #find right device with same DeviceID
	    RadioNetwork_Parse($hash,$rmsg);
  	}
  }
  $hash->{PARTIAL} = $pdata;
}
 
#####################################
sub RadioNetwork_Parse($$) {

	my ($hash, $rmsg) = @_;
	my $dmsg = $rmsg;
	
  if(	$rmsg =~ m/^Available commands:/ ||    # ignore Help/Debug messages
  		$rmsg =~ m/^<</ ||
  		$rmsg =~ m/^##/ ||
  		$rmsg =~ m/^\* \[/ ||
  		$rmsg =~ m/^Protocols:</ ||
  		$rmsg =~ m/^\[00-99:DeviceID\]/
  	) {
  	Log3 $hash->{NAME}, 3, $hash->{NAME} . ": $rmsg";
  	return;
  }
  
  if($rmsg =~ m/^Ringbuffer overrun/) {
  	my ($hexData,$moduleData,$typeData) = split(/\|/, substr($rmsg,index($rmsg,"<")+1,-1));
  	Log3 $hash->{NAME}, 2, $hash->{NAME} . ": RingBuffer Overrun <$moduleData:$typeData [$hexData]>";
  	return;
  }
  
	
	my $devID = ($rmsg =~ m/^[0-9]{2}/ ? substr($rmsg,0,2) : undef);
  my $devHash = undef;
  foreach my $name (keys %{ $modules{RadioNetwork_Gateway}{defptr} }) {
		if(defined($modules{RadioNetwork_Gateway}{defptr}{$name}->{DeviceID}) && defined($devID) && $modules{RadioNetwork_Gateway}{defptr}{$name}->{DeviceID} eq $devID) {
			$devHash = $modules{RadioNetwork_Gateway}{defptr}{$name};
		}
	}
	
	#Forward RSSI to the right RFM-Module (if "[RSSI" is present)
	my $rssi;
	if( $rmsg =~ m/\[RSSI:([0-9]):([\-0-9]+)\s*[|\]]?/) {
		$rssi = $2;
		my $modId = $1;
		$dmsg = substr($rmsg,0,index($rmsg," \[RSSI")) ; #remove RSSI value
		if(defined($devHash->{TYPE})) {
			my $RFMhash = RadioNetwork_getMyHashByModuleID($devHash,$modId);
			if(defined($RFMhash) && defined($RFMhash->{TYPE})) {
				readingsSingleUpdate($RFMhash,"RSSI",$rssi,1);
				Log3 $RFMhash, 5, "$RFMhash->{NAME} (ModID $modId) rssi update $rssi";
			} else {
				Log3 $RFMhash, 5, "$devHash->{NAME} rssi of cmd <$rmsg> can not be set. (ModID $modId)";
			}
		}
	}

	
	my $foreward2ParseFn = 1;
	
	#safe acknowledge information in right Gateway-Module
	if(defined($devHash->{TYPE})) {
		if( $rmsg =~ m/^[0-9]{2} (ACK|CMD) ([0-9]+)ms-([0-9])?/) {
			readingsSingleUpdate($devHash, "acknowledge_responseTime", $2 . " ms", 1);
			readingsSingleUpdate($devHash, "acknowledge_retries", $3, 1);
			$foreward2ParseFn = 0;
		} elsif( $rmsg =~ m/^[0-9]{2} (ACK|CMD) missed/) {
			readingsSingleUpdate($devHash, "acknowledge_missed", ReadingsVal($devHash->{NAME}, "acknowledge_missed","0")+1, 1);
			$foreward2ParseFn = 0;
		}	
	}
	
	#Parse command in the right Device
  if(defined($devHash->{TYPE})) {
		RadioNetwork_QueueAnswer($devHash,"recv",undef,substr($dmsg,2,length($dmsg)-2));
	 	#$devHash->{helper}{last_RSSI} = $rssi if($rssi);
	 	CallFn($devHash->{NAME}, "ParseFn", $devHash, substr($dmsg,2,length($dmsg)-2)) if($foreward2ParseFn);
  } elsif(!defined($devID)) {
  	RadioNetwork_QueueAnswer($hash,"recv",undef,$dmsg);
  	#$hash->{helper}{last_RSSI} = $rssi if($rssi);
	 	CallFn($hash->{NAME}, "ParseFn", $hash, $dmsg) if($foreward2ParseFn); #first 2 char of $rmsg are no numbers!!
  } else {
	 	Log3 $hash,2,"$hash->{NAME}: Device RadioNetwork_Gateway with ID <".substr($dmsg,0,2)."> not defined. ($rmsg)";
  }
  
}
#####################################
sub
RadioNetwork_Ready($)
{
  my ($hash) = @_;

  return DevIo_OpenDev($hash, 1, "RadioNetwork_DoInit")
                if(ReadingsVal($hash->{NAME},"state","") eq "disconnected" && !defined($hash->{IODev}));

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
	my $ret;
	
	Log3 $hash->{NAME},4, "$hash->{NAME}: reset device";
			
	if(defined($hash->{IODev})) { #Satellite
		$ret = RadioNetwork_DoInit($hash);
	} else { #physical Device
		DevIo_CloseDev($hash);
		$ret = DevIo_OpenDev($hash, 0, "RadioNetwork_DoInit");
	}
  return $ret;
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
