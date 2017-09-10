<?php
echo "<!-- default:
ratio_gender=\"50%\"
catch_rate=\"100\"
luck is in percent
ratio_gender: 0% only male, 100% only female, -1 no gender
second line is stat at level 100
if level is 0, it's base attack, default: attack_level=\"1\"
evolution type can be: level (attribute level is the level), item (attribute level is the item id), trade (attribute level is useless)
-->
<monsters>\n";

function simplifedName($string)
{
    $string=str_replace(' ','',$string);
    $string=str_replace('-','',$string);
    $string=str_replace('_','',$string);
    $string=strtoupper($string);
    return $string;
}

$datapackexplorergeneratorinclude=true;
$lang_to_load=array('en');
$datapack_path='/home/user/Desktop/CatchChallenger/Grab-test/datapack/';
require '/home/user/Desktop/www/catchchallenger.first-world.info/official-server/datapack-explorer-generator/function.php';

require '/home/user/Desktop/www/catchchallenger.first-world.info/official-server/datapack-explorer-generator/load/items.php';
foreach($item_meta as $id=>$item)
    $item_name_to_id[simplifedName($item['name']['en'])]=$id;

require '/home/user/Desktop/www/catchchallenger.first-world.info/official-server/datapack-explorer-generator/load/skill.php';
foreach($skill_meta as $id=>$skill)
    $movetypes_name_to_id[simplifedName($skill['name']['en'])]=$id;

require '/home/user/Desktop/www/catchchallenger.first-world.info/official-server/datapack-explorer-generator/load/monster.php';
    
$file=file_get_contents('species.xml');
preg_match_all('#<pokemonSpecies>(.*)</pokemonSpecies>#isU',$file,$entry_list);
$species=array();
$name_to_id=array();
foreach($entry_list[1] as $entry)
{
	if(!preg_match('#<m_species>[0-9]+</m_species>#isU',$entry))
		continue;
	$id=preg_replace('#^.*<m_species>([0-9]+)</m_species>.*$#isU','$1',$entry);
	if(!preg_match('#<m_name>[^<]+</m_name>#isU',$entry))
		continue;
	$name=preg_replace('#^.*<m_name>([^<]+)</m_name>.*$#isU','$1',$entry);
	if(!preg_match('#<m_type>[0-9]+</m_type>#isU',$entry))
		continue;
	preg_match_all('#<m_type>([0-9]+)</m_type>#isU',$entry,$entry_list_type);
	$type=$entry_list_type[1];
	if(!preg_match('#<m_genders>[0-9]+</m_genders>#isU',$entry))
		continue;
	$gender=preg_replace('#^.*<m_genders>([0-9]+)</m_genders>.*$#isU','$1',$entry);
	switch($gender)
	{
		default:
		case 3:
			$gender='50%';
		break;
		case 1:
			$gender='0%';
		break;
		case 2:
			$gender='100%';
		break;
		case 0:
			$gender='-1';
		break;
	}
	preg_match_all('#<int>([0-9]+)</int>#isU',$entry,$entry_list_stat);
	if(count($entry_list_stat[1])!=6)
		continue;
	$base_hp=$entry_list_stat[1][0];
	$base_attack=$entry_list_stat[1][1];
	$base_defense=$entry_list_stat[1][2];
	$base_speed=$entry_list_stat[1][3];
	$base_spattack=$entry_list_stat[1][4];
	$base_spdefense=$entry_list_stat[1][5];
	
	$name_to_id[$name]=$id;
	$species[$id]=array(
		'name'=>$name,
		'type'=>$type,
		'gender'=>$gender,

		'base_hp'=>$base_hp,
		'base_attack'=>$base_attack,
		'base_defense'=>$base_defense,
		'base_speed'=>$base_speed,
		'base_spattack'=>$base_spattack,
		'base_spdefense'=>$base_spdefense,
	);
}

$file=file_get_contents('polrdb.xml');
preg_match_all('#<POLRDataEntry>(.*)</POLRDataEntry>#isU',$file,$entry_list);
foreach($entry_list[1] as $entry)
{
	$type=$entry_list_type[1];
	if(!preg_match('#<name>[^<]+</name>#isU',$entry))
		continue;
	$name=preg_replace('#^.*<name>([^<]+)</name>.*$#isU','$1',$entry);
	$name=preg_replace('#POK.{1,3}MON#','POKEMON',$name);
	$name=htmlentities($name);
	if(!preg_match('#<kind>[^<]+</kind>#isU',$entry))
		continue;
	$kind=preg_replace('#^.*<kind>([^<]+)</kind>.*$#isU','$1',$entry);
	if(!preg_match('#<pokedex>[^<]+</pokedex>#isU',$entry))
		continue;
	$description=preg_replace('#^.*<pokedex>([^<]+)</pokedex>.*$#isU','$1',$entry);
	$description=preg_replace("#[\r\t\n]+#isU",' ',$description);
	$description=preg_replace('# +#',' ',$description);
	$description=preg_replace('#POK.{1,3}MON#','POKEMON',$description);
	$description=htmlentities($description);
	if(!preg_match('#<type[0-9]>[^<]+</type[0-9]>#isU',$entry))
		continue;
	preg_match_all('#<type[0-9]>([^<]+)</type[0-9]>#isU',$entry,$entry_list_type);
	if(!preg_match('#<rareness>[0-9]+</rareness>#isU',$entry))
		continue;
	$rareness=preg_replace('#^.*<rareness>([0-9]+)</rareness>.*$#isU','$1',$entry);
	if(!preg_match('#<baseEXP>[0-9]+</baseEXP>#isU',$entry))
		continue;
	$baseEXP=preg_replace('#^.*<baseEXP>([0-9]+)</baseEXP>.*$#isU','$1',$entry);
	if(!preg_match('#<happiness>[0-9]+</happiness>#isU',$entry))
		continue;
	$happiness=preg_replace('#^.*<happiness>([0-9]+)</happiness>.*$#isU','$1',$entry);
	if(!preg_match('#<stepsToHatch>[0-9]+</stepsToHatch>#isU',$entry))
		continue;
	$stepsToHatch=preg_replace('#^.*<stepsToHatch>([0-9]+)</stepsToHatch>.*$#isU','$1',$entry);
	if(!preg_match('#<color>[^<]+</color>#isU',$entry))
		continue;
	$color=preg_replace('#^.*<color>([^<]+)</color>.*$#isU','$1',$entry);
	if(preg_match('#<habitat>[^<]+</habitat>#isU',$entry))
        $habitat=preg_replace('#^.*<habitat>([^<]+)</habitat>.*$#isU','$1',$entry);
    else
        $habitat='';
	if(!preg_match('#<height>[0-9]+(\.[0-9]+)?</height>#isU',$entry))
		continue;
	$height=preg_replace('#^.*<height>([0-9]+(\.[0-9]+)?)</height>.*$#isU','$1',$entry);
	if(!preg_match('#<weight>[0-9]+(\.[0-9]+)?</weight>#isU',$entry))
		continue;
	$weight=preg_replace('#^.*<weight>([0-9]+(\.[0-9]+)?)</weight>.*$#isU','$1',$entry);
	if(!preg_match('#<moves>.*</moves>#isU',$entry))
		continue;
	$temp_move=preg_replace('#^.*<moves>([0-9]+(\.[0-9]+)?)</moves>.*$#isU','$1',$entry);
	preg_match_all('#<entry>(.*)</entry>#isU',$temp_move,$entry_list_moves);
	$moves=array();
	foreach($entry_list_moves[1] as $move)
	{
		if(!preg_match('#<integer>[0-9]+</integer>#isU',$move))
			continue;
		$level=preg_replace('#^.*<integer>([0-9]+)</integer>.*$#isU','$1',$move);
		if(!preg_match('#<string>[^<]+</string>#isU',$move))
			continue;
		$string=preg_replace('#^.*<string>([^<]+)</string>.*$#isU','$1',$move);
		$moves[]=array($level,$string);
	}
	if(preg_match('#<starterMoves>.*</starterMoves>#isU',$entry))
	{
        $temp_starterMoves=preg_replace('#^.*<starterMoves>(.*)</starterMoves>.*$#isU','$1',$entry);
        preg_match_all('#<string>(.*)</string>#isU',$temp_starterMoves,$temp_starterMoves_list);
        foreach($temp_starterMoves_list[1] as $starterMove)
            $moves[]=array(0,$starterMove);
    }
	$evolutions=array();
	if(preg_match('#<POLREvolution>.*</POLREvolution>#isU',$entry))
	{
		preg_match_all('#<POLREvolution>(.*)</POLREvolution>#isU',$entry,$entry_list_evolutions);
		foreach($entry_list_evolutions[1] as $evolution_text)
		{
			if(!preg_match('#<m_type>[^<]+</m_type>#isU',$evolution_text))
				continue;
			$m_type=preg_replace('#^.*<m_type>([^<]+)</m_type>.*$#isU','$1',$evolution_text);
			if(!preg_match('#<m_level>[0-9]+</m_level>#isU',$evolution_text))
				continue;
			$m_level=preg_replace('#^.*<m_level>([0-9]+)</m_level>.*$#isU','$1',$evolution_text);
			if(!preg_match('#<m_evolveTo>[^<]+</m_evolveTo>#isU',$evolution_text))
				continue;
			if($m_type=='Level')
			{
				$m_type='level';
				if($m_level>100)
					continue;
			}
			else if($m_type=='Item')
			{
				$m_type='item';
				if(!preg_match('#<m_attribute>[^<]+</m_attribute>#isU',$evolution_text))
					continue;
				$m_level=preg_replace('#^.*<m_attribute>([^<]+)</m_attribute>.*$#isU','$1',$evolution_text);
				$m_level=str_replace(' ','',strtolower($m_level));
				if(!isset($item_name_to_id[simplifedName($m_level)]))
					continue;
				$m_level=$item_name_to_id[simplifedName($m_level)];
			}
			else if($m_type=='Trade')
			{
				$m_type='trade';
				$m_level='0';
			}
			/*else if($m_type=='TradeItem')
			{
				$m_type='tradeWithItem';
				if(!preg_match('#<m_attribute>[^<]+</m_attribute>#isU',$evolution_text))
					continue;
				$m_level=preg_replace('#^.*<m_attribute>([^<]+)</m_attribute>.*$#isU','$1',$evolution_text);
				$m_level=str_replace(' ','',strtolower($m_level));
				if(!isset($item_name_to_id[simplifedName($m_level)]))
					continue;
				$m_level=$item_name_to_id[simplifedName($m_level)];
			}*/
			/*else if($m_type=='Happiness' || $m_type=='HappinessDay' || $m_type=='HappinessNight')
			{
				$m_type='happiness';
				$m_level='3';
			}*/
			else
				continue;
			$m_evolveTo=preg_replace('#^.*<m_evolveTo>([^<]+)</m_evolveTo>.*$#isU','$1',$evolution_text);
			$evolutions[]=array(
				'type'=>$m_type,
				'level'=>$m_level,
				'evolveTo'=>$m_evolveTo,
			);
		}
	}

	if(isset($name_to_id[$name]))
	{
		$species[$name_to_id[$name]]=array_merge(array(
			'kind'=>$kind,
			'description'=>$description,
			'type'=>$type,
			'rareness'=>$rareness,
			'baseEXP'=>$baseEXP,
			'happiness'=>$happiness,
			'stepsToHatch'=>$stepsToHatch,
			'color'=>$color,
			'habitat'=>$habitat,
			'height'=>$height,
			'weight'=>$weight,
			'moves'=>$moves,
			'evolutions'=>$evolutions,
		),$species[$name_to_id[$name]]);
	}
	/*else
        echo 'Monster not found: '.$name."\n";*/
}

$file=file_get_contents('itemdrops.txt');
$entry_list=preg_split("#[\r\n\t]+#isU",$file);
foreach($entry_list as $entry)
{
	if(!preg_match('#^.*( [0-9]+ [0-9]+)+#isU',$entry))
		continue;
	$name=preg_replace('#^(.*)( [0-9]+ [0-9]+)+$#isU','$1',$entry);
	$rest=str_replace($name,'',$entry);
	$rest=preg_replace('#^ #','',$rest);
	$rest_list=preg_split("# +#isU",$rest);
	if((count($rest_list)%2)!=0)
		continue;
	if(isset($name_to_id[$name]))
	{
		$species[$name_to_id[$name]]['drops']=array();
		$index=0;
		while($index < count($rest_list))
		{
			$species[$name_to_id[$name]]['drops'][$rest_list[$index]]=$rest_list[$index+1];
			$index+=2;
		}
	}
}

foreach($species as $id=>$specie)
{
	if(isset($specie['name']) && isset($specie['height'])
 && (file_exists('monsters/'.$id.'/small.png') || file_exists('monsters/'.$id.'/small.gif'))
 && (file_exists('monsters/'.$id.'/front.png') || file_exists('monsters/'.$id.'/front.gif'))
 && (file_exists('monsters/'.$id.'/back.png') || file_exists('monsters/'.$id.'/back.gif'))
 && !isset($monster_meta[$id])
)
	{
	?>
	<monster id="<?php echo $id; ?>"<?php
$index=0;
while($index<count($specie['type']))
{
	if($index==0)
		echo ' type';
	else
		echo ' type'.($index+1);
	echo '="'.$specie['type'][$index].'"';
	$index++;
}

?> height="<?php echo $specie['height']; ?>m" weight="<?php echo $specie['weight']; ?>kg" ratio_gender="<?php echo $specie['gender']; ?>" catch_rate="100" egg_step="<?php echo $specie['stepsToHatch']; ?>" xp_max="300"
		hp="<?php echo $specie['base_hp']*5.0; ?>" attack="<?php echo $specie['base_attack']*5.0; ?>" defense="<?php echo $specie['base_defense']*5.0; ?>" special_attack="<?php echo $specie['base_spattack']*5.0; ?>" special_defense="<?php echo $specie['base_spdefense']*5.0; ?>" speed="<?php echo $specie['base_speed']*5.0; ?>" give_sp="100" give_xp="110" pow="1.2">
		<attack_list>
<?php
$temp_attack_list_duplicate=array();
$temp_move=$specie['moves'];
foreach($temp_move as $move)
{
	if(isset($movetypes_name_to_id[simplifedName($move[1])]))
		if(!isset($temp_attack_list_duplicate[$movetypes_name_to_id[simplifedName($move[1])]]))
		{
			$temp_attack_list_duplicate[$movetypes_name_to_id[simplifedName($move[1])]]=0;
			echo '			<attack level="'.$move[0].'" id="'.$movetypes_name_to_id[simplifedName($move[1])].'" />'."\n";
		}
}
?>
		</attack_list>
<?php
if(isset($specie['drops']) && count($specie['drops'])>0)
{
?>
		<drops>
<?php
$temp_drops=$specie['drops'];
foreach($temp_drops as $item=>$luck)
	if(isset($item_meta[$item]))
		echo '			<drop item="'.$item.'" luck="'.$luck.'" />'."\n";
?>
		</drops>
<?php
}
?>
<?php
if(isset($specie['evolutions']) && count($specie['evolutions'])>0)
{
?>
		<evolutions>
<?php
$temp_evolutions=$specie['evolutions'];
foreach($temp_evolutions as $evolution)
{
	$level='';
	if($evolution['type']!='trade')
		$level=' level="'.$evolution['level'].'"';
	if(isset($name_to_id[$evolution['evolveTo']]))
		echo '			<evolution type="'.$evolution['type'].'"'.$level.' evolveTo="'.$name_to_id[$evolution['evolveTo']].'" />'."\n";
	else
		echo '			<evolution type="'.$evolution['type'].'"'.$level.' evolveTo="'.$evolution['evolveTo'].'" />'."\n";
}
?>
		</evolutions>
<?php
}
?>
		<name><?php echo $specie['name']; ?></name>
		<kind><?php echo $specie['kind']; ?></kind>
		<habitat><?php echo $specie['habitat']; ?></habitat>
		<description><?php echo $specie['description']; ?></description>
	</monster>
<?php
	}
	else
	{
        /*if(!isset($specie['name']))
            echo 'No name: '.$id;
        if(!isset($specie['height']))
            echo 'No height: '.$id;
        if(!(file_exists('monsters/'.$id.'/small.png') || file_exists('monsters/'.$id.'/small.gif')))
            echo 'No small.png: '.$id;*/
        /*if(isset($monster_meta[$id]))
            echo 'No $monster_meta: '.$id;*/
	}
}

echo "</monsters>\n";
