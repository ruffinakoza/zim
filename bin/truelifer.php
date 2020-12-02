<html>
<head>
<title>LnL: True Lifers</title>
<link rel="stylesheet" type="text/css" href="lnl.css"/>
</head>
<body class="DataElement">
<table border="0" cellpadding="5" cellspacing="3" style="height:90%;width:100%" class="ShadeElement">
<tr>
	<td colspan="4" align="center" height="1%">
		<table border="2" cellspacing="0" cellpadding="5" style="height:100%;width:100%">
			<tr>
				<td align="center" class="DataElement">
					<div class="header">True Lifers</div>
				</td>
			</tr>
		</table>
	</td>
</tr>
<tr>
	<td colspan="4" align="center" height="1%">
		<table border="2" cellspacing="0" cellpadding="5" style="height:100%;width:100%">
			<tr>
				<td align="center" class="DataElement">
				<div class="menu">
	<b>Contests:</b> 
	<a href="truelifer.php?sort=level&year=2005&month=09&race=halfling&class=samurai">2005-09 Halfling Samurai</a>, 
	<a href="truelifer.php?sort=level&year=2006&month=02&race=svirfnebli&class=cleric">2006-02 Svirfnebli Evil Cleric</a>, 
	<a href="truelifer.php?sort=level&year=2006&month=03&class=warrior">2006-03 Elven vs Drow Warriors</a>
	<a href="truelifer.php?sort=levelpk&year=2006&month=10">2006-10 Bloody October</a>
				</div>
				</td>
			</tr>
		</table>
	</td>
</tr>
<?php
	if ($argv)
		for ($i=1;$i<count($argv);$i++) {
			$it = split("=",$argv[$i]);
			$_GET[$it[0]] = $it[1];
		}

	$filename = "tmp/truelifer.log";
	$sort_type = (isset($_GET['sort'])) ? $_GET['sort'] : 'level';
	$year = (isset($_GET['year'])) ? $_GET['year'] : 0;
	$month = (isset($_GET['month'])) ? $_GET['month'] : 0;
	$race = (isset($_GET['race'])) ? $_GET['race'] : 0;
	$class = (isset($_GET['class'])) ? $_GET['class'] : 0;
	$minlevel = (isset($_GET['minlevel'])) ? $_GET['minlevel'] : 0;
	$top = (isset($_GET['top'])) ? $_GET['top'] : 20;

	$DATE = 'date';
	$YEAR = 'year';
	$MONTH = 'month';
	$MDAY = 'mday';
	$TIME = 'time';
	$NAME = 'name';
	$LEVEL = 'level';
	$CLAN = 'clan';
	$RACE = 'race';
	$CLASS = 'class';
	$EXP = 'exp';
	$QUESTS = 'quests';
	$PK = 'pk';
	$MK = 'multi-killer';
	$MOBS = 'mobs';
	$KILLER = 'killedby';
	$KLEVEL = 'killerlevel';
	$LEVELPK = 'levelpk';
	$IP = 'ip';

	$players = array();


	if (!($filearray = file($filename))) {
		print "Can't open file $filename";
	}
	else {
		##load file
		while (list ($line_number, $_) = each ($filearray)) {
			#echo $_;
			preg_match(
				  '#^(\d\d\d\d)'
				. '(\d\d)'
				. '(\d\d)'
				. '-(\S+)'
				. '\s(\w+)'
				. '\[(\d+)\]'
				. '\s\((.+)/(.+)/(.+)\)'
				. '\sXP:(\d+)'
				. '\sQuests:(\d+)'
				. '\sPK:(\d+)'
				. '\sMK:(\w+)'
				. '\sMobs:(\d+)'
				. '\sdied to (.*)'
				. '\[(\d+)\]'
				. '\s-\s(.*)\.$'

				. '#',
				$_,
				$matches
				);
			#print_r ($matches);
			$record = array();
			$i = 1;
			$record[$YEAR] = $matches[$i++];
			$record[$MONTH] = $matches[$i++];
			$record[$MDAY] = $matches[$i++];
			$record[$TIME] = $matches[$i++];
			$record[$NAME] = $matches[$i++];
			$record[$LEVEL] = $matches[$i++];
			$record[$CLAN] = $matches[$i++];
			$record[$RACE] = strtolower($matches[$i++]);
			$record[$CLASS] = $matches[$i++];
			$record[$EXP] = $matches[$i++];
			$record[$QUESTS] = $matches[$i++];
			$record[$PK] = $matches[$i++];
			$record[$MK] = $matches[$i++];
			$record[$MOBS] = $matches[$i++];
			$record[$KILLER] = $matches[$i++];
			$record[$KLEVEL] = $matches[$i++];
			$record[$IP] = $matches[$i++];
			$record[$DATE] = $record[$YEAR].$record[$MONTH] . $record[$MDAY];
			$record[$LEVELPK] = $record[$LEVEL] + $record[PK];
			#print_r ($record);

			if ($race && $race != $record[$RACE])
				continue;
			if ($class && $class != $record[$CLASS])
				continue;

			if ($year && $year != $record[$YEAR])
				continue;

			if ($month && $month != $record[$MONTH])
				continue;

			if ($minlevel && $record[$LEVEL] < $minlevel )
				continue;

			array_push($players, $record);
		}

		##sort players array
		usort($players, "cmp_by" . $sort_type);

?>
<tr>
	<td colspan="4" align="center" height="1%">
		<table border="2" cellspacing="0" cellpadding="5" style="height:100%;width:100%">
			<tr>
				<td align="center" class="DataElement">
<table border="0" cellpadding="2" cellspacing="2" style="height:90%;width:100%" class="DataElement">
<tr>
	<th>Name</th>
	<th><a href="truelifer.php?sort=level">Level</a></th>
	<th><a href="truelifer.php?sort=race">Race</a></th>
	<th><a href="truelifer.php?sort=class">Class</a></th>
	<th>Clan</th>
	<th>PKs</th>
	<th><a href="truelifer.php?sort=quests">Quests</a></th>
	<th><a href="truelifer.php?sort=exp">EXP</a></th>
	<th><a href="truelifer.php?sort=date">Death</a></th>
	<th align="left">Killed By</th>
<?php
		$count = 0;
		foreach($players as $key => $value) {
			if (++$count > $top)
				break;
?>
<tr>
	<td class="row"><?php echo $value[$NAME] ?></td>
	<td class="row" align="center"><?php echo $value[$LEVEL] ?></td>
	<td class="row" align="center"><?php echo $value[$RACE] ?></td>
	<td class="row" align="center"><?php echo $value[$CLASS] ?></td>
	<td class="row" align="center"><?php echo $value[$CLAN] ?></td>
	<td class="row" align="center"><?php echo $value[$PK] ?></td>
	<td class="row" align="center"><?php echo $value[$QUESTS] ?></td>
	<td class="row" align="right"><?php echo $value[$EXP] ?></td>
	<td class="row" align="center"><?php echo "$value[$YEAR]-$value[$MONTH]-$value[$MDAY]"; ?></td>
	<td class="row"><?php 
		$killer = preg_replace('/({.)/', '', $value[$KILLER]);
		echo "$killer"; ?></td>
</tr>
<?php
		}
?>
</td>
</tr>
</table>
</td>
</tr>
</center>
</body>
</html>
<?php
	}

	function cmp_generic($a, $b, $sortby, $second) {
		if ($a[$sortby] == $b[$sortby]) {
			if ($a[$second] == $b[$second])
				return 0;
			else
				return ($a[$second] > $b[$second]) ? -1 : 1;
		}
		return ($a[$sortby] > $b[$sortby]) ? -1 : 1;
	}
	function cmp_generic_reverse($a, $b, $sortby, $second) {
		if ($a[$sortby] == $b[$sortby]) {
			if ($a[$second] == $b[$second])
				return 0;
			else
				return ($a[$second] > $b[$second]) ? -1 : 1;
		}
		return ($a[$sortby] < $b[$sortby]) ? -1 : 1;
	}
	function cmp_bydate ($a, $b) {
		global $DATE;
		global $LEVEL;
		return cmp_generic($a, $b, $DATE, $LEVEL);
	}
	function cmp_bydater ($a, $b) {
		global $DATE;
		global $LEVEL;
		return cmp_generic_reverse($a, $b, $DATE, $LEVEL);
	}
	function cmp_bylevel ($a, $b) {
		global $LEVEL;
		global $NAME;
		return cmp_generic($a, $b, $LEVEL, $NAME);
	}
	function cmp_bylevelr ($a, $b) {
		global $LEVEL;
		global $NAME;
		return cmp_generic_reverse($a, $b, $LEVEL, $NAME);
	}
	function cmp_bylevelpk ($a, $b) {
		global $LEVELPK;
		global $NAME;
		return cmp_generic($a, $b, $LEVELPK, $NAME);
	}
	function cmp_byexp ($a, $b) {
		global $EXP;
		global $NAME;
		return cmp_generic($a, $b, $EXP, $NAME);
	}
	function cmp_byexpr ($a, $b) {
		global $EXP;
		global $NAME;
		return cmp_generic_reverse($a, $b, $EXP, $NAME);
	}
	function cmp_byquests ($a, $b) {
		global $QUESTS;
		global $LEVEL;
		return cmp_generic($a, $b, $QUESTS, $LEVEL);
	}
	function cmp_byquestsr ($a, $b) {
		global $QUESTS;
		global $LEVEL;
		return cmp_generic_reverse($a, $b, $QUESTS, $LEVEL);
	}
	function cmp_byrace ($a, $b) {
		global $RACE;
		global $LEVEL;
		return cmp_generic_reverse($a, $b, $RACE, $LEVEL);
	}
	function cmp_byclass ($a, $b) {
		global $CLASS;
		global $EXP;
		return cmp_generic_reverse($a, $b, $CLASS, $EXP);
	}
?>
