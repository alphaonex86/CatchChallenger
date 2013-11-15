<?php
echo "<!-- Default:
success=\"100%\"
applyOn=\"aloneEnemy\"
applyOn can be: aloneEnemy, themself, allEnemy, allAlly
sp=\"0\", can't be learn naturaly
endurance=\"40\"
the id 0 is the attack when have no attack
-->

<list>\n";

$file=file_get_contents('battle/movetypes.xml');
preg_match_all('#<entry>(.*)</entry>#isU',$file,$entry_list);
$movetypes=array();
$movetypes_name_to_id=array();
foreach($entry_list[1] as $entry)
{
	$applyOn='aloneEnemy';
	if(!preg_match('#<id>[0-9]+</id>#isU',$entry))
		continue;
	$id=preg_replace('#^.*<id>([0-9]+)</id>.*$#isU','$1',$entry);
	if(!preg_match('#<name>[^<]+</name>#isU',$entry))
		continue;
	$name=preg_replace('#^.*<name>([^<]+)</name>.*$#isU','$1',$entry);
	if(!preg_match('#<description>[^<]*</description>#isU',$entry))
		continue;
	$description=preg_replace('#^.*<description>([^<]*)</description>.*$#isU','$1',$entry);
	if(!preg_match('#<type>[^<]+</type>#isU',$entry))
		continue;
	$type=strtolower(preg_replace('#^.*<type>([^<]+)</type>.*$#isU','$1',$entry));
	if(!preg_match('#<category>[^<]+</category>#isU',$entry))
		continue;
	$category=preg_replace('#^.*<category>([^<]+)</category>.*$#isU','$1',$entry);
	if(!preg_match('#<pp>[0-9]+</pp>#isU',$entry))
		continue;
	$pp=preg_replace('#^.*<pp>([0-9]+)</pp>.*$#isU','$1',$entry);
	if(!preg_match('#<power>[0-9]+</power>#isU',$entry))
	{
		if(preg_match('#heal#isU',$name))
		{
			$applyOn='themself';
			$power='+100%';
		}
		else if(preg_match('#<power>Varies</power>#isU',$name))
		{
			if($description=='')
				$description='Touch the target multiple times';
			$power='-60';
		}
		else
			$power='0';
	}
	else
		$power='-'.preg_replace('#^.*<power>([0-9]+)</power>.*$#isU','$1',$entry);
	if(!preg_match('#<accuracy>[0-9]+</accuracy>#isU',$entry))
		$accuracy='100';
	else
		$accuracy=preg_replace('#^.*<accuracy>([0-9]+)</accuracy>.*$#isU','$1',$entry);
	$movetypes[$id]=array(
		'name'=>$name,
		'description'=>$description,
		'type'=>$type,
		'category'=>$category,
		'pp'=>$pp,
		'power'=>$power,
		'accuracy'=>$accuracy,
		'applyOn'=>$applyOn,
	);
	$movetypes_name_to_id[$name]=$id;
}

foreach($movetypes as $id=>$entry)
{
	?>
	<skill id="<?php echo $id; ?>" type="<?php echo $entry['type']; ?>" category="<?php echo $entry['category']; ?>">
		<name><?php echo $entry['name']; ?></name>
		<description><?php echo $entry['description']; ?></description>
		<effect>
			<level number="1" sp="100" endurance="<?php echo $entry['pp']; ?>">
				<life quantity="<?php echo $entry['power']; ?>" applyOn="<?php echo $entry['applyOn']; ?>"<?php
if($entry['accuracy']<100)
	echo ' success="'.$entry['accuracy'].'%"';
?> />
			</level>
		</effect>
	</skill>
<?php
}

echo "</list>\n";
