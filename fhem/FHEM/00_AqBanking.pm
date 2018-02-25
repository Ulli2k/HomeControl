
# $Id: 00_AqBanking.pm 2015-08-03 20:10:00Z Ulli $
package main;

use strict;
use warnings;
use Time::HiRes qw(gettimeofday);
use JSON qw( decode_json ); #read/write DBFiles

sub AqBanking_GetUpdateTimer($);

my %sets = (
  "update" 								=> "noArg",
  "deleteAccountReadings"	=> "noArg",
);

my %gets = (    # Name, Data to send to the CUL, Regexp for the answer
  "table"      						=> "all,new",
  "data"									=> "all,new",
);

#####################################
sub
AqBanking_Initialize($)
{
  my ($hash) = @_;
	
# Normal devices
  $hash->{DefFn}        	= "AqBanking_Define";
  $hash->{UndefFn}      	= "AqBanking_Undef";
  $hash->{GetFn}        	= "AqBanking_Get";
  $hash->{SetFn}        	= "AqBanking_Set";
#  $hash->{AttrFn}       	= "AqBanking_Attr";
  $hash->{AttrList} 			= "accounts reverseTransactionList:1,0 " .
  													$readingFnAttributes;
}

#####################################

sub AqBanking_Define($$) {
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if(@a != 4 && $a[3] !~ m/^\d+$/) {
    my $msg = "wrong syntax: define <name> AqBanking <BankingDBFile inkl. Path> <Interval (sec)>";
    Log3 undef, 2, $msg;
    return $msg;
  }

  my $name = $a[0];
  my $DBFile = $a[2];
  return "Banking Database File does not exists or wrong filepath!" if(! -f $DBFile);
  
	$hash->{BankingDBFile} 	= $DBFile;
	$hash->{INTERVAL} 			= $a[3];
	
  InternalTimer(gettimeofday()+$hash->{INTERVAL}, "AqBanking_GetUpdateTimer", $hash, 0);
	
	$hash->{STATE} = "Initialized";
	
  return undef;
}

#####################################

sub AqBanking_Undef($$) {
  my ($hash, $arg) = @_;

  RemoveInternalTimer($hash);
  return undef;
}

#####################################

sub AqBanking_GetUpdateTimer($) {
  my ($hash) = @_;
  my $name = $hash->{NAME};
 
	InternalTimer(gettimeofday()+$hash->{INTERVAL}, "AqBanking_GetUpdateTimer", $hash, 1);
 	
  my %updates = AqBanking_checkBankingDBFile($hash);
  my $updateStr="";
  foreach my $key (keys %updates) { ## "A05:0 A00:1"
  	$updateStr .= "A$key:$updates{$key} ";
	}
 	$updateStr =~ s/^\s+|\s+$//g; #löscht Leerzeichen
  Log3 $name, 4, "$name: Banking Database File checked. ($updateStr)";
  
  readingsSingleUpdate($hash, "state", "$updateStr", 1);

  return 1;
}

#####################################

sub AqBanking_Set($@) {
  my ($hash, @a) = @_;
  
  return "\"set AqBanking\" needs at least one parameter" if(@a < 2);
  
  #problematic...does not work with predefined sets values!?
  #return "Unknown argument $a[1], choose one of " . join(" ", sort keys %sets)
  #	if(!defined($sets{$a[1]}));

  my $name = shift @a;
  my $cmd = shift @a;
  my $arg = join(" ", @a);
  
  my $list = join(" ", map { $sets{$_} eq "" ? $_ : "$_:$sets{$_}" } sort keys %sets);
  return $list if( $cmd eq '?' || $cmd eq '');


	###### set commands ######
  if($cmd eq "update") {
    RemoveInternalTimer($hash);
    AqBanking_GetUpdateTimer($hash);
    
  } elsif ($cmd eq "deleteAccountReadings") {
  	my @accounts = split(",",AttrVal($name, "accounts", ""));
  	my $state;
		foreach my $acc (@accounts) { ## Accounts
			$state .= "A" . $acc .":0 ";
			# Delete Readings of specific account
			foreach my $readings (keys %{$defs{$name}{READINGS}}) {
				my $readingRegEx = "^" . $acc . "_";
				if($readings =~ m/$readingRegEx/) {
					delete $defs{$name}{READINGS}{$readings};
					#Log3 $name, 1 , "$name delete $readings";
				}
			}  
  	}
  	$state =~ s/^\s+|\s+$//g; #löscht Leerzeichen
  	readingsSingleUpdate($hash, "state", "$state", 1);
  }
  return undef;
}

#####################################

sub AqBanking_Get($@) {
  my ($hash, $name, $cmd, @msg ) = @_;
  my $arg = join(" ", @msg);
  my $type = $hash->{TYPE};
  my $value;
  
  return "\"get $type\" needs at least one parameter" if(!defined($name) || $name eq "");
  if(!defined($gets{$cmd})) {
    #my @list = map { $_ =~ m/^(raw)$/ ? $_ : "$_:noArg" } sort keys %gets;
    my $list = join(" ", map { $gets{$_} eq "" ? $_ : "$_:$gets{$_}" } sort keys %gets);
    return "Unknown argument $cmd, choose one of " . $list;
  }

	if($cmd eq "table") {
		$value = AqBanking_getDataTable($hash,$arg);
	} elsif($cmd eq "data") {
		$value = AqBanking_getData($hash,$arg);
	}
 # if(defined($hash->{READINGS}{$cmd})) {
 #       $value= $hash->{READINGS}{$cmd}{VAL};
 # } else {
 #       my $rt= "";
 #       if(defined($hash->{READINGS})) {
 #               $rt= join(" ", sort keys %{$hash->{READINGS}});
 #       }   
 #       return "Unknown reading $cmd, choose one of " . $rt;
 # }	

  #return "$name $cmd => $value";
  return $value;
}

sub AqBanking_getData($$) {

  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $table;
  my $value;
  my @date;
  my $ReadingPart;
  
  my @accounts = split(",",AttrVal($name, "accounts", ""));
  my $nr;
  foreach my $acc (@accounts) { ## Accounts
  	$nr=1;
  	$ReadingPart = $acc . "_" . ($nr<10 ? '0'.$nr : $nr) ."_";
  	$value .= "Account: $acc - ". $defs{$name}{READINGS}{$acc . '_Balance'}{VAL} . "\n";  	
		while(defined($defs{$name}{READINGS}{$ReadingPart . 'State'})) {
			if(($arg eq "new" && $defs{$name}{READINGS}{$ReadingPart . 'State'}{VAL} eq "new") || $arg eq "all") {
				@date = split('/',$defs{$name}{READINGS}{$ReadingPart . 'Date'}{VAL});
				$value .= " " . uc(substr($defs{$name}{READINGS}{$ReadingPart . 'State'}{VAL},0,1)) . "\t" .
									"" . $date[2] . "." . $date[1] . "\t" .
									"" . substr($defs{$name}{READINGS}{$ReadingPart . 'Purpose'}{VAL},0,1) . "\t" .
									"" . $defs{$name}{READINGS}{$ReadingPart . 'Value'}{VAL} . "\t" .
									"" . $defs{$name}{READINGS}{$ReadingPart . 'RemoteName'}{VAL} . "\t" .
									"\n";
			}
	  	$nr++;
	  	$ReadingPart = $acc . "_" . ($nr<10 ? '0'.$nr : $nr) ."_";
  	}
	} 
	$value .= "\n"; 
	return $value;
}

sub AqBanking_getDataTable($$) {
  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $table;
  my $value;
  my @date;
  my $ReadingPart;
  
  my @accounts = split(",",AttrVal($name, "accounts", ""));
  my $nr;
  $value .= "<style> table { border-spacing: 20px 0px; }" . 
  									"th, td { padding: 0px; }" .
  					"</style>";
  $value .= "<front size=2><table>\n";
  foreach my $acc (@accounts) { ## Accounts
  	$nr=1;
  	$ReadingPart = $acc . "_" . ($nr<10 ? '0'.$nr : $nr) ."_";
  	$value .= "<tr style='font-weight:bold'><td colspan='5'>Account: $acc - ". $defs{$name}{READINGS}{$acc . '_Balance'}{VAL} . "<td></tr>\n";  	
		while(defined($defs{$name}{READINGS}{$ReadingPart . 'State'})) {
			if(($arg eq "new" && $defs{$name}{READINGS}{$ReadingPart . 'State'}{VAL} eq "new") || $arg eq "all") {
				$value .= "<tr style='color:" . ($defs{$name}{READINGS}{$ReadingPart . 'Value'}{VAL} =~ m/-/ ? "#FF0000": "#00FF00") . "'>\n";
				@date = split('/',$defs{$name}{READINGS}{$ReadingPart . 'Date'}{VAL});
				$value .= "<td>" . uc(substr($defs{$name}{READINGS}{$ReadingPart . 'State'}{VAL},0,1)) . "</td>" .
									"<td>" . $date[2] . "." . $date[1] . "</td>" .
									"<td>" . substr($defs{$name}{READINGS}{$ReadingPart . 'Purpose'}{VAL},0,1) . "</td>" .
									"<td>" . $defs{$name}{READINGS}{$ReadingPart . 'Value'}{VAL} . "</td>" .
									"<td>" . $defs{$name}{READINGS}{$ReadingPart . 'RemoteName'}{VAL} . "</td>" .
									"\n";
				$value .= "</tr>\n";						
			}
			$value .= "<tr><td colspan='5'><td></tr>\n";  	
	  	$nr++;
	  	$ReadingPart = $acc . "_" . ($nr<10 ? '0'.$nr : $nr) ."_";
  	}
	} 
	$value .= "</font></table>\n"; 
	return $value;
}

#####################################

sub AqBanking_readDBFile($) {
	my $file = shift(@_);
	my $href;
	my $JSONdata;

	return () if(!defined($file));
	open(IFIL,"<$file") || return ();
	#open(IFIL,"<:encoding(ISO-8859-1)",$file) || return ();
	$JSONdata= <IFIL>; close(IFIL);
	$href = decode_json($JSONdata);
	#$href = JSON->new->utf8->decode($JSONdata);
	
	return %$href;
}

sub date2number($) {

	my ($datestr) = $_[0];
	return "" if(!defined($datestr) || !length($datestr));
	$datestr =~ tr/\///d;
	return $datestr;
}

sub AqBanking_checkBankingDBFile($) {
	my ($hash) = @_;
	my $name = $hash->{NAME};
	
	my $DBFile = $hash->{BankingDBFile};
	my @accounts = split(",",AttrVal($name, "accounts", ""));
	
	if(!$#accounts) {
		Log3 $name, 3, "$name: No Accounts defined.";
		return 0;
	}
	
	my %DBhash = AqBanking_readDBFile($DBFile);
	if(!%DBhash) {
		Log3 $name, 1, "$name: Can not read DB Banking File!";
		return 0;
	}
	
	# Update Readings
	my %updates;
	readingsBeginUpdate($hash);
	foreach my $acc (@accounts) { ## Accounts
		my $nr=0;
		my $ReadingNamePart;
		last if(!defined($DBhash{$acc}));
		$updates{$acc}=0;

		# Delete Readings
		#foreach my $readings (keys $defs{$name}{READINGS}) {
		#	my $readingRegEx = "^" . $acc . "_";
		#	if($readings =~ m/$readingRegEx/) {
		#		delete $defs{$name}{READINGS}{$readings};
		#		#Log3 $name, 1 , "$name delete $readings";
		#	}
		#}

		#set Balance Readings
		$ReadingNamePart = $acc . "_";
		readingsBulkUpdate($hash, $ReadingNamePart . "Balance", $DBhash{$acc}{'Balance'} . " " . $DBhash{$acc}{'Balance_currency'});
		
		#Reverse Transaction List (Last on top)
		my @sorted;
		if(AttrVal($name, "reverseTransactionList", "0") eq "1" && defined($DBhash{$acc}{'Transactions'}) && @{$DBhash{$acc}{'Transactions'}}) {
			@sorted =  sort { date2number($b->{'date'}) <=> date2number($a->{'date'}) } @{$DBhash{$acc}{'Transactions'}};
			delete $DBhash{$acc}{'Transactions'};
			push(@{$DBhash{$acc}{'Transactions'}}, @sorted);
		}		

		#set Transaction Readings
		foreach my $trans (@{ $DBhash{$acc}{'Transactions'} }) {  ## Transaction
			$nr++;
			$updates{$acc}++ if($trans->{'state'} eq "new");
			
			$ReadingNamePart = $acc . "_" . ($nr<10 ? "0".$nr : $nr) . "_";
			readingsBulkUpdate($hash, $ReadingNamePart . "State", $trans->{'state'});
			readingsBulkUpdate($hash, $ReadingNamePart . "Date", $trans->{'date'});
			readingsBulkUpdate($hash, $ReadingNamePart . "Value", $trans->{'value_value'} . " " . $trans->{'value_currency'});
			readingsBulkUpdate($hash, $ReadingNamePart . "RemoteName", $trans->{'remoteName'});
			readingsBulkUpdate($hash, $ReadingNamePart . "Purpose", $trans->{'purpose'});

			Log3 $name, 4, "$name: Transaction - [$acc] $trans->{'state'} $trans->{'date'} $trans->{'value_value'}$trans->{'value_currency'} $trans->{'remoteName'} $trans->{'purpose'}";
		}

		# Delete Readings with higher Index which are available from last checkBankingDBFile 
		$nr++;
		$ReadingNamePart = $acc . "_" . ($nr<10 ? "0".$nr : $nr) . "_";
		while(defined($defs{$name}{READINGS}{$ReadingNamePart . "State"})) {
			delete $defs{$name}{READINGS}{$ReadingNamePart . "State"}; 
			delete $defs{$name}{READINGS}{$ReadingNamePart . "Date"}; 
			delete $defs{$name}{READINGS}{$ReadingNamePart . "Value"}; 
			delete $defs{$name}{READINGS}{$ReadingNamePart . "RemoteName"}; 
			delete $defs{$name}{READINGS}{$ReadingNamePart . "Purpose"}; 
			$nr++;
			$ReadingNamePart = $acc . "_" . ($nr<10 ? "0".$nr : $nr) . "_";
		}
	}	
	readingsEndUpdate($hash, 1);
	
	return %updates;
}

1;

=pod
=begin html



=end html
=cut
