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
<list>\n";


$file=file_get_contents('items/items.xml');
preg_match_all('#<entry>(.*)</entry>#isU',$file,$entry_list);
$general_items_list=array();
$item_name_to_id=array();
foreach($entry_list[1] as $entry)
{
	if(!preg_match('#<m_id>[0-9]+</m_id>#isU',$entry))
		continue;
	$id=preg_replace('#^.*<m_id>([0-9]+)</m_id>.*$#isU','$1',$entry);
	if(!preg_match('#<m_name>[^<]+</m_name>#isU',$entry))
		continue;
	$name=preg_replace('#^.*<m_name>([^<]+)</m_name>.*$#isU','$1',$entry);
	$general_items_list[$id]=array('name'=>$name);
	$item_name_to_id[str_replace(' ','',strtolower($name))]=$id;
}

$file=file_get_contents('battle/movetypes.xml');
preg_match_all('#<entry>(.*)</entry>#isU',$file,$entry_list);
$movetypes=array();
$movetypes_name_to_id=array();
$movetypes_nameupper_to_name=array();
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
	$type=preg_replace('#^.*<type>([^<]+)</type>.*$#isU','$1',$entry);
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
	$movetypes_nameupper_to_name[strtoupper($name)]=$name;
	$movetypes_name_to_id[$name]=$id;
}

$species=array();
$name_to_id=array();
$name_upper_to_name=array();

$dir = 'maps/';
if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#-?[0-9]{1,2}.-?[0-9]{1,2}\.tmx#',$file))
	{
		$water=array();
		$grass=array();
		$grassNight=array();
		$fish=array();
		$content=file_get_contents($dir.$file);
		preg_match_all('#<property name="([^"]+)" value="([^"]+)"#isU',$content,$entry_list);
		$property=array();
		foreach($entry_list[1] as $base_key => $value)
			$property[$value]=$entry_list[2][$base_key];
		if(isset($property['dayPokemonLevels']) && isset($property['dayPokemonChances']))
		{
			$dayPokemonLevelsList=preg_split('#;#',$property['dayPokemonLevels']);
			$dayPokemonChancesList=preg_split('#;#',$property['dayPokemonChances']);
			if(count($dayPokemonLevelsList)>0 && count($dayPokemonLevelsList)==count($dayPokemonChancesList))
			{
				$total_luck=0;
				$index=0;
				while($index<count($dayPokemonLevelsList))
				{
					$tempSplit=preg_split('#-#',$dayPokemonLevelsList[$index]);
					if(count($tempSplit)!=2)
					{
						$index++;
						continue;
					}
					$minLevel=$tempSplit[0];
					$maxLevel=$tempSplit[1];
					$tempSplit=preg_split('#,#',$dayPokemonChancesList[$index]);
					if(count($tempSplit)!=2)
					{
						$index++;
						continue;
					}
					$name=$tempSplit[0];
					$luck=$tempSplit[1];
					$name_upper_to_name[strtoupper($name)]=$name;
					$index++;
				}
				if(count($grass)>0 && $total_luck!=100)
					$grass[0]['luck']+=100-$total_luck;
			}
		}
		if(isset($property['nightPokemonLevels']) && isset($property['nightPokemonChances']))
		{
			$nightPokemonLevelsList=preg_split('#;#',$property['nightPokemonLevels']);
			$nightPokemonChancesList=preg_split('#;#',$property['nightPokemonChances']);
			if(count($nightPokemonLevelsList)>0 && count($nightPokemonLevelsList)==count($nightPokemonChancesList))
			{
				$total_luck=0;
				$index=0;
				while($index<count($nightPokemonLevelsList))
				{
					$tempSplit=$tempSplit=preg_split('#-#',$nightPokemonLevelsList[$index]);
					if(count($tempSplit)!=2)
					{
						$index++;
						continue;
					}
					$minLevel=$tempSplit[0];
					$maxLevel=$tempSplit[1];
					$tempSplit=$tempSplit=preg_split('#,#',$nightPokemonChancesList[$index]);
					if(count($tempSplit)!=2)
					{
						$index++;
						continue;
					}
					$name=$tempSplit[0];
					$luck=$tempSplit[1];
					$name_upper_to_name[strtoupper($name)]=$name;
					$index++;
				}
				if(count($grassNight)>0 && $total_luck!=100)
					$grassNight[0]['luck']+=100-$total_luck;
			}
		}
		if(isset($property['waterPokemonLevels']) && isset($property['waterPokemonChances']))
		{
			$waterPokemonLevelsList=preg_split('#;#',$property['waterPokemonLevels']);
			$waterPokemonChancesList=preg_split('#;#',$property['waterPokemonChances']);
			if(count($waterPokemonLevelsList)>0 && count($waterPokemonLevelsList)==count($waterPokemonChancesList))
			{
				$total_luck=0;
				$index=0;
				while($index<count($waterPokemonLevelsList))
				{
					$tempSplit=$tempSplit=preg_split('#-#',$waterPokemonLevelsList[$index]);
					if(count($tempSplit)!=2)
					{
						$index++;
						continue;
					}
					$minLevel=$tempSplit[0];
					$maxLevel=$tempSplit[1];
					$tempSplit=$tempSplit=preg_split('#,#',$waterPokemonChancesList[$index]);
					if(count($tempSplit)!=2)
					{
						$index++;
						continue;
					}
					$name=$tempSplit[0];
					$luck=$tempSplit[1];
					$name_upper_to_name[strtoupper($name)]=$name;
					$index++;
				}
				if(count($water)>0 && $total_luck!=100)
					$water[0]['luck']+=100-$total_luck;
			}
		}
		if(isset($property['fishPokemonLevels']) && isset($property['fishPokemonChances']))
		{
			$fishPokemonLevelsList=preg_split('#;#',$property['fishPokemonLevels']);
			$fishPokemonChancesList=preg_split('#;#',$property['fishPokemonChances']);
			if(count($fishPokemonLevelsList)>0 && count($fishPokemonLevelsList)==count($fishPokemonChancesList))
			{
				$total_luck=0;
				$index=0;
				while($index<count($fishPokemonLevelsList))
				{
					$tempSplit=$tempSplit=preg_split('#-#',$fishPokemonLevelsList[$index]);
					if(count($tempSplit)!=2)
					{
						$index++;
						continue;
					}
					$minLevel=$tempSplit[0];
					$maxLevel=$tempSplit[1];
					$tempSplit=$tempSplit=preg_split('#,#',$fishPokemonChancesList[$index]);
					if(count($tempSplit)!=2)
					{
						$index++;
						continue;
					}
					$name=$tempSplit[0];
					$luck=$tempSplit[1];
					$name_upper_to_name[strtoupper($name)]=$name;
					$index++;
				}
				if(count($fish)>0 && $total_luck!=100)
					$fish[0]['luck']+=100-$total_luck;
			}
		}
		unset($property);
	}
    }
    closedir($dh);
}

$file=file_get_contents('pokemon.ini');
preg_match_all('#(\[[0-9]+\].*)BattlerAltitude=[0-9]+#isU',$file,$entry_list);
foreach($entry_list[1] as $entry)
{
	if(!preg_match('#\[[0-9]+\]#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$id=preg_replace('#^.*\[([0-9]+)\].*$#isU','$1',$entry);
	$id=preg_replace('#^.*\[([0-9]+)\].*$#isU','$1',$entry);
	$id=preg_replace("#[\r\t\n ]+$#isU",'',$id);
	if(!preg_match('#Name='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$name=preg_replace('#^.*Name=('."[^\n\r]*".')'."[\n\r]".'.*$#isU','$1',$entry);
	$name=preg_replace('#POK.{1,3}MON#','POKEMON',$name);
	$name=preg_replace("#[\r\t\n ]+$#isU",'',$name);
	$name=htmlentities($name);
	$name_raw=$name;
	if(isset($name_upper_to_name[$name]))
		$name=$name_upper_to_name[$name];
	else
		$name=ucfirst(strtolower($name));
	if(!preg_match('#Kind='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$kind=ucfirst(strtolower(preg_replace('#^.*Kind=('."[^\n\r]*".')'."[\n\r]".'.*$#isU','$1',$entry)));
	$kind=preg_replace("#[\r\t\n ]+$#isU",'',$kind);
	if(!preg_match('#Pokedex='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$description=htmlentities($description);
	$description=preg_replace('#^.*Pokedex=('."[^\n\r]*".')'."[\n\r]".'.*$#isU','$1',$entry);
	$description=preg_replace("#[\r\t\n]+#isU",' ',$description);
	$description=preg_replace("#[\r\t\n ]+$#isU",'',$description);
	$description=preg_replace('# +#',' ',$description);
	$description=preg_replace('#POK.{1,3}MON#','POKEMON',$description);
	$description=str_replace($name_raw,$name,$description);
	if(!preg_match('#Type[0-9]='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	preg_match_all('#Type[0-9]=('."[^\n\r]*".')'."[\n\r]".'#isU',$entry,$entry_list_type);
	if(!preg_match('#Rareness=[0-9]+'."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$rareness=preg_replace('#^.*Rareness=([0-9]+)'."[\n\r]".'.*$#isU','$1',$entry);
	$rareness=preg_replace("#[\r\t\n ]+$#isU",'',$rareness);
	if(!preg_match('#BaseEXP=[0-9]+'."[\n\r]".'#isU',$entry))
		$baseEXP=preg_replace('#^.*BaseEXP=([0-9]+)'."[\n\r]".'.*$#isU','$1',$entry);
	else
		$baseEXP=0;
	$baseEXP=preg_replace("#[\r\t\n ]+$#isU",'',$baseEXP);
	if(!preg_match('#GenderRate='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
	{
		$baseGender=preg_replace('#^.*GenderRate=('."[^\n\r]*".')'."[\n\r]".'.*$#isU','$1',$entry);
		$baseGender=preg_replace("#[\r\t\n ]+$#isU",'',$baseGender);
		switch($baseGender)
		{
			case 'AlwaysMale':
				$gender='0%';
			break;
			case 'FemaleOneEighth':
				$gender='20%';
			break;
			case 'Female25Percent':
				$gender='25%';
			break;
			default:
			case 'Female50Percent':
				$gender='50%';
			break;
			case 'Female75Percent':
				$gender='75%';
			break;
			case 'AlwaysFemale':
				$gender='100%';
			break;
			case 'Genderless':
				$gender='-1';
			break;
		}
	}
	else
		$gender='50%';
	if(preg_match('#Happiness=[0-9]+'."[\n\r]".'#isU',$entry))
	{
		$happiness=preg_replace('#^.*Happiness=([0-9]+)'."[\n\r]".'.*$#isU','$1',$entry);
		$happiness=preg_replace("#[\r\t\n ]+$#isU",'',$happiness);
	}
	else
		$happiness=70;
	if(preg_match('#StepsToHatch=[0-9]+'."[\n\r]".'#isU',$entry))
	{
		$stepsToHatch=preg_replace('#^.*StepsToHatch=([0-9]+)'."[\n\r]".'.*$#isU','$1',$entry);
		$stepsToHatch=preg_replace("#[\r\t\n ]+$#isU",'',$stepsToHatch);
	}
	else
		$stepsToHatch=0;
	if(preg_match('#Color='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
	{
		$color=preg_replace('#^.*Color=('."[^\n\r]*".')'."[\n\r]".'.*$#isU','$1',$entry);
		$color=preg_replace("#[\r\t\n ]+$#isU",'',$color);
	}
	else
		$color='';
	if(preg_match('#Habitat='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
	{
		$habitat=preg_replace('#^.*Habitat=('."[^\n\r]*".')'."[\n\r]".'.*$#isU','$1',$entry);
		$habitat=preg_replace("#[\r\t\n ]+$#isU",'',$habitat);
	}
	else
		$habitat='';
	if(!preg_match('#BaseStats='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$tempBaseStats=preg_split('#,#',preg_replace("#[\r\t\n ]+$#isU",'',preg_replace('#^.*BaseStats=('."[^\n\r]*".')'."[\n\r]".'.*$#isU','$1',$entry)),-1,PREG_SPLIT_NO_EMPTY);
	if(count($tempBaseStats)!=6)
		die('Bug ligne: '.__LINE__);//continue;
	$base_hp=$tempBaseStats[0];
	$base_attack=$tempBaseStats[1];
	$base_defense=$tempBaseStats[2];
	$base_speed=$tempBaseStats[3];
	$base_spattack=$tempBaseStats[4];
	$base_spdefense=$tempBaseStats[5];
	if(!preg_match('#Height=[0-9]+(\.[0-9]+)?'."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$height=preg_replace('#^.*Height=([0-9]+(\.[0-9]+)?)'."[\n\r]".'.*$#isU','$1',$entry);
	$height=preg_replace("#[\r\t\n ]+$#isU",'',$height);
	if(!preg_match('#Weight=[0-9]+(\.[0-9]+)?'."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$weight=preg_replace('#^.*Weight=([0-9]+(\.[0-9]+)?)'."[\n\r]".'.*$#isU','$1',$entry);
	$weight=preg_replace("#[\r\t\n ]+$#isU",'',$weight);

	if(!preg_match('#Moves='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$temp_move=preg_replace('#^.*Moves=([^\n\r]*)'."[\n\r]".'.*$#isU','$1',$entry);
	$temp_move=preg_replace("#[\r\t\n ]+$#isU",'',$temp_move);
	$entry_list_moves=preg_split('#,#',$temp_move,-1,PREG_SPLIT_NO_EMPTY);
	if((count($entry_list_moves)%2)!=0)
		die('Bug ligne: '.__LINE__);//continue;
	$moves=array();
	$index=0;
	while($index<count($entry_list_moves))
	{
		$level=$entry_list_moves[$index];
		if(isset($movetypes_nameupper_to_name[$entry_list_moves[$index+1]]))
			$string=$movetypes_nameupper_to_name[$entry_list_moves[$index+1]];
		else
			$string=ucwords(strtolower($entry_list_moves[$index+1]));
		$moves[]=array($level,$string);
		$index+=2;
	}

	$evolutions=array();
	if(!preg_match('#Evolutions='."[^\n\r]*".''."[\n\r]".'#isU',$entry))
		die('Bug ligne: '.__LINE__);//continue;
	$temp_evolutions=preg_replace('#^.*Evolutions=('."[^\n\r]*".')'."[\n\r]".'.*$#isU','$1',$entry);
	$temp_evolutions=preg_replace("#[\r\t\n ]+$#isU",'',$temp_evolutions);
	$entry_list_evolutions=preg_split('#,#',$temp_evolutions,-1,PREG_SPLIT_NO_EMPTY);
	if((count($entry_list_evolutions)%3)==0)
	{
		$index=0;
		while($index<count($entry_list_evolutions))
		{
			$m_type=$entry_list_evolutions[$index+1];
			$m_level=$entry_list_evolutions[$index+2];
			if(isset($name_upper_to_name[$entry_list_evolutions[$index]]))
				$m_evolveTo=$name_upper_to_name[$entry_list_evolutions[$index]];
			else
				$m_evolveTo=ucfirst(strtolower($entry_list_evolutions[$index]));
			if($m_type=='Level')
			{
				$m_type='level';
				if($m_level>100)
				{
					$index+=3;
					continue;
				}
			}
			else if($m_type=='Item')
			{
				$m_type='item';
				$m_level=str_replace(' ','',strtolower($m_level));
				if(!isset($item_name_to_id[$m_level]))
				{
					$index+=3;
					continue;
				}
				$m_level=$item_name_to_id[$m_level];
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
				{
					$index+=3;
					continue;
				}
				$m_level=preg_replace('#^.*<m_attribute>([^<]+)</m_attribute>.*$#isU','$1',$evolution_text);
				$m_level=str_replace(' ','',strtolower($m_level));
				if(!isset($item_name_to_id[$m_level]))
				{
					$index+=3;
					continue;
				}
				$m_level=$item_name_to_id[$m_level];
			}*/
			/*else if($m_type=='Happiness' || $m_type=='HappinessDay' || $m_type=='HappinessNight')
			{
				$m_type='happiness';
				$m_level='3';
			}*/
			else
			{
				$index+=3;
				continue;
			}
			$evolutions[]=array(
				'type'=>$m_type,
				'level'=>$m_level,
				'evolveTo'=>$m_evolveTo,
			);
			$index+=3;
		}
	}

	$type=array();
	$index=0;
	while($index<count($entry_list_type[1]))
	{
		$type[]=strtolower($entry_list_type[1][$index]);
		$index++;
	}

	$species[$id]=array(
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
	$name_to_id[str_replace(' ','',strtolower($name))]=$id;
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
	if(isset($name_to_id[str_replace(' ','',strtolower($name))]))
	{
		$monster_id=$name_to_id[str_replace(' ','',strtolower($name))];
		$species[$monster_id]['drops']=array();
		$index=0;
		while($index < count($rest_list))
		{
			$species[$monster_id]['drops'][$rest_list[$index]]=$rest_list[$index+1];
			$index+=2;
		}
	}
}

foreach($species as $id=>$specie)
{
	$temp_id=str_pad($id, 3, '0', STR_PAD_LEFT);
	if(isset($specie['name']) && isset($specie['height'])
 && (file_exists('pokemon/back/normal/'.$temp_id.'-0.png') || file_exists('pokemon/back/normal/'.$temp_id.'-0.gif'))
 && (file_exists('pokemon/front/normal/'.$temp_id.'-2.png') || file_exists('pokemon/front/normal/'.$temp_id.'-2.gif'))
 && (file_exists('pokemon/icons/'.$temp_id.'.png') || file_exists('pokemon/icons/'.$temp_id.'.gif'))
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
	if(isset($movetypes_name_to_id[$move[1]]))
		if(!isset($temp_attack_list_duplicate[$movetypes_name_to_id[$move[1]]]))
		{
			$temp_attack_list_duplicate[$movetypes_name_to_id[$move[1]]]=0;
			echo '			<attack level="'.$move[0].'" id="'.$movetypes_name_to_id[$move[1]].'" />'."\n";
		}
}
?>
		</attack_list>
<?php
if(isset($specie['drops']))
{
?>
		<drops>
<?php
$temp_drops=$specie['drops'];
foreach($temp_drops as $item=>$luck)
	if(isset($general_items_list[$item]))
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
	if(isset($name_to_id[str_replace(' ','',strtolower($evolution['evolveTo']))]))
		echo '			<evolutions type="'.$evolution['type'].'"'.$level.' evolveTo="'.$name_to_id[str_replace(' ','',strtolower($evolution['evolveTo']))].'" />'."\n";
	else
		echo '			<evolutions type="'.$evolution['type'].'"'.$level.' evolveTo="'.$evolution['evolveTo'].'" />'."\n";
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
}

echo "</list>\n";
