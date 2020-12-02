#!/usr/bin/perl -w
#
#displays the racetable in wiki format
@classes = ('warrior',
	    'paladin',
	    'anti-paladin',
	    'ranger',
	    'samurai',
	    'thief',
	    'ninja',
	    'cleric',
	    'warlock',
	    'witch',
	    'necromancer',
	    'vampire');

sub main () {
   $odd = 1;
   while (@ARGV) {
      $file = shift @ARGV;
          open (FILE, $file) || die ("couldn't open: $file");
   	  %class = ();
	   while (<FILE>) {
	      $name = $1 if (/^Name (.*)~$/);
	      $class{$1} = 1 if (/^Class \'(.*)\'/);
	      if (/^Stats (.*)$/) {
		 $statline = $1;
		 @stats = split(" ", $statline);
	      }
	   }
	   $row = ($odd) ? "#556655" : "#445544";
	   $rrow = ($odd) ? "#555555" : "#444444";
	   $srow = ($odd) ? "#665555" : "#554444";
print "|-bgcolor=\"$row\"\n";
print "|bgcolor=\"$rrow\" | '''[[$name]]'''\n";
print "|bgcolor=\"$rrow\" |&nbsp;\n";
print "|bgcolor=\"$rrow\" |&nbsp;\n";

	   foreach (@classes) {
	      print "|align=center|" ;
	      if (defined($class{$_} && $class{$_} == 1)) {
	         print 'X';
	      } else {
	         print '&nbsp;' ;
	      }
	      print "\n";
	   }

	   foreach (@stats) {
		print "|align=center bgcolor=\"$srow\" | <tt>$_</tt>\n";
	   }
	   close(FILE);
	   print "\n";
	   $odd = !$odd;
   }
}




main();
