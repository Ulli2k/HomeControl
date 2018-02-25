
# $Id: 36_myPowerNode.pm 3889 2013-09-10 10:40:15Z ulli $
#
# TODO:

package main;

use strict;
use warnings;
use SetExtensions;

my $matchStrmyPowerNode = "^P([1-6][0-9A-F]{16}|p)\$"; # P6 000259D800 0001A6
																											 # P6 014655F000 01180B			

my %sets = (
  "refresh" => "noArg",
  "readData" => "noArg",
  "deleteReadings" => "noArg",
);

#####################################
sub myPowerNode_Initialize($) {
  my ($hash) = @_;

  $hash->{Match}    	 		= $matchStrmyPowerNode;
  $hash->{SetFn}     			= "myPowerNode_Set";
  #$hash->{GetFn}     		= "myPowerNode_Get";
  $hash->{DefFn}     			= "myPowerNode_Define";
  $hash->{UndefFn}   			= "myPowerNode_Undef";
  #$hash->{FingerprintFn}  = "myPowerNode_Fingerprint";
  $hash->{ParseFn}   			= "myPowerNode_Parse";
  $hash->{AttrFn}    			= "myPowerNode_Attr";
  $hash->{AttrList}  			= " CurrentOnly:1,0"
  													." $readingFnAttributes";
  
}

#####################################
sub myPowerNode_Define($$) {	#<Protocol> <Device>

  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

	#Check Parameter count
  if(@a != 3) {
    my $msg = "wrong syntax: define <name> myPowerNode <RadioNetwork-Device>";
    Log3 undef, 2, $msg;
    return $msg;
  }

	my $name = $a[0];
	my $dev = $a[2];
	
	#Check if Device already available with defined name
  return "myPowerNode device $name already used for $dev. (Just one myPowerNode is allowed)" if($modules{myPowerNode}{defptr}{$dev});
	$modules{myPowerNode}{defptr}{$dev} = $hash;
  
  AssignIoPort($hash,$dev);
  if(defined($hash->{IODev}->{NAME})) {
    Log3 $name, 3, "$name: I/O device is " . $hash->{IODev}->{NAME};
  } else {
    Log3 $name, 1, "$name: no I/O device.";
  }
	
	readingsSingleUpdate($hash, "state", "none", 1);
	
  return undef;
}

#####################################
sub myPowerNode_Undef($$) {

  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $dev = $hash->{IODev}->{NAME};

	delete( $modules{myPowerNode}{defptr}{$dev} );
	
  return undef;
}

#####################################
sub myPowerNode_Set($@) {
  my ($hash, @msg) = @_;

  my $name = shift @msg;
  my $cmd = shift @msg;
  my $arg = join("",@msg);

  my $list = join(" ", map { $sets{$_} eq "" ? $_ : "$_:$sets{$_}" } sort keys %sets);
	return $list if( $cmd eq '?' || $cmd eq '');
	return "Unknown argument $cmd, choose one of " . join(", ", sort keys %sets) if(!defined($sets{$cmd}));  

	Log3 $name, 4, "set $name $cmd";

  if ($cmd =~ m/^refresh/i) {
	  IOWrite($hash, "P");
	
	} elsif ($cmd =~ m/^readData/i) {
	  IOWrite($hash, "Pc");
	
	} elsif ($cmd =~ m/^deleteReadings/i) {
		# Delete CT Readings
		foreach my $readings (keys %{$defs{$name}{READINGS}}) {		
			next if($readings !~ m/^(CT|Pulse)/i);
			delete $defs{$name}{READINGS}{$readings};
		}
		readingsSingleUpdate($hash, "state", "none", 1);
		
  } else {
    return "Unknown argument $cmd, choose one of ".$list;
  }

  return undef;
}

sub myPowerNode_Hex2LongInt32($) {
    my ($hexstr) = @_;
    my $num = hex($hexstr);
    return $num >> 31 ? $num - 2 ** 32 : $num;
}

sub myPowerNode_Parse($$) {
	
  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};
  
  my( $CT, $I, $V, $P, $F, $pulse, $CurrentOnly ) = 0;
	
	my $defName = $name;
	my $rhash = $modules{myPowerNode}{defptr}{$defName};
	$name = $rhash->{NAME};

	$pulse = ReadingsVal($name, "Pulse",0);	
	$CurrentOnly = AttrVal($name, "CurrentOnly", "0");

	readingsBeginUpdate($rhash);
	
	if(substr($msg,1,1) eq "p") { #Pulse	
		$pulse += 1;
		readingsBulkUpdate($rhash, "Pulse", $pulse);

	} else {
		#02P 6 0002 59D8 00000199
		$CT = hex(substr($msg, 1, 1));
		$I = (myPowerNode_Hex2LongInt32(substr($msg, 2, 4)))/100;		#long int
		readingsBulkUpdate($rhash, "CT" . $CT. "_I", $I);
		
		if(!$CurrentOnly) {
			$V = (myPowerNode_Hex2LongInt32(substr($msg, 6, 4)))/100;		#long int
			$P = (myPowerNode_Hex2LongInt32(substr($msg, 10, 8)))/100;	#long int
			$F = (myPowerNode_Hex2LongInt32(substr($msg, 18, 8)))/100;	#long int
	
			readingsBulkUpdate($rhash, "CT" . $CT. "_V", $V);
			readingsBulkUpdate($rhash, "CT" . $CT. "_P", $P);
			readingsBulkUpdate($rhash, "CT" . $CT. "_F", $F);
		}
	}

	#create state text
	my $state;
	my $var;
	for(my $i=0; $i<=6; $i++) {
		if(!$CurrentOnly) {
			$var = ReadingsVal($name, "CT" . $i . "_P",undef);
			$state .= "$i: " . int($var+1) . "W " if(defined($var));
		} else {
			$var = ReadingsVal($name, "CT" . $i . "_I",undef);
			$state .= "$i: " . $var . "A " if(defined($var));		
		}
	}
	$state .= "Pulse: $pulse ";
	chop($state);
	
	readingsBulkUpdate($rhash, "state", $state);
	readingsEndUpdate($rhash,1);	

	my @list;
	push(@list, $rhash->{NAME});	
  return @list;
}

sub myPowerNode_Attr(@) {

	my ($cmd,$name,$aName,$aVal) = @_;
  my $hash = $defs{$name};

  if($aName =~ m/^CurrentOnly/i) {

	}
  return undef;
}
  
#####################################
#sub myPowerNode_Get($@) {
#
#  my ($hash, $name, $cmd, @args) = @_;

#  return "\"get $name\" needs at least one parameter" if(@_ < 3);
#
#  my $list = "";
#
#  return "Unknown argument $cmd, choose one of $list";
#}

#####################################
#sub myPowerNode_Fingerprint($$) {
#
#  my ($name, $msg) = @_;
#
#  return ( "", $msg );
#}

1;

