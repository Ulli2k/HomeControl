
# $Id: 36_myBME.pm 3889 2013-09-10 10:40:15Z ulli $
#
# TODO:

package main;

use strict;
use warnings;
use SetExtensions;

my $matchStrmyBME = "^E[0-9]{4}t[0-9]{4}h\$"; # E2277t5400h

my %sets = (
  "readData" => "noArg",
);

#####################################
sub myBME_Initialize($) {
  my ($hash) = @_;

  $hash->{Match}    	 		= $matchStrmyBME;
  $hash->{SetFn}     			= "myBME_Set";
  #$hash->{GetFn}     		= "myBME_Get";
  $hash->{DefFn}     			= "myBME_Define";
  $hash->{UndefFn}   			= "myBME_Undef";
  #$hash->{FingerprintFn}  = "myBME_Fingerprint";
  $hash->{ParseFn}   			= "myBME_Parse";
  #$hash->{AttrFn}    			= "myBME_Attr";
  $hash->{AttrList}  			= $readingFnAttributes;
  
}

#####################################
sub myBME_Define($$) {	#<Device>

  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

	#Check Parameter count
  if(@a != 3) {
    my $msg = "wrong syntax: define <name> myBME <RadioNetwork-Device>";
    Log3 undef, 2, $msg;
    return $msg;
  }

	my $name = $a[0];
	my $dev = $a[2];
	
	#Check if Device already available with defined name
  return "myBME device $name already used for $dev. (Just one myBME is allowed)" if($modules{myBME}{defptr}{$dev});
	$modules{myBME}{defptr}{$dev} = $hash;
  
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
sub myBME_Undef($$) {

  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $dev = $hash->{IODev}->{NAME};

	delete( $modules{myBME}{defptr}{$dev} );
	
  return undef;
}

#####################################
sub myBME_Set($@) {
  my ($hash, @msg) = @_;

  my $name = shift @msg;
  my $cmd = shift @msg;
  my $arg = join("",@msg);

  my $list = join(" ", map { $sets{$_} eq "" ? $_ : "$_:$sets{$_}" } sort keys %sets);
	return $list if( $cmd eq '?' || $cmd eq '');
	return "Unknown argument $cmd, choose one of " . join(", ", sort keys %sets) if(!defined($sets{$cmd}));  

	Log3 $name, 4, "set $name $cmd";

	if ($cmd =~ m/^readData/i) {
	  IOWrite($hash, "Er");
			
  } else {
    return "Unknown argument $cmd, choose one of ".$list;
  }

  return undef;
}

sub myBME_Parse($$) {
	
  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};
  
  my( $T, $H ) = undef;
	
	my $defName = $name;
	my $rhash = $modules{myBME}{defptr}{$defName};
	$name = $rhash->{NAME};

	readingsBeginUpdate($rhash);
	
	#E2277t5400h
	$T = substr($msg,1,4)/100 if($msg =~ m/[0-9]{4}t/i);
	$H = substr($msg,6,4)/100 if($msg =~ m/[0-9]{4}h/i);

	readingsBulkUpdate($rhash, "temperature", $T) if($T);
	readingsBulkUpdate($rhash, "humidity", $H) if($H);

	#create state text
	my $state;
	$state .= "T: $T " if($T);
	$state .= "H: $H " if($H);
	chop($state);
	readingsBulkUpdate($rhash, "state", $state);
	
	readingsEndUpdate($rhash,1);	

	my @list;
	push(@list, $rhash->{NAME});	
  return @list;
}

#sub myBME_Attr(@) {

#	my ($cmd,$name,$aName,$aVal) = @_;
#  my $hash = $defs{$name};

#  return undef;
#}
  
#####################################
#sub myBME_Get($@) {
#
#  my ($hash, $name, $cmd, @args) = @_;

#  return "\"get $name\" needs at least one parameter" if(@_ < 3);
#
#  my $list = "";
#
#  return "Unknown argument $cmd, choose one of $list";
#}

#####################################
#sub myBME_Fingerprint($$) {
#
#  my ($name, $msg) = @_;
#
#  return ( "", $msg );
#}

1;

