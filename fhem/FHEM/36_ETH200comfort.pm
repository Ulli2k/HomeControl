
# $Id: 36_ETH200comfort.pm 3889 2013-09-10 10:40:15Z ulli $
#
# TODO:

package main;

use strict;
use warnings;
use SetExtensions;

my $matchStrETH200Comfort = "^E[0-9A-F]{8}\$"; # E0A01004300

#####################################
sub ETH200comfort_Initialize($) {
  my ($hash) = @_;

  $hash->{Match}    	 		= $matchStrETH200Comfort;
  $hash->{SetFn}     			= "ETH200comfort_Set";
  #$hash->{GetFn}     		= "ETH200comfort_Get";
  $hash->{DefFn}     			= "ETH200comfort_Define";
  $hash->{UndefFn}   			= "ETH200comfort_Undef";
  #$hash->{FingerprintFn}  = "ETH200comfort_Fingerprint";
  $hash->{ParseFn}   			= "ETH200comfort_Parse";
  $hash->{AttrFn}    			= "ETH200comfort_Attr";
  $hash->{AttrList}  			= "IODev DayValue NightValue "
                       			." $readingFnAttributes";
}

#####################################
sub ETH200comfort_Define($$) {	#<Protocol> <Device>

  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

	#Check Parameter count
  if(@a != 3 ) {
    my $msg = "wrong syntax: define <name> ETH200comfort <address>";
    Log3 undef, 2, $msg;
    return $msg;
  }

	#Check valid Parameter 
  $a[2] =~ m/^([\d]{4})$/i; #address
  return "$a[2] is not a valid ETH200comfort address" if( !defined($1) );
  
  my $name = $a[0];
  my $addr = $a[2];

	#Check if Device already available with defined address/parameter
  return "ETH200comfort device $addr already used for $modules{ETH200comfort}{defptr}{$addr}->{NAME}." if( $modules{ETH200comfort}{defptr}{$addr}
                                                                                             && $modules{ETH200comfort}{defptr}{$addr}->{NAME} ne $name );

  $hash->{addr} = $addr;
  $modules{ETH200comfort}{defptr}{$addr} = $hash;

  AssignIoPort($hash);
  if(defined($hash->{IODev}->{NAME})) {
    Log3 $name, 3, "$name: I/O device is " . $hash->{IODev}->{NAME};
  } else {
    Log3 $name, 1, "$name: no I/O device.";
  }
	
  return undef;
}

#####################################
sub ETH200comfort_Undef($$) {

  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $addr = $hash->{addr};

  delete( $modules{ETH200comfort}{defptr}{$addr} );

  return undef;
}

#####################################
# * f<Protocol> s <Address> <Value>                [RFM868-FSK ETH-SetTemp]
# * f<Protocol> l <Address>                        [RFM868-FSK ETH-Learn]
# * f<Protocol> o                                  [RFM868-FSK ETH-MinTemp]
#* f<Protocol> c <Cycles>                         [RFM868-FSK ETH-SendCycles]
sub ETH200comfort_Set($@) {
  my ($hash, @msg) = @_;

  my $name = shift @msg;
  my $cmd = shift @msg;
  my $arg = join("",@msg);

  my $list = "setTemp:slider,5,0.5,29.5 learn setMode:day,night,off";
  return $list if( $cmd eq '?' );

  if($cmd eq "setTemp") {
	  return "Unknown value of $cmd, choose a value between 5 and 29.5." if($arg < 5 || $arg > 29.5);

    Log3 $name, 4, "set $name $cmd $arg";
    my $steps = ($arg-5)/0.5;
    IOWrite($hash, "E" . $hash->{addr} . "40CB");
    IOWrite($hash, "E" . $hash->{addr} . "40" . sprintf("%x",$steps));

  } elsif($cmd eq "setMode") {
  	 return "Unknown value of $cmd, choose day or night. $arg" if($arg !~ m/(day|night|off)/i);
  	 
  	 Log3 $name, 4, "set $name $cmd $arg";
  	 IOWrite($hash, "E" . $hash->{addr} . "4200") if($arg =~ m/day/i);
  	 IOWrite($hash, "E" . $hash->{addr} . "4300") if($arg =~ m/night/i);
		 IOWrite($hash, "E" . $hash->{addr} . "40CB") if($arg =~ m/off/i);   #CB works better than CA 
  
	} elsif($cmd eq "learn") {
		#F020A01004000
		Log3 $name, 4, "set $name $cmd $arg";
    IOWrite($hash, "E" . $hash->{addr} . "4000");
  	
#	} elsif($cmd eq "SetCycles") {
#		return "Unknown value of $cmd, only integer values allowed." if(!($arg =~ m/^[\d]+$/));
#		Log3 $name, 4, "set $name $cmd $arg";
#		
#    IOWrite($hash, "E" . "0C" . $arg);
            
  } else {
    return "Unknown argument $cmd, choose one of ".$list;
  }

  return undef;
}

sub ETH200comfort_Parse($$) { 

  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};
  
  my( $device, $command, $value );

	$device = substr($msg, 1, 4);
	$command = substr($msg, 5, 2);
	$value = substr($msg, 7, 2);

	my $defName = uc($device);
  my $rhash = $modules{ETH200comfort}{defptr}{$defName};
  my $rname = $rhash?$rhash->{NAME}:$defName;

  if( !$modules{ETH200comfort}{defptr}{$defName} ) {
		Log3 $name, 3, "ETH200comfort Unknown device $rname, please define it";
    return "UNDEFINED ETH200comfort_$rname ETH200comfort $defName";
  }
   
	my @list;
  push(@list, $rname);

	my($mode, $state, $temperature) = 0;
	if($command == 42 && $value == 0) { #day
		$mode = "day";
		$temperature = AttrVal($rname, "DayValue", "DayValue");
		$state = "T: " . $temperature;
		
	} elsif($command == 43 && $value == 0) { #night
		$mode = "night";
		$temperature =  AttrVal($rname, "NightValue", "NightValue");
		$state = "T: " . $temperature;
	
	} elsif($command == 40 && $value eq "CB") { #off
		$mode = "off";
		$temperature = 5;
		$state = "T: " . $temperature;
		
#	} elsif($command == 40 && $value == 0) { #learn
#		$mode = "learned";
#		$temperature = "-";
#		$state = "learned";		
	
	} elsif($command == 40) { #set temp value
		$mode = "setTemp";
		$temperature = hex($value)*0.5+5;
		$state = "T: " . $temperature;

	} else {
		return undef;
	} 
	
	$rhash->{ETH200comfort} = TimeNow();
	readingsBeginUpdate($rhash);
	readingsBulkUpdate($rhash, "setMode", $mode);
	readingsBulkUpdate($rhash, "setTemp", $temperature);
	readingsBulkUpdate($rhash, "state", $state) if( Value($rname) ne $state );
	readingsEndUpdate($rhash,1);	

  return @list;
}

sub ETH200comfort_Attr(@) {

	my ($cmd,$name,$aName,$aVal) = @_;
  my $hash = $defs{$name};

#  if($aName =~ m/^IODev/i) {

    #$hash->{ModuleID} = $defs{$aVal}->{ModuleID} if(defined($defs{$aVal}->{ModuleID}));
    #$hash->{DeviceID} = $defs{$aVal}->{DeviceID} if(defined($defs{$aVal}->{DeviceID}));
#    Log3 $name, 3, "$name: I/O device is " . $defs{$aVal}->{NAME};
#  }
  
  return undef;
} 
  
#####################################
#sub ETH200comfort_Get($@) {
#
#  my ($hash, $name, $cmd, @args) = @_;

#  return "\"get $name\" needs at least one parameter" if(@_ < 3);
#
#  my $list = "";
#
#  return "Unknown argument $cmd, choose one of $list";
#}

#####################################
#sub ETH200comfort_Fingerprint($$) {
#
#  my ($name, $msg) = @_;
#
#  return ( "", $msg );
#}

1;

