
# $Id: 36_myRollo.pm 3889 2013-09-10 10:40:15Z ulli $
#
# TODO:

package main;

use strict;
use warnings;
use SetExtensions;

my $myRollo_matchStr = "^J([0-2]|t[0-9999])\$"; # J<0:off,1:down,2:up><sec*10> oder Jt<sec*10> 

my %sets = (
  "position" 	=> "0,25,50,75,100",
  "stop" 		 	=> "noArg",
  "reset"		 	=> "noArg",
  "automatic"	=> "enable,disable",
);

sub myRollo_Notify_ParameterUpdate($$$$$);
sub myRollo_Notify_Automatic($);

my $myRollo_Default_cmThickness			=	0.6;
my $myRollo_Default_cmDiameter			=	14.5;
my $myRollo_Default_cmWindowHight		=	220;
my $myRollo_Default_secondsDown			= 21;
my $myRollo_Default_secondsUp				=	25;
my $myRollo_Default_secondsOffset		=	0;
my $myRollo_Direction_up						= 2;
my $myRollo_Direction_down					= 1;

#####################################
sub myRollo_Initialize($) {
  my ($hash) = @_;

  $hash->{Match}    	 		= $myRollo_matchStr;
  $hash->{SetFn}     			= "myRollo_Set";
  #$hash->{GetFn}     		= "myRollo_Get";
  $hash->{DefFn}     			= "myRollo_Define";
  $hash->{UndefFn}   			= "myRollo_Undef";
  #$hash->{FingerprintFn}  = "myRollo_Fingerprint";
  $hash->{ParseFn}   			= "myRollo_Parse";
  $hash->{AttrFn}    			= "myRollo_Attr";
  $hash->{AttrList}  			=  " secondsDown secondsUp secondsOffset"
  													." cmWindowHight cmThickness cmDiameter Speed"
  													." autoLimits autoDevice autoOrientation:nord,ost,sued,west"
  													." $readingFnAttributes";
  
}

#####################################
sub myRollo_Define($$) {	#<Device>

  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

	#Check Parameter count
  if(@a != 3) {
    my $msg = "wrong syntax: define <name> myRollo <RadioNetwork-Device>";
    Log3 undef, 2, $msg;
    return $msg;
  }

	my $name = $a[0];
	my $dev = $a[2];
	
	#Check if Device already available with defined name
  return "myRollo device $name already used for $dev. (Just one myRollo is allowed)" if($modules{myRollo}{defptr}{$dev});
	$modules{myRollo}{defptr}{$dev} = $hash;
  
  AssignIoPort($hash,$dev);
  if(defined($hash->{IODev}->{NAME})) {
    Log3 $name, 3, "$name: I/O device is " . $hash->{IODev}->{NAME};
  } else {
    Log3 $name, 1, "$name: no I/O device.";
  }
	
	$hash->{pi} = 3.14159265359;
	$attr{$name}{"cmThickness"} = $myRollo_Default_cmThickness;
	$attr{$name}{"cmDiameter"} = $myRollo_Default_cmDiameter;
	$attr{$name}{"cmWindowHight"} = $myRollo_Default_cmWindowHight;
	$attr{$name}{"secondsUp"} = $myRollo_Default_secondsUp;
	$attr{$name}{"secondsDown"} = $myRollo_Default_secondsDown;
	
	myRollo_Attr("",$name,"",""); #recalculate Values
	
	readingsSingleUpdate($hash, "state", "none", 1);
	
  return undef;
}

#####################################
sub myRollo_Undef($$) {

  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $dev = $hash->{IODev}->{NAME};

	delete( $modules{myRollo}{defptr}{$dev} );
	
  return undef;
}

#####################################
sub myRollo_movingTime($$$) {
	
	my ($hash, $pos, $speed) = @_;
	
	my $name = $hash->{NAME};
	my $cmThickness = AttrVal($name, "cmThickness", $myRollo_Default_cmThickness); #cm
	my $cmDiameter = AttrVal($name, "cmDiameter", $myRollo_Default_cmDiameter); #cm
	my $maxHight = AttrVal($name, "cmWindowHight", $myRollo_Default_cmWindowHight); #cm
	my $distance = $pos/100*$maxHight;
	
	#my $phiTop = $hash->{pi}*$cmDiameter/$cmThickness;
	my $phiBottom = sqrt(($distance*2/($cmThickness/2/$hash->{pi})-($hash->{phiTop}*$hash->{phiTop}))*-1);

	return int(($hash->{phiTop}-$phiBottom)*60/(2*$hash->{pi}*$speed));
}

sub myRollo_bufferedPosition($) {

	my ($argHash) = @_;
	my $hash = $argHash->{HASH};
	my $msg = $argHash->{MSG};
	myRollo_Set($hash, @{$msg});
}

sub myRollo_Set($@) {
  my ($hash, @msg) = @_;
	
  shift @msg;
  my $name = $hash->{NAME};
  my $cmd = shift @msg;
  my $arg = join("",@msg);
	
  my $list = join(" ", map { $sets{$_} eq "" ? $_ : "$_:$sets{$_}" } sort keys %sets);
	return $list if( $cmd eq '?' || $cmd eq '');
	return "Unknown argument $cmd, choose one of " . join(", ", sort keys %sets) if(!defined($sets{$cmd}));  

	Log3 $name, 4, "set $name $cmd $arg";
	
	#my $upTime = AttrVal($name, "secondsUp", "24"); #seconds
	#my $downTime = AttrVal($name, "secondsDown", "21"); #seconds
	#my $maxDistance = AttrVal($name, "cmWindowHight", "205"); #cm
	#my $cmThickness = AttrVal($name, "cmThickness", "0.6"); #cm
	#my $cmDiameter = AttrVal($name, "cmDiameter", "14,5"); #cm

	#my $hash->{pi} = 3.14159265359;
	#my $attr{$name}{"phiTop"} = $hash->{pi}*$cmDiameter/$cmThickness;
	#my $phiBottom = sqrt(($maxDistance*2/($cmThickness/2/$hash->{pi})-($hash->{phiTop}*$hash->{phiTop}))*-1);
	#my $SpeedDown = 60*($hash->{phiTop}-$phiBottom)/($downTime*2*$hash->{pi}); 
	#my $SpeedUp = 60*($hash->{phiTop}-$phiBottom)/($upTime*2*$hash->{pi});#variable in case of up/down -> calculated by secondsUP/Down

	#Log3 $name, 3, "$name: down: $SpeedDown | up: $SpeedUp";
	
	my $Position = ReadingsVal($name, "position", "0"); #0,25,50,75,100 [%]

START:
	if ($cmd =~ m/^position/i) {
		if($Position ne $arg && $Position !~ m/^unknown/) {
			my $moveTime = 0;
			my $direction = ( $Position-$arg > 0 ? $myRollo_Direction_up : $myRollo_Direction_down ); #up -> positiv, down -> negativ
			my $oldTime = myRollo_movingTime($hash, $Position,($direction==$myRollo_Direction_up?$hash->{SpeedUp}:$hash->{SpeedDown}));
			my $newTime = myRollo_movingTime($hash, $arg,($direction==$myRollo_Direction_up?$hash->{SpeedUp}:$hash->{SpeedDown}));
			
			my $secondsOffset = AttrVal($name, "secondsOffset", $myRollo_Default_secondsOffset); #due to repositioning on end positions
			
			$moveTime = int(abs($oldTime-$newTime)) + $secondsOffset;
		
			Log3 $name, 4, "$name: moving rollo " . ($direction == $myRollo_Direction_up ? "up" : "down") . " for $moveTime seconds.";
			IOWrite($hash, "J" . $direction . int($moveTime *10)); #2:up, 1:down
			
		} elsif($Position =~ m/^unknown/) {

			if($arg == 100 || $arg == 0) {
				my $direction = ($arg == 0 ? $myRollo_Direction_up : $myRollo_Direction_down);
				my $moveTime = ($direction == $myRollo_Direction_down ? AttrVal($name, "secondsDown", $myRollo_Default_secondsDown) : AttrVal($name, "secondsUp", $myRollo_Default_secondsUp));
				Log3 $name, 4, "$name: moving rollo from unknown position " . ($direction == $myRollo_Direction_down ? "down" : "up") . " for $moveTime seconds.";
				IOWrite($hash, "J" . $direction . int($moveTime *10)); #2:up, 1:down

			} else {
				Log3 $name, 4, "$name: moving rollo from unknown position.";
				my(%argHash) = ( 'HASH' => $hash, 'MSG' => [$name, $cmd, $arg]);
				my $waitTime = gettimeofday()+AttrVal($name, "secondsUp", $myRollo_Default_secondsUp);
				RemoveInternalTimer(\%argHash,"myRollo_bufferedPosition");
				InternalTimer($waitTime, "myRollo_bufferedPosition", \%argHash, 0);
				$cmd = "reset";
				goto START;
			}
		
		}
	  readingsSingleUpdate($hash, "position", "$arg", 1);
	  readingsSingleUpdate($hash, "state", ( ($arg==0) ? "up" : ($arg==100) ? "down" : $arg), 1);
	
	} elsif ($cmd =~ m/^stop/i) {
		IOWrite($hash, "J0");
		readingsSingleUpdate($hash, "state", "undefined", 1);

	} elsif ($cmd =~ m/^reset/i) {
		Log3 $name, 4, "$name: reset rollo position. (moving up)";
		
		my $upTime = AttrVal($name, "secondsUp", $myRollo_Default_secondsUp); #seconds
		IOWrite($hash, "J" . $myRollo_Direction_up . int($upTime *10));
		readingsSingleUpdate($hash, "position", "0", 1);
		readingsSingleUpdate($hash, "state", "up", 1);
	
	} elsif ($cmd =~ m/^automatic/i) {
		my $AutomaticDevice = AttrVal($name,"autoDevice",undef);
		if(!$AutomaticDevice) {
			Log3 $name, 1, "$name: automatic can just be use if autoDevice Attribute was set."; 
			readingsSingleUpdate($hash, "automatic", "disabled", 1);
			return;
		}
		readingsSingleUpdate($hash, "automatic", $arg ."d", 1);
		myRollo_Notify_Automatic($AutomaticDevice);
		
	
  } else {
    return "Unknown argument $cmd, choose one of ".$list;
  }

  return undef;
}

sub myRollo_Parse($$) {
	
  my ($hash, $rmsg) = @_;
  my $name = $hash->{NAME};
  
	my $defName = $name;
	my $rhash = $modules{myRollo}{defptr}{$defName};
	$name = $rhash->{NAME};
  
	if ( $rmsg =~ m/^J[0-2]/) {
		my $position = substr($rmsg,1,1);
		$position = ($position eq "0" ? "unknown" : ($position eq "1" ? "100" : "0"));
		Log3 $name, 4, "$name: setting new position to $position% ($rmsg).";
		readingsSingleUpdate($rhash, "position", $position, 1);	
	} elsif ( $rmsg =~ m/^Jt/) {
	}
	#readingsBeginUpdate($rhash);
	
	#E2277t5400h
	#$T = substr($msg,1,4)/100 if($msg =~ m/[0-9]{4}t/i);
	#$H = substr($msg,6,4)/100 if($msg =~ m/[0-9]{4}h/i);

	#readingsBulkUpdate($rhash, "temperature", $T) if($T);
	#readingsBulkUpdate($rhash, "humidity", $H) if($H);

	#create state text
	#my $state;
	#$state .= "T: $T " if($T);
	#$state .= "H: $H " if($H);
	#chop($state);
	#readingsBulkUpdate($rhash, "state", $state);
	
	#readingsEndUpdate($rhash,1);	

	my @list;
	push(@list, $rhash->{NAME});	
  return @list;
}

sub myRollo_Attr(@) {

	my ($cmd,$name,$aName,$aVal) = @_;
  my $hash = $defs{$name};

	if($aName =~ m/autoOrientation/) {
		return "Usage: attr $name $aName <nord,ost,sued,west>" if($aVal !~ m/^(nord|ost|sued|west)$/);
	
	} elsif($aName =~ m/autoDevice/) {
		return "Usage: attr $name $aName with $aVal $cmd as undefined device." if(!$defs{$aVal} && $cmd ne "del");
		
	} elsif($aName =~ m/autoLimits/) {
		return "Usage: attr $name $aName for different HomeStates <day-night|day-night|...>" if($aVal !~ m/^(\d+\|*|(\d+\-\d+\|*))+$/);
		
	} elsif($aName =~ m/(secondsUp|secondsDown|cmWindowHight|cmThickness|cmDiameter)/) {
		return "Usage: attr $name $aName <digits>" if($aVal !~ m/^\d+$/);  
		
		my $secondsUp = AttrVal($name, "secondsUp", $myRollo_Default_secondsUp); #seconds
		my $secondsDown = AttrVal($name, "secondsDown", $myRollo_Default_secondsDown); #seconds
		my $maxHight = AttrVal($name, "cmWindowHight", $myRollo_Default_cmWindowHight); #cm
		my $cmThickness = AttrVal($name, "cmThickness", $myRollo_Default_cmThickness); #cm
		my $cmDiameter = AttrVal($name, "cmDiameter", $myRollo_Default_cmDiameter); #cm
		
		if($aName =~ m/secondsUp/) {
			$secondsUp = $aVal;
			IOWrite($hash, "Jt" . ($secondsUp >  $secondsDown ? $secondsUp : $secondsDown)  *10);
		} elsif($aName =~ m/secondsDown/) {
			$secondsDown = $aVal;
			IOWrite($hash, "Jt" . ($secondsUp >  $secondsDown ? $secondsUp : $secondsDown)  *10);
		} elsif($aName =~ m/cmWindowHight/) {
			$maxHight = $aVal;
		} elsif($aName =~ m/cmThickness/) {
			$cmThickness = $aVal;
		} elsif($aName =~ m/cmDiameter/) {
			$cmDiameter = $aVal;
		} else {
			IOWrite($hash, "Jt" . ($secondsUp >  $secondsDown ? $secondsUp : $secondsDown)  *10);
		}
		
		$hash->{phiTop} = sprintf("%.2f",$hash->{pi}*$cmDiameter/$cmThickness);
		$hash->{phiBottom} = sprintf("%.2f",sqrt(($maxHight*2/($cmThickness/2/$hash->{pi})-($hash->{phiTop}*$hash->{phiTop}))*-1));
		$hash->{SpeedDown} 	= sprintf("%.2f",60*($hash->{phiTop}-$hash->{phiBottom})/($secondsDown*2*$hash->{pi})); 
		$hash->{SpeedUp}		= sprintf("%.2f",60*($hash->{phiTop}-$hash->{phiBottom})/($secondsUp*2*$hash->{pi}));
	
	} elsif($aName =~ m/secondsOffset/) {
	
	}
	
  return undef;
}
  
#####################################
#sub myRollo_Get($@) {
#
#  my ($hash, $name, $cmd, @args) = @_;

#  return "\"get $name\" needs at least one parameter" if(@_ < 3);
#
#  my $list = "";
#
#  return "Unknown argument $cmd, choose one of $list";
#}

#####################################
#sub myRollo_Fingerprint($$) {
#
#  my ($name, $msg) = @_;
#
#  return ( "", $msg );
#}

######################################
# AUTOMATIC Funktion
# myRollo_Notify_ParameterUpdate("Rollosteuerung","wdt_Rollo_Tageslicht","HomeScene","HS_Climate:temperature","Weather:fc0_sun");
######################################

sub myRollo_Notify_Automatic($) {
	my ($name) = @_;
  my $hash = $defs{$name};	
  
  return if(ReadingsVal($name,"state","on") eq "off");
  
  my $devFilter = "TYPE=myRollo:FILTER=automatic!=disabled";
  my @HomeStates = ("Home","Away","Night","Vacation"); #index in Attribute "autoHomeSceneLimit"
  
	my $indirektHeizen = ReadingsVal($name,"indirektHeizen",0);
	my $Beschattung = ReadingsVal($name,"Beschattung",0);
	my $warmHalten = ReadingsVal($name,"warmHalten",0);
	my $Tageslicht = ReadingsVal($name,"Tageslicht","sued");
	my $Nacht	= ($Tageslicht eq "dunkel");
	my $HomeScene = ReadingsVal($name,"HomeState","Home");
	my ($iHomeScene) = grep { $HomeStates[$_] ~~ $HomeScene } 0 .. $#HomeStates;
	$iHomeScene = 0 if(!$iHomeScene);
	
	my @devList=devspec2array($devFilter);
	my $cnt_devs=@devList;
	foreach my $devName (@devList) {
		next if(ReadingsVal($devName,"automatic","enabled") eq "disabled");
		
		my $value = 0; #offen	
		my $posLimit = 100;
		my $pos = ReadingsVal($devName,"position",0);
		my $orientation = AttrVal($devName,"autoOrientation",$Tageslicht);
		my $autoLimits = AttrVal($devName,"autoLimits","100");
		my $iHeizen = ($indirektHeizen && ($orientation eq $Tageslicht));
		$posLimit = (split(/\|/, $autoLimits))[$iHomeScene] if(($autoLimits =~ tr/\|//) >= $iHomeScene); #HomeScene split 
		$posLimit = (split(/\-/,$posLimit))[$Nacht] if($posLimit =~ m/[0-9]+\-[0-9]+/); #day-night split

		$value = ( $Nacht || ($warmHalten && !$iHeizen) || $Beschattung ); #just close states
		$value *= $posLimit;
		
		fhem("set $devName position $value") if($value != $pos);
		
		Log3 $name, 1, "$name: $devName move to position $value with $posLimit as limit <"
										. "$Tageslicht"
										. ($Nacht?",Nacht":",Tag") 
										. ($warmHalten?",warmHalten":"")										
										. ($iHeizen?",indirektHeizen":"") 
										. ($Beschattung?",Beschattung":"")
										. ">";
	}
}

#{myRollo_Notify_ParameterUpdate("Rollosteuerung","wdt_Rollo_Tageslicht","HomeScene","HS_Climate","Weather")}
sub myRollo_Notify_ParameterUpdate($$$$$) {
	my ($AutoDev, $DayLightDev, $HomeStateDev, $TDev, $WeatherDev) = @_;
	my $hash = $defs{$AutoDev};
	if(!$hash) {
		Log3 undef, 1, "myRollo_Notify_ParameterUpdate: Automatic Device not defined!";
		return;
	}
		
	my $HS = ReadingsVal($HomeStateDev,"state",undef);
	my $T = ReadingsVal($TDev,"temperature",undef);
	my $DL = ReadingsVal($DayLightDev,"state",undef);
	my $S = undef;
	
	#calculate sun for current time periode out of cloud values
	my $c = ReadingsVal($WeatherDev,"fc0_sun" ,"0"); #Ersatzwert
	my $tb = "00";
  my (undef,undef,$hour,undef,undef,undef,undef,undef,undef) = localtime(time);
	foreach my $t ("00","03","06","09","12","15","18","21") {
		$c = ReadingsVal($WeatherDev,"fc0_cloud" . $t ,undef);
		next if(!defined($c));
		
		last if($t == $hour);
		if($t > $hour) {
			my $cb = ReadingsVal($WeatherDev,"fc0_cloud" . $tb,undef);
			$S = ($cb-$c)/($tb - $t) * ($hour - $t) + $c;
			last;
		}
		$tb = $t;
	}
	$S = $c if(!defined($S));
	$S = 100-$S; #cloud to sun
	
	if(!defined($T) || !defined($S) || !defined($HS) || !defined($DL)) {
		Log3 $AutoDev, 1, "$AutoDev: not all readings defined.";
		return;
	}
	
	my $ifIndirektHeizen 	= AttrVal($AutoDev,"IF_indirektHeizen",	"$T > 0 && $S > 25");
	my $ifwarmHalten 			= AttrVal($AutoDev,"IF_warmHalten",			"$T < 5");
	my $ifBeschattung 		= AttrVal($AutoDev,"IF_Beschatten",			"$T > 23 && $S >= 75");
	
	my $indirektHeizen 	= eval("($ifIndirektHeizen)	? 1 : 0");
	my $warmHalten 			= eval("($ifwarmHalten)			? 1 : 0");
	my $Beschattung			= eval("($ifBeschattung)		? 1 : 0");
	
	readingsBeginUpdate($hash);
	readingsBulkUpdate($hash, "T", sprintf("%.0f",$T));
	readingsBulkUpdate($hash, "S", sprintf("%.0f",$S));
	readingsBulkUpdate($hash, "indirektHeizen", $indirektHeizen);
	readingsBulkUpdate($hash, "Beschattung", 		$Beschattung);
	readingsBulkUpdate($hash, "warmHalten",			$warmHalten);
	readingsBulkUpdate($hash, "HomeState", 				$HS); #Home, Away, Night, Vacation
	readingsBulkUpdate($hash, "Tageslicht", 			$DL) if($DL !~ m/(active|inactive)/); #dunkel, ost, west, sued
	
	$DL = ($DL eq "dunkel" ? "Nacht" : ($DL eq "ost" ? "Sonnenaufgang" : ($DL eq "sued" ? "Mittag" : ($DL eq "west" ? "Sonnenuntergang" : ""))));
	readingsBulkUpdate($hash, "sum_state", "$HS - $DL" . ($indirektHeizen?", indirektHeizen":"") . ($warmHalten?", warmHalten":"") . ($Beschattung?", beschatten":""));
	readingsEndUpdate($hash,1);	
		
}

1;

