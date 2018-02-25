
# $Id: 36_HX2272.pm 3889 2013-09-10 10:40:15Z ulli $
#
# TODO:

package main;

use strict;
use warnings;
use SetExtensions;

my $matchStrHX2272 = "^H[F01]{12}\$";  #O0112D1 // 05F32 0FF0F0FFFF0F

my %HX2272_convert2Device = ("0FFFF" => "A", "F0FFF" => "B", "FF0FF" => "C", "FFF0F" => "D", "FFFF0" => "E" );
my %HX2272_revconvert2Device = reverse %HX2272_convert2Device;

my %sets = (
  "on" => "noArg",
  "off" => "noArg",
  "toggle" => "noArg",
  "on-for-timer" => "",
);

#####################################
sub HX2272_Initialize($) {
  my ($hash) = @_;

  $hash->{Match}    	 		= $matchStrHX2272;
  $hash->{SetFn}     			= "HX2272_Set";
  #$hash->{GetFn}     		= "HX2272_Get";
  $hash->{DefFn}     			= "HX2272_Define";
  $hash->{UndefFn}   			= "HX2272_Undef";
  #$hash->{FingerprintFn}  = "HX2272_Fingerprint";
  $hash->{ParseFn}   			= "HX2272_Parse";
  $hash->{AttrFn}    			= "HX2272_Attr";
  $hash->{AttrList}  			= "IODev "
                       			." $readingFnAttributes";
}

#####################################
sub HX2272_Define($$) {	#<Protocol> <Device>

  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

	#Check Parameter count
  if(@a != 4 ) {
    my $msg = "wrong syntax: define <name> HX2272 <address> <device>";
    Log3 undef, 2, $msg;
    return $msg;
  }

	#Check valid Parameter 
#  $a[2] =~ m/^([\d]{5})$/i; #address 0FF0F 0FFFF 0F
  return "$a[2] is not a valid HX2272 address" if( $a[2] !~ m/^[0F]{5}$/ );
  return "$a[3] is not a valid HX2272 device" if( $a[3] !~ m/^[A-F]$/ );
  
  my $name = $a[0];
  my $addr = $a[2];
  my $device = $a[3];

  #return "$addr is not a 1 byte hex value" if( $addr !~ /^[\da-f]{2}$/i );
  #return "$addr is not an allowed address" if( $addr eq "00" );

	#Check if Device already available with defined address/parameter
  return "HX2272 device $addr already used for $modules{HX2272}{defptr}{$addr}->{NAME}." if( $modules{HX2272}{defptr}{$addr}
                                                                                             && $modules{HX2272}{defptr}{$addr}->{NAME} ne $name );
	return "HX2272 device $a[3] not vailid!" if($device !~ m/[ABCDE]/);
		
  $hash->{addr} = $addr;
  $hash->{device} = $device;
	my $defName = $addr . " " . $device;
	
  $modules{HX2272}{defptr}{$defName} = $hash;

  AssignIoPort($hash);
  if(defined($hash->{IODev}->{NAME})) {
    #$hash->{ModuleID} = $hash->{IODev}->{ModuleID};
    #$hash->{DeviceID} = $hash->{IODev}->{DeviceID};
    #Log3 $name, 3, "$name: I/O device is " . $hash->{IODev}->{NAME} . " (" . $hash->{DeviceID} . ") with Module (" . $hash->{ModuleID} .").";
		Log3 $name, 3, "$name: I/O device is " . $hash->{IODev}->{NAME};
  } else {
    Log3 $name, 1, "$name: no I/O device and Module-ID";
  }


  return undef;
}

#####################################
sub HX2272_Undef($$) {

  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $addr = $hash->{addr};

  delete( $modules{HX2272}{defptr}{$addr} );

  return undef;
}

#####################################
sub HX2272_Parse($$) {

  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};

  my( $addr, $device, $flag, $defName ); #@bytes  "H 0FF0F 0FFFF 0F"
		$addr = substr($msg,1, 5);
		$device = $HX2272_convert2Device{substr($msg, 6, 5)};
		$flag = substr($msg, 11, 2);
		
		#Log3 $name, 1, "$name: Parse ($addr," . substr($msg, 10, 5) . ",$device,$flag)";
		
		if(!defined($device) || $device !~ m/[ABCDE]/ || $flag !~ m/(0F|F0)/) {
			Log3 $name, 4, "$name: Command not valid. ($addr," . substr($msg, 6, 5) . ",$flag)";
			return;
		}
		$defName = $addr . " " . $device;		
#  } else {
#    DoTrigger($name, "UNKNOWNCODE $msg");
#    Log3 $name, 3, "$name: Unknown code $msg, help me!";
#    return undef;
#  }

  my $rhash = $modules{HX2272}{defptr}{$defName};
  my $rname = $rhash?$rhash->{NAME}:$defName;

   if( !$modules{HX2272}{defptr}{$defName} ) {
     Log3 $name, 3, "HX2272 Unknown device $defName, please define it";

     return "UNDEFINED HX2272_$rname HX2272 $defName";
   }

  my @list;
  push(@list, $rname);

  $rhash->{HX2272_lastRcv} = TimeNow();

  readingsBeginUpdate($rhash);
  #readingsBulkUpdate($rhash, "device", $device);
  my $condition = ($flag eq "F0" ? "off" : "on");
  #readingsBulkUpdate($rhash, "condition", $condition);
  #my $state = "$device $condition";
  readingsBulkUpdate($rhash, "state", $condition); # if( Value($rname) ne $condition ); ##<- Verhindert event-on-update-reading
  readingsEndUpdate($rhash,1);

  return @list;
}

#####################################
sub HX2272_Set($@) {
  my ($hash, @msg) = @_;

	my $name = shift @msg;
	my $cmd = shift @msg;
	my $arg = join(" ", @msg);
	my $state;

	my $list = join(" ", map { $sets{$_} eq "" ? $_ : "$_:$sets{$_}" } sort keys %sets);
	#my $list = join(" ", map { "$_:$sets{$_}" } keys %sets) if( $cmd eq '?' );
	#return join(" ", map { "$_:$sets{$_}" } keys %sets) if( $cmd eq '?' );  #return valid sets and arguments
	return $list if( $cmd eq '?' || $cmd eq '');
	return "Unknown argument $cmd, choose one of " . join(", ", sort keys %sets) if(!defined($sets{$cmd}));
	#return "Unknown argument $cmd, choose one of ($list)" if($cmd !~ m/^(on|off|toggle|on-for-timer)$/i);

  if ($cmd =~ m/^on-for-timer/i) {
    return "Unknown argument $cmd, choose [0-9]" if($arg !~ m/^[0-9]+$/i);

    #remove timer if there is one active
    if($modules{HX2272}{ldata}{$name}) {
	    CommandDelete(undef, $name . "_timer");
  	  delete $modules{HX2272}{ldata}{$name};
    }

		$state = "0F"; #turn device on
		
    my $to = sprintf("%02d:%02d:%02d", $arg/3600, ($arg%3600)/60, $arg%60);
    $modules{HX2272}{ldata}{$name} = $to;
    Log3 $name, 4, "Follow: +$to on-for-timer $name off";
    CommandDefine(undef, $name."_timer at +$to {fhem(\"set $name off\")}");
    
  } elsif($cmd =~ m/^on$/i) {
  	$state = "0F";
  } elsif($cmd =~ m/^off$/i) {
  	$state = "F0";
  } elsif($cmd =~ m/^toggle$/i) {
  	$state = ($hash->{STATE} =~ m/^off$/i ? "0F" : "F0");
  }
  
  Log3 $name, 4, "set $name $cmd";
  IOWrite($hash, "H" . $hash->{addr} . $HX2272_revconvert2Device{$hash->{device}} . $state );
	readingsSingleUpdate($hash, "state", ($state eq "0F" ? "on" : "off"), 1);
	
  return undef;
}


sub HX2272_Attr(@) {

	my ($cmd,$name,$aName,$aVal) = @_;
  my $hash = $defs{$name};

  if($aName =~ m/^IODev/i) {

    #$hash->{ModuleID} = $defs{$aVal}->{ModuleID} if(defined($defs{$aVal}->{ModuleID}));
    #$hash->{DeviceID} = $defs{$aVal}->{DeviceID} if(defined($defs{$aVal}->{DeviceID}));
    #Log3 $name, 3, "$name: I/O device is " . $defs{$aVal}->{NAME} . " (" . $hash->{DeviceID} . ") with Module (" . $hash->{ModuleID} .").";
    Log3 $name, 3, "$name: I/O device is " . $defs{$aVal}->{NAME};
  }
  
  return undef;
} 

#####################################
#sub HX2272_Get($@) {
#
#  my ($hash, $name, $cmd, @args) = @_;

#  return "\"get $name\" needs at least one parameter" if(@_ < 3);
#
#  my $list = "";
#
#  return "Unknown argument $cmd, choose one of $list";
#}

#####################################
#sub HX2272_Attr(@) {
#
#  my ($cmd, $name, $attrName, $attrVal) = @_;
#
#  return undef;
#}

1;

=pod
=begin html

<a name="HX2272"></a>
<h3>HX2272</h3>
<ul>

  <tr><td>
  The HX2272 is a RF controlled AC mains plug with integrated power meter functionality from ELV.<br><br>

  It can be integrated in to FHEM via a <a href="#JeeLink">JeeLink</a> as the IODevice.<br><br>

  The JeeNode sketch required for this module can be found in .../contrib/36_HX2272-pcaSerial.zip.<br><br>

  <a name="HX2272Define"></a>
  <b>Define</b>
  <ul>
    <code>define &lt;name&gt; HX2272 &lt;addr&gt; &lt;channel&gt;</code> <br>
    <br>
    addr is a 6 digit hex number to identify the HX2272 device.
    channel is a 2 digit hex number to identify the HX2272 device.<br><br>
    Note: devices are autocreated on reception of the first message.<br>
  </ul>
  <br>

  <a name="HX2272_Set"></a>
  <b>Set</b>
  <ul>
    <li>on</li>
    <li>off</li>
    <li>identify<br>
      Blink the status led for ~5 seconds.</li>
    <li>reset<br>
      Reset consumption counters</li>
    <li>statusRequest<br>
      Request device status update.</li>
    <li><a href="#setExtensions"> set extensions</a> are supported.</li>
  </ul><br>

  <a name="HX2272_Get"></a>
  <b>Get</b>
  <ul>
  </ul><br>

  <a name="HX2272_Readings"></a>
  <b>Readings</b>
  <ul>
    <li>power</li>
    <li>consumption</li>
    <li>consumptionTotal<br>
      will be created as a default user reading to have a continous consumption value that is not influenced
      by the regualar reset or overflow of the normal consumption reading</li>
  </ul><br>

  <a name="HX2272_Attr"></a>
  <b>Attributes</b>
  <ul>
    <li>readonly<br>
    if set to a value != 0 all switching commands (on, off, toggle, ...) will be disabled.</li>
    <li>forceOn<br>
    try to switch on the device whenever an off status is received.</li>
  </ul><br>
</ul>

=end html
=cut
