#!/usr/bin/perl -w

$areaDir		= ".";
$areaListFilename	= "area.lst";

%areaList		= ();
%areas			= ();

#area table sub hash keys
$MIN_VNUM		= "min_vnum";
$MAX_VNUM		= "max_vnum";

sub loadAreaNames ($)
{
	my $areaListFile = shift;
	my $i = 0;

	open (AREA_LIST, $areaListFile) || die ("Can't open $areaListFile");

	while (<AREA_LIST>) {
		chomp();
		if ($_ !~ /^\$/) {
			$areaList[$i++] = $_;
			$areas{$_} = 1;
		}
	}

	close (AREA_LIST);
}

sub prune ()
{
	my $junkDir = $areaDir . "/junk";

	print "Moving old area files to junk directory: $junkDir\n";

	opendir(AREA_DIR, $areaDir) || die ("Can't open directory $areaDir");
	while ($filename = readdir(AREA_DIR)) {
		if ($filename =~ /.*\.are$/ && !$areas{$filename}) {
			print "\t" . $filename . "\n";
			`mv $filename $junkDir`;
		}
	}
	closedir(AREA_DIR);
}

sub loadAreas ()
{
	foreach $areaFile (keys %areas) {
		$areas{$areaFile} = loadAreaInfo($areaFile);
	}
}

sub loadAreaInfo ($)
{
	my $areaFile = shift;
	my %area = ();

	open (AREA_FILE, $areaFile) || die ("couldn't open area $areaFile");

	while (<AREA_FILE>) {
		if (/^VNUMs\s+(\d+)\s+(\d+)/) {
			$area{$MIN_VNUM} = $1;
			$area{$MAX_VNUM} = $2;
		}
	}
	return \%area;
}

sub printAreaInfo ($)
{
	my $areaFile = shift;

	print $areaFile . " :: " . $areas{$areaFile}->{$MIN_VNUM} 
		. "-" . $areas{$areaFile}->{$MAX_VNUM} . "\n";
}

sub by_vnum
{
	if ($areas{$a}->{$MIN_VNUM} < $areas{$b}->{$MIN_VNUM}) {
		return -1;
	}
	elsif ($areas{$a}->{$MIN_VNUM} == $areas{$b}->{$MIN_VNUM}) {
		return 0;
	}
	else {
		return 1;
	}
}

sub printSortedAreaList ()
{
	@areaListSorted = sort by_vnum @areaList;
	foreach (@areaListSorted) {
		print $_ . "\n";
	}
	print "\$\n";
}
sub main ()
{
	loadAreaNames($areaListFilename);
	prune();
	loadAreas();
	printSortedAreaList();
}

main();

