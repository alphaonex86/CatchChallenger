<?php

function filewrite($file,$content)
{
	if($filecurs=fopen($file, 'w'))
	{
		if(fwrite($filecurs,$content) === false)
			die('Unable to write the file: '.$file);
		fclose($filecurs);
	}
	else
		die('Unable to write or create the file: '.$file);
}

$dir = "map/";
$bot_id=1;

$file=file_get_contents('items/items.xml');
preg_match_all('#<entry>(.*)</entry>#isU',$file,$entry_list);
$general_items_list=array();
foreach($entry_list[1] as $entry)
{
	if(!preg_match('#<m_id>[0-9]+</m_id>#isU',$entry))
		continue;
	$id=preg_replace('#^.*<m_id>([0-9]+)</m_id>.*$#isU','$1',$entry);
	if(!preg_match('#<m_category>[^<]+</m_category>#isU',$entry))
		continue;
	$category=preg_replace('#^.*<m_category>([^<]+)</m_category>.*$#isU','$1',$entry);
	if($category!='TM')
		$image=$id.'.png';
	else
		$image='TM.png';
	if(file_exists($image))
	{
		if(!preg_match('#<m_name>[^<]+</m_name>#isU',$entry))
			continue;
		$name=preg_replace('#^.*<m_name>([^<]+)</m_name>.*$#isU','$1',$entry);
		if(!preg_match('#<m_description>[^<]+</m_description>#isU',$entry))
			continue;
		$description=preg_replace('#^.*<m_description>([^<]+)</m_description>.*$#isU','$1',$entry);
		$description=preg_replace("#[\n\r\t]+.*$#isU",'',$description);
		if(!preg_match('#<m_price>[0-9]+</m_price>#isU',$entry))
			continue;
		$price=preg_replace('#^.*<m_price>([0-9]+)</m_price>.*$#isU','$1',$entry);
		$general_items_list[$id]=array('name'=>$name,'description'=>$description,'price'=>$price);
	}
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

$name_to_id=array();
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
	$name_to_id[$name]=$id;
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

$map_file_to_name=array();
$content=preg_split("#[\r\n]+#isU",file_get_contents('language/english/_MAPNAMES.txt'));
$index=0;
while($index<count($content))
{
	$values=preg_split('#,#',$content[$index]);
	if(count($values)==3)
	{
		$values[2]=str_replace('Route ','Road ',$values[2]);
		$map_file_to_name[$values[0].'.'.$values[1].'.tmx']=$values[2];
	}
	$index++;
}

if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#-?[0-9]{1,2}.-?[0-9]{1,2}\.tmx#',$file))
	{
		$water=array();
		$cave=array();
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
					if(!isset($name_to_id[$name]))
					{
						$index++;
						continue;
					}
					$id=$name_to_id[$name];
					$total_luck+=$luck;
					if(isset($property['isCave']) && $property['isCave']=='true')
						$cave[]=array('minLevel'=>$minLevel,'maxLevel'=>$maxLevel,'id'=>$id,'luck'=>$luck);
					else
						$grass[]=array('minLevel'=>$minLevel,'maxLevel'=>$maxLevel,'id'=>$id,'luck'=>$luck);
					$index++;
				}
				if(isset($property['isCave']) && $property['isCave']=='true')
				{
					if(count($cave)>0 && $total_luck!=100)
						$cave[0]['luck']+=100-$total_luck;
				}
				else
				{
					if(count($grass)>0 && $total_luck!=100)
						$grass[0]['luck']+=100-$total_luck;
				}
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
					if(!isset($name_to_id[$name]))
					{
						$index++;
						continue;
					}
					$id=$name_to_id[$name];
					$total_luck+=$luck;
					$grassNight[]=array('minLevel'=>$minLevel,'maxLevel'=>$maxLevel,'id'=>$id,'luck'=>$luck);
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
					if(!isset($name_to_id[$name]))
					{
						$index++;
						continue;
					}
					$id=$name_to_id[$name];
					$total_luck+=$luck;
					$water[]=array('minLevel'=>$minLevel,'maxLevel'=>$maxLevel,'id'=>$id,'luck'=>$luck);
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
					if(!isset($name_to_id[$name]))
					{
						$index++;
						continue;
					}
					$id=$name_to_id[$name];
					$total_luck+=$luck;
					$fish[]=array('minLevel'=>$minLevel,'maxLevel'=>$maxLevel,'id'=>$id,'luck'=>$luck);
					$index++;
				}
				if(count($fish)>0 && $total_luck!=100)
					$fish[0]['luck']+=100-$total_luck;
			}
		}
		if(isset($property['isCave']) && $property['isCave']=='true')
			$type='cave';
		else if(isset($property['pvp']) && $property['pvp']=='disabled' && count($grass)==0 && count($cave)==0 && count($grassNight)==0)
			$type='city';
		else
			$type='outdoor';
		$xmlcontent='<map type="'.$type.'">'."\n";
		if(isset($map_file_to_name[$file]))
			$xmlcontent.='	<name>'.$map_file_to_name[$file].'</name>'."\n";
		if(count($cave)>0)
		{
			$xmlcontent.='	<cave>'."\n";
			foreach($cave as $monster)
			{
				if($monster['minLevel']==$monster['maxLevel'])
					$xmlcontent.='		<monster level="'.$monster['minLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
				else
					$xmlcontent.='		<monster minLevel="'.$monster['minLevel'].'" maxLevel="'.$monster['maxLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
			}
			$xmlcontent.='	</cave>'."\n";
		}
		if(count($grass)>0)
		{
			$xmlcontent.='	<grass>'."\n";
			foreach($grass as $monster)
			{
				if($monster['minLevel']==$monster['maxLevel'])
					$xmlcontent.='		<monster level="'.$monster['minLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
				else
					$xmlcontent.='		<monster minLevel="'.$monster['minLevel'].'" maxLevel="'.$monster['maxLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
			}
			$xmlcontent.='	</grass>'."\n";
		}
		if(count($grassNight)>0)
		{
			$xmlcontent.='	<grassNight>'."\n";
			foreach($grassNight as $monster)
			{
				if($monster['minLevel']==$monster['maxLevel'])
					$xmlcontent.='		<monster level="'.$monster['minLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
				else
					$xmlcontent.='		<monster minLevel="'.$monster['minLevel'].'" maxLevel="'.$monster['maxLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
			}
			$xmlcontent.='	</grassNight>'."\n";
		}
		if(count($water)>0)
		{
			$xmlcontent.='	<water>'."\n";
			foreach($water as $monster)
			{
				if($monster['minLevel']==$monster['maxLevel'])
					$xmlcontent.='		<monster level="'.$monster['minLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
				else
					$xmlcontent.='		<monster minLevel="'.$monster['minLevel'].'" maxLevel="'.$monster['maxLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
			}
			$xmlcontent.='	</water>'."\n";
		}
		if(count($fish)>0)
		{
			$xmlcontent.='	<fish>'."\n";
			foreach($fish as $monster)
			{
				if($monster['minLevel']==$monster['maxLevel'])
					$xmlcontent.='		<monster level="'.$monster['minLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
				else
					$xmlcontent.='		<monster minLevel="'.$monster['minLevel'].'" maxLevel="'.$monster['maxLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
			}
			$xmlcontent.='	</fish>'."\n";
		}
		$xmlcontent.='</map>';
		$filexml=str_replace('.tmx','.xml',$file);
		preg_match_all('#<layer[^>]*>'."[ \r\n\t]+".'<data[^>]*>'."[ \r\n\t]+(".preg_quote('H4sIAAAAAAAAAO3BMQEAAADCoPVPbQwfoAAAAAC+BmbyAUigEAAA')."|".preg_quote('eJztwQEBAAAAgiD/r25IQAEAAPBoDhAAAQ==')."|".preg_quote('eNrtwTEBAAAAwqD1T20MH6AAAAAAvgYQoAAB').")[ \r\n\t]+".'</data>'."[ \r\n\t]+".'</layer>#isU',$content,$empty_layer);
		foreach($empty_layer as $layer_content)
			$content=str_replace($layer_content,'',$content);
		filewrite($dir.$filexml,$xmlcontent);
		filewrite($dir.$file,$content);
		$temptxtfile='language/english/NPC/'.str_replace('.tmx','.txt',$file);
		if(file_exists($temptxtfile))
		{
			$content=preg_split("#[\r\n]+#isU",file_get_contents($temptxtfile),-1,PREG_SPLIT_NO_EMPTY);
			if(count($content)>0)
			{
				$xmlcontent="<bots>\n";
				$index=0;
				while($index<count($content))
				{
					$text=$content[$index];
					$text=preg_replace("#[\r\t\n]+#isU",' ',$text);
					$text=preg_replace("#[\r\t\n ]+$#isU",'',$text);
					$text=preg_replace('# +#',' ',$text);
					$xmlcontent.='	<bot id="'.$bot_id.'">'."\n";
					$xmlcontent.='		<step id="1" type="text">'."\n";
					$xmlcontent.='			<text><![CDATA['.$text.']]></text>'."\n";
					$xmlcontent.='		</step>'."\n";
					$xmlcontent.='	</bot>'."\n";
					$bot_id++;
					$index++;
				}
				$xmlcontent.='</bots>';
				filewrite($dir.''.str_replace('.tmx','-bots.xml',$file),$xmlcontent);
			}
		}
		unset($property);
	}
    }
    closedir($dh);
}
