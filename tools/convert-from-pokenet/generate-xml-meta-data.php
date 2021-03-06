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

//clean text and return string to "toto toto2 toto3", the word lower than X value are dropped
function text_operation_clean_text($text,$minimum_word_length=4,$minimum_string_length=15,$maximum_string_length=64)
{
	$text=text_operation_lower_case($text);

	$text=str_replace('â','a',str_replace('à','a',$text));
	$text=str_replace('ã','a',str_replace('á','a',str_replace('ã','a',str_replace('ä','a',$text))));

	$text=str_replace('ç','c',$text);

	$text=str_replace('é','e',str_replace('è','e',str_replace('ê','e',str_replace('ë','e',$text))));

	$text=str_replace('ì','i',str_replace('í','i',str_replace('î','i',str_replace('ï','i',$text))));

	$text=str_replace('ñ','n',$text);
	
	$text=str_replace('õ','o',str_replace('ö','o',str_replace('ó','o',str_replace('ô','o',$text))));
	$text=str_replace('ô','o',str_replace('ò','o',$text));
	
	$text=str_replace('û','u',str_replace('ü','u',str_replace('ú','u',str_replace('ù','u',$text))));
	
	$text=str_replace('ý','y',str_replace('ÿ','y',$text));
	
	if(strlen($text)>$maximum_string_length)
		$text=substr($text,0,$maximum_string_length);
	$text=preg_replace('#([0-9]+)(\.|-| )+#','$1 ',$text);
	$text=preg_replace('#[^a-zA-Z0-9_-]+#',' ',$text);
	$text=preg_replace('# +#',' ',$text);
	if($minimum_word_length>2)
	{
		$a=$minimum_word_length-1;
		do
		{
			$text_temp=preg_replace('#\b[a-zA-Z_-]{1,'.$a.'}\b#',' ',$text);
			$text_temp=preg_replace('# +#',' ',$text_temp);
			$text_temp=preg_replace('# +$#','',$text_temp);
			$text_temp=preg_replace('#^ +#','',$text_temp);
			$a--;
		}
		while(strlen($text_temp)<=$minimum_string_length && $a>1);
		if(strlen($text_temp)>$minimum_string_length && $a>1)
			$text=$text_temp;
	}
	$text=preg_replace('# +#',' ',$text);
	$text=preg_replace('# +$#','',$text);
	$text=preg_replace('#^ +#','',$text);
	return $text;
}

function text_operation_do_for_url($text,$minimum_word_length=4,$minimum_string_length=15,$maximum_string_length=64)
{
	$text=text_operation_clean_text($text,$minimum_word_length,$minimum_string_length,$maximum_string_length);
	$text=str_replace(' ','-',$text);
	$text=preg_replace('#-+#','-',$text);
	$text=preg_replace('#^-+#','',$text);
	$text=preg_replace('#-+$#','',$text);
	return $text;
}

function text_operation_lower_case($text)
{
	$text=strtolower($text);
	$text=str_replace('Â','â',$text);
	$text=str_replace('À','à',$text);
	$text=str_replace('Ä','ä',$text);
	$text=str_replace('Ç','ç',$text);
	$text=str_replace('Ê','ê',$text);
	$text=str_replace('È','è',$text);
	$text=str_replace('Ë','ë',$text);
	$text=str_replace('É','é',$text);
	$text=str_replace('Ï','ï',$text);
	$text=str_replace('Ö','ö',$text);
	$text=str_replace('Ô','ô',$text);
	$text=str_replace('Û','û',$text);
	$text=str_replace('Ù','ù',$text);
	$text=str_replace('Ü','ü',$text);
	return $text;
}

$dir = "./";

$file=file_get_contents('../species.xml');
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

/// \note this function require -text-operation.php
function url_remake_for_url($text,$minimum_word_length=4,$minimum_string_length=15,$maximum_string_length=64)
{
	$url=text_operation_clean_text($text,$minimum_word_length,$minimum_string_length,$maximum_string_length);
	$url=str_replace(' ','-',$url);
	$url=preg_replace('#-+#','-',$url);
	$url=preg_replace('#-+$#','',$url);
	$url=preg_replace('#^-+#','',$url);
	return $url;
}

$file=file_get_contents('../polrdb.xml');
preg_match_all('#<POLRDataEntry>(.*)</POLRDataEntry>#isU',$file,$entry_list);
foreach($entry_list[1] as $entry)
{
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
	if(!preg_match('#<habitat>[^<]+</habitat>#isU',$entry))
		continue;
	$habitat=preg_replace('#^.*<habitat>([^<]+)</habitat>.*$#isU','$1',$entry);
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
	if(!preg_match('#<starterMoves>.*</starterMoves>#isU',$entry))
		continue;
	$temp_starterMoves=preg_replace('#^.*<starterMoves>(.*)</starterMoves>.*$#isU','$1',$entry);
	preg_match_all('#<string>(.*)</string>#isU',$temp_starterMoves,$temp_starterMoves_list);
	foreach($temp_starterMoves_list[1] as $starterMove)
		$moves[]=array(0,$starterMove);
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
			$m_evolveTo=preg_replace('#^.*<m_evolveTo>([^<]+)</m_evolveTo>.*$#isU','$1',$evolution_text);
			$evolutions[]=array(
				'type'=>$m_type,
				'level'=>$m_level,
				'evolveTo'=>$m_evolveTo,
			);
		}
	}

	$type=$entry_list_type[1];
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
}

$map_file_to_nameEN=array();
if(file_exists('../language/english/_MAPNAMES.txt'))
{
    $content=preg_split("#[\r\n]+#isU",file_get_contents('../language/english/_MAPNAMES.txt'));
    $index=0;
    while($index<count($content))
    {
        $values=preg_split('#,#',$content[$index]);
        if(count($values)==3)
        {
            //$values[2]=str_replace('Route ','Road ',$values[2]);
            $map_file_to_nameEN[$values[0].'.'.$values[1].'.tmx']=preg_replace("#[ \n\r\t]+$#",'',$values[2]);
        }
        $index++;
    }
}
else
    echo '../language/english/_MAPNAMES.txt not found'."\n";

$dirtyNameToClean=array();
$cleanNameToDirty=array();
//clean the file name
if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#-?[0-9]{1,2}.-?[0-9]{1,2}\.tmx$#',$file))
	{
        $content=file_get_contents($file);
		preg_match_all('#source="([^"]+)"#isU',$content,$entry_list);
		foreach($entry_list[1] as $base_key => $value)
		{
            if(preg_match('#\.tsx$#i',$value))
                $ext='tsx';
            else if(preg_match('#\.png$#i',$value))
                $ext='png';
            else
                die('unknown file type: '.$value);
            $valueClean=preg_replace('#\.'.$ext.'$#i','',$value);
            $cleanName=url_remake_for_url($valueClean,0);
            $cleanName.='.'.$ext;
            if(isset($dirtyNameToClean[$value]) && $cleanName!=$dirtyNameToClean[$value])
                die('1 isset($dirtyNameToClean[$value]) && $cleanName!=$dirtyNameToClean[$value]: '.$value.' -> '.$cleanName.' for '.$file.' =! '.$dirtyNameToClean[$value]);
            $content=str_replace($value,$cleanName,$content);
            $dirtyNameToClean[$value]=$cleanName;
            $cleanNameToDirty[$cleanName]=$value;
        }

        filewrite($file,$content);
        //extra for tmx
        $dirtyNameToClean[$file]=$file;
        $cleanNameToDirty[$file]=$file;
	}
	elseif(preg_match('#\.tsx$#',$file))
	{
        $content=file_get_contents($file);
        preg_match_all('#source="([^"]+)"#isU',$content,$entry_list);
		foreach($entry_list[1] as $base_key => $value)
		{
            if(preg_match('#\.png$#i',$value))
                $ext='png';
            else
                die('unknown file type: '.$value);
            $valueClean=preg_replace('#\.'.$ext.'$#i','',$value);
            $cleanName=url_remake_for_url($valueClean,0);
            $cleanName.='.png';
            if(isset($dirtyNameToClean[$value]) && $cleanName!=$dirtyNameToClean[$value])
                die('2 isset($dirtyNameToClean[$value]) && $cleanName!=$dirtyNameToClean[$value]: '.$value.' -> '.$cleanName.' for '.$file.' =! '.$dirtyNameToClean[$value]);
            $content=str_replace($value,$cleanName,$content);
            $dirtyNameToClean[$value]=$cleanName;
            $cleanNameToDirty[$cleanName]=$value;
        }
        
        $fileTemp=str_replace('.tsx','',$file);
        $cleanFile=url_remake_for_url($fileTemp,0);
        $content=preg_replace('#tileset name="([^"]+)"#isU','tileset name="'.$cleanFile.'"',$content);
        
        filewrite($file,$content);
	}
    }
    closedir($dh);
}
//drop the not used file
//rename to cleaner name the used file
$fileList=array();
if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(is_file($file))
        $fileList[]=$file;
    }
    closedir($dh);
}
$removedFile=0;
foreach($fileList as $file)
{
    if(isset($dirtyNameToClean[$file]))
        rename($file,$dirtyNameToClean[$file]);
    elseif(!isset($cleanNameToDirty[$file]))
    {
        if($file!='invisible.png' && $file!='invisible.tsx' && $file!='normal1.png' && $file!='normal1.tsx' && $file!='normal16.png' && $file!='normal16.tsx' && !preg_match('#\.xml$#',$file))
        {
            $removedFile++;
            unlink($file);
        }
    }
}
if($removedFile>0)
    echo 'Removed files: '.$removedFile."\n";
    
if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#-?[0-9]{1,2}.-?[0-9]{1,2}\.tmx#',$file))
	{
		$water=array();
		$grass=array();
		$grassNight=array();
		$fish=array();
		$content=file_get_contents($file);
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
					$grass[]=array('minLevel'=>$minLevel,'maxLevel'=>$maxLevel,'id'=>$id,'luck'=>$luck);
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
		if(isset($property['pvp']) && $property['pvp']=='disabled')
		{
			$type='city';
			$grassType='grass';
        }
		elseif(isset($property['isCave']) && $property['isCave']=='true')
		{
			$type='cave';
			$grassType='cave';
        }
		else
		{
			$type='outdoor';
			$grassType='grass';
        }
		$xmlcontent='<map type="'.$type.'">'."\n";
		
		$fileNPC=str_replace('.tmx','',$file);
		foreach(array('dutch'=>'nl','english'=>'en','finnish'=>'fi','french'=>'fr','german'=>'de','italian'=>'it','portuguese'=>'pt','spanish'=>'es') as $fullName=>$countryCode)
            if(file_exists('../language/'.$fullName.'/NPC/'.$fileNPC.'.txt'))
            {
                $contentNPSFile=preg_split("#[\r\n]+#isU",file_get_contents('../language/'.$fullName.'/NPC/'.$fileNPC.'.txt'));
                if(count($contentNPSFile)>0)
                    $map_file_to_name[$file][$countryCode]=preg_replace("#[ \n\r\t]+$#",'',$contentNPSFile[0]);
            }
		if(isset($map_file_to_name[$file]['en']))
		{
            if(isset($map_file_to_name[$file]['en']) && isset($map_file_to_nameEN[$file]))
            {
                if($map_file_to_name[$file]['en']==$map_file_to_nameEN[$file])
                {
                    foreach($map_file_to_name[$file] as $countryCode=>$text)
                    {
                        if($countryCode=='en')
                            $xmlcontent.='	<name>'.$text.'</name>'."\n";
                        elseif($text!=$map_file_to_name[$file]['en'])
                            $xmlcontent.='	<name lang="'.$countryCode.'">'.$text.'</name>'."\n";
                    }
                }
                else
                    $xmlcontent.='	<name>'.$map_file_to_nameEN[$file].'</name>'."\n";
            }
            elseif(isset($map_file_to_nameEN[$file]))
                $xmlcontent.='	<name>'.$map_file_to_nameEN[$file].'</name>'."\n";
        }
		if(count($grass)>0)
		{
			$xmlcontent.='	<'.$grassType.'>'."\n";
			foreach($grass as $monster)
			{
				if($monster['minLevel']==$monster['maxLevel'])
					$xmlcontent.='		<monster level="'.$monster['minLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
				else
					$xmlcontent.='		<monster minLevel="'.$monster['minLevel'].'" maxLevel="'.$monster['maxLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
			}
			$xmlcontent.='	</'.$grassType.'>'."\n";
		}
		if(count($grassNight)>0 && $grass!=$grassNight)
		{
			$xmlcontent.='	<'.$grassType.'Night>'."\n";
			foreach($grassNight as $monster)
			{
				if($monster['minLevel']==$monster['maxLevel'])
					$xmlcontent.='		<monster level="'.$monster['minLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
				else
					$xmlcontent.='		<monster minLevel="'.$monster['minLevel'].'" maxLevel="'.$monster['maxLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
			}
			$xmlcontent.='	</'.$grassType.'Night>'."\n";
		}
		if(count($water)>0 && preg_match('#layer[^>]* name="Water"#isU',$content))
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
		if(count($fish)>0 && preg_match('#layer[^>]* name="Water"#isU',$content))
		{
			$xmlcontent.='	<waterOldRod>'."\n";
			foreach($fish as $monster)
			{
				if($monster['minLevel']==$monster['maxLevel'])
					$xmlcontent.='		<monster level="'.$monster['minLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
				else
					$xmlcontent.='		<monster minLevel="'.$monster['minLevel'].'" maxLevel="'.$monster['maxLevel'].'" luck="'.$monster['luck'].'" id="'.$monster['id'].'" />'."\n";
			}
			$xmlcontent.='	</waterOldRod>'."\n";
		}
		$xmlcontent.='</map>';
		$filexml=str_replace('.tmx','.xml',$file);
		if(is_string($content))
		{
            preg_match_all('#<layer[^>]*>'."[ \r\n\t]+".'<data[^>]*>'."[ \r\n\t]+(".preg_quote('H4sIAAAAAAAAAO3BMQEAAADCoPVPbQwfoAAAAAC+BmbyAUigEAAA')."|".preg_quote('eJztwQEBAAAAgiD/r25IQAEAAPBoDhAAAQ==')."|".preg_quote('eNrtwTEBAAAAwqD1T20MH6AAAAAAvgYQoAAB').")[ \r\n\t]+".'</data>'."[ \r\n\t]+".'</layer>#isU',$content,$empty_layer);
            foreach($empty_layer as $layer_content)
                $content=str_replace($layer_content,'',$content);
            filewrite($file,$content);
        }
		filewrite($filexml,$xmlcontent);
		unset($property);
	}
	elseif(preg_match('#-?[0-9]{1,2}.-?[0-9]{1,2}\.tsx#',$file))
	{
	}
    }
    closedir($dh);
}
