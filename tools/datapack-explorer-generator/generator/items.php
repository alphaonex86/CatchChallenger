<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator items'."\n");

foreach($item_meta as $id=>$item)
{
	if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['items/']))
		mkdir($datapack_explorer_local_path.$translation_list[$current_lang]['items/'],0777,true);
	$map_descriptor='';

	$map_descriptor.='<div class="map item_details">'."\n";
		$map_descriptor.='<div class="subblock"><h1>'.$item['name'][$current_lang].'</h1>'."\n";
		$map_descriptor.='<h2>#'.$id.'</h2>'."\n";
		if($item['group']!='')
			$map_descriptor.='<h3>'.$item_group[$item['group']]['name'][$current_lang].'</h3>'."\n";
		$map_descriptor.='</div>'."\n";
		$map_descriptor.='<div class="value datapackscreenshot"><center>'."\n";
		if($item['image']!='' && file_exists($datapack_path.'items/'.$item['image']))
			$map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item['image'].'" width="24" height="24" alt="'.$item['name'][$current_lang].'" title="'.$item['name'][$current_lang].'" />'."\n";
		$map_descriptor.='</center></div>'."\n";
		if($item['price']>0)
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Price'].'</div><div class="value">'.$item['price'].'$</div></div>'."\n";
		else
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Price'].'</div><div class="value">'.$translation_list[$current_lang]['Can\'t be sold'].'</div></div>'."\n";
		if($item['description'][$current_lang]!='')
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Description'].'</div><div class="value">'.$item['description'][$current_lang].'</div></div>'."\n";
		if(isset($item['trap']))
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Trap'].'</div><div class="value">'.$translation_list[$current_lang]['Bonus rate:'].' '.$item['trap'].'x</div></div>'."\n";
		if(isset($item['repel']))
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Repel'].'</div><div class="value">Repel the monsters during '.$item['repel'].' steps</div></div>'."\n";
        if(isset($item_to_skill_of_monster[$id]))
        {
            $sameuniqueskill=true;
            $lastskill=-1;
            foreach($item_to_skill_of_monster[$id] as $entry)
            {
                if($lastskill==-1)
                    $lastskill=$entry['id'];
                else if($lastskill!=$entry['id'])
                {
                    $sameuniqueskill=false;
                    break;
                }
            }
            
            if($sameuniqueskill)
                foreach($item_to_skill_of_monster[$id] as $entry)
                {
                    if(isset($skill_meta[$entry['id']]))
                    {
                        $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Skill'].'</div><div class="value">'."\n";
                        $map_descriptor.='<table><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['skills/'].text_operation_do_for_url($skill_meta[$entry['id']]['name'][$current_lang]).'.html">'.$skill_meta[$entry['id']]['name'][$current_lang];
                        if($entry['attack_level']>1)
                            $map_descriptor.=' at level '.$entry['attack_level'];
                        $map_descriptor.='</a></td>'."\n";
                        if(isset($type_meta[$skill_meta[$entry['id']]['type']]))
                            $map_descriptor.='<td><span class="type_label type_label_'.$skill_meta[$entry['id']]['type'].'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$skill_meta[$entry['id']]['type'].'.html">'.ucfirst($type_meta[$skill_meta[$entry['id']]['type']]['name'][$current_lang]).'</a></span></td>'."\n";
                        else
                            $map_descriptor.='<td>&nbsp;</td>'."\n";
                        $map_descriptor.='</table>'."\n";
                        $map_descriptor.='</div></div>'."\n";
                    }
                    else
                        echo '$skill_meta[$entry[id]] not found'."\n";
                    break;
                }
        }
		if(isset($item_to_plant[$id]) && isset($plant_meta[$item_to_plant[$id]]))
		{
			$image='';
			if(file_exists($datapack_path.'plants/'.$item_to_plant[$id].'.png'))
				$image.=$base_datapack_site_path.'plants/'.htmlspecialchars($item_to_plant[$id]).'.png';
			elseif(file_exists($datapack_path.'plants/'.$item_to_plant[$id].'.gif'))
				$image.=$base_datapack_site_path.'plants/'.htmlspecialchars($item_to_plant[$id]).'.gif';
			if($image!='')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Plant'].'</div><div class="value">'."\n";
				$map_descriptor.=str_replace('[fruits]',$plant_meta[$item_to_plant[$id]]['quantity'],str_replace('[mins]',($plant_meta[$item_to_plant[$id]]['fruits']/60),$translation_list[$current_lang]['After <b>[mins]</b> minutes you will have <b>[fruits]</b> fruits']))."\n";
				$map_descriptor.='<table class="item_list item_list_type_normal">
				<tr class="item_list_title item_list_title_type_normal">
					<th>'.$translation_list[$current_lang]['Seed'].'</th>
					<th>'.$translation_list[$current_lang]['Sprouted'].'</th>
					<th>'.$translation_list[$current_lang]['Taller'].'</th>
					<th>'.$translation_list[$current_lang]['Flowering'].'</th>
					<th>'.$translation_list[$current_lang]['Fruits'].'</th>
				</tr><tr class="value">'."\n";
				$map_descriptor.='<td><center><div style="width:16px;height:32px;background-image:url(\''.$image.'\');background-repeat:no-repeat;background-position:0px 0px;"></div></center></td>'."\n";
				$map_descriptor.='<td><center><div style="width:16px;height:32px;background-image:url(\''.$image.'\');background-repeat:no-repeat;background-position:-16px 0px;"></div></center></td>'."\n";
				$map_descriptor.='<td><center><div style="width:16px;height:32px;background-image:url(\''.$image.'\');background-repeat:no-repeat;background-position:-32px 0px;"></div></center></td>'."\n";
				$map_descriptor.='<td><center><div style="width:16px;height:32px;background-image:url(\''.$image.'\');background-repeat:no-repeat;background-position:-48px 0px;"></div></center></td>'."\n";
				$map_descriptor.='<td><center><div style="width:16px;height:32px;background-image:url(\''.$image.'\');background-repeat:no-repeat;background-position:-64px 0px;"></div></center></td>'."\n";
				$map_descriptor.='</tr><tr>
				<td colspan="5" class="item_list_endline item_list_title_type_normal"></td>
				</tr>
				</table>'."\n";
				$map_descriptor.='</div></div>'."\n";
			}

            $requirements=$plant_meta[$item_to_plant[$id]]['requirements'];
            if(count($requirements)>0)
            {
                $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Requirements'].'</div><div class="value">'."\n";
                if(isset($requirements['quests']))
                {
                    foreach($requirements['quests'] as $quest_id)
                    {
                        $map_descriptor.=$translation_list[$current_lang]['Quest:'].' <a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['quests/'].$maindatapackcode.'/'.$quest_id.'-'.text_operation_do_for_url($quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang]).'.html" title="'.$quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang].'">'."\n";
                        $map_descriptor.=$quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang];
                        $map_descriptor.='</a><br />'."\n";
                    }
                }
                if(isset($requirements['reputation']))
                    foreach($requirements['reputation'] as $reputation)
                        $map_descriptor.=reputationLevelToText($reputation['type'],$reputation['level']).'<br />'."\n";
                $map_descriptor.='</div></div>'."\n";
            }
            $rewards=$plant_meta[$item_to_plant[$id]]['rewards'];
            if(count($rewards)>0)
            {
                $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Rewards'].'</div><div class="value">'."\n";
                if(isset($rewards['items']))
                {
                    $map_descriptor.='<table class="item_list item_list_type_outdoor"><tr class="item_list_title item_list_title_type_outdoor">
                    <th colspan="2">Item</th></tr>'."\n";
                    foreach($rewards['items'] as $item)
                    {
                        $map_descriptor.='<tr class="value"><td>'."\n";
                        if(isset($item_meta[$item['item']]))
                        {
                            $link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item['item']]['name'][$current_lang]);
                            $link.='.html';
                            $name=$item_meta[$item['item']]['name'][$current_lang];
                            if($item_meta[$item['item']]['image']!='')
                                $image=$base_datapack_site_path.'/items/'.$item_meta[$item['item']]['image'];
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
                        if($item['quantity']>1)
                            $quantity_text=$item['quantity'].' ';
                        
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
                            $map_descriptor.=$quantity_text.$name."\n";
                        else
                            $map_descriptor.=$quantity_text.'Unknown item'."\n";
                        if($link!='')
                            $map_descriptor.='</a>'."\n";
                        $map_descriptor.='</td></tr>'."\n";
                    }
                    $map_descriptor.='<tr>
                    <td colspan="2" class="item_list_endline item_list_title_type_outdoor"></td>
                    </tr></table>'."\n";
                }
                if(isset($rewards['reputation']))
                    foreach($rewards['reputation'] as $reputation)
                    {
                        if($reputation['point']<0)
                            $map_descriptor.=$translation_list[$current_lang]['Less reputation in:'].' '.reputationToText($reputation['type']);
                        else
                            $map_descriptor.=$translation_list[$current_lang]['More reputation in:'].' '.reputationToText($reputation['type']);
                    }
                if(isset($rewards['allow']))
                    foreach($rewards['allow'] as $allow)
                    {
                        if($allow=='clan')
                            $map_descriptor.=$translation_list[$current_lang]['Able to create clan'];
                        else
                            $map_descriptor.=$translation_list[$current_lang]['Allow'].' '.$allow;
                    }
                $map_descriptor.='</div></div>'."\n";
            }
		}
		if(isset($item['effect']))
		{
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Effect'].'</div><div class="value"><ul>'."\n";
			if(isset($item['effect']['regeneration']))
			{
				if($item['effect']['regeneration']=='all')
					$map_descriptor.='<li>'.$translation_list[$current_lang]['Regenerate all the hp'].'</li>'."\n";
				else
					$map_descriptor.='<li>Regenerate '.$item['effect']['regeneration'].' hp</li>'."\n";
			}
			if(isset($item['effect']['buff']))
			{
				if($item['effect']['buff']=='all')
					$map_descriptor.='<li>'.$translation_list[$current_lang]['Remove all the buff and debuff'].'</li>'."\n";
				else
				{
					$buff_id=$item['effect']['buff'];
					$map_descriptor.='<li>'.$translation_list[$current_lang]['Remove the buff:'];
					$map_descriptor.='<center><table><td>'."\n";
					if(file_exists($datapack_path.'/monsters/buff/'.$buff_id.'.png'))
						$map_descriptor.='<img src="'.$base_datapack_site_path.'monsters/buff/'.$buff_id.'.png" alt="" width="16" height="16" />'."\n";
					else
						$map_descriptor.='&nbsp;'."\n";
					$map_descriptor.='</td>'."\n";
					if(isset($buff_meta[$buff_id]))
						$map_descriptor.='<td>'.$translation_list[$current_lang]['Unknown buff'].'</td>'."\n";
					else
						$map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['buffs/'].text_operation_do_for_url($buff_meta[$buff_id]['name'][$current_lang]).'.html">'.$buff_meta[$buff_id]['name'][$current_lang].'</a></td>'."\n";
					$map_descriptor.='</table></center>'."\n";
					$map_descriptor.='</li>'."\n";
				}
			}
			$map_descriptor.='</ul></div></div>'."\n";
		}

        if(isset($item_to_crafting[$id]))
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Do the item'].'</div><div class="value">'."\n";
            $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item_to_crafting[$id]['doItemId']]['name'][$current_lang]).'.html" title="'.$item_meta[$item_to_crafting[$id]['doItemId']]['name'][$current_lang].'">'."\n";
                $map_descriptor.='<table><tr><td>'."\n";
                if($item_meta[$item_to_crafting[$id]['doItemId']]['image']!='' && file_exists($datapack_path.'items/'.$item_meta[$item_to_crafting[$id]['doItemId']]['image']))
                    $map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$item_to_crafting[$id]['doItemId']]['image'].'" width="24" height="24" alt="'.$item_meta[$item_to_crafting[$id]['doItemId']]['name'][$current_lang].'" title="'.$item_meta[$item_to_crafting[$id]['doItemId']]['name'][$current_lang].'" />'."\n";
                $map_descriptor.='</td><td>'.$item_meta[$item_to_crafting[$id]['doItemId']]['name'][$current_lang].'</td></tr></table>'."\n";
            $map_descriptor.='</a>'."\n";
            $map_descriptor.='</div></div>'."\n";

            $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Material'].'</div><div class="value">'."\n";
            foreach($item_to_crafting[$id]['material'] as $material=>$quantity)
            {
                $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$material]['name'][$current_lang]).'.html" title="'.$item_meta[$material]['name'][$current_lang].'">'."\n";
                    $map_descriptor.='<table><tr><td>'."\n";
                    if($item_meta[$material]['image']!='' && file_exists($datapack_path.'items/'.$item_meta[$material]['image']))
                        $map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$material]['image'].'" width="24" height="24" alt="'.$item_meta[$material]['name'][$current_lang].'" title="'.$item_meta[$material]['name'][$current_lang].'" />'."\n";
                    $map_descriptor.='</td><td>'."\n";
                if($quantity>1)
                    $map_descriptor.=$quantity.'x ';
                $map_descriptor.=$item_meta[$material]['name'][$current_lang].'</td></tr></table>'."\n";
                $map_descriptor.='</a>'."\n";
            }
            $map_descriptor.='</div></div>'."\n";
            $requirements=$item_to_crafting[$id]['requirements'];
            if(count($requirements)>0)
            {
                $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Requirements'].'</div><div class="value">'."\n";
                if(isset($requirements['quests']))
                {
                    foreach($requirements['quests'] as $quest_id)
                    {
                        $map_descriptor.=$translation_list[$current_lang]['Quest:'].' <a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['quests/'].$maindatapackcode.'/'.$quest_id.'-'.text_operation_do_for_url($quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang]).'.html" title="'.$quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang].'">'."\n";
                        $map_descriptor.=$quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang];
                        $map_descriptor.='</a><br />'."\n";
                    }
                }
                if(isset($requirements['reputation']))
                    foreach($requirements['reputation'] as $reputation)
                        $map_descriptor.=reputationLevelToText($reputation['type'],$reputation['level']).'<br />'."\n";
                $map_descriptor.='</div></div>'."\n";
            }
            $rewards=$item_to_crafting[$id]['rewards'];
            if(count($rewards)>0)
            {
                $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Rewards'].'</div><div class="value">'."\n";
                if(isset($rewards['items']))
                {
                    $map_descriptor.='<table class="item_list item_list_type_outdoor"><tr class="item_list_title item_list_title_type_outdoor">
                    <th colspan="2">Item</th></tr>'."\n";
                    foreach($rewards['items'] as $item)
                    {
                        $map_descriptor.='<tr class="value"><td>'."\n";
                        if(isset($item_meta[$item['item']]))
                        {
                            $link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item['item']]['name'][$current_lang]);
                            $link.='.html';
                            $name=$item_meta[$item['item']]['name'][$current_lang];
                            if($item_meta[$item['item']]['image']!='')
                                $image=$base_datapack_site_path.'/items/'.$item_meta[$item['item']]['image'];
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
                        if($item['quantity']>1)
                            $quantity_text=$item['quantity'].' ';
                        
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
                            $map_descriptor.=$quantity_text.$name."\n";
                        else
                            $map_descriptor.=$quantity_text.'Unknown item'."\n";
                        if($link!='')
                            $map_descriptor.='</a>'."\n";
                        $map_descriptor.='</td></tr>'."\n";
                    }
                    $map_descriptor.='<tr>
                    <td colspan="2" class="item_list_endline item_list_title_type_outdoor"></td>
                    </tr></table>'."\n";
                }
                if(isset($rewards['reputation']))
                    foreach($rewards['reputation'] as $reputation)
                    {
                        if($reputation['point']<0)
                            $map_descriptor.=$translation_list[$current_lang]['Less reputation in:'].' '.reputationToText($reputation['type']);
                        else
                            $map_descriptor.=$translation_list[$current_lang]['More reputation in:'].' '.reputationToText($reputation['type']);
                    }
                if(isset($rewards['allow']))
                    foreach($rewards['allow'] as $allow)
                    {
                        if($allow=='clan')
                            $map_descriptor.=$translation_list[$current_lang]['Able to create clan'];
                        else
                            $map_descriptor.=$translation_list[$current_lang]['Allow'].' '.$allow;
                    }
                $map_descriptor.='</div></div>'."\n";
            }
        }

        if(isset($doItemId_to_crafting[$id]))
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Product by crafting'].'</div><div class="value">'."\n";
            foreach($doItemId_to_crafting[$id] as $material)
            {
                $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$material]['name'][$current_lang]).'.html" title="'.$item_meta[$material]['name'][$current_lang].'">'."\n";
                    $map_descriptor.='<table><tr><td>'."\n";
                    if($item_meta[$material]['image']!='' && file_exists($datapack_path.'items/'.$item_meta[$material]['image']))
                        $map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$material]['image'].'" width="24" height="24" alt="'.$item_meta[$material]['name'][$current_lang].'" title="'.$item_meta[$material]['name'][$current_lang].'" />'."\n";
                    $map_descriptor.='</td><td>'."\n";
                $map_descriptor.=$item_meta[$material]['name'][$current_lang].'</td></tr></table>'."\n";
                $map_descriptor.='</a>'."\n";
            }
            $map_descriptor.='</div></div>'."\n";
        }

        if(isset($material_to_crafting[$id]))
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Used into crafting'].'</div><div class="value">'."\n";
            foreach($material_to_crafting[$id] as $material)
            {
                $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$material]['name'][$current_lang]).'.html" title="'.$item_meta[$material]['name'][$current_lang].'">'."\n";
                    $map_descriptor.='<table><tr><td>'."\n";
                    if($item_meta[$material]['image']!='' && file_exists($datapack_path.'items/'.$item_meta[$material]['image']))
                        $map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$material]['image'].'" width="24" height="24" alt="'.$item_meta[$material]['name'][$current_lang].'" title="'.$item_meta[$material]['name'][$current_lang].'" />'."\n";
                    $map_descriptor.='</td><td>'."\n";
                $map_descriptor.=$item_meta[$material]['name'][$current_lang].'</td></tr></table>'."\n";
                $map_descriptor.='</a>'."\n";
            }
            $map_descriptor.='</div></div>'."\n";
        }

		if(isset($item_to_evolution[$id]) && count($item_to_evolution[$id])>0)
		{
			$count_evol=0;
			foreach($item_to_evolution[$id] as $evolution)
			{
				if(isset($monster_meta[$evolution['from']]) && isset($monster_meta[$evolution['to']]))
					$count_evol++;
			}
			foreach($item_to_evolution[$id] as $evolution)
			{
				if(isset($monster_meta[$evolution['from']]) && isset($monster_meta[$evolution['to']]))
				{
                    $link='#';
					$map_descriptor.='<table class="item_list item_list_type_normal map_list">
					<tr class="item_list_title item_list_title_type_normal">
						<th colspan="'.$count_evol.'">'.$translation_list[$current_lang]['Evolve from'].'</th>
					</tr>'."\n";
					$map_descriptor.='<tr class="value">'."\n";
					$map_descriptor.='<td>'."\n";
					$map_descriptor.='<table class="monsterforevolution">'."\n";
					if(file_exists($datapack_path.'monsters/'.$evolution['from'].'/front.png'))
						$map_descriptor.='<tr><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['from']]['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$evolution['from'].'/front.png" width="80" height="80" alt="'.$monster_meta[$evolution['from']]['name'][$current_lang].'" title="'.$monster_meta[$evolution['from']]['name'][$current_lang].'" /></a></td></tr>'."\n";
					else if(file_exists($datapack_path.'monsters/'.$evolution['from'].'/front.gif'))
						$map_descriptor.='<tr><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['from']]['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$evolution['from'].'/front.gif" width="80" height="80" alt="'.$monster_meta[$evolution['from']]['name'][$current_lang].'" title="'.$monster_meta[$evolution['from']]['name'][$current_lang].'" /></a></td></tr>'."\n";
					$map_descriptor.='<tr><td class="evolution_name"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['from']]['name'][$current_lang]).'.html">'.$monster_meta[$evolution['from']]['name'][$current_lang].'</a></td></tr>'."\n";
					$map_descriptor.='</table>'."\n";
					$map_descriptor.='</td>'."\n";
					$map_descriptor.='</tr>'."\n";

					$map_descriptor.='<tr><td class="evolution_type">'.$translation_list[$current_lang]['Evolve with'].'<br /><a href="'.$link.'" title="'.$item_meta[$id]['name'][$current_lang].'">'."\n";
					if($item_meta[$id]['image']!='')
						$map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$id]['image'].'" alt="'.$item_meta[$id]['name'][$current_lang].'" title="'.$item_meta[$id]['name'][$current_lang].'" style="float:left;" />'."\n";
					$map_descriptor.=$item_meta[$id]['name'][$current_lang].'</a></td></tr>'."\n";

					$map_descriptor.='<tr class="value">'."\n";
					$map_descriptor.='<td>'."\n";
					$map_descriptor.='<table class="monsterforevolution">'."\n";
					if(file_exists($datapack_path.'monsters/'.$evolution['to'].'/front.png'))
						$map_descriptor.='<tr><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['to']]['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$evolution['to'].'/front.png" width="80" height="80" alt="'.$monster_meta[$evolution['to']]['name'][$current_lang].'" title="'.$monster_meta[$evolution['to']]['name'][$current_lang].'" /></a></td></tr>'."\n";
					else if(file_exists($datapack_path.'monsters/'.$evolution['to'].'/front.gif'))
						$map_descriptor.='<tr><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['to']]['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$evolution['to'].'/front.gif" width="80" height="80" alt="'.$monster_meta[$evolution['to']]['name'][$current_lang].'" title="'.$monster_meta[$evolution['to']]['name'][$current_lang].'" /></a></td></tr>'."\n";
					$map_descriptor.='<tr><td class="evolution_name"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_meta[$evolution['to']]['name'][$current_lang]).'.html">'.$monster_meta[$evolution['to']]['name'][$current_lang].'</a></td></tr>'."\n";
					$map_descriptor.='</table>'."\n";
					$map_descriptor.='</td>'."\n";
					$map_descriptor.='</tr>'."\n";

					$map_descriptor.='<tr>
						<th colspan="'.$count_evol.'" class="item_list_endline item_list_title item_list_title_type_normal">'.$translation_list[$current_lang]['Evolve to'].'</th>
					</tr>
					</table>'."\n";
				}
			}
			$map_descriptor.='<br style="clear:both" />'."\n";
		}

	$map_descriptor.='</div>'."\n";

    //shop
    if(isset($item_to_shop[$id]))
    {
        //$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Shop'].'</div><div class="value">'."\n";
        $map_descriptor.='<table class="item_list item_list_type_normal">
        <tr class="item_list_title item_list_title_type_normal">
            <th colspan="4">'.$translation_list[$current_lang]['Shop'].'</th>
        </tr>'."\n";
        foreach($item_to_shop[$id] as $maindatapackcode=>$shop_list)
        {
            $bot_list=array();
            foreach($shop_list as $shop)
                if(isset($shop_to_bot[$shop][$maindatapackcode]))
                    $bot_list=array_merge($bot_list,$shop_to_bot[$shop][$maindatapackcode]);
            if(count($bot_list)>0)
            {
                foreach($bot_list as $bot_id)
                {
                    $map_descriptor.='<tr class="value">'."\n";
                    if(isset($bots_meta[$maindatapackcode][$bot_id]))
                    {
                        $bot=$bots_meta[$maindatapackcode][$bot_id];
                        if($bot['name'][$current_lang]=='')
                            $final_url_name='bot-'.$bot_id;
                        else if($bots_name_count[$maindatapackcode][$current_lang][text_operation_do_for_url($bot['name'][$current_lang])]==1)
                            $final_url_name=$bot['name'][$current_lang];
                        else
                            $final_url_name=$bot_id.'-'.$bot['name'][$current_lang];
                        if($bot['name'][$current_lang]=='')
                            $final_name='Bot #'.$bot_id;
                        else
                            $final_name=$bot['name'][$current_lang];
                        $skin_found=true;
                        if(isset($bot_id_to_skin[$bot_id][$maindatapackcode]))
                        {
                            if(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
                                $map_descriptor.='<td><center><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png\');background-repeat:no-repeat;background-position:-16px -48px;"></div></center></td>'."\n";
                            elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
                                $map_descriptor.='<td><center><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png\');background-repeat:no-repeat;background-position:-16px -48px;"></div></center></td>'."\n";
                            elseif(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
                                $map_descriptor.='<td><center><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif\');background-repeat:no-repeat;background-position:-16px -48px;"></div></center></td>'."\n";
                            elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
                                $map_descriptor.='<td><center><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif\');background-repeat:no-repeat;background-position:-16px -48px;"></div></center></td>'."\n";
                            else
                                $skin_found=false;
                        }
                        else
                            $skin_found=false;
                        $map_descriptor.='<td';
                        if(!$skin_found)
                            $map_descriptor.=' colspan="2"';
                        $map_descriptor.='><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['bots/'].$maindatapackcode.'/'.text_operation_do_for_url($final_url_name).'.html" title="'.$final_name.'">'.$final_name.'</a></td>'."\n";
                        if(isset($bot_id_to_map[$bot_id][$maindatapackcode]))
                        {
                            $entry=$bot_id_to_map[$bot_id][$maindatapackcode];
                            if(isset($maps_list[$maindatapackcode][$entry['map']]))
                            {
                                $item_current_map=$maps_list[$maindatapackcode][$entry['map']];
                                if(isset($zone_meta[$maindatapackcode][$item_current_map['zone']]))
                                {
                                    $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$entry['map']).'" title="'.$item_current_map['name'][$current_lang].'">'.$item_current_map['name'][$current_lang].'</a></td>'."\n";
                                    $map_descriptor.='<td>'.$zone_meta[$maindatapackcode][$item_current_map['zone']]['name'][$current_lang].'</td>'."\n";
                                }
                                else
                                    $map_descriptor.='<td colspan="2"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$entry['map']).'" title="'.$item_current_map['name'][$current_lang].'">'.$item_current_map['name'][$current_lang].'</a></td>'."\n";
                            }
                            else
                                $map_descriptor.='<td colspan="2">'.$translation_list[$current_lang]['Unknown map'].'</td>'."\n";
                        }
                        else
                            $map_descriptor.='<td colspan="2">&nbsp;'.$final_name.' not found for '.$maindatapackcode.' for map</td>'."\n";
                    }
                    else
                    {
                        $map_descriptor.='<td colspan="4">Bot id not found: '.$bot_id.' for '.$maindatapackcode.'</td>'."\n";
                        echo 'Bot id not found: '.$bot_id.' for '.$maindatapackcode."\n";
                    }
                    $map_descriptor.='</tr>'."\n";
                }
            }
        }

        $map_descriptor.='<tr>
            <td colspan="4" class="item_list_endline item_list_title_type_normal"></td>
        </tr>
        </table>'."\n";
        //$map_descriptor.='</div></div>'."\n";

    }

	if(isset($item_to_monster[$id]))
	{
        $only_one=true;
        foreach($item_to_monster[$id] as $item_to_monster_list)
            if($item_to_monster_list['quantity_min']!=1 || $item_to_monster_list['quantity_max']!=1)
                $only_one=false;
		$map_descriptor.='<table class="itemmonster_list item_list item_list_type_normal">
		<tr class="item_list_title item_list_title_type_normal">
			<th colspan="2">'.$translation_list[$current_lang]['Monster'].'</th>'."\n";
            if(!$only_one)
                $map_descriptor.='<th>'.$translation_list[$current_lang]['Quantity'].'</th>'."\n";
			$map_descriptor.='<th>'.$translation_list[$current_lang]['Luck'].'</th>
		</tr>'."\n";
        $monster_count=0;
		foreach($item_to_monster[$id] as $item_to_monster_list)
		{
			if(isset($monster_meta[$item_to_monster_list['monster']]))
			{
                if($monster_count%10==0 && $monster_count!=0)
                {
                    $map_descriptor.='<tr>'."\n";
                    if(!$only_one)
                        $map_descriptor.='<td colspan="4" class="item_list_endline item_list_title_type_normal"></td>'."\n";
                    else
                        $map_descriptor.='<td colspan="3" class="item_list_endline item_list_title_type_normal"></td>'."\n";
                    $map_descriptor.='</tr>
                    </table>'."\n";

                    $map_descriptor.='<table class="itemmonster_list item_list item_list_type_normal">
                    <tr class="item_list_title item_list_title_type_normal">
                        <th colspan="2">'.$translation_list[$current_lang]['Monster'].'</th>'."\n";
                        if(!$only_one)
                            $map_descriptor.='<th>'.$translation_list[$current_lang]['Quantity'].'</th>'."\n";
                        $map_descriptor.='<th>'.$translation_list[$current_lang]['Luck'].'</th>
                    </tr>'."\n";
                }
                $monster_count++;
				if($item_to_monster_list['quantity_min']!=$item_to_monster_list['quantity_max'])
					$quantity_text=$item_to_monster_list['quantity_min'].' to '.$item_to_monster_list['quantity_max'];
				else
					$quantity_text=$item_to_monster_list['quantity_min'];
				$name=$monster_meta[$item_to_monster_list['monster']]['name'][$current_lang];
				$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
                $link.='.html';
				$map_descriptor.='<tr class="value">'."\n";
				$map_descriptor.='<td>'."\n";
				if(file_exists($datapack_path.'monsters/'.$item_to_monster_list['monster'].'/small.png'))
					$map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$item_to_monster_list['monster'].'/small.png" width="32" height="32" alt="'.$monster_meta[$item_to_monster_list['monster']]['name'][$current_lang].'" title="'.$monster_meta[$item_to_monster_list['monster']]['name'][$current_lang].'" /></a></div>'."\n";
				else if(file_exists($datapack_path.'monsters/'.$item_to_monster_list['monster'].'/small.gif'))
					$map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$item_to_monster_list['monster'].'/small.gif" width="32" height="32" alt="'.$monster_meta[$item_to_monster_list['monster']]['name'][$current_lang].'" title="'.$monster_meta[$item_to_monster_list['monster']]['name'][$current_lang].'" /></a></div>'."\n";
				$map_descriptor.='</td>
				<td><a href="'.$link.'">'.$name.'</a></td>'."\n";
                if(!$only_one)
                    $map_descriptor.='<td>'.$quantity_text.'</td>'."\n";
				$map_descriptor.='<td>'.$item_to_monster_list['luck'].'%</td>'."\n";
				$map_descriptor.='</tr>'."\n";
			}
            else
                echo '$item_to_monster[$id] monster not found'."\n";
		}
		$map_descriptor.='<tr>'."\n";
        if(!$only_one)
			$map_descriptor.='<td colspan="4" class="item_list_endline item_list_title_type_normal"></td>'."\n";
        else
            $map_descriptor.='<td colspan="3" class="item_list_endline item_list_title_type_normal"></td>'."\n";
		$map_descriptor.='</tr>
		</table>'."\n";
	}
    $map_descriptor.='<br style="clear:both;" />'."\n";

	if(isset($items_to_quests[$id]))
	{
		$map_descriptor.='<table class="item_list item_list_type_normal">
		<tr class="item_list_title item_list_title_type_normal">
			<th>'.$translation_list[$current_lang]['Quests'].'</th>
			<th>'.$translation_list[$current_lang]['Quantity rewarded'].'</th>
		</tr>'."\n";
		foreach($items_to_quests[$id] as $maindatapackcode=>$quest_list)
        foreach($quest_list as $quest_id=>$quantity)
		{
			if(isset($quests_meta[$maindatapackcode][$quest_id]))
			{
				$map_descriptor.='<tr class="value">'."\n";
				$map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['quests/'].$maindatapackcode.'/'.$quest_id.'-'.text_operation_do_for_url($quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang]).'.html" title="'.$quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang].'">'."\n";
				$map_descriptor.=$quests_meta[$maindatapackcode][$quest_id]['name'][$current_lang];
				$map_descriptor.='</a></td>'."\n";
				$map_descriptor.='<td>'.$quantity.'</td>'."\n";
				$map_descriptor.='</tr>'."\n";
			}
		}
		$map_descriptor.='<tr>
			<td colspan="2" class="item_list_endline item_list_title_type_normal"></td>
		</tr>
		</table>'."\n";
	}
	if(isset($items_to_quests_for_step[$id]))
	{
		$full_details=false;
        foreach($items_to_quests_for_step[$id] as $maindatapackcode=>$quest_list)
        foreach($quest_list as $items_to_quests_for_step_details)
		{
			if(isset($quests_meta[$maindatapackcode][$items_to_quests_for_step_details['quest']]))
				if(isset($items_to_quests_for_step_details['monster']) && isset($monster_meta[$items_to_quests_for_step_details['monster']]))
					$full_details=true;
		}
		if($full_details)
			$map_descriptor.='<table class="item_list item_list_type_normal">
			<tr class="item_list_title item_list_title_type_normal">
				<th>'.$translation_list[$current_lang]['Quests'].'</th>
				<th>'.$translation_list[$current_lang]['Quantity needed'].'</th>
				<th colspan="2">'.$translation_list[$current_lang]['Monster'].'</th>
				<th>'.$translation_list[$current_lang]['Luck'].'</th>
			</tr>'."\n";
		else
			$map_descriptor.='<table class="item_list item_list_type_normal">
			<tr class="item_list_title item_list_title_type_normal">
				<th>'.$translation_list[$current_lang]['Quests'].'</th>
				<th>'.$translation_list[$current_lang]['Quantity needed'].'</th>
			</tr>'."\n";
		foreach($items_to_quests_for_step[$id] as $maindatapackcode=>$quest_list)
        foreach($quest_list as $items_to_quests_for_step_details)
		{
			if(isset($quests_meta[$maindatapackcode][$items_to_quests_for_step_details['quest']]))
			{
				$map_descriptor.='<tr class="value">'."\n";
				$map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['quests/'].$maindatapackcode.'/'.$items_to_quests_for_step_details['quest'].'-'.text_operation_do_for_url($quests_meta[$maindatapackcode][$items_to_quests_for_step_details['quest']]['name'][$current_lang]).'.html" title="'.$quests_meta[$maindatapackcode][$items_to_quests_for_step_details['quest']]['name'][$current_lang].'">'."\n";
				$map_descriptor.=$quests_meta[$maindatapackcode][$items_to_quests_for_step_details['quest']]['name'][$current_lang];
				$map_descriptor.='</a></td>'."\n";
				$map_descriptor.='<td>'.$items_to_quests_for_step_details['quantity'].'</td>'."\n";
				if(isset($items_to_quests_for_step_details['monster']) && isset($monster_meta[$items_to_quests_for_step_details['monster']]))
				{
					$name=$monster_meta[$items_to_quests_for_step_details['monster']]['name'][$current_lang];
					$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
                    $link.='.html';
					$map_descriptor.='<td>'."\n";
					if(file_exists($datapack_path.'monsters/'.$items_to_quests_for_step_details['monster'].'/small.png'))
						$map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$items_to_quests_for_step_details['monster'].'/small.png" width="32" height="32" alt="'.$monster_meta[$items_to_quests_for_step_details['monster']]['name'][$current_lang].'" title="'.$monster_meta[$items_to_quests_for_step_details['monster']]['name'][$current_lang].'" /></a></div>'."\n";
					else if(file_exists($datapack_path.'monsters/'.$items_to_quests_for_step_details['monster'].'/small.gif'))
						$map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$items_to_quests_for_step_details['monster'].'/small.gif" width="32" height="32" alt="'.$monster_meta[$items_to_quests_for_step_details['monster']]['name'][$current_lang].'" title="'.$monster_meta[$items_to_quests_for_step_details['monster']]['name'][$current_lang].'" /></a></div>'."\n";
					$map_descriptor.='</td>
					<td><a href="'.$link.'">'.$name.'</a></td>'."\n";
					$map_descriptor.='<td>'.$items_to_quests_for_step_details['rate'].'%</td>'."\n";
				}
				else if($full_details)
					$map_descriptor.='<td></td><td></td><td></td>'."\n";
				$map_descriptor.='</tr>'."\n";
			}
		}
		if($full_details)
			$map_descriptor.='<tr>
				<td colspan="5" class="item_list_endline item_list_title_type_normal"></td>
			</tr>
			</table>'."\n";
		else
			$map_descriptor.='<tr>
				<td colspan="2" class="item_list_endline item_list_title_type_normal"></td>
			</tr>
			</table>'."\n";
	}

	if(isset($item_consumed_by[$id]))
	{
		$map_descriptor.='<table class="item_list item_list_type_normal">
		<tr class="item_list_title item_list_title_type_normal">
			<th>'.$translation_list[$current_lang]['Resource of the industry'].'</th>
			<th>'.$translation_list[$current_lang]['Quantity'].'</th>
		</tr>'."\n";
        foreach($item_consumed_by[$id] as $industrie_path=>$id_list)
        {
            foreach($id_list as $industry_id=>$quantity)
            {
                if(isset($industrie_meta[$industrie_path][$industry_id]))
                {
                    $map_descriptor.='<tr class="value">'."\n";
                    if($industrie_path=='')
                    {
                        $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['industries/'].$industry_id.'.html">'."\n";
                        $map_descriptor.=str_replace('[industryid]',$industry_id,$translation_list[$current_lang]['Industry #[industryid]']);
                    }
                    else
                    {
                        $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['industries/'].$industrie_path.'/'.$industry_id.'.html">'."\n";
                        $map_descriptor.=str_replace('[industryid]',$industry_id,$translation_list[$current_lang]['Industry #[industryid]']);
                    }
                    $map_descriptor.='</a></td>'."\n";
                    $map_descriptor.='<td>'.$quantity.'</td>'."\n";
                    $map_descriptor.='</tr>'."\n";
                }
            }
        }
		$map_descriptor.='<tr>
			<td colspan="2" class="item_list_endline item_list_title_type_normal"></td>
		</tr>
		</table>'."\n";
	}

	if(isset($item_produced_by[$id]))
	{
		$map_descriptor.='<table class="item_list item_list_type_normal">
		<tr class="item_list_title item_list_title_type_normal">
			<th>'.$translation_list[$current_lang]['Product of the industry'].'</th>
			<th>'.$translation_list[$current_lang]['Quantity'].'</th>
		</tr>'."\n";
		foreach($item_produced_by[$id] as $industrie_path=>$id_list)
		{
            foreach($id_list as $industry_id=>$quantity)
            {
                if(isset($industrie_meta[$industrie_path][$industry_id]))
                {
                    $map_descriptor.='<tr class="value">'."\n";
                    if($industrie_path=='')
                    {
                        $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['industries/'].$industry_id.'.html">'."\n";
                        $map_descriptor.=str_replace('[industryid]',$industry_id,$translation_list[$current_lang]['Industry #[industryid]']);
                    }
                    else
                    {
                        $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['industries/'].$industrie_path.'/'.$industry_id.'.html">'."\n";
                        $map_descriptor.=str_replace('[industryid]',$industry_id,$translation_list[$current_lang]['Industry #[industryid]']);
                    }
                    $map_descriptor.='</a></td>'."\n";
                    $map_descriptor.='<td>'.$quantity.'</td>'."\n";
                    $map_descriptor.='</tr>'."\n";
                }
            }
		}
		$map_descriptor.='<tr>
			<td colspan="2" class="item_list_endline item_list_title_type_normal"></td>
		</tr>
		</table>'."\n";
	}

    if(isset($item_to_skill_of_monster[$id]))
    {
        $map_descriptor.='<table class="itemskillmonster item_list item_list_type_normal">
        <tr class="item_list_title item_list_title_type_normal">
            <th colspan="3">'.$translation_list[$current_lang]['Monster able to learn'].'</th>'."\n";
        if(!$sameuniqueskill)
            $map_descriptor.='<th>'.$translation_list[$current_lang]['Skill'].'</th>
            <th>'.$translation_list[$current_lang]['Type'].'</th>'."\n";
        $map_descriptor.='</tr>'."\n";
        $itemskillmonster_count=0;
        foreach($item_to_skill_of_monster[$id] as $entry)
        {
            if($itemskillmonster_count%10==0 && $itemskillmonster_count!=0)
            {
                $map_descriptor.='<tr>
                    <td colspan="';
                if(!$sameuniqueskill)
                    $map_descriptor.='5';
                else
                    $map_descriptor.='3';
                $map_descriptor.='" class="item_list_endline item_list_title_type_normal"></td>
                </tr>
                </table>'."\n";

                $map_descriptor.='<table class="itemskillmonster item_list item_list_type_normal">
                <tr class="item_list_title item_list_title_type_normal">
                    <th colspan="3">'.$translation_list[$current_lang]['Monster able to learn'].'</th>'."\n";
                if(!$sameuniqueskill)
                    $map_descriptor.='<th>'.$translation_list[$current_lang]['Skill'].'</th>
                    <th>'.$translation_list[$current_lang]['Type'].'</th>'."\n";
                $map_descriptor.='</tr>'."\n";
            }
            $itemskillmonster_count++;
            $map_descriptor.='<tr class="value">'."\n";
            if(isset($monster_meta[$entry['monster']]))
            {
                $name=$monster_meta[$entry['monster']]['name'][$current_lang];
                $link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
                $link.='.html';
                $map_descriptor.='<td>'."\n";
                if(file_exists($datapack_path.'monsters/'.$entry['monster'].'/small.png'))
                    $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$entry['monster'].'/small.png" width="32" height="32" alt="'.$monster_meta[$entry['monster']]['name'][$current_lang].'" title="'.$monster_meta[$entry['monster']]['name'][$current_lang].'" /></a></div>'."\n";
                else if(file_exists($datapack_path.'monsters/'.$entry['monster'].'/small.gif'))
                    $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$entry['monster'].'/small.gif" width="32" height="32" alt="'.$monster_meta[$entry['monster']]['name'][$current_lang].'" title="'.$monster_meta[$entry['monster']]['name'][$current_lang].'" /></a></div>'."\n";
                $map_descriptor.='</td>
                <td><a href="'.$link.'">'.$name.'</a></td>'."\n";
                $type_list=array();
                foreach($monster_meta[$entry['monster']]['type'] as $type_monster)
                    if(isset($type_meta[$type_monster]))
                        $type_list[]='<span class="type_label type_label_'.$type_monster.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type_monster.'.html">'.ucfirst($type_meta[$type_monster]['name'][$current_lang]).'</a></span>'."\n";
                $map_descriptor.='<td><div class="type_label_list">'.implode(' ',$type_list).'</div></td>'."\n";
            }
            else
                echo '$item_to_skill_of_monster[$id] not found'."\n";
            if(!$sameuniqueskill)
            {
                if(isset($skill_meta[$entry['id']]))
                {
                    $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['skills/'].text_operation_do_for_url($skill_meta[$entry['id']]['name'][$current_lang]).'.html">'.$skill_meta[$entry['id']]['name'][$current_lang];
                    if($entry['attack_level']>1)
                        $map_descriptor.=' at level '.$entry['attack_level'];
                    $map_descriptor.='</a></td>'."\n";
                    if(isset($type_meta[$skill_meta[$entry['id']]['type']]))
                        $map_descriptor.='<td><span class="type_label type_label_'.$skill_meta[$entry['id']]['type'].'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$skill_meta[$entry['id']]['type'].'.html">'.ucfirst($type_meta[$skill_meta[$entry['id']]['type']]['name'][$current_lang]).'</a></span></td>'."\n";
                    else
                        $map_descriptor.='<td>&nbsp;</td>'."\n";
                }
                else
                    echo '$skill_meta[$entry[id]] not found'."\n";
            }
            $map_descriptor.='</tr>'."\n";
        }
        $map_descriptor.='<tr>
            <td colspan="';
        if(!$sameuniqueskill)
            $map_descriptor.='5';
        else
            $map_descriptor.='3';
        $map_descriptor.='" class="item_list_endline item_list_title_type_normal"></td>
        </tr>
        </table>'."\n";

        $map_descriptor.='<br style="clear:both;" />'."\n";

    }

    if(isset($item_to_map[$id]))
    {
        $map_descriptor.='<table class="item_list item_list_type_normal">
        <tr class="item_list_title item_list_title_type_normal">
            <th colspan="2">'.$translation_list[$current_lang]['On the map'].'</th>
        </tr>'."\n";
        foreach($item_to_map[$id] as $entry)
        {
            $map_descriptor.='<tr class="value">'."\n";
            $maindatapackcode=$entry['maindatapackcode'];
                if(isset($maps_list[$maindatapackcode][$entry['map']]))
                {
                    $item_current_map=$maps_list[$maindatapackcode][$entry['map']];
                    if(isset($zone_meta[$maindatapackcode][$item_current_map['zone']]))
                    {
                        $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$entry['map']).'" title="'.$item_current_map['name'][$current_lang].'">'.$item_current_map['name'][$current_lang].'</a></td>'."\n";
                        $map_descriptor.='<td>'.$zone_meta[$maindatapackcode][$item_current_map['zone']]['name'][$current_lang].'</td>'."\n";
                    }
                    else
                        $map_descriptor.='<td colspan="2"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$entry['map']).'" title="'.$item_current_map['name'][$current_lang].'">'.$item_current_map['name'][$current_lang].'</a></td>'."\n";
                }
                else
                    $map_descriptor.='<td colspan="2">'.$translation_list[$current_lang]['Unknown map'].'</td>'."\n";
                $map_descriptor.='</tr>'."\n";
        }
        $map_descriptor.='<tr>
            <td colspan="2" class="item_list_endline item_list_title_type_normal"></td>
        </tr>
        </table>'."\n";
    }

    $fights_for_items_list=array();
    if(isset($item_to_fight[$id]))
        foreach($item_to_fight[$id] as $maindatapackcode=>$fight_list)
        {
            foreach($fight_list as $fight)
                if(isset($fight_to_bot[$maindatapackcode][$fight]))
                    foreach($fight_to_bot[$maindatapackcode][$fight] as $bot)
                        $fights_for_items_list[]=$bot;
            if(count($fights_for_items_list)>0)
            {
                $map_descriptor.='<table class="item_list item_list_type_normal">
                <tr class="item_list_title item_list_title_type_normal">
                    <th colspan="2">'.$translation_list[$current_lang]['Fight'].'</th>
                    <th>'.$translation_list[$current_lang]['Monster'].'</th>
                </tr>'."\n";
                foreach($fights_for_items_list as $bot)
                {
                    if($bots_meta[$maindatapackcode][$bot]['name'][$current_lang]=='')
                        $link=text_operation_do_for_url('bot '.$bot);
                    else if($bots_name_count[$maindatapackcode][$current_lang][text_operation_do_for_url($bots_meta[$maindatapackcode][$bot]['name'][$current_lang])]==1)
                        $link=text_operation_do_for_url($bots_meta[$maindatapackcode][$bot]['name'][$current_lang]);
                    else
                        $link=text_operation_do_for_url($bot.'-'.$bots_meta[$maindatapackcode][$bot]['name'][$current_lang]);
                    $bot_id=$bot;
                    $bot=$bots_meta[$maindatapackcode][$bot_id];
                    foreach($bot['step'] as $step_id=>$step)
                    {
                        if($step['type']=='fight')
                        {
                            $map=$bot_id_to_map[$bot_id][$maindatapackcode]['map'];
                            if(!isset($map_to_function[$maindatapackcode][$map]))
                                $map_to_function[$maindatapackcode][$map]=array();
                            if(!isset($map_to_function[$maindatapackcode][$map][$step['type']]))
                                $map_to_function[$maindatapackcode][$map][$step['type']]=1;
                            else
                                $map_to_function[$maindatapackcode][$map][$step['type']]++;

                            if(!isset($zone_to_function[$maindatapackcode][$maps_list[$maindatapackcode][$map]['zone']]))
                                $zone_to_function[$maindatapackcode][$maps_list[$maindatapackcode][$map]['zone']]=array();
                            if(!isset($zone_to_function[$maindatapackcode][$maps_list[$maindatapackcode][$map]['zone']][$step['type']]))
                                $zone_to_function[$maindatapackcode][$maps_list[$maindatapackcode][$map]['zone']][$step['type']]=1;
                            else
                                $zone_to_function[$maindatapackcode][$maps_list[$maindatapackcode][$map]['zone']][$step['type']]++;

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
                            
                            $map_descriptor.='<td>'."\n";
                            if(isset($fight_meta[$maindatapackcode][$step['fightid']]['monsters']))
                            {
                                foreach($fight_meta[$maindatapackcode][$step['fightid']]['monsters'] as $monster)
                                    $map_descriptor.=monsterAndLevelToDisplay($monster,$step['leader']);
                            }
                            else
                                echo 'Unable to resolv: $fight_meta['.$maindatapackcode.']['.$step['fightid'].'][\'monsters\']'."\n";

                            $map_descriptor.='</td>'."\n";
                        }
                        $map_descriptor.='</tr>'."\n";
                    }
                }
                $map_descriptor.='<tr>
                    <td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
                </tr>
                </table>'."\n";
            }

    }

    $content=$template;
    $content=str_replace('${TITLE}',$item['name'][$current_lang],$content);
    $content=str_replace('${CONTENT}',$map_descriptor,$content);
    $content=str_replace('${AUTOGEN}',$automaticallygen,$content);
    $content=clean_html($content);
    $filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item['name'][$current_lang]).'.html';
    if(file_exists($filedestination))
        die('Item The file already exists: '.$filedestination);
    filewrite($filedestination,$content);
}
