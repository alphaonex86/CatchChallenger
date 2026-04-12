<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator monsters'."\n");

foreach($monster_meta as $id=>$monster)
{
	$resolved_type=$monster['type'][0];
	if(!isset($type_meta[$resolved_type]))
	{
		if(!isset($type_meta['normal']) || count($type_meta)<=0)
			$resolved_type='normal';
		else
		{
			foreach($type_meta as $type=>$type_content)
			{
				$resolved_type=$type;
				break;
			}
		}
	}
	if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['monsters/']))
		mkdir($datapack_explorer_local_path.$translation_list[$current_lang]['monsters/'],0777,true);
	$map_descriptor='';

	$effectiveness_list=array();
	foreach($type_meta as $realtypeindex=>$typecontent)
	{
		$effectiveness=(float)1.0;
		foreach($monster['type'] as $type)
			if(isset($typecontent['multiplicator'][$type]))
				$effectiveness*=$typecontent['multiplicator'][$type];
		if($effectiveness!=1.0)
		{
			if(!isset($effectiveness_list[(string)$effectiveness]))
				$effectiveness_list[(string)$effectiveness]=array();
			$effectiveness_list[(string)$effectiveness][]=$realtypeindex;
		}
	}
	$map_descriptor.='<div class="map monster_type_'.$resolved_type.'">'."\n";
		$map_descriptor.='<div class="subblock"><h1>'.$monster['name'][$current_lang].'</h1>'."\n";
		$map_descriptor.='<h2>#'.$id.'</h2>'."\n";
		$map_descriptor.='</div>'."\n";
		$map_descriptor.='<div class="value datapackscreenshot"><center>'."\n";
		if(file_exists($datapack_path.'monsters/'.$id.'/front.png'))
			$map_descriptor.='<img src="'.$base_datapack_site_path.'monsters/'.$id.'/front.png" width="80" height="80" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" />'."\n";
		else if(file_exists($datapack_path.'monsters/'.$id.'/front.gif'))
			$map_descriptor.='<img src="'.$base_datapack_site_path.'monsters/'.$id.'/front.gif" width="80" height="80" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" />'."\n";
		$map_descriptor.='</center></div>'."\n";
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Type'].'</div><div class="value">'."\n";
		$type_list=array();
		foreach($monster['type'] as $type)
			if(isset($type_meta[$type]))
				$type_list[]='<span class="type_label type_label_'.$type.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>'."\n";
		$map_descriptor.='<div class="type_label_list">'.implode(' ',$type_list).'</div></div></div>'."\n";
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Gender ratio'].'</div><div class="value">'."\n";
		if($monster['ratio_gender']<0 || $monster['ratio_gender']>100)
		{
			$map_descriptor.='<center><table class="genderbar"><tr><td class="genderbarunknown" style="width:100%"></td></tr></table></center>'."\n";
			$map_descriptor.=$translation_list[$current_lang]['Unknown gender'];
		}
		else
		{
			$map_descriptor.='<center><table class="genderbar"><tr><td class="genderbarmale" style="width:'.$monster['ratio_gender'].'%"></td><td class="genderbarfemale" style="width:'.(100-$monster['ratio_gender']).'%"></td></tr></table></center>'."\n";
			$map_descriptor.=$monster['ratio_gender'].'% '.$translation_list[$current_lang]['male'].', '.(100-$monster['ratio_gender']).'% '.$translation_list[$current_lang]['female'];
		}
		$map_descriptor.='</div></div>'."\n";
		if($monster['description'][$current_lang]!='')
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Description'].'</div><div class="value">'.$monster['description'][$current_lang].'</div></div>'."\n";
		if($monster['kind'][$current_lang]!='')
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Kind'].'</div><div class="value">'.$monster['kind'][$current_lang].'</div></div>'."\n";
		if($monster['habitat'][$current_lang]!='')
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Habitat'].'</div><div class="value">'.$monster['habitat'][$current_lang].'</div></div>'."\n";
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Catch rate'].'</div><div class="value">'.$monster['catch_rate'].'</div></div>'."\n";

        if(count($monster['game']))
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Game to catch it'].'</div><div class="value">'."\n";
            foreach($monster['game'] as $maindatapackcode=>$tempSubList)
            foreach($tempSubList as $tempSub)
            {
                $text_temp_sub='?';
                $color_temp_sub='';
                if($tempSub=='')
                {
                    if(isset($informations_meta['main'][$maindatapackcode]))
                    {
                        $text_temp_sub=$informations_meta['main'][$maindatapackcode]['initial'];
                        $color_temp_sub=$informations_meta['main'][$maindatapackcode]['color'];
                    }
                }
                else
                {
                    if(isset($informations_meta['main'][$maindatapackcode]['sub'][$tempSub]))
                    {
                        $text_temp_sub=$informations_meta['main'][$maindatapackcode]['sub'][$tempSub]['initial'];
                        $color_temp_sub=$informations_meta['main'][$maindatapackcode]['sub'][$tempSub]['color'];
                    }
                }
                if($color_temp_sub!='')
                    $map_descriptor.='<span style="background-color:'.$color_temp_sub.';" class="datapackinital">'.$text_temp_sub.'</span>'."\n";
                else
                    $map_descriptor.='<span class="datapackinital">'.$text_temp_sub.'</span>'."\n";
            }
            $map_descriptor.='</div></div>'."\n";
        }

        $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Rarity'].'</div><div class="value">'."\n";
        if(isset($exclusive_monster_reverse[$id]))
            $map_descriptor.=$translation_list[$current_lang]['Version exclusive!'];
        else if(!isset($monster_to_map[$id]))
            $map_descriptor.=$translation_list[$current_lang]['Not found on any map'];
        else
        {
            if(!isset($monster_to_rarity[$id]))
                $map_descriptor.=$translation_list[$current_lang]['Very rare'];
            else
            {
                $percent=100*($monster_to_rarity[$id]['position'])/count($monster_to_rarity);
                if($percent>90)
                    $map_descriptor.=$translation_list[$current_lang]['Very common'];
                else if($percent>70)
                    $map_descriptor.=$translation_list[$current_lang]['Common'];
                else if($percent>40)
                    $map_descriptor.=$translation_list[$current_lang]['Less common'];
                else if($percent>10)
                    $map_descriptor.=$translation_list[$current_lang]['Rare'];
                else
                    $map_descriptor.=$translation_list[$current_lang]['Very rare'];
            }
        }
        $map_descriptor.='</div></div>'."\n";

		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Steps for hatching'].'</div><div class="value">'.$monster['egg_step'].'</div></div>'."\n";
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Body'].'</div><div class="value">'.$translation_list[$current_lang]['Height'].': '.$monster['height'].'m, '.$translation_list[$current_lang]['width'].': '.$monster['weight'].'kg</div></div>'."\n";
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Stat'].'</div><div class="value">'.$translation_list[$current_lang]['Hp'].': <i>'.$monster['hp'].'</i>, '.$translation_list[$current_lang]['Attack'].': <i>'.$monster['attack'].'</i>, '.$translation_list[$current_lang]['Defense'].': <i>'.$monster['defense'].'</i>, '.$translation_list[$current_lang]['Special attack'].': <i>'.$monster['special_attack'].'</i>, '.$translation_list[$current_lang]['Special defense'].': <i>'.$monster['special_defense'].'</i>, '.$translation_list[$current_lang]['Speed'].': <i>'.$monster['speed'].'</i></div></div>'."\n";
		if(isset($effectiveness_list['4']) || isset($effectiveness_list['2']))
		{
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Weak to'].'</div><div class="value">'."\n";
			$type_list=array();
			if(isset($effectiveness_list['2']))
				foreach($effectiveness_list['2'] as $type)
					if(isset($type_meta[$type]))
						$type_list[]='<span class="type_label type_label_'.$type.'">2x: <a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>'."\n";
			if(isset($effectiveness_list['4']))
				foreach($effectiveness_list['4'] as $type)
					if(isset($type_meta[$type]))
						$type_list[]='<span class="type_label type_label_'.$type.'">4x: <a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>'."\n";
			$map_descriptor.=implode(' ',$type_list);
			$map_descriptor.='</div></div>'."\n";
		}
		if(isset($effectiveness_list['0.25']) || isset($effectiveness_list['0.5']))
		{
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Resistant to'].'</div><div class="value">'."\n";
			$type_list=array();
			if(isset($effectiveness_list['0.25']))
				foreach($effectiveness_list['0.25'] as $type)
					if(isset($type_meta[$type]))
						$type_list[]='<span class="type_label type_label_'.$type.'">0.25x: <a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>'."\n";
			if(isset($effectiveness_list['0.5']))
				foreach($effectiveness_list['0.5'] as $type)
					if(isset($type_meta[$type]))
						$type_list[]='<span class="type_label type_label_'.$type.'">0.5x: <a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>'."\n";
			$map_descriptor.=implode(' ',$type_list);
			$map_descriptor.='</div></div>'."\n";
		}
		if(isset($effectiveness_list['0']))
		{
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Immune to'].'</div><div class="value">'."\n";
			$type_list=array();
			foreach($effectiveness_list['0'] as $type)
				if(isset($type_meta[$type]))
					$type_list[]='<span class="type_label type_label_'.$type.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>'."\n";
			$map_descriptor.=implode(' ',$type_list);
			$map_descriptor.='</div></div>'."\n";
		}
	$map_descriptor.='</div>'."\n";
	
	if(count($monster['drops'])>0)
	{
		$map_descriptor.='<table class="item_list item_list_type_'.$resolved_type.'">
		<tr class="item_list_title item_list_title_type_'.$resolved_type.'">
			<th colspan="2">'.$translation_list[$current_lang]['Item'].'</th>
			<th>'.$translation_list[$current_lang]['Location'].'</th>
		</tr>'."\n";
		$drops=$monster['drops'];
		foreach($drops as $drop)
		{
			if(isset($item_meta[$drop['item']]))
			{
				$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$drop['item']]['name'][$current_lang]);
                $link.='.html';
				$name=$item_meta[$drop['item']]['name'][$current_lang];
				if($item_meta[$drop['item']]['image']!='')
					$image=$base_datapack_site_path.'/items/'.$item_meta[$drop['item']]['image'];
				else
					$image='';
			}
			else
			{
				$link='';
				$name='';
				$image='';
			}
			$quantity_text='';
			if($drop['quantity_min']!=$drop['quantity_max'])
				$quantity_text=$drop['quantity_min'].' to '.$drop['quantity_max'].' ';
			elseif($drop['quantity_min']>1)
				$quantity_text=$drop['quantity_min'].' ';
			$map_descriptor.='<tr class="value">
				<td>'."\n";
				if($image!='')
				{
					if($link!='')
						$map_descriptor.='<a href="'.$link.'">'."\n";
					$map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
					if($link!='')
						$map_descriptor.='</a>'."\n";
				}
				$map_descriptor.='</td>
				<td>'."\n";
				if($link!='')
					$map_descriptor.='<a href="'.$link.'">'."\n";
				if($name!='')
					$map_descriptor.=$quantity_text.$name;
				else
					$map_descriptor.=$quantity_text.$translation_list[$current_lang]['Unknown item'];
				if($link!='')
					$map_descriptor.='</a>'."\n";
				$map_descriptor.='</td>'."\n";
				$map_descriptor.='<td>'.str_replace('[luck]',$drop['luck'],$translation_list[$current_lang]['Drop luck of [luck]%']).'</td>
			</tr>'."\n";
		}
		$map_descriptor.='<tr>
			<td colspan="3" class="item_list_endline item_list_title_type_'.$resolved_type.'"></td>
		</tr>
		</table>'."\n";
	}

	if(count($monster['attack_list'])>0)
	{
		$map_descriptor.='<table class="item_list item_list_type_'.$resolved_type.'">
		<tr class="item_list_title item_list_title_type_'.$resolved_type.'">
			<th>'.$translation_list[$current_lang]['Level'].'</th>
			<th>'.$translation_list[$current_lang]['Skill'].'</th>
			<th>'.$translation_list[$current_lang]['Type'].'</th>
			<th>'.$translation_list[$current_lang]['Endurance'].'</th>
		</tr>'."\n";
		$attack_list=$monster['attack_list'];
		foreach($attack_list as $level=>$attack_at_level)
		{
			foreach($attack_at_level as $attack)
			{
				if(isset($skill_meta[$attack['id']]))
				{
					$map_descriptor.='<tr class="value">'."\n";
					$map_descriptor.='<td>'."\n";
					if($level==0)
						$map_descriptor.='Start'."\n";
					else
						$map_descriptor.=$level;
					$map_descriptor.='</td>'."\n";
					$map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['skills/'].text_operation_do_for_url($skill_meta[$attack['id']]['name'][$current_lang]).'.html">'.$skill_meta[$attack['id']]['name'][$current_lang];
					if($attack['attack_level']>1)
						$map_descriptor.=' at level '.$attack['attack_level'];
					$map_descriptor.='</a></td>'."\n";
					if(isset($type_meta[$skill_meta[$attack['id']]['type']]))
						$map_descriptor.='<td><span class="type_label type_label_'.$skill_meta[$attack['id']]['type'].'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$skill_meta[$attack['id']]['type'].'.html">'.ucfirst($type_meta[$skill_meta[$attack['id']]['type']]['name'][$current_lang]).'</a></span></td>'."\n";
					else
						$map_descriptor.='<td>&nbsp;</td>'."\n";
					if(isset($skill_meta[$attack['id']]['level_list'][$attack['attack_level']]))
						$map_descriptor.='<td>'.$skill_meta[$attack['id']]['level_list'][$attack['attack_level']]['endurance'].'</td>'."\n";
					else
						$map_descriptor.='<td>&nbsp;</td>'."\n";
					$map_descriptor.='</tr>'."\n";
				}
			}
		}
		$map_descriptor.='<tr>
			<td colspan="4" class="item_list_endline item_list_title_type_'.$resolved_type.'"></td>
		</tr>
		</table>'."\n";
	}
	if(count($monster['attack_list_byitem'])>0)
	{
        $attack_list_count=0;
		$map_descriptor.='<table class="skilltm_list item_list item_list_type_'.$resolved_type.'">
		<tr class="item_list_title item_list_title_type_'.$resolved_type.'">
			<th colspan="2">'.$translation_list[$current_lang]['Item'].'</th>
			<th>'.$translation_list[$current_lang]['Skill'].'</th>
			<th>'.$translation_list[$current_lang]['Type'].'</th>
			<th>'.$translation_list[$current_lang]['Endurance'].'</th>
		</tr>'."\n";
		$attack_list_byitem=$monster['attack_list_byitem'];
		foreach($attack_list_byitem as $item=>$attack_at_level)
		{
			foreach($attack_at_level as $attack)
			{
				if(isset($skill_meta[$attack['id']]))
				{
                    $attack_list_count++;
                    if($attack_list_count>10)
                    {
                        $map_descriptor.='<tr>
                            <td colspan="5" class="item_list_endline item_list_title_type_'.$resolved_type.'"></td>
                        </tr>
                        </table>'."\n";

                        $map_descriptor.='<table class="skilltm_list item_list item_list_type_'.$resolved_type.'">
                        <tr class="item_list_title item_list_title_type_'.$resolved_type.'">
                            <th colspan="2">'.$translation_list[$current_lang]['Item'].'</th>
                            <th>'.$translation_list[$current_lang]['Skill'].'</th>
                            <th>'.$translation_list[$current_lang]['Type'].'</th>
                            <th>'.$translation_list[$current_lang]['Endurance'].'</th>
                        </tr>'."\n";
                        $attack_list_count=1;
                    }

					$map_descriptor.='<tr class="value">'."\n";
					if(isset($item_meta[$item]))
					{
						$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item]['name'][$current_lang]);
                        $link.='.html';
						$name=$item_meta[$item]['name'][$current_lang];
						if($item_meta[$item]['image']!='')
							$image=$base_datapack_site_path.'/items/'.$item_meta[$item]['image'];
						else
							$image='';
					}
					else
					{
						$link='';
						$name='';
						$image='';
					}
					$map_descriptor.='<tr class="value">
					<td>'."\n";
					if($image!='')
					{
						if($link!='')
							$map_descriptor.='<a href="'.$link.'">'."\n";
						$map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
						if($link!='')
							$map_descriptor.='</a>'."\n";
					}
					$map_descriptor.='</td>
					<td>'."\n";
					if($link!='')
						$map_descriptor.='<a href="'.$link.'">'."\n";
					if($name!='')
						$map_descriptor.=$name;
					else
						$map_descriptor.=$translation_list[$current_lang]['Unknown item'];
					if($link!='')
						$map_descriptor.='</a>'."\n";
					$map_descriptor.='</td>'."\n";
					$map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['skills/'].text_operation_do_for_url($skill_meta[$attack['id']]['name'][$current_lang]).'.html">'.$skill_meta[$attack['id']]['name'][$current_lang];
					if($attack['attack_level']>1)
						$map_descriptor.=' at level '.$attack['attack_level'];
					$map_descriptor.='</a></td>'."\n";
					if(isset($type_meta[$skill_meta[$attack['id']]['type']]))
						$map_descriptor.='<td><span class="type_label type_label_'.$skill_meta[$attack['id']]['type'].'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$skill_meta[$attack['id']]['type'].'.html">'.ucfirst($type_meta[$skill_meta[$attack['id']]['type']]['name'][$current_lang]).'</a></span></td>'."\n";
					else
						$map_descriptor.='<td>&nbsp;</td>'."\n";
					if(isset($skill_meta[$attack['id']]['level_list'][$attack['attack_level']]))
						$map_descriptor.='<td>'.$skill_meta[$attack['id']]['level_list'][$attack['attack_level']]['endurance'].'</td>'."\n";
					else
						$map_descriptor.='<td>&nbsp;</td>'."\n";
					$map_descriptor.='</tr>'."\n";
				}
                else
                    echo '$skill_meta[$attack[id]] not found'."\n";
			}
		}
		$map_descriptor.='<tr>
			<td colspan="5" class="item_list_endline item_list_title_type_'.$resolved_type.'"></td>
		</tr>
		</table>'."\n";
        $map_descriptor.='<br style="clear:both;" />'."\n";
	}

	if(isset($monster_to_quests[$id]))
	{
		$map_descriptor.='<table class="item_list item_list_type_outdoor"><tr class="item_list_title item_list_title_type_outdoor">'."\n";
		$map_descriptor.='<th colspan="2">'.$translation_list[$current_lang]['Item'].'</th><th>'.$translation_list[$current_lang]['Quests'].'</th><th>'.$translation_list[$current_lang]['Luck'].'</th></tr>'."\n";
		foreach($monster_to_quests[$id] as $maindatapackcode=>$monsterlist)
        foreach($monsterlist as $quests_monsters_details)
		{
			$map_descriptor.='<tr class="value"><td>'."\n";
			if(isset($item_meta[$quests_monsters_details['item']]))
			{
				$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$quests_monsters_details['item']]['name'][$current_lang]);
                $link.='.html';
				$name=$item_meta[$quests_monsters_details['item']]['name'][$current_lang];
				if($item_meta[$quests_monsters_details['item']]['image']!='')
					$image=$base_datapack_site_path.'/items/'.$item_meta[$quests_monsters_details['item']]['image'];
				else
					$image='';
			}
			else
			{
				$link='';
				$name='';
				$image='';
			}
			$quantity_text='';
			if($quests_monsters_details['quantity']>1)
				$quantity_text=$quests_monsters_details['quantity'].' ';
			if($image!='')
			{
				if($link!='')
					$map_descriptor.='<a href="'.$link.'">'."\n";
				$map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
				if($link!='')
					$map_descriptor.='</a>'."\n";
			}
			$map_descriptor.='</td><td>'."\n";
			if($link!='')
				$map_descriptor.='<a href="'.$link.'">'."\n";
			if($name!='')
				$map_descriptor.=$quantity_text.$name;
			else
				$map_descriptor.=$quantity_text.$translation_list[$current_lang]['Unknown item'];
			if($link!='')
				$map_descriptor.='</a>'."\n";
			$map_descriptor.='</td>'."\n";

			$map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['quests/'].$maindatapackcode.'/'.$quests_monsters_details['quest'].'-'.text_operation_do_for_url($quests_meta[$maindatapackcode][$quests_monsters_details['quest']]['name'][$current_lang]).'.html" title="'.$quests_meta[$maindatapackcode][$quests_monsters_details['quest']]['name'][$current_lang].'">'."\n";
			$map_descriptor.=$quests_meta[$maindatapackcode][$quests_monsters_details['quest']]['name'][$current_lang];
			$map_descriptor.='</a></td>'."\n";
			$map_descriptor.='<td>'.$quests_monsters_details['rate'].'%</td>'."\n";
			$map_descriptor.='</tr>'."\n";
		}
		$map_descriptor.='<tr>
		<td colspan="4" class="item_list_endline item_list_title_type_outdoor"></td>
		</tr></table>'."\n";
	}

	if(count($monster['evolution_list'])>0 || isset($reverse_evolution[$id]))
	{
		$map_descriptor.='<table class="item_list item_list_type_'.$resolved_type.'">
		<tr class="item_list_title item_list_title_type_'.$resolved_type.'">
			<th>'."\n";
		if(isset($reverse_evolution[$id]))
			$map_descriptor.=$translation_list[$current_lang]['Evolve from'];
		$map_descriptor.='</th>
		</tr>'."\n";

		if(isset($reverse_evolution[$id]))
		{
			$map_descriptor.='<tr class="value">'."\n";
			$map_descriptor.='<td>'."\n";
			$map_descriptor.='<table class="monsterforevolution">'."\n";
			foreach($reverse_evolution[$id] as $evolution)
			{
				if(file_exists($datapack_path.'monsters/'.$evolution['evolveFrom'].'/front.png'))
					$map_descriptor.='<tr><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['evolveFrom']]['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$evolution['evolveFrom'].'/front.png" width="80" height="80" alt="'.$monster_meta[$evolution['evolveFrom']]['name'][$current_lang].'" title="'.$monster_meta[$evolution['evolveFrom']]['name'][$current_lang].'" /></a></td></tr>'."\n";
				else if(file_exists($datapack_path.'monsters/'.$evolution['evolveFrom'].'/front.gif'))
					$map_descriptor.='<tr><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['evolveFrom']]['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$evolution['evolveFrom'].'/front.gif" width="80" height="80" alt="'.$monster_meta[$evolution['evolveFrom']]['name'][$current_lang].'" title="'.$monster_meta[$evolution['evolveFrom']]['name'][$current_lang].'" /></a></td></tr>'."\n";
				$map_descriptor.='<tr><td class="evolution_name"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['evolveFrom']]['name'][$current_lang]).'.html">'.$monster_meta[$evolution['evolveFrom']]['name'][$current_lang].'</a></td></tr>'."\n";
				if($evolution['type']=='level')
					$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['At level'].' '.$evolution['level'].'</td></tr>'."\n";
				elseif($evolution['type']=='item')
				{
					if(isset($item_meta[$evolution['level']]))
					{
						$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$evolution['level']]['name'][$current_lang]);
                        $link.='.html';
						$name=$item_meta[$evolution['level']]['name'][$current_lang];
						$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['Evolve with'].'<br /><a href="'.$link.'" title="'.$name.'">'."\n";
						if($item_meta[$evolution['level']]['image']!='')
							$map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$evolution['level']]['image'].'" alt="'.$name.'" title="'.$name.'" style="float:left;" />'."\n";
						$map_descriptor.=$name.'</a></td></tr>'."\n";
					}
					else
						$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['With unknown item'].'</td></tr>'."\n";
				}
				elseif($evolution['type']=='trade')
					$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['After trade'].'</td></tr>'."\n";
				else
					$map_descriptor.='<tr><td class="evolution_type">&nbsp;</td></tr>'."\n";
			}
			$map_descriptor.='</table>'."\n";
			$map_descriptor.='</td>'."\n";
			$map_descriptor.='</tr>'."\n";
		}

		$map_descriptor.='<tr class="value">'."\n";
		$map_descriptor.='<td>'."\n";
		$map_descriptor.='<table class="monsterforevolution">'."\n";
		if(file_exists($datapack_path.'monsters/'.$id.'/front.png'))
			$map_descriptor.='<tr><td><img src="'.$base_datapack_site_path.'monsters/'.$id.'/front.png" width="80" height="80" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" /></td></tr>'."\n";
		else if(file_exists($datapack_path.'monsters/'.$id.'/front.gif'))
			$map_descriptor.='<tr><td><img src="'.$base_datapack_site_path.'monsters/'.$id.'/front.gif" width="80" height="80" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" /></td></tr>'."\n";
		$map_descriptor.='<tr><td class="evolution_name">'.$monster['name'][$current_lang].'</td></tr>'."\n";
		$map_descriptor.='</table>'."\n";
		$map_descriptor.='</td>'."\n";
		$map_descriptor.='</tr>'."\n";

		if(count($monster['evolution_list'])>0)
		{
			$map_descriptor.='<tr class="value">'."\n";
			$map_descriptor.='<td>'."\n";
			$map_descriptor.='<table class="monsterforevolution">'."\n";
			foreach($monster['evolution_list'] as $evolution)
			{
				if(file_exists($datapack_path.'monsters/'.$evolution['evolveTo'].'/front.png'))
					$map_descriptor.='<tr><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['evolveTo']]['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$evolution['evolveTo'].'/front.png" width="80" height="80" alt="'.$monster_meta[$evolution['evolveTo']]['name'][$current_lang].'" title="'.$monster_meta[$evolution['evolveTo']]['name'][$current_lang].'" /></a></td></tr>'."\n";
				else if(file_exists($datapack_path.'monsters/'.$evolution['evolveTo'].'/front.gif'))
					$map_descriptor.='<tr><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['evolveTo']]['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$evolution['evolveTo'].'/front.gif" width="80" height="80" alt="'.$monster_meta[$evolution['evolveTo']]['name'][$current_lang].'" title="'.$monster_meta[$evolution['evolveTo']]['name'][$current_lang].'" /></a></td></tr>'."\n";
                $evolutionname='???';
                if(isset($monster_meta[$evolution['evolveTo']]))
                    $evolutionname=$monster_meta[$evolution['evolveTo']]['name'][$current_lang];
				$map_descriptor.='<tr><td class="evolution_name">';
				if($evolutionname!='???')
                    $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($evolutionname).'.html">';
				$map_descriptor.=$evolutionname;
				if($evolutionname!='???')
                    $map_descriptor.='</a>';
				$map_descriptor.='</td></tr>'."\n";
				if($evolution['type']=='level')
					$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['At level'].' '.$evolution['level'].'</td></tr>'."\n";
				elseif($evolution['type']=='item')
				{
					if(isset($item_meta[$evolution['level']]))
					{
						$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$evolution['level']]['name'][$current_lang]);
                        $link.='.html';
						$name=$item_meta[$evolution['level']]['name'][$current_lang];
						$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['Evolve with'].'<br /><a href="'.$link.'" title="'.$name.'">'."\n";
						if($item_meta[$evolution['level']]['image']!='')
							$map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$evolution['level']]['image'].'" alt="'.$name.'" title="'.$name.'" style="float:left;" />'."\n";
						$map_descriptor.=$name.'</a></td></tr>'."\n";
					}
					else
						$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['With unknown item'].'</td></tr>'."\n";
				}
				elseif($evolution['type']=='trade')
					$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['After trade'].'</td></tr>'."\n";
				else
					$map_descriptor.='<tr><td class="evolution_type">&nbsp;</td></tr>'."\n";
			}
			$map_descriptor.='</table>'."\n";
			$map_descriptor.='</td>'."\n";
			$map_descriptor.='</tr>'."\n";
		}

		$map_descriptor.='<tr>
			<th class="item_list_endline item_list_title item_list_title_type_'.$resolved_type.'">'."\n";
		if(count($monster['evolution_list'])>0)
			$map_descriptor.=$translation_list[$current_lang]['Evolve to'];
		$map_descriptor.='</th>
		</tr>
		</table>'."\n";
	}

	if(isset($monster_to_map[$id]))
	{
		$map_descriptor.='<table class="item_list item_list_type_'.$resolved_type.'">
		<tr class="item_list_title item_list_title_type_'.$resolved_type.'">
			<th colspan="2">'.$translation_list[$current_lang]['Map'].'</th>
			<th>'.$translation_list[$current_lang]['Location'].'</th>
			<th>'.$translation_list[$current_lang]['Levels'].'</th>
			<th colspan="3">'.$translation_list[$current_lang]['Rate'].'</th>
		</tr>'."\n";


        foreach($monster_to_map[$id] as $monsterType=>$monster_list_temp)
        foreach($monster_list_temp as $maindatapackcode=>$map_list)
        {
            $full_monsterType_name='Cave';
            if(isset($layer_event[$monsterType]))
            {
                if($layer_event[$monsterType]['layer']!='')
                    $full_monsterType_name=$layer_event[$monsterType]['layer'];
                $monsterType_top=$layer_event[$monsterType]['monsterType'];
                $full_monsterType_name_top='Cave';
                if(isset($layer_meta[$monsterType_top]))
                    if($layer_meta[$monsterType_top]['layer']!='')
                        $full_monsterType_name_top=$layer_meta[$monsterType_top]['layer'];
            }
            elseif(isset($layer_meta[$monsterType]))
            {
                if($layer_meta[$monsterType]['layer']!='')
                    $full_monsterType_name=$layer_meta[$monsterType]['layer'];
                $monsterType_top=$monsterType;
                $full_monsterType_name_top=$full_monsterType_name;
            }
            $map_descriptor.='<tr class="item_list_title_type_'.$resolved_type.'">
                    <th colspan="7">'."\n";
            $link='';
            $name='';
            $image='';
            if(isset($layer_meta[$monsterType_top]['item']) && $item_meta[$layer_meta[$monsterType_top]['item']])
            {
                $link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$layer_meta[$monsterType_top]['item']]['name'][$current_lang]);
                $link.='.html';
                $name=$item_meta[$layer_meta[$monsterType_top]['item']]['name'][$current_lang];
                if($item_meta[$layer_meta[$monsterType_top]['item']]['image']!='')
                    $image=$base_datapack_site_path.'/items/'.$item_meta[$layer_meta[$monsterType_top]['item']]['image'];
                else
                    $image='';
                $map_descriptor.='<center><table><tr>'."\n";
                
                if($link!='')
                    $map_descriptor.='<td><a href="'.$link.'">'."\n";
                if($image!='')
                    $map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
                if($link!='')
                    $map_descriptor.='</a></td>'."\n";

                if($link!='')
                    $map_descriptor.='<td><a href="'.$link.'">'."\n";
                $map_descriptor.=$item_meta[$layer_meta[$monsterType_top]['item']]['name'][$current_lang]."\n";
                if($link!='')
                    $map_descriptor.='</a></td>'."\n";

                $map_descriptor.='</tr></table></center>'."\n";
            }
            else
            {
                if(isset($translation_list[$current_lang][$full_monsterType_name]))
                    $map_descriptor.=$translation_list[$current_lang][$full_monsterType_name]."\n";
                else
                    $map_descriptor.=$full_monsterType_name."\n";
            }
            if(isset($layer_event[$monsterType]))
            {
                if($layer_event[$monsterType]['id']=='day' && $layer_event[$monsterType]['value']=='night')
                    $map_descriptor.=' at night'."\n";
                else
                    $map_descriptor.=' condition '.$layer_event[$monsterType]['id'].' at '.$layer_event[$monsterType]['value']."\n";
            }
            $map_descriptor.='</th>
                </tr>'."\n";
            foreach($map_list as $map=>$subdatapackcode_list)
            foreach($subdatapackcode_list as $subdatapackcode=>$monster_on_map)
            {
                $map_descriptor.='<tr class="value">'."\n";
                if(isset($maps_list[$maindatapackcode][$map]))
                {
                    if(isset($zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$map]['zone']]))
                    {
                        $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$map).'" title="'.$maps_list[$maindatapackcode][$map]['name'][$current_lang].'">'.$maps_list[$maindatapackcode][$map]['name'][$current_lang].'</a></td>'."\n";
                        $map_descriptor.='<td>'.$zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$map]['zone']]['name'][$current_lang].'</td>'."\n";
                    }
                    else
                        $map_descriptor.='<td colspan="2"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$map).'" title="'.$maps_list[$maindatapackcode][$map]['name'][$current_lang].'">'.$maps_list[$maindatapackcode][$map]['name'][$current_lang].'</a></td>'."\n";
                }
                else
                    $map_descriptor.='<td colspan="2">'.$translation_list[$current_lang]['Unknown map'].'</td>'."\n";
                $map_descriptor.='<td>'."\n";
                $map_descriptor.='<img src="/images/datapack-explorer/'.$full_monsterType_name_top.'.png" alt="" class="locationimg" width="16px" height="16px">'.$full_monsterType_name_top;
                $map_descriptor.='</td>
                <td>'."\n";
                if($monster_on_map['minLevel']==$monster_on_map['maxLevel'])
                    $map_descriptor.=$monster_on_map['minLevel'];
                else
                    $map_descriptor.=$monster_on_map['minLevel'].'-'.$monster_on_map['maxLevel'];
                $map_descriptor.='</td>'."\n";
                $map_descriptor.='<td colspan="3">'.$monster_on_map['luck'].'%</td>
                </tr>'."\n";
                break;
            }
        }

		$map_descriptor.='<tr>
			<td colspan="7" class="item_list_endline item_list_title_type_'.$resolved_type.'"></td>
		</tr>
		</table>'."\n";
	}
	
	//$monster_to_fight[$monster][$maindatapackcode][]=$id;
	if(isset($monster_to_fight[$id]))
	{
		$map_descriptor.='<center><table class="item_list item_list_type_'.$resolved_type.'">
		<tr class="item_list_title item_list_title_type_'.$resolved_type.'">
			<th colspan="2">'.$translation_list[$current_lang]['Bot'].'</th>
			<th>'.$translation_list[$current_lang]['Type'].'</th>
			<th>'.$translation_list[$current_lang]['Content'].'</th>
		</tr>'."\n";

        foreach($monster_to_fight[$id] as $maindatapackcode=>$fights_list)
        foreach($fights_list as $fightid)
        {
            if(isset($fight_to_bot[$maindatapackcode][$fightid]))
            foreach($fight_to_bot[$maindatapackcode][$fightid] as $bot_id)
            {
                $bot=$bots_meta[$maindatapackcode][$bot_id];
				if($bot['name'][$current_lang]=='')
					$link=text_operation_do_for_url('bot '.$bot_id);
				else if($bots_name_count[$maindatapackcode][$current_lang][text_operation_do_for_url($bot['name'][$current_lang])]==1)
					$link=text_operation_do_for_url($bot['name'][$current_lang]);
				else
					$link=text_operation_do_for_url($bot_id.'-'.$bot['name'][$current_lang]);
					
                foreach($bot['step'] as $step_id=>$step)
                {
                    if($step['type']=='fight' && $step['fightid']==$fightid)
                    {
                        $map_descriptor.='<tr class="value">'."\n";
                        $have_skin=true;
                        if(isset($bot_id_to_skin[$bot_id][$maindatapackcode]))
                        {
                            if(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
                                $map_descriptor.='<td><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png\');background-repeat:no-repeat;background-position:-16px -48px;"></div></td>'."\n";
                            elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
                                $map_descriptor.='<td><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png\');background-repeat:no-repeat;background-position:-16px -48px;"></div></td>'."\n";
                            elseif(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
                                $map_descriptor.='<td><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif\');background-repeat:no-repeat;background-position:-16px -48px;"></div></td>'."\n";
                            elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
                                $map_descriptor.='<td><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif\');background-repeat:no-repeat;background-position:-16px -48px;"></div></td>'."\n";
                            else
                                $have_skin=false;
                        }
                        else
                            $have_skin=false;
                        $map_descriptor.='<td';
                        if(!$have_skin)
                            $map_descriptor.=' colspan="2"';
                        if($bot['name'][$current_lang]=='')
                            $map_descriptor.='><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['bots/'].$maindatapackcode.'/'.$link.'.html" title="Bot #'.$bot_id.'">Bot #'.$bot_id.'</a></td>'."\n";
                        else
                            $map_descriptor.='><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['bots/'].$maindatapackcode.'/'.$link.'.html" title="'.$bot['name'][$current_lang].'">'.$bot['name'][$current_lang].'</a></td>'."\n";
            
                        $map_descriptor.='<td><center>'.$translation_list[$current_lang]['Fight'].'<div style="background-position:-16px -16px;" class="flags flags16"></div></center></td><td>'."\n";
                        if($step['leader'])
                        {
                            $map_descriptor.='<b>'.$translation_list[$current_lang]['Leader'].'</b><br />'."\n";
                            if(isset($bot_id_to_skin[$bot_id][$maindatapackcode]))
                            {
                                if(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.png'))
                                    $map_descriptor.='<center><img src="'.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.png" width="80" height="80" alt="" /></center>'."\n";
                                else if(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.png'))
                                    $map_descriptor.='<center><img src="'.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.png" width="80" height="80" alt="" /></center>'."\n";
                                elseif(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.gif'))
                                    $map_descriptor.='<center><img src="'.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.gif" width="80" height="80" alt="" /></center>'."\n";
                                else if(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.gif'))
                                    $map_descriptor.='<center><img src="'.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.gif" width="80" height="80" alt="" /></center>'."\n";
                            }
                        }
                        if($fight_meta[$maindatapackcode][$step['fightid']]['cash']>0)
                            $map_descriptor.=$translation_list[$current_lang]['Rewards'].': <b>'.$fight_meta[$maindatapackcode][$step['fightid']]['cash'].'$</b><br />'."\n";

                        if(count($fight_meta[$maindatapackcode][$step['fightid']]['items'])>0)
                        {
                            $map_descriptor.='<center><table class="item_list item_list_type_'.$map_current_object['type'].'">
                            <tr class="item_list_title item_list_title_type_'.$map_current_object['type'].'">
                                <th colspan="2">'.$translation_list[$current_lang]['Item'].'</th>
                            </tr>'."\n";
                            foreach($fight_meta[$maindatapackcode][$step['fightid']]['items'] as $item)
                            {
                                if(isset($item_meta[$item['item']]))
                                {
                                    $link_item=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item['item']]['name'][$current_lang]);
                                    $link_item.='.html';
                                    $name=$item_meta[$item['item']]['name'][$current_lang];
                                    if($item_meta[$item['item']]['image']!='')
                                        $image=$base_datapack_site_path.'/items/'.$item_meta[$item['item']]['image'];
                                    else
                                        $image='';
                                }
                                else
                                {
                                    $link_item='';
                                    $name='';
                                    $image='';
                                }
                                $quantity_text='';
                                if($item['quantity']>1)
                                    $quantity_text=$item['quantity'].' ';
                                $map_descriptor.='<tr class="value">
                                    <td>'."\n";
                                    if($image!='')
                                    {
                                        if($link_item!='')
                                            $map_descriptor.='<a href="'.$link_item.'">'."\n";
                                        $map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
                                        if($link_item!='')
                                            $map_descriptor.='</a>'."\n";
                                    }
                                    $map_descriptor.='</td>
                                    <td>'."\n";
                                    if($link_item!='')
                                        $map_descriptor.='<a href="'.$link_item.'">'."\n";
                                    if($name!='')
                                        $map_descriptor.=$quantity_text.$name;
                                    else
                                        $map_descriptor.=$quantity_text.$translation_list[$current_lang]['Unknown item'];
                                    if($link_item!='')
                                        $map_descriptor.='</a>'."\n";
                                    $map_descriptor.='</td>'."\n";
                                    $map_descriptor.='</tr>'."\n";
                            }
                            $map_descriptor.='<tr>
                                <td colspan="2" class="item_list_endline item_list_title_type_'.$map_current_object['type'].'"></td>
                            </tr>
                            </table></center>'."\n";
                        }

                        foreach($fight_meta[$maindatapackcode][$step['fightid']]['monsters'] as $monsterListentry)
                            $map_descriptor.=monsterAndLevelToDisplay($monsterListentry,$step['leader']);
                        $map_descriptor.='<br style="clear:both;" />'."\n";

                        $map_descriptor.='</td>'."\n";
                        $map_descriptor.='</tr>'."\n";
                    }
                }
            }
        }
            
        $map_descriptor.='<tr>
			<td colspan="4" class="item_list_endline item_list_title_type_'.$resolved_type.'"></td>
		</tr>
		</table></center>'."\n";
	}

	$content=$template;
	$content=str_replace('${TITLE}',$monster['name'][$current_lang],$content);
	$content=str_replace('${CONTENT}',$map_descriptor,$content);
	$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
	$content=clean_html($content);
	$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster['name'][$current_lang]).'.html';
	if(file_exists($filedestination))
		die('Monster The file already exists: '.$filedestination);
	filewrite($filedestination,$content);
}

$map_descriptor='';
$map_descriptor.='<img src="/images/datapack-explorer/Grass.png" alt="" class="locationimg" width="16px" height="16px">: Wild monster<br style="clear:both" />';
$map_descriptor.='<img src="/images/datapack-explorer/GrassUp.png" alt="" class="locationimg" width="16px" height="16px">: Evole from wild monster<br style="clear:both" />';
$map_descriptor.='<div style="background-position:-16px -16px;" class="flags flags16" class="locationimg"></div>: Used in bot fight<br style="clear:both" />';
$map_descriptor.='<img src="/official-server/images/top-1.png" alt="" class="locationimg" width="16px" height="16px">: Unique to a version<br style="clear:both" />';

$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#e5eaff;"></div>: Very common<br style="clear:both" />';
$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#e0ffdd;"></div>: Common<br style="clear:both" />';
$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#fbfdd3;"></div>: Less common<br style="clear:both" />';
$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#ffefdb;"></div>: Rare<br style="clear:both" />';
$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#ffe5e5;"></div>: Very rare<br style="clear:both" />';

$map_descriptor.='<br style="clear:both" />';
$map_descriptor.='<table class="item_list item_list_type_normal monster_list">
<tr class="item_list_title item_list_title_type_normal">
	<th colspan="4">'.$translation_list[$current_lang]['Monster'].'</th>
</tr>'."\n";
$monster_count=0;
foreach($monster_meta as $id=>$monster)
{
    $monster_count++;
    if($monster_count>20)
    {
        $map_descriptor.='<tr>
            <td colspan="4" class="item_list_endline item_list_title_type_normal"></td>
        </tr>
        </table>'."\n";
        $map_descriptor.='<table class="item_list item_list_type_normal monster_list">
        <tr class="item_list_title item_list_title_type_normal">
            <th colspan="4">'.$translation_list[$current_lang]['Monster'].'</th>
        </tr>'."\n";
        $monster_count=1;
    }

	$name=$monster['name'][$current_lang];
	$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
    $link.='.html';
	$map_descriptor.='<tr class="value"';
	
    if(!isset($monster_to_map[$id]))
    {}
    else
    {
        if(!isset($monster_to_rarity[$id]))
            $map_descriptor.=' style="background-color:#ffe5e5;"';
        else
        {
            if(isset($monster_to_rarity[$id]))
            {
                $percent=100*($monster_to_rarity[$id]['position'])/count($monster_to_rarity);
                if($percent>90)
                    $map_descriptor.=' style="background-color:#e5eaff;"';
                else if($percent>70)
                    $map_descriptor.=' style="background-color:#e0ffdd;"';
                else if($percent>40)
                    $map_descriptor.=' style="background-color:#fbfdd3;"';
                else if($percent>10)
                    $map_descriptor.=' style="background-color:#ffefdb;"';
                else
                    $map_descriptor.=' style="background-color:#ffe5e5;"';
            }
        }
    }
    
	$map_descriptor.='>'."\n";
	$map_descriptor.='<td>'."\n";
	if(file_exists($datapack_path.'monsters/'.$id.'/small.png'))
		$map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$id.'/small.png" width="32" height="32" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" /></a></div>'."\n";
	else if(file_exists($datapack_path.'monsters/'.$id.'/small.gif'))
		$map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$id.'/small.gif" width="32" height="32" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" /></a></div>'."\n";
    $map_descriptor.='</td>'."\n";
	
	//flags
	$flagsstring=array();
	if(isset($monster_to_map[$id]))
        $flagsstring[]='<img src="/images/datapack-explorer/Grass.png" alt="" class="locationimg" width="16px" height="16px">';
    if(isset($reverse_evolution[$id]))
	{
        $evolFromWild=false;
        $idToScanList=array($id);
        while(count($idToScanList)>0)
        {
            $idToScanListNew=array();
            foreach($idToScanList as $arrayIndex=>$idToScan)
            {
                if(isset($reverse_evolution[$idToScan]))
                    foreach($reverse_evolution[$idToScan] as $evolution)
                    {
                        if(isset($monster_to_map[(int)$evolution['evolveFrom']]))
                        {
                            $evolFromWild=true;
                            break;
                        }
                        else
                            $idToScanListNew[]=(int)$evolution['evolveFrom'];
                    }
                if($evolFromWild==true)
                    break;
            }
            $idToScanList=$idToScanListNew;
        }
        if($evolFromWild)
            $flagsstring[]='<img src="/images/datapack-explorer/GrassUp.png" alt="" class="locationimg" width="16px" height="16px">';
    }
    if(isset($monster_to_fight[$id]))
        $flagsstring[]='<div style="background-position:-16px -16px;" class="flags flags16" class="locationimg"></div>';
    if(isset($exclusive_monster_reverse[$id]))
        $flagsstring[]='<img src="/official-server/images/top-1.png" alt="" class="locationimg" width="16px" height="16px">';
	if(count($flagsstring)>0)
	{
        $map_descriptor.='<td class="monsterlistflag">'."\n";
        $map_descriptor.=implode('',$flagsstring);
        $map_descriptor.='</td>'."\n";
	}
	
    $map_descriptor.='<td';
    if(count($flagsstring)<=0)
        $map_descriptor.=' colspan="2"';
    $map_descriptor.='><a href="'.$link.'">'.$name.'</a>';
	$map_descriptor.='</td>'."\n";
	
	$map_descriptor.='<td>'."\n";
	$type_list=array();
	foreach($monster['type'] as $type)
		if(isset($type_meta[$type]))
			$type_list[]='<span class="type_label type_label_'.$type.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>'."\n";
	$map_descriptor.='<div class="type_label_list">'.implode(' ',$type_list).'</div>'."\n";
	$map_descriptor.='</td>'."\n";
	$map_descriptor.='</tr>'."\n";
}
$map_descriptor.='<tr>
	<td colspan="4" class="item_list_endline item_list_title_type_normal"></td>
</tr>
</table>'."\n";

$content=$template;
$content=str_replace('${TITLE}',$translation_list[$current_lang]['Monsters list'],$content);
$content=str_replace('${CONTENT}',$map_descriptor,$content);
$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
$content=clean_html($content);
$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['monsters.html'];
if(file_exists($filedestination))
	die('Monster 2 The file already exists: '.$filedestination);
filewrite($filedestination,$content);

