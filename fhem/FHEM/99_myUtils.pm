
##############################################
# $Id: 99_Utils.pm 6660 2014-10-03 06:35:43Z rudolfkoenig $

# !! Notwendige CPAN Module: sudo cpan install Net::Telnet DateTime JSON 

package main;

use strict;
use warnings;
use POSIX;
use Time::Local 'timelocal';

sub getEMailPassword() {
	#eMail: "Ulli.Dahoam@gmail.com"
	#return "dimBBB85";
	return "didPf4Dh";
}

sub getGoogleBrowserKey() {
	#http://www.blog-gedanken.de/smarthome-2/smarthome-mit-fhem-fahrzeiten-mit-verkehr-mittels-google-maps-api-anzeigen/
	return "AIzaSyDTAXLLeKbwreZLRRzdlMxO1gznsdKoNj8";
}

sub getTelegramBotKey() {
	return"138333052:AAENblO45u3aXMRG6hh1CtMKMBFqx2vEIsM";
}

##########################################################
# Backup to DiskStation
#
sub doBackupHome($) {
	
	my ($DevName) = @_;
	my $hash = $defs{$DevName};
	
	my $shell = `/bin/date "+%Y-%m-%d_%H-%M-%S"`;
	chop($shell);

	#Trigger the Backup Script
	my $triggerFile = "/home/store/.trigger";
	open(File, '>', $triggerFile);
	print $shell;
	close(File);

	#Wait till backup is done
	$hash->{helper}{RUNNING_PID} = BlockingCall("doBackupHome_waiting", "$DevName|$triggerFile", "doBackupHome_done", 0, 0, 0);	
	return;
}

sub doBackupHome_waiting($) {
	my ($string) = @_;
	my ($DevName, $triggerFile) = split("\\|", $string,2);
	my $hash = $defs{$DevName};
	
	my $done = "/usr/bin/inotifywait --event close_write -q $triggerFile";
	my $shell_done = `$done`;
	my $exit_value  = $? >> 8;
	if($exit_value) {
  	Log 1, "doBackupHome: inotify error ($exit_value):\n   $done\n$shell_done\n";
	}
	return "$DevName|$exit_value";
}

sub doBackupHome_done($) {
	my ($string) = @_;
	my ($DevName, $ret) = split("\\|", $string,2);
	my $hash = $defs{$DevName};
	
	if(!$ret) {
		Log 3, "Backup of Nuc's home directories done.";
	} else {
		Log 1, "Backup of Nuc's home directories failed.";
	}
	
	delete($hash->{helper}{RUNNING_PID});
	fhem("delete $DevName");

	#Shutdown DiskStation passiert über DiskStation Task täglich um 3:30 Uhr 
	#fhem("set DiskStation shutdown");
}
#
##########################################################

sub checkBankingUpdate($) {
	my $name = shift;
	#my $state;
	
	#my @accounts = split(",",AttrVal($name, "accounts", ""));
	#foreach my $acc (@accounts) { ## Accounts
	#	$state .= "A" . $acc .":0 ";
	#}
	#$state =~ s/^\s+|\s+$//g; #löscht Leerzeichen
	#Log3 undef,1, "checkBankingUpdate: $state eq " . ReadingsVal($name,"state",$state);
	return if(!defined($defs{$name}));
  RemoveInternalTimer($defs{$name});
  AqBanking_GetUpdateTimer($defs{$name});	
	#return if(ReadingsVal($name,"state","A00:0") !~ m/(A[0-9]{2}:[1-99].*)+/); #any updates?

	my $text = AqBanking_getDataTable($defs{$name},"all");
	sendMail("mittermaier.ulli\@gmail.com","[HomeControl] AqBanking", $text, "");
	#my $text = AqBanking_getData($defs{$name},"all");
	#fhem("set Telegram message [HomeControl] AqBanking \n" . $text);
	#fhem("set Telegram message " . $text);
}

######## Mail versenden ############ 
# sendMail('email@@email.domain','Subject','Text','Anhang');
# http://www.fhemwiki.de/wiki/E-Mail_senden
# BugFix bei gMail über TSL
		# /usr/bin/sendEmail. In Zeile 1907 muss
		#if (! IO::Socket::SSL->start_SSL($SERVER, SSL_version => 'SSLv3 TLSv1')) {
		#in folgendes geändert werden
		#if (! IO::Socket::SSL->start_SSL($SERVER, SSL_version => 'SSLv23:!SSLv2')) {
#
sub sendMail($$$$) { 
 my $rcpt = shift;
 my $subject = shift; 
 my $text = shift;
 my $attach = shift; 
 my $ret = "";
 my $sender = "Ulli.Dahoam\@gmail.com"; 
 my $konto = "Ulli.Dahoam\@gmail.com";
 my $passwrd = getEMailPassword();;
 my $provider = "smtp.gmail.com:587";#25";
 
 $ret .= qx(sendEmail -f '$sender' -t '$rcpt' -u '$subject' -m '$text' -a '$attach' -s '$provider' -xu '$konto' -xp '$passwrd' -o message-content-type=html -o tls=auto -o message-charset=utf-8);
 $ret =~ s,[\r\n]*,,g;    # remove CR from return-string 
 Log3 undef, 4, "sendMail: RCP=$rcpt Subject=$subject Text=$text Anhang=$attach Returned=$ret";
}

##########################################################
# getSumState
# Gibt die Summe an devies zurück mit dem entsprechenden Status
#
#Abfall: BioTonne, GelbeTonne <=1  (<name>: <value>)
#Traffic_Hko, Traffic_Ost: delay, return_delay >=0 (<value>/<value>)
#Zug: plan_departure_1,plan_departure_1 >=0 (<value> - <value>)
#Fenster: offen reutrn cnt

sub getSumState($$$$) {
	my ($devFilter, $readingNames, $readingCheck, $txtMask) = @_;
	
	my $retText;
	my @devList=devspec2array($devFilter);
	my $cnt_devs=@devList;
	my @readingsArray = split(/,/,$readingNames);
	
	foreach my $name (@devList) {
		my $valid=0;
		my $txt = $txtMask;
		$txt =~ s/<name>/$name/;
		my $room = AttrVal($name,"room",undef);
		$txt =~ s/<room>/$room/ if($room);
		foreach my $r (@readingsArray) {
			$r =~ s/^\s+|\s+$//g;
			my $val = ReadingsVal($name,$r,undef);
			$val =~ s/^\s+|\s+$//g;
			($val) = $val =~ /(\d+)/;
			my $evalString = "($val $readingCheck ? 1:0)";
			if($val && eval($evalString)) {
				$txt =~ s/<value>/$val/;
				$txt =~ s/<reading>/$r/;
			}
		}
		$retText .= " | " if($retText);
		$txt =~ s/<value>/-/g;
		$retText .= $txt;
	}
	$retText = "-" if(!$retText);
	return ($cnt_devs, $retText);
}
##########################################################
# MinHumidty
# Minimal zu erreichende Luftfeuchtigkeit bei der gegebenen AUssentemperatur und Feuchtigkeit
#

sub getMinHumidity($$) {
	my ($OutsideDef,$TempDef) = @_;
	
	my $TempOutside = ReadingsVal($OutsideDef, "temperature", 999);
	my $HumOutside = ReadingsVal($OutsideDef, "humidity", 999);
	my $Temp = ReadingsVal($TempDef, "temperature", 999);
	
	return if($TempOutside == 999 || $HumOutside == 999 || $Temp == 999);
	
	my $sed = 611.2*exp(17.62*$Temp/(243.12+$Temp));
	my $sedOut = 611.2*exp(17.62*$TempOutside/(243.12+$TempOutside));
	my $pdOut = $HumOutside/100*$sedOut;
	my $minHumidity = sprintf("%00.0f",$pdOut/$sed*100);
	
	#Log 1, "getMinHumidity: $TempDef -> $minHumidity";
	readingsSingleUpdate($defs{$TempDef}, "minHumidity",$minHumidity,1);
	
	#return $minHumidity;
}


##########################################################
# MüllKalender
sub KalenderDatum($$) {
   my ($KalenderName, $KalenderUid) = @_;
   my $dt = fhem("get $KalenderName start uid=$KalenderUid 1",1);
   my $ret = time - (2*86400);  #falls kein Datum ermittelt wird Rückgabewert auf "vorgestern" -> also vergangener Termin;
 
   if ($dt and $dt ne "") {
      my @SplitDt = split(/ /,$dt);
      my @SplitDate = split(/\./,$SplitDt[0]);
      $ret = timelocal(0,0,0,$SplitDate[0],$SplitDate[1]-1,$SplitDate[2]);
   }
 
   return $ret;
}

sub MuellTermine() {

   my $t  = time;
   my $KalenderDefName = "MuellKalender";
   my $DummyMuellTermine = "MuellTermine";
   my @Tonnen = ("GelbeTonne", "Restmuell", "BioTonne", "Sperrmuell");
   my @SuchTexte = (".*Gelb.*", ".*Restmüll.vierzehntägig.*", ".*Bio.*", ".*Sperr.*");
   my $uid;
   my $dayDiff;
   my $minDiff = -1;
   my $abholung = "";
  
   for(my $i=0; $i<4; $i++) {
      $dayDiff = -1; #BUG behoben
      my @uids = split(/;/,fhem("get $KalenderDefName find $SuchTexte[$i]",1));
       
      # den nächsten Termine finden
      foreach $uid (@uids) {
         my $eventDate = KalenderDatum($KalenderDefName, $uid);
         my $dayDiffNeu = floor(($eventDate - $t) / 60 / 60 / 24 + 1);
         if ($dayDiffNeu >= 0 && ($dayDiffNeu < $dayDiff || $dayDiff == -1)) { #BUG behoben
            $dayDiff = $dayDiffNeu;
         }
      }
       
      fhem("setreading $DummyMuellTermine $Tonnen[$i] $dayDiff");
      if($dayDiff<=1) {
      	$abholung .= " | " if($abholung);
	      $abholung .= $Tonnen[$i] . ":" . $dayDiff;
	    }
      $minDiff = $dayDiff if($minDiff > $dayDiff || $minDiff==-1);
   }
   fhem("setreading $DummyMuellTermine naechsteAbholung $minDiff") if($minDiff != -1);
   $abholung = "-" if(!$abholung);
   fhem("setreading $DummyMuellTermine Abholung $abholung");
}

##########################################################
# Kodi Audio Steuerung für InternetRadio
#
sub KodiAudio(@) {
	my ($Ereignis) = @_;
	my $deviceName = "WK_KodiAudio";
	my $hash = $defs{$deviceName};
	#Log3 undef, 1, "KodiAudio: $Ereignis";
	my %Channels = (
								"egoFM"		=> ["../images/radio/egofm.png","http://www.egofm.de/stream/192kb.pls"],
								"egoFMpure"		=> ["../images/radio/egopure.png","http://www.egofm.de/rawstream/192kb.pls"],
								"egoFMflash"		=> ["../images/radio/egoflash.png","http://www.egofm.de/flashstream/192kb.pls"],
								"egoYour"		=> ["../images/radio/yourego.png","http://www.egofm.de/yourstream/192kb.pls"],
								"egoFMriff"		=> ["../images/radio/egoriff.png","http://www.egofm.de/riffstream/192kb.pls"],
								"Energy"		=> ["../images/radio/energy.png","http://energyradio.de/muenchen"],
								"Gong"		=> ["../images/radio/gong.png","http://webradio.radiogong.de/live/live.m3u"],																
							);

	if ($Ereignis =~ m/now_playing_name/) {
		my $value = (split / /,$Ereignis,2)[1];
		if(!defined($Channels{$value})) {
			Log3 undef, 1, "KodiAudio: Channel \"$value\" ist nicht definiert.";
			return;
		}
#  	fhem("setreading $deviceName now_playing_name $value; setreading $deviceName now_playing_img $Channels{$value}[0]; setreading $deviceName now_playing_url $Channels{$value}[1]; setstate $deviceName setPLAY");
		readingsBeginUpdate($hash);
		readingsBulkUpdate($hash,"now_playing_name",$value);
		readingsBulkUpdate($hash,"now_playing_img",$Channels{$value}[0]);
		readingsBulkUpdate($hash,"now_playing_url",$Channels{$value}[1]);
		readingsBulkUpdate($hash,"state","setPLAY");
		readingsEndUpdate($hash, 1);

  	$Ereignis="setPLAY";

	} elsif ($Ereignis =~ m/next/) {
		
		my $curName = ReadingsVal($deviceName,'now_playing_name','');
		return if($curName eq '');
		
		my $found=0;
		my $value = '';
		foreach my $key (keys %Channels) {
			$value=$key if($value eq '');
			if($found==1) {
				$value=$key;
				last;
			}			
			$found=1 if($key eq $curName);
		}
#    fhem("setreading $deviceName now_playing_name $value; setreading $deviceName now_playing_img $Channels{$value}[0]; setreading $deviceName now_playing_url $Channels{$value}[1]; setstate $deviceName setPLAY");
		readingsBeginUpdate($hash);
		readingsBulkUpdate($hash,"now_playing_name",$value);
		readingsBulkUpdate($hash,"now_playing_img",$Channels{$value}[0]);
		readingsBulkUpdate($hash,"now_playing_url",$Channels{$value}[1]);
		readingsBulkUpdate($hash,"state","setPLAY");
		readingsEndUpdate($hash, 1);

  	$Ereignis="setPLAY";
	}
	
	if ($Ereignis eq "setPLAY") {
    Log3 undef, 4, "KodiAudio: " . ReadingsVal($deviceName,'now_playing_name','') . " <" . ReadingsVal($deviceName,'now_playing_url','') . ">";			
		fhem("set Kodi open ".ReadingsVal($deviceName,'now_playing_url',''));
		
	} elsif ($Ereignis eq "setSTOP") {
		fhem("set Kodi stop");
	}
}

##########################################################
# Download Proplanta Icons - Wetter
# einmalig in fhem ausführen "{ppicondl()}"
#
sub icondl {
	my $dllink = shift;
	my $reticon = "";
	my $subicon = "";
	$reticon .= qx(wget -T 5 -N --directory-prefix=/home/fhem/fhem/www/images/weather/ --user-agent='Mozilla/5.0 Firefox/4.0.1' '$dllink');
	$subicon = substr $dllink,51,-4;
	return $subicon;
}

# alle Proplanta Icons laden
sub ppicondl {
	my $b="http://www.proplanta.de/wetterdaten/images/symbole/";
	foreach my $i (1..14) {
		icondl($b."t".$i.".gif");
	}
	foreach my $i (1..14) {
		icondl($b."n".$i.".gif");
	}
	foreach my $i (0..10) {
		icondl($b."w".$i.".gif");
	}
	foreach my $i (27..34) {
		icondl($b."w".$i.".gif");
	}
	fhem("set WEB rereadicons");
	fhem("set WEBout rereadicons");
	fhem("set WEBphone rereadicons");
	fhem("set WEBtablet rereadicons");
}
##########################################################
# Command Memory
# Speichert Kommandos solange keiner Zuhause ist. Wird der Status auf Zuhause gewechselt werden die im MemotyDummy gespeicherten Kommandos ausgeführt
#
sub GlobalMemory($$$) {
	my($func, $MemName,$value) = @_;
	my $MemoryDummyName = "MemoryDummy";
	
	my $ReadingValue = ReadingsVal($MemoryDummyName, $MemName, '');
	
	if($func eq "BackHome") {
		if(length($value)) { #append new command	
			$ReadingValue .= ";";
			$ReadingValue =~ s/;/;;/g;
			$ReadingValue =~ s/;;;;//g;
			$ReadingValue = "" if($ReadingValue eq ";" || $ReadingValue eq ";;");
			$ReadingValue .= $value;
			fhem("setreading $MemoryDummyName $MemName $ReadingValue");

		} else { #execute stored command
			return if(!length($ReadingValue));
		
			fhem("$ReadingValue");
			fhem("deletereading $MemoryDummyName $MemName");
		}
	}
}


##########################################################
# TrafficExpr
# Analysiert die Traffic Meldung von google maps
#
sub TrafficExpr($$$) {
	my($traffic,$source,$value) = @_;

	if($source eq "google") {
		if($traffic =~ /(id="altroute_0" altid="0" oi="alt_0" .*?Bei aktueller Verkehrslage:.{50})/) {
			$traffic = $1;
			if($value eq "Standard") {
				if($traffic =~ /(.{20}<span>.*? Minuten?)/) {
					my $Standard = $1;
					Log3 undef, 5, "$Standard";
					$Standard =~ /([\d]+ Minuten?)/;
					my $Std_Min = $1;
					my $Std_Std = "0 Stunden";
					if($Standard =~ /([\d]+ Stunden?)/) {
						$Std_Std = $1;
					}
					(my $val_m,my $txt_m) = split(' ',$Std_Min);
					(my $val_h,my $txt_h) = split(' ',$Std_Std);
					my $Std_Calc = ($val_h*60)+$val_m;
					#Log3 undef, 3, "Normal -> $Std_Std - $Std_Min";
					# return "$Std_Std - $Std_Min";
					return "$Std_Calc";
				}
			} elsif($value eq "Verkehrslage") {
				if($traffic =~ /(Bei aktueller Verkehrslage:.{50})/) {
					my $VerkehrsLage = $1;
					Log3 undef, 5, "$VerkehrsLage";
					$VerkehrsLage =~ /([\d]+ Minuten?)/;
					my $VkL_Min = $1;
					my $VkL_Std = "0 Stunden";
					if($VerkehrsLage =~ /([\d]+ Stunden?)/) {
					$VkL_Std = $1;
					}
					(my $val_m,my $txt_m) = split(' ',$VkL_Min);
					(my $val_h,my $txt_h) = split(' ',$VkL_Std);
					my $VkL_Calc = ($val_h*60)+$val_m;
					#Log3 undef, 3, "Verkehr -> $VkL_Std - $VkL_Min";
					# return "$VkL_Std - $VkL_Min";
					return "$VkL_Calc";		
				}
			} else {
				# <Delay><Standard><Verkehr>	
			}
		}
	} elsif($source eq "bahn") {
	
	
	}
	return "Fehler";
}

##########################################################
# BBB_LEDs
# Erlaubt die Steuerung der BBB Leds
# sudo chmod o+w /sys/class/leds/beaglebone\:green\:usr0/trigger
# sudo chmod g+w /sys/class/leds/beaglebone\:green\:usr0/trigger
sub BBB_LEDs(@) {
	my ($led,$cmd) = @_;
	my $path = "/sys/class/leds/beaglebone\\:green\\:usr";

	if(length($cmd)>0) { #set
		my $ret = `echo $cmd > $path$led/trigger`;
		return $ret;
	} 
	
	#get
	my $ret = `cat $path$led/trigger`;
	$ret =~ /\[(.+?)\]/;
	return $1;
}

##########################################################
# InfraRed
# Erlaubt das Auftrennen der TX und RX Funktion auf zwei verschiedenen Devices 
sub InfraRed($) {
	my ($cmd) = @_;
	my $TXdev = "InfraRedTX";
	my $RXdev = "InfraRedRX";
	#my $TXIOdevName = $defs{$TXdev}{IODev}{NAME};
	#my $IRcmd = (split(' ',$attr{$RXdev}{$cmd}))[0];
	#my $ACKaddTriggerString = "!$IRcmd:attr $RXdev irReceive ON";
	#my $ACKTriggerString = AttrVal($defs{$TXdev}{IODev}{NAME},"MsgACK_Trigger", "");
	
	#fhem("attr $RXdev irReceive OFF");
	#if($ACKTriggerString !~ m/$ACKaddTriggerString/) {
	#	$ACKTriggerString .= ";;" if(length($ACKTriggerString));
	#	$ACKTriggerString .= $ACKaddTriggerString;
	#	fhem("attr $TXIOdevName MsgACK_Trigger $ACKTriggerString");
	#}
	#fhem("set $TXdev irSend $cmd");
	
	fhem("attr $RXdev irReceive OFF;set $TXdev irSend $cmd;sleep 0.3;attr $RXdev irReceive ON",1);
}

##########################################################
# Archiv Documents
#

sub archiveDoc($$) {
	
	my ($DevName, $a) = @_;
	my $hash = $defs{$DevName};
	my ($event, $arg) = split(' ', $a, 2);
	
	if(exists($hash->{helper}{RUNNING_PID})) {
		Log3 $DevName, 1, "$DevName: ArchiveDocument is already running!";
		return;
	}

	readingsSingleUpdate($hash,"locked",1,1) if($event =~ m/^(scan|ascan)$/);
	
	if($event =~ m/^(scan|ascan|optimize|merge|push|clean|unlock)$/) {
		$hash->{helper}{RUNNING_PID} = BlockingCall("doArchiveDocument", "$DevName|$event|$arg", "archiveDoc_done", 120, "archiveDoc_abort", $hash);	
	}
	return;
}

sub doArchiveDocument($) {
	my ($string) = @_;
	my ($DevName, $event, $arg) = split("\\|", $string,3);
	
	my $cmd = "/home/paperless/script/manage $event $arg";
	
	my $ret = qx/$cmd/;
	$ret =~ s/\v$//g;
	
	Log3 $DevName, 4, "$DevName: doArchivDocument '$cmd' > $ret ($?)";	
	return "$DevName" if($?);
	return "$DevName|$ret";
}

sub archiveDoc_done($) {

	my ($string) = @_;
	my $i=0;
	my ($DevName, $filename) = split("\\|", $string,2);
	
	my $hash = $defs{$DevName};
	delete($hash->{helper}{RUNNING_PID});
	if(!$filename) {
		Log3 $DevName, 1, "$DevName: ArchiveDocument failed";
		readingsSingleUpdate($hash,"state","failed",1);
		return;
	}
	Log3 $DevName, 3, "$DevName: ArchiveDocument <$filename> sucessfully done";
		
	readingsBeginUpdate($hash);
	if($filename =~ m/^unlocked$/) {
		readingsBulkUpdate($hash,"locked",0);
	} else {
		readingsBulkUpdate($hash,"locked",1);
		readingsBulkUpdate($hash,"lastFileName",$filename);
		$filename =~ s/_[0-9]+$//;
		$filename =~ s/^\d+Z-//;
		readingsBulkUpdate($hash,"lastName",$filename);
	}
	readingsBulkUpdate($hash,"state","done");
	readingsEndUpdate($hash, 1);
}

sub archiveDoc_abort($) {
  my ($hash) = @_;
  
  delete($hash->{helper}{RUNNING_PID});
  Log3 $hash->{NAME}, 1, "$hash->{NAME}: BlockingCall aborted";
  
  readingsBeginUpdate($hash);
	readingsBulkUpdate($hash,"state","abort");
	readingsEndUpdate($hash, 1);
}

##########################################################
# myTimeTillUpdate
# Berechnet verbleibende Zeit
# Perl Modul notwendig: sudo cpan install DateTime
use DateTime;
sub myTimeTillUpdate($$) {
	
	#my ($dev,$readingName) = @_;
	
	#my ($mydatetime) = ReadingsVal($dev,$readingName,"");
	my ($mydatetime, $format) = @_; #22.03.2015 22:00:00
	return 0 if($mydatetime !~ m/^[0-9]{2}\.[0-9]{2}\.[0-9]{4} [0-9]{2}:[0-9]{2}:[0-9]{2}$/);
	
	my ($mydate,$mytime) = split(" ",$mydatetime);
	my ($myday,$mymonth,$myyear) = split(/\./,$mydate);
	my ($myhour,$myminute,$mysecond) = split(/:/,$mytime); 
	my $dt2 = DateTime->new(
		                     year   => $myyear,
		                     month  => $mymonth,
		                     day    => $myday,
		                     hour   => $myhour,
		                     minute => $myminute,
		                     second => $mysecond
		                   );
	my $dt1 = DateTime->now;
	$dt1->set_time_zone( 'Europe/Berlin' );
	my $duration = $dt1->subtract_datetime($dt2);
	return ($format ne 's') ? (($duration->days()*24+$duration->hours()) .'h '. $duration->minutes() . 'm') : ($duration->days()*24*60*60+$duration->hours()*60*60+$duration->minutes()*60+$duration->seconds());
}
##########################################################
# myReadingsGroupClimateColor
# setzt Styling von Temperatur und Feuchigkeitswerten in einem ReadingsGroup define "attr Raumklima valueStyle {myReadingsGroupClimateValueStyle($DEVICE,$READING,$VALUE)}"
sub myReadingsGroupClimateValueStyle($$$) {

	my ($dev,$reading,$value) = @_;
	my $valueAge=0;
	my $newtimestamp="-";
	my %valueAgedMinutes = 	( 'temperature' => 1,
														'humidity' 		=> 1,
														'dewpoint' 		=> 1,
														'battery' 		=> 35,
														'VCC' 				=> 35
													);
	
	return "{ 'style=\"visibility:hidden;;\"' }" if( (ReadingsVal($dev,$reading,"-") eq "-") && ($value eq $reading) );

	my $vtimestamp = ReadingsTimestamp($dev,$reading,'-');  #2015-03-22 18:48:00 -> 22.03.2015 22:00:00
	$newtimestamp = (substr($vtimestamp,8,2) . "." . substr($vtimestamp,5,2) . "." . substr($vtimestamp,0,4) . " " . substr($vtimestamp,11,8)) if($vtimestamp ne "-");
	$valueAge = myTimeTillUpdate($newtimestamp, "s");
	
	return "{ 'style=\"text-align:center;;color:gray;;\"' }" if(defined($valueAgedMinutes{$reading}) && ($valueAgedMinutes{$reading}*60) <= $valueAge);
	
	
	my $style = "{ 'style=\"text-align:center;;";
	
	if($reading eq "temperature") {
		if ($value > 23 ) {
			$style .= "color:red;;";
		} elsif($value >= 18) {
			$style .= "color:green;;font-weight:bold;;";
		} else {
			$style .= "color:darkorange;;";
		}
	} elsif($reading eq "humidity") {
		if($value >= 60) {
			$style .= "color:red;;";
		} elsif ($value < 55) {
			$style .= "color:green;;font-weight:bold;;";
		} else {
			$style .= "color:darkorange;;";
		}
	} elsif($reading eq "dewpoint") {
#		my $temp = ReadingsVal($dev,$reading,"-"); #<-- Muss Fenstertemperatur sein!
#		if($temp eq "-") {
			$style .= "color:darkorange;;";
#		} elsif($reading <= $temp) {
#			$style .= "color:red;;";
#		} else {
#			$style .= "color:green;;font-weight:bold;;";
#		}
	} elsif($reading eq "battery" ) {# && $value ~= m/^\d+$/) {
		if($value =~ m/^\d+$/) {
			if($value <= 900 || $value >= 1700) {
				$style .= "color:red;;";
			} else {
				$style .= "color:green;;";
			}
		}	
	} elsif($reading eq "VCC") {
		if($value <= 1800 || $value >= 5500) {
			$style .= "color:red;;";
		} else {
			$style .= "color:white;;";
		}
	} else {
		$style .= "color:white;;";
	}
	
	$style .= "\"' }";
	return $style;
}

##########################################################
# myAverage
# berechnet den Mittelwert aus LogFiles über einen beliebigen Zeitraum
sub myAverage($$$){
	my ($offset,$logfile,$cspec) = @_;
	my $period_s = strftime "%Y-%m-%d\x5f%H:%M:%S", localtime(time-$offset);
	my $period_e = strftime "%Y-%m-%d\x5f%H:%M:%S", localtime;
	my $oll = $attr{global}{verbose};
	$attr{global}{verbose} = 2; 
	my @logdata = split("\n", fhem("get $logfile - - $period_s $period_e $cspec"));
	$attr{global}{verbose} = $oll; 
	my ($cnt, $cum, $avg) = (0)x3;

	foreach (@logdata){
		my @line = split(" ", $_);
		if(defined $line[1] && "$line[1]" ne ""){
			$cnt += 1;
			$cum += $line[1];
		}
	}
	if("$cnt" > 0){$avg = sprintf("%0.1f", $cum/$cnt)};
	Log 4, ("myAverage: File: $logfile, Field: $cspec, Period: $period_s bis $period_e, Count: $cnt, Cum: $cum, Average: $avg");
	return $avg;
}

######## Remote_FB_WLANswitch ############################################
# What  : Switches Remote FB WLAN on or off
# Call  : { Remote_FB_WLANswitch("xxx.xxx.xxx.xxx","password","on") } or
#         { Remote_FB_WLANswitch("xxx.xxx.xxx.xxx","user,"password","on") }
#
sub Remote_FB_WLANswitch (@) {
	my ($ip,$password,$cmd) = @_;
	my $ret="";
	# my ($ip,$user,$password,$cmd) = @_;        #Wenn User und PW erforderlich
	use Net::Telnet;

	my $telnet = new Net::Telnet ( Timeout=>15, Errmode=>'die');
	$telnet->open($ip);
	# $telnet->waitfor('/Fritz!Box user: $/i');  #Wenn User und PW erforderlich
	# $telnet->print($user);                     #Wenn User und PW erforderlich
	# $telnet->waitfor('/# $/i');                #Wenn User und PW erforderlich
	$telnet->waitfor('/password: $/i');
	$telnet->print($password);
	$telnet->waitfor('/# $/i');

	if ($cmd =~ m/on/i) {            # on or ON
		$telnet->cmd('ctlmgr_ctl w wlan settings/ap_enabled 1');
		$ret=1;
	} elsif ($cmd =~ m/off/i) {           # off or OFF
		$telnet->cmd('ctlmgr_ctl w wlan settings/ap_enabled 0');
		$ret=0;
	} elsif ($cmd =~ m/status/i) {
		$ret = $telnet->cmd("ctlmgr_ctl r wlan settings/ap_enabled");
	}
	
	Log 3, "Remote_FB_WLANswitch($cmd)";
	#$telnet->waitfor('/# $/i');		# Ulli: verursacht einen timeout
	$telnet->print('exit');
	
	return $ret;
}

#sudo -u fhem ssh-keygen -t rsa #Schlüsselgenerierung
#client> ssh-copy-id -i /home/ulli/.ssh/id_rsa.pub ulli@nuc
#{Remote_SSH("diskstation","admin","/opt/fhem/id_rsa", "pwd")}
#id_rsa in /home/fhem/.ssh kopieren und "chown fhem:root id_rsa" ausführen
sub Remote_SSH(@) {
	my($host,$user,$rsaFile ,$cmd) = @_;
	my $ret;

	my $ssh = qx(which ssh);    
	chomp( $ssh );
	$ssh .= ' ';
	$ssh .= "-i $rsaFile " if(defined($rsaFile) && $rsaFile ne "");
	$ssh .= $user."\@" if(defined($user) && $user ne "");
	$ssh .= $host ." ". $cmd . " 2>/dev/null";
	
	if( open(my $fh, '-|', "$ssh" ) ) {    
			$ret = <$fh>;
			$ret =~ s/^\s+|(\s+|\\n\\r)$//g if(defined($ret) && $ret ne "");
			$ret = "" if(!defined($ret));
			Log 3, "Remote_SSH($ssh) returned <$ret>";
			close($fh);
	} else {
		Log 3, "Remote_SSH($ssh) failed.";
	}

	return $ret;
}

# EventZeit: Die Zeit wird nicht in der fhem-Reihenfolge, sondern für "human Interface" dargestellt
sub EventZeit() {
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time());
	return sprintf ("%2d:%02d:%02d %2d.%02d.%4d", $hour,$min,$sec,$mday,($mon+1),($year+1900));
}
# fuer Telefonanrufe
our @A;
our @B;
our @C;
our @D;
our @E;
our %TelefonAktionsListe;
sub TelefonMonitor($) {

	our $extnum;
	our $intnum;
	our $extname;
	our $callID;
	our $callDuration;
	our $stat;
	my $i; my $j;
	our $ab;
	my $Fritz_Box = $defs{"FritzBox"};

	my ($event,$arg) = split (':',$_[0]);
	$arg = ltrim($arg);

	# Log(3,"TM: event: $event arg: $arg");
	if ($event eq "event") {
		$stat = $arg; 
		if ($arg eq "disconnect")	{ 

		}
		return;
	}

	if ($stat eq "ring") {
		if ($event eq "external_number") {
		  $extnum = $arg;
			return;
		}
		if ($event eq "external_name") {
			$extname = $arg;
		 	return;
		}
		if ($event eq "internal_number") {
			$intnum = $arg;
			return;
		}
		if ($event eq "call_id") {
			$callID = $arg;
			# hier koennen wir eine anrufgesteuerte Aktion starten
	 	  #TelefonAktion($extname, $intnum);
			$A[$callID] = "in";
		 	$B[$callID] = EventZeit();
		 	$C[$callID] = $extname;
		 	$D[$callID] = $extnum;
			return;
		}
		return;
	}

	if ($stat eq "connect") {
		if (($event eq"internal_connection") && ($arg =~m/Answering_Machine_.*/)) {
			$ab = "ab";
		}
		if ($event eq "call_id") {
			$callID = $arg;
	 		if ($ab && ($ab eq "AB")) {
	 			$A[$callID] = "AB";
	 		}
		 	$ab = undef; # zuruecksetzen
		}
	}


	if ($stat eq "call") {
		if ($event eq"external_number")	{
			$extnum = $arg;
			return;	    
		}
		if ($event eq "external_name") {
			$extname = $arg;
		 	return;
		}
		if ($event eq "call_id") {
			$callID = $arg;
	 		$A[$callID] = "out";
		 	$B[$callID] = EventZeit();
		 	$C[$callID] = $extname;
		 	$D[$callID] = $extnum;
			return;
		}
		return;
	}

	if ($stat eq "disconnect") {
		if ($event eq "call_duration") {
			$callDuration = sprintf("%2d:%02d", ($arg/60),$arg%60);
			return;
		}
		if ($event eq "call_id") {
			$callID = $arg;
			#shiften der alten Inhalte
		 	my $tt;
		  readingsBeginUpdate($Fritz_Box);
		  for ($i=4;$i>0; $i--) {
				foreach $j ('A'..'E') {
					#$defs{"Fritz_Box"}{READINGS}{$j.($i-1)}{VAL};
				  $tt = ReadingsVal("Fritz_Box",$j.($i-1),"-");
					readingsBulkUpdate($Fritz_Box,$j.$i,$tt);
				}
			}
			$E[$callID] = $callDuration;
			readingsBulkUpdate($Fritz_Box,"A0",$A[$callID]);
			readingsBulkUpdate($Fritz_Box,"B0",$B[$callID]);
			readingsBulkUpdate($Fritz_Box,"C0",$C[$callID]);
			readingsBulkUpdate($Fritz_Box,"D0",$D[$callID]);
			readingsBulkUpdate($Fritz_Box,"E0",$E[$callID]);
			readingsEndUpdate($Fritz_Box, 1);
		 	$stat = "";
			return;
		}
	}
}

