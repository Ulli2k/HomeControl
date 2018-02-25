
# $Id: 00_RadioNetwork.pm 2015-08-03 20:10:00Z Ulli $
#TODO: sendpool integrieren für mehrere RadioNetwork. Keine Sendekonflikte (CUL)

package main;

use strict;
use warnings;
use Time::HiRes qw(gettimeofday);

sub RadioNetwork_RFM_Define($$);
sub RadioNetwork_RFM_PreConfigure($$);
sub RadioNetwork_RFM_LogicalDeviceReset($);

sub RadioNetwork_RFM_Set($@);
sub RadioNetwork_RFM_Get($@);
sub RadioNetwork_RFM_Attr(@);
sub RadioNetwork_RFM_Parse($$);

sub RadioNetwork_RFM_WriteTranslate($$);

sub RadioNetwork_RFM_addUpdateRadioModule($$);
#sub RadioNetwork_RFM_UpdateRSSI($$);

my $clientsRadioNetworkRFM = ":HX2272:LaCrosse:ETH200comfort:FS20:CUL_HM:";

my %matchListRadioNetworkRFM = ( ##Protokolle
    "1:HX2272"  												=> "^H[01F]{12}\$",									#Transformed HX2272 command for 36_HX2272.pm
    "2:LaCrosse"                        => "^OK 9", 												#Transformed LaCrosse command for 36_LaCrosse.pm
    "3:ETH200comfort"                   => "^E[0-9A-F]{8}\$",								#Transformed ETH200 command for 36_ETH200comfort.pm
    "4:FS20"                            => "^81..(04|0c)..0101a001",				#Transformed FS20 command for 10_FS20.pm
    "5:CUL_HM"													=> "^A....................",				#Transformed HomeMatic command for 10_CUL_HM.pm
);

my $matchStringRadioNetworkRFM = "^(R[0-9]|f[0-9]{4})"; #regex for forward messages to other RadioNetwork_RFM defines

my %RadioNetworkProtocols = (
	"01" => "myProtocol",
	"02" => "HX2272",
	"03" => "FS20",
	"04" => "LaCrosse",
	"05" => "ETH",
	"06" => "HomeMatic",
);
my %revRadioNetworkProtocols = reverse %RadioNetworkProtocols;

my $LaCrosse_Recv_MatchRegEx 					= "^R[0-9]"  	. substr($revRadioNetworkProtocols{"LaCrosse"},1,1) 		. "[A-F0-9]{8}\$";	#R149B04916A
my $FS20_Recv_MatchRegEx 							= "^R[0-9]"  	. substr($revRadioNetworkProtocols{"FS20"},1,1)				. "[A-F0-9]{8}\$";  #R23D28C0000
my $HX2272_Recv_MatchRegEx 						= "^R[0-9]"  	. substr($revRadioNetworkProtocols{"HX2272"},1,1)			. "[01F]{12}\$";  	#R320FF0F0FFFF0F
my $ETH200_Recv_MatchRegEx 						= "^R[0-9]s" 	. substr($revRadioNetworkProtocols{"ETH"},1,1)					.	"[0-9A-F]{8}\$";  #R1s501004300
my $HomeMatic_Recv_MatchRegEx 				= "^R[0-9]"		. substr($revRadioNetworkProtocols{"HomeMatic"},1,1)		.	"[0-9A-F]+\$";  	#R160C2C8641467B2D000000012B00

my $HomeMatic_Send_MatchRegEx					= "^R[0-9]s" 	. substr($revRadioNetworkProtocols{"HomeMatic"},1,1) 	. "#[0-9A-F]+#\$";		#R1s6#100BA0015F8E3A467B2D00050000000000#

my $LogDev_HX2272_Recv_MatchRegEx 		= "^H[01F]{12}\$";  					#R320FF0F0FFFF0F
my $LogDev_ETH200_Recv_MatchRegEx 		= "^E[0-9A-F]{8}\$";  				#R320FF0F0FFFF0F
my $LogDev_HomeMatic_Recv_MatchRegEx	= "^As....................";	#As0902A112F10000467B2D

my $SendDelay_HomeMatic 	= 0; #Ist jetzt in der Firmware integriert! #0.100; #ms delay before sending next command
  
my %sets = (
  "raw" 								=> "",
  "reset"								=> "noArg",
  "hmPairForSec"				=> "",
  "LaCrossePairForSec"	=> "",
  "setReceiverMode"			=> join(",", sort values %RadioNetworkProtocols),
  "updateRFMconfig"			=> "noArg",
);

#my %gets = (    # Name, Data to send to the CUL, Regexp for the answer
##  "version"  				=> ["v", '^V[0-9]$'],
#  "raw"      						=> ["", '.*'],
#  "initRadioNetwork" 		=> 1,
#  "RFMconfig"      			=> ["f", "^f[0-9]{4}"],
#);

#Direct RFM69 commands. No adding of Prefix!
my @RadioNetworkDirectCommands =	(	"^f[0-9]\$",
																	"^R[0-9]r[0-9]{2}\$",
																	"^R[0-9](s|b)[0-9]",																
																	"^R[0-9]l[0-9]\$",
																	"^T[0-9]c[0-9]\$"
																);

sub RadioNetwork_RFM_Initialize($) {
  my ($hash) = @_;

  require "$attr{global}{modpath}/FHEM/00_RadioNetwork_Base.pm";

	RadioNetwork_Initialize($hash);

	$hash->{Match}     						= $matchStringRadioNetworkRFM;	#needed for Dispatch
	$hash->{ParseFn}							= "RadioNetwork_RFM_Parse";
	
	$hash->{WriteTranslateFn}			= "RadioNetwork_RFM_WriteTranslate";
	$hash->{getRightQueueHashFn}	= "RadioNetwork_RFM_getRightQueueHash";
	$hash->{LogicalDeviceReset} 	= "RadioNetwork_RFM_LogicalDeviceReset";
	
# Normal devices
  $hash->{DefFn}        				= "RadioNetwork_RFM_Define";
  #$hash->{GetFn}        				= "RadioNetwork_RFM_Get";
  $hash->{SetFn}        				= "RadioNetwork_RFM_Set";
  $hash->{AttrFn}       				= "RadioNetwork_RFM_Attr";
  $hash->{AttrList} 						= "IODev"
  																." lowPower:active,deactivated"  
			  													." CmdTunneling:Basic,Burst,Satellite"
	  															." hmId rfmode longids"
	  															." hmProtocolEvents:0_off,1_dump,2_dumpFull,3_dumpTrigger"
									                ." $readingFnAttributes";
  #$hash->{ShutdownFn} 		= "RadioNetwork_RFM_Shutdown";
}

#####################################

sub RadioNetwork_RFM_Define($$) {
#define <name> RadioNetwork_RFM <device> <ModuleID> 
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if(@a != 4) {
    my $msg = "wrong syntax: define <name> RadioNetwork { <phyDevice> | devicename[\@baudrate] ".
                        "| devicename\@directio } <ModuleID>";
    Log3 undef, 2, $msg;
    return $msg;
  }

  DevIo_CloseDev($hash);

  my $name = $a[0];
  my $dev = $a[2];
  return "ModuleID must be [0-9]." if(defined($a[3]) && uc($a[3]) !~ m/^[0-9]$/);
	my $modID = uc($a[3]);

  #check if DeviceID and ModuleID is already defined in combination
  if(RadioNetwork_getMyHash($defs{$dev},"RadioNetwork_RFM", $defs{$dev}{DeviceID},$modID)) {
 		my $m = "$name: Cannot define multiple RadioNetwork_RFM with identical ModuleID ($modID) and same DeviceID ($defs{$dev}{DeviceID}).";
    Log3 $name, 1, $m;
    return $m;
  }
  #foreach my $d (keys %defs) {
	#	next if($d eq $name);
	#	if($defs{$d}{TYPE} eq "RadioNetwork_RFM") {
	#    if(uc($defs{$d}{DeviceID}) eq uc($defs{$dev}{DeviceID}) && uc($defs{$d}{ModuleID}) =~ m/^$modID/) {
  #        my $m = "$name: Cannot define multiple RadioNetwork-Gateways with identical ModuleID's ($modID).";
  #        Log3 $name, 1, $m;
  #        return $m;
  #      }
  #    }
	#}

  $hash->{ModuleID} = $modID;
	$hash->{CmdPrefix} = "R" . $hash->{ModuleID} . "s" . substr($revRadioNetworkProtocols{"myProtocol"},1,1);
  $hash->{Clients} = $clientsRadioNetworkRFM;
  $hash->{MatchList} = \%matchListRadioNetworkRFM;
  $hash->{NoPrefix} = \@RadioNetworkDirectCommands;
	
	AssignIoPort($hash,$dev);
	Log3 $name, 3, "$name device is linked to $dev as logical device.";

  RadioNetwork_DoInit($hash);
}

sub RadioNetwork_RFM_LogicalDeviceReset($) {
	my ($hash) = @_;
	RadioNetwork_RFM_PreConfigure($hash,"RFM69");
}

#####################################
sub RadioNetwork_RFM_PreConfigure($$) {
	
	my ($hash, $rmsg) = @_;
	my $name = $hash->{NAME};
	
  $hash->{model} = $rmsg;

	if(defined($hash->{IODev}) && AttrVal($hash->{IODev}{NAME}, "preConfigured", undef)) {
		RadioNetwork_RFM_Parse($hash->{IODev}, "f" . $hash->{ModuleID} . $revRadioNetworkProtocols{"myProtocol"} . "8");
		return;
	}	
	
	readingsSingleUpdate($hash, "state", "opened", 0);

	if(!defined($hash->{IODev}->{GatewayRFM4Satellite})) { #no Protocol set for satellites
		RadioNetwork_SimpleWrite($hash, "R" . $hash->{ModuleID} . "r" . $revRadioNetworkProtocols{"myProtocol"});# myProtocol as default Protocol
	}
	RadioNetwork_SimpleWrite($hash, "f" . $hash->{ModuleID});   # get all RFM frequence configs

	#readingsSingleUpdate($hash, "state", "Initialized", 1);
	#Log3 $hash->{NAME},3,$hash->{NAME} . " device initialized";
	#RadioNetwork_Write($hash, "", "f" . $hash->{ModuleID}, "^f" . $hash->{ModuleID}, 1);
	#RadioNetwork_wait4PreConfigureComplete($hash);
}

#####################################
sub
RadioNetwork_RFM_Set($@)
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
  	
  } elsif ($cmd =~ m/reset/i) { #phyDevice must be already initialized
  	RadioNetwork_RFM_PreConfigure($hash,"RFM69"); #will set device state to initialized
  	
  } elsif( $cmd eq "LaCrossePairForSec" ) {
    my @args = split(' ', $arg);

    return "Usage: set $name LaCrossePairForSec <seconds_active> [ignore_battery]" 
    	if(!$arg || $args[0] !~ m/^\d+$/ || ($args[1] && $args[1] ne "ignore_battery") );
    $hash->{LaCrossePair} = $args[1]?2:1;
    InternalTimer(gettimeofday()+$args[0], "RadioNetwork_RFM_RemoveLaCrossePair", $hash, 0);

	} elsif( $cmd eq "hmPairForSec" ) {
		my @args = split(' ', $arg);
		
    return "Usage: set $name hmPairForSec <seconds_active>"
    	if(!$arg || $args[0] !~ m/^\d+$/);
    $hash->{hmPair} = 1;
    InternalTimer(gettimeofday()+$args[0], "RadioNetwork_RFM_RemoveHMPair", $hash, 1);
    
  } elsif( $cmd =~ m/^setReceiverMode/ ) {
    return "Usage: set $name setReceiverMode (" . join(", ", sort values %RadioNetworkProtocols) .")"  
    	if(!defined($revRadioNetworkProtocols{$arg}));
		
		Log3 $name, 1, "set $name $cmd $arg";
		
		#set receiver mode
   	RadioNetwork_Write($hash, "R" . $hash->{ModuleID} . "r" . $revRadioNetworkProtocols{$arg});  

		$hash->{CmdPrefix} = "R" . $hash->{ModuleID} . "s" . substr($revRadioNetworkProtocols{$arg},1,1);
		
		#update RFM configuration in FHEM (returns e.g. "FSK-868MHz")
    RadioNetwork_Write($hash, "f" . $hash->{ModuleID});
	
  	readingsSingleUpdate($hash, $cmd, $arg, 0);  	

  } elsif( $cmd =~ m/^updateRFMconfig/ ) {
  	RadioNetwork_Write($hash, "f" . $hash->{ModuleID});
  }

  return undef;
}

#####################################
sub RadioNetwork_RFM_RemoveLaCrossePair($) {
  my $hash = shift;
  delete($hash->{LaCrossePair});
}
#####################################
sub RadioNetwork_RFM_RemoveHMPair($) {
  my $hash = shift;
  delete($hash->{hmPair});
}
#####################################

#sub
#RadioNetwork_RFM_Get($@)
#{
#  my ($hash, $name, $cmd, @msg ) = @_;
#  my $arg = join(" ", @msg);
#  my $type = $hash->{TYPE};
#  
#  return "\"get $type\" needs at least one parameter" if(!defined($name) || $name eq "");
#  if(!defined($gets{$cmd})) {
#    my @list = map { $_ =~ m/^(raw)$/ ? $_ : "$_:noArg" } sort keys %gets;
#    return "Unknown argument $cmd, choose one of " . join(" ", @list);
#  }

#	return "No $cmd for devices which are defined as none." if(IsDummy($hash->{NAME}));
#	
#	my ($query, $regex);
#	if ($cmd =~ m/RFMconfig/) {
#		$query = $gets{$cmd}[0] . $hash->{ModuleID};
#		$regex = $gets{$cmd}[1];
#			
#  } elsif (defined($gets{$cmd})) {
#		$query = $gets{$cmd}[0];
#		$regex = $gets{$cmd}[1];
#		
#	} else { ## raw
#		$query = $arg;
#		$regex = $gets{$arg}[1];
#	}
#	
#	RadioNetwork_SimpleWrite($hash, $query);
#	my ($err, $rmsg) = RadioNetwork_ReadAnswer($hash, $query, 1, $regex); #Dispatchs all commands
#  $rmsg = "No answer. $err" if(!defined($rmsg));
#    
#  readingsSingleUpdate($hash, $cmd, $rmsg, 0);  	

#  return "$name $query => $rmsg";
#}
 
#####################################
sub
RadioNetwork_RFM_Attr(@)
{
  my ($cmd,$name,$aName,$aVal) = @_;
  my $hash = $defs{$name};


  if($aName =~ m/^lowPower/i) {
  	
	  return "Usage: attr $name $aName <active|deactivated>" if($aVal !~ m/^(active|deactivated)$/);  	
	  
	 	RadioNetwork_Write($hash, "R" . $hash->{ModuleID} . "l" . ($aVal eq "active" ? "1" : "0")); #ListenMode of RFM69
	 	CallFn($hash->{IODev}->{NAME}, "SatelliteBurstFn", $hash->{IODev}, 1);

	 	Log3 $hash->{NAME},2,"set $hash->{NAME} " . ($aVal eq "active" ? "in lowPower mode." : "lowPower mode off.");

  } elsif($aName eq "hmId") {

		return "wrong syntax: hmId must be 6-digit-hex-code (3 byte)"
	    if($aVal !~ m/^[A-F0-9]{6}$/i);
	}
  return undef;
}

#####################################
sub
RadioNetwork_RFM_getRightQueueHash($$) {

	 my ($hash, $dmsg) = @_;
	 my $id;

	 #Log3 $hash->{NAME},1,"$hash->{NAME}: RadioNetwork_RFM_getRightQueueHash <$dmsg>";
	 
	#HomeMatic
	if($dmsg =~ m/${HomeMatic_Send_MatchRegEx}/) { #R1s6#100BA0015F8E3A467B2D00050000000000#
    $id = substr($dmsg,19,6);#get HMID destination
    #Log3 $hash->{NAME},1,"$hash->{NAME}: RadioNetwork_RFM_getRightQueueHash hmId=$id";
    $hash->{helper}{$id}{nextSend} = \$modules{CUL_HM}{defptr}{$id}{helper}{io}{nextSend}; #link to nextSend of Device
    $hash->{helper}{$id}{delaySend} = $SendDelay_HomeMatic;
    $hash->{helper}{$id}{NAME} = $modules{CUL_HM}{defptr}{$id}{NAME};
	}
	
	return $id;
}

#####################################
sub
RadioNetwork_RFM_WriteTranslate($$)
{
  my ($hash, $dmsg) = @_;
  my $dmsgMod = $dmsg;
  my $setProtocol;
  
  if( $dmsg =~ m/${LogDev_HX2272_Recv_MatchRegEx}/ ) {
  	$dmsgMod = substr($dmsg,1,length($dmsg)-1);
  	$setProtocol = "HX2272";
  }
  elsif( $dmsg =~ m/${LogDev_ETH200_Recv_MatchRegEx}/ ) {
  	$dmsgMod = substr($dmsg,1,length($dmsg)-1);
   	$setProtocol = "ETH";
	}	
	elsif( $dmsg =~ m/${LogDev_HomeMatic_Recv_MatchRegEx}/ ) { #"As..."
  	$dmsgMod = "#" . substr($dmsg,2,length($dmsg)-2) . "#"; #Data in Hex Format marked with '#'
   	$setProtocol = "HomeMatic";
	}
	
	Log3 $hash->{NAME}, 5, $hash->{NAME} . ": WriteTranslate <$dmsg> transformed to <$dmsgMod>";	
	
	#send Command with different Protocol as currently set
	my $curProtocol = ReadingsVal($hash->{NAME},"setReceiverMode",undef);
	$dmsgMod =	"-" . substr($revRadioNetworkProtocols{$setProtocol},1,1) . $dmsgMod if(defined($curProtocol) && defined($setProtocol) && $curProtocol !~ m/$setProtocol/);
	
  return $dmsgMod;
}

#####################################
sub
RadioNetwork_RFM_ReadTranslate($$)
{
  my ($hash, $dmsg) = @_;
	my $dmsgMod = $dmsg;
	
#Adapt RadioNetwork command (O02D28C0000) to match FS20 command ("^81..(04|0c)..0101a001") from CUL	
  if( $dmsg =~ m/${FS20_Recv_MatchRegEx}/ ) {  #O02D28C0100 ---> FS20
  	my $dev = substr($dmsg, 3, 4);
    my $btn = substr($dmsg, 7, 2);
    my $cde = substr($dmsg, 9, 2);
    # Msg format:
    # 81 0b 04 f7 0101 a001 HHHH 01 00 11
    $dmsgMod = "810b04f70101a001" . lc($dev) . lc($btn) . "00" . lc($cde);
    #Log 1, "Modified F20 command: " . $dmsgMod;
	}

#Adapt RadioNetwork command (R32 0FF0F0FFFF0F) to match HX2272 command "H.."
	elsif( $dmsg =~ m/${HX2272_Recv_MatchRegEx}/ ) {
		$dmsgMod = "H" . substr($dmsg,3,length($dmsg)-3);
	}
	
#Adapt RadioNetwork command (R1s5 01004300) to match HX2272 command "E..."
	elsif( $dmsg =~ m/${ETH200_Recv_MatchRegEx}/ ) {
		$dmsgMod = "E" . substr($dmsg,4,length($dmsg)-4);
	}

#Adapt RadioNetwork command (R16 0C2C8641467B2D000000012B00) to match CUL_HM command "A..."
	elsif( $dmsg =~ m/${HomeMatic_Recv_MatchRegEx}/ ) {
		$dmsgMod = "A" . substr($dmsg,3,length($dmsg)-3);

		my $src = substr($dmsgMod,9,6);
    if($modules{CUL_HM}{defptr}{$src}){
      $modules{CUL_HM}{defptr}{$src}{helper}{io}{nextSend} = gettimeofday() + $SendDelay_HomeMatic;
    }
	}
#Adapt RadioNetwork command (F019204356A) to LaCrosse module standard syntax "OK 9 32 1 4 91 62" ("^\\S+\\s+9 ")
	elsif( $dmsg =~ m/${LaCrosse_Recv_MatchRegEx}/ ) {
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
		$addr = (((hex(substr($dmsg,3,2)) & 0x0F) << 2) | ((hex(substr($dmsg,5,2)) & 0xC0) >> 6));
		$type = ((hex(substr($dmsg,5,2)) & 0xF0) >> 4); # not needed by LaCrosse Module
		#$channel = 1; ## $channel = (hex(substr($dmsg,5,2)) & 0x0F);

		$temperature = ( ( ((hex(substr($dmsg,5,2)) & 0x0F) * 100) + (((hex(substr($dmsg,7,2)) & 0xF0) >> 4) * 10) + (hex(substr($dmsg,7,2)) & 0x0F) ) / 10) - 40;
		return if($temperature >= 60 || $temperature <= -40);

		$humidity = hex(substr($dmsg,9,2));
		$batInserted = ( (hex(substr($dmsg,5,2)) & 0x20) << 2 );

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

  return $dmsgMod;
}

sub RadioNetwork_RFM_Parse($$) {

	my ($iohash, $rmsg) = @_;
	my $name = $iohash->{NAME};
	my $RFMid;
	
	my $hash = RadioNetwork_getMyHashByModuleID($iohash, substr($rmsg,1,1)); #looking for right RadioNetwork_RFM with right ModuleID
	if(!$hash) {
		Log3 $iohash->{NAME},1,"$iohash->{NAME}: Device RadioNetwork_RFM not defined for command processing <$rmsg>.";
		return;
	}
	Log3 $name, 4, "$name: Forward message <$rmsg> to right RadioNetwork_RFM device $hash->{NAME}." if($iohash != $hash); #to Device ($devID) with Module ($modID).";
	$name = $hash->{NAME};

	#RadioNetwork_QueueAnswer($hash,"recv",undef,$rmsg);	

  #update RSSI Readings if info is available
#	if($rmsg =~ /\[RSSI:[0-9]:([\-0-9]+)\s*[|\]]?/) {
#		#my $RSSI = substr($rmsg,index($rmsg, "\[RSSI:")+6,index($rmsg, "\]\n"));
#		my $RSSI = $1;
#		readingsSingleUpdate($hash,"RSSI",$RSSI,0);
#		$rmsg = substr($rmsg,0,index($rmsg," \[RSSI")); #remove RSSI value
#	}
#	
#  if(	$rmsg =~ m/^Available commands:/ ||    # ignore Help/Debug messages
#  		$rmsg =~ m/^<</ ||
#  		$rmsg =~ m/^##/ ||
#  		$rmsg =~ m/^\* \[/ ||
#  		$rmsg =~ m/^Protocols:</ ||
#  		$rmsg =~ m/^\[00-99:DeviceID\]/
#  	) {
#  	Log3 $name,5, "$name: $rmsg";
#  	return "";
#  }
  	
	# Commands which can only be received from the main physical Module/Device
	#Log3 $name, 5, "$name Parse: <$rmsg>";
	if ( $rmsg =~ m/^f[0-9][0-9]{2}[48]/) {
		RadioNetwork_RFM_addUpdateRadioModule($hash,$rmsg);
		if(ReadingsVal($hash->{NAME},"state","") eq "opened") {
			readingsSingleUpdate($hash, "state", "Initialized", 1);
			Log3 $hash->{NAME},3,$hash->{NAME} . " device initialized";
		}
		return "";
		
	} elsif ( $rmsg =~ m/^ (ACK|CMD) /) {
		Log3 $name,1, "$name: ACK_MISSED $rmsg" if($rmsg =~ m/missed/);
		return "";
  }
	
	#Translate Msg if necessary for other Modules for dispatching
	my $dmsg = RadioNetwork_RFM_ReadTranslate($hash,$rmsg);
	
	$hash->{"${name}_MSGCNT"}++;
  $hash->{"${name}_TIME"} = TimeNow();
  #$hash->{READINGS}{state}{TIME} = TimeNow();      # showtime attribute
  # showtime attribute
  #readingsSingleUpdate($hash, "state", $hash->{READINGS}{state}{VAL}, 0);
  $hash->{RAWMSG} = $rmsg;
  my %addvals = (RAWMSG => $dmsg);
  #$addvals{RSSI} = $hash->{helper}{last_RSSI} if($hash->{helper}{last_RSSI});

  Dispatch($hash, $dmsg, \%addvals);
  return "";
}

#####################################
#sub RadioNetwork_RFM_UpdateRSSI($$) {

#	my ($hash, $value) = @_;
#	Log3 $hash->{NAME}, 5 ,$hash->{NAME} . " update rssi value ($value)";
#	readingsSingleUpdate($hash,"RSSI",$value,1);
#}


#####################################
# return 1 ModeID richtig
# return 0 dispatch
sub RadioNetwork_RFM_addUpdateRadioModule($$) { 
 # [0-9]-(OOK|FSK)\-(433|868)MHz
 #  # f <ModuleID>{1} Protocol{2} Frequenz{1} --> 05f2018
	my ($hash, $a) = @_;
	my $modID = substr($a,1,1);
	my $protocol = substr($a,2,2);
	my $freq = (substr($a,4,1) eq "4" ? "433 MHz" : "868 MHz");
	
	if($modID eq $hash->{ModuleID}) {
		readingsSingleUpdate($hash,"Frequence", $freq,0);
		readingsSingleUpdate($hash,"Protocol", $protocol,0);
		readingsSingleUpdate($hash,"setReceiverMode", $RadioNetworkProtocols{$protocol},0);

		if($RadioNetworkProtocols{$protocol} eq "HomeMatic") { #rfmode is needed for 10_CUL_HM.pm
			$attr{$hash->{NAME}}{rfmode} = "HomeMatic";
		} else {
			delete $attr{$hash->{NAME}}{rfmode} if(defined($attr{$hash->{NAME}}{rfmode}));
		}
		return 1;
	}
	return 0;
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

  <a name="RadioNetwork_RFM_Define"></a>
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

  <a name="RadioNetwork_RFM_Set"></a>
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

  <a name="RadioNetwork_RFM_Get"></a>
  <b>Get</b>
  <ul>
  </ul>
  <br>

  <a name="RadioNetwork_RFM_Attr"></a>
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
