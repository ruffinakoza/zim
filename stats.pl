#!/usr/bin/perl -w

use diagnostics;
#use strict;

my $PLAYER_DIR = "player/";
my %players;

sub main () {
	readPlayerFiles($PLAYER_DIR);
	print "LastLevel-------\n";
	top("LastLevel", 10);
	print "QPs-------------\n";
	top("QPs", 10);
	print "PKs-------------\n";
	top("PKs", 10);
	print "PKDs------------\n";
	top("PKDs", 10);
	print "GPs------------\n";
	top("GPs", 30);
	print "Level------------\n";
	top("Level", 30);

}


##read in the player files and load the database of players
sub readPlayerFiles($) {
	$dir = shift;

	opendir(PLAYERS_DIR, $dir) || die ("Can't open directory $dir");
	while ($filename = readdir(PLAYERS_DIR)) {
		if ($filename =~ /[A-Z][a-z][a-z]+$/) {
			if (!open(PLAYER, $dir . $filename)) {
				print "Couldn't open $filename\n";
				return;
			}
			my %attr = ();

			#foreach (@keys) {
			#	$attr{$_} = '';
			#}

			$attr{'Name'} = $filename;
			while (<PLAYER>) {
				if (/^(Hometown) (.*)~$/)	{ $attr{$1} = $2; }
				if (/^(Race) (.*)~$/)		{ $attr{$1} = $2; }
				if (/^(Ethos) (.*)$/)		{ $attr{$1} = $2; }
				if (/^(Sex) (.*)$/)		{ $attr{$1} = $2; }
				if (/^(Class) (.*)~$/)		{ $attr{$1} = $2; }
				if (/^(Levl) (.*)$/)		{ $attr{'Level'} = $2; }
				if (/^(Alig) (.*)$/)		{ $attr{'Align'} = $2; }
				if (/^(ACs) (\d+) (\d+) (\d+) (\d+)$/) { 
					$attr{'magic'} = $2;
					$attr{'bash'} = $3;
					$attr{'pierce'} = $4;
					$attr{'slash'} = $5; 
				}
				if (/^(HMVP) \d+ (\d+) \d+ (\d+) \d+ (\d+)$/) {
					$attr{'hp'} = $2;
					$attr{'mana'} = $3;
					$attr{'move'} = $4;
				}
				if (/^(Bankg) (\d+)$/)		{ $attr{'GPs'} += $2;  }
				if (/^(Gold) (\d+)$/)		{ $attr{'GPs'} += $2;  }
				if (/^(LLev) (\d+)$/)		{ $attr{'LastLevel'} = $2; }
				if (/^(QuestPnts) (\d+)$/)	{ $attr{'QPs'} = $2; }
				if (/^(PK_Kills) (\d+)$/)	{ $attr{'PKs'} = $2; }
				if (/^(PK_Deaths) (\d+)$/)	{ $attr{'PKDs'} = $2; }
				if (/^(GoodKilled) (\d+)$/)	{ $attr{'GoodKilled'} = $2; }
				if (/^(NeutKilled) (\d+)$/)	{ $attr{'NeutKilled'} = $2; }
				if (/^(EvilKilled) (\d+)$/)	{ $attr{'EvilKilled'} = $2; }
				if (/^(GoodStanding) (\d+)$/)	{ $attr{'GoodStanding'} = $2; }
				if (/^(NeutStanding) (\d+)$/)	{ $attr{'NeutStanding'} = $2; }
				if (/^(EvilStanding) (\d+)$/)	{ $attr{'EvilStanding'} = $2; }
			}
			close(PLAYER);

			if ($attr{'Level'} < 92) {
				$players{$filename} = \%attr;
				#dumpPlayer($filename);
			}
		}
	}
	closedir(PLAYERS_DIR);
}

##dump a player's stats
sub dumpPlayer ($) {
	my $playername = shift;
	my $player = $players{$playername};

	foreach (sort(keys(%$player))) {
		print $_ . ": \t" . $player->{$_} . "\n";
	}
}

sub top($$) {
	$sortby = shift;
	my $num = shift;
	my @sorted = ();

	@sorted = sort {compare($a,$b)} (keys %players);
	@top = @sorted[0..($num-1)];

	foreach(@top) {
		if (defined($players{$_}->{$sortby})) {
			print "$players{$_}->{$sortby}\t$_\n";
		}
	}
}

sub compare ($$) {
	my $a = shift;
	my $b = shift;

	my $_a = (defined($players{$a}->{$sortby})) ? $players{$a}->{$sortby} : 0;
	my $_b = (defined($players{$b}->{$sortby})) ? $players{$b}->{$sortby} : 0;

	return ($_b <=> $_a);
}


main();

