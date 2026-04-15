<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator skills'."\n");

$only_one_level=true;
foreach($skill_meta as $skill_id=>$skill)
{
    foreach($skill['level_list'] as $level=>$effect)
    {
        if(count($skill['level_list'])>1)
            $only_one_level=false;
    }
}


foreach($skill_meta as $skill_id=>$skill)
{
    $skill_monster_duplicate=array();
	if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['monsters/']))
		mkdir($datapack_explorer_local_path.$translation_list[$current_lang]['monsters/'],0777,true);
	if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['skills/']))
		mkdir($datapack_explorer_local_path.$translation_list[$current_lang]['skills/'],0777,true);
	$map_descriptor='';

	$type=$skill['type'];
	$effectiveness_list=array();
	if(isset($type_meta[$type]))
	{
		foreach($type_meta[$type]['multiplicator'] as $temp_type=>$multiplicator)
		{
			if(!isset($effectiveness_list[(string)$multiplicator]))
				$effectiveness_list[(string)$multiplicator]=array();
			$effectiveness_list[(string)$multiplicator][]=$temp_type;
		}
	}
	$map_descriptor.='<div class="map monster_type_'.$type.'">'."\n";
		$map_descriptor.='<div class="subblock"><h1>'.$skill['name'][$current_lang].'</h1><h2>#'.$skill_id.'</h2></div>'."\n";
		$map_descriptor.='<div class="type_label_list"><span class="type_label type_label_'.$type.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span></div>'."\n";
		if(isset($effectiveness_list['2']) || isset($effectiveness_list['4']))
		{
			
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Effective against'].'</div><div class="value">'."\n";
			$type_list=array();
			if(isset($effectiveness_list['2']))
				foreach($effectiveness_list['2'] as $type_effectiveness)
					if(isset($type_meta[$type_effectiveness]))
						$type_list[]='<span class="type_label type_label_'.$type_effectiveness.'">2x: <a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type_effectiveness.'.html">'.ucfirst($type_meta[$type_effectiveness]['name'][$current_lang]).'</a></span>'."\n";
			if(isset($effectiveness_list['4']))
				foreach($effectiveness_list['4'] as $type_effectiveness)
					if(isset($type_meta[$type_effectiveness]))
						$type_list[]='<span class="type_label type_label_'.$type_effectiveness.'">4x: <a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type_effectiveness.'.html">'.ucfirst($type_meta[$type_effectiveness]['name'][$current_lang]).'</a></span>'."\n";
			$map_descriptor.=implode(' ',$type_list);
			$map_descriptor.='</div></div>'."\n";
		}
		if(isset($effectiveness_list['0.25']) || isset($effectiveness_list['0.5']))
		{
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Not effective against'].'</div><div class="value">'."\n";
			$type_list=array();
			if(isset($effectiveness_list['0.25']))
				foreach($effectiveness_list['0.25'] as $type_effectiveness)
					if(isset($type_meta[$type_effectiveness]))
						$type_list[]='<span class="type_label type_label_'.$type_effectiveness.'">0.25x: <a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type_effectiveness.'.html">'.ucfirst($type_meta[$type_effectiveness]['name'][$current_lang]).'</a></span>'."\n";
			if(isset($effectiveness_list['0.5']))
				foreach($effectiveness_list['0.5'] as $type_effectiveness)
					if(isset($type_meta[$type_effectiveness]))
						$type_list[]='<span class="type_label type_label_'.$type_effectiveness.'">0.5x: <a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type_effectiveness.'.html">'.ucfirst($type_meta[$type_effectiveness]['name'][$current_lang]).'</a></span>'."\n";
			$map_descriptor.=implode(' ',$type_list);
			$map_descriptor.='</div></div>'."\n";
		}
		if(isset($effectiveness_list['0']))
		{
			$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Useless against'].'</div><div class="value">'."\n";
			$type_list=array();
			foreach($effectiveness_list['0'] as $type_effectiveness)
				if(isset($type_meta[$type_effectiveness]))
					$type_list[]='<span class="type_label type_label_'.$type_effectiveness.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type_effectiveness.'.html">'.ucfirst($type_meta[$type_effectiveness]['name'][$current_lang]).'</a></span>'."\n";
			$map_descriptor.=implode(' ',$type_list);
			$map_descriptor.='</div></div>'."\n";
		}
		foreach($skill['level_list'] as $level=>$effect)
		{
            $map_descriptor.='<div class="subblock">'."\n";
            if(count($skill['level_list'])>1)
                $map_descriptor.='<div class="valuetitle">'.$translation_list[$current_lang]['Level'].' '.$level.'</div>'."\n";
            $map_descriptor.='<div class="value">'."\n";
			$map_descriptor.=$translation_list[$current_lang]['Endurance'].': '.$effect['endurance'].'<br />'."\n";
			if($effect['sp']!='0')
				$map_descriptor.=$translation_list[$current_lang]['Skill point (SP) to learn'].': '.$effect['sp'].'<br />'."\n";
			else
				$map_descriptor.=$translation_list[$current_lang]['You can\'t learn this skill'].'<br />'."\n";
			if($effect['life_quantity']!='0' || $effect['life_quantity']!='0%')
				$map_descriptor.='Life quantity: '.$effect['life_quantity'].'<br />'."\n";
			if(count($effect['buff'])>0)
			{
				$map_descriptor.=$translation_list[$current_lang]['Add buff:'];
				$map_descriptor.='<center><table class="item_list item_list_type_'.$type.'">
				<tr class="item_list_title item_list_title_type_'.$type.'">
					<th colspan="2">'.$translation_list[$current_lang]['Buff'].'</th>
					<th>'.$translation_list[$current_lang]['Success'].'</th>
				</tr>'."\n";
				foreach($effect['buff'] as $buff)
				{
					$buff_id=$buff['id'];
					$map_descriptor.='<tr class="value"><td>'."\n";
					if(file_exists($datapack_path.'/monsters/buff/'.$buff_id.'.png'))
						$map_descriptor.='<img src="'.$base_datapack_site_path.'monsters/buff/'.$buff_id.'.png" alt="" width="16" height="16" />'."\n";
					else
						$map_descriptor.='&nbsp;'."\n";
					$map_descriptor.='</td>'."\n";
					$map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['buffs/'].text_operation_do_for_url($buff_meta[$buff_id]['name'][$current_lang]).'.html">'.$buff_meta[$buff_id]['name'][$current_lang].'</a></td>'."\n";
					$map_descriptor.='<td>'.$buff['success'].'%</td>'."\n";
					$map_descriptor.='</tr>'."\n";
				}
				$map_descriptor.='<tr>
				<td colspan="3" class="item_list_endline item_list_title_type_'.$type.'"></td>
				</tr>
				</table></center>'."\n";
			}
			if($effect['base_level_luck']!='100')
				$map_descriptor.=$translation_list[$current_lang]['Luck:'].' '.$effect['base_level_luck'].'%<br />'."\n";
			$map_descriptor.='</div></div>'."\n";
		}
	$map_descriptor.='</div>'."\n";

	$skill_level_displayed=0;
	if(isset($skill_to_monster[$skill_id]) && count($skill_to_monster[$skill_id])>0)
	{
		$map_descriptor.='<table class="item_list item_list_type_'.$type.' skill_list">
		<tr class="item_list_title item_list_title_type_'.$type.'">
			<th colspan="2">'.$translation_list[$current_lang]['Monster'].'</th>
			<th>'.$translation_list[$current_lang]['Type'].'</th>'."\n";
			if(count($skill_to_monster[$skill_id])>1)
				$map_descriptor.='<th>'.$translation_list[$current_lang]['Skill level'].'</th>'."\n";
		$map_descriptor.='</tr>'."\n";
        $monster_count=0;
		foreach($skill_to_monster[$skill_id] as $skill_level=>$monster_list_content)
		{
			if($skill_level_displayed!=$skill_level && count($skill_to_monster[$skill_id])>1)
			{
				$map_descriptor.='<tr class="item_list_title_type_'.$type.'"><th colspan="4">'.$translation_list[$current_lang]['Level'].' '.$skill_level.'</th></tr>'."\n";
				$skill_level_displayed=$skill_level;
			}
			foreach($monster_list_content as $monster)
			{
				if(isset($monster_meta[$monster]) && !in_array($monster,$skill_monster_duplicate))
				{
                    $monster_count++;
                    if($monster_count>20)
                    {
                        $map_descriptor.='<tr>
                            <td colspan="';
                            if(count($skill_to_monster[$skill_id])>1)
                                $map_descriptor.='4';
                            else
                                $map_descriptor.='3';
                            $map_descriptor.='" class="item_list_endline item_list_title_type_'.$type.'"></td>
                        </tr>
                        </table>'."\n";
                        $map_descriptor.='<table class="item_list item_list_type_'.$type.' skill_list">
                        <tr class="item_list_title item_list_title_type_'.$type.'">
                            <th colspan="2">'.$translation_list[$current_lang]['Monster'].'</th>
                            <th>'.$translation_list[$current_lang]['Type'].'</th>'."\n";
                            if(count($skill_to_monster[$skill_id])>1)
                                $map_descriptor.='<th>'.$translation_list[$current_lang]['Skill level'].'</th>'."\n";
                        $map_descriptor.='</tr>'."\n";
                        $monster_count=1;
                    }

                    $skill_monster_duplicate[]=$monster;
					$name=$monster_meta[$monster]['name'][$current_lang];
					$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
                    $link.='.html';
					$map_descriptor.='<tr class="value">
						<td>'."\n";
						if(file_exists($datapack_path.'monsters/'.$monster.'/small.png'))
							$map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$monster.'/small.png" width="32" height="32" alt="'.$monster_meta[$monster]['name'][$current_lang].'" title="'.$monster_meta[$monster]['name'][$current_lang].'" /></a></div>'."\n";
						else if(file_exists($datapack_path.'monsters/'.$monster.'/small.gif'))
							$map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$monster.'/small.gif" width="32" height="32" alt="'.$monster_meta[$monster]['name'][$current_lang].'" title="'.$monster_meta[$monster]['name'][$current_lang].'" /></a></div>'."\n";
						$map_descriptor.='</td>
						<td><a href="'.$link.'">'.$name.'</a></td>'."\n";
						$type_list=array();
						foreach($monster_meta[$monster]['type'] as $type_monster)
							if(isset($type_meta[$type_monster]))
								$type_list[]='<span class="type_label type_label_'.$type_monster.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type_monster.'.html">'.ucfirst($type_meta[$type_monster]['name'][$current_lang]).'</a></span>'."\n";
						$map_descriptor.='<td><div class="type_label_list">'.implode(' ',$type_list).'</div></td>'."\n";
						if(count($skill_to_monster[$skill_id])>1)
							$map_descriptor.='<td>'.$skill_level.'</td>'."\n";
					$map_descriptor.='</tr>'."\n";
				}
			}
		}
		$map_descriptor.='<tr>
			<td colspan="';
			if(count($skill_to_monster[$skill_id])>1)
				$map_descriptor.='4';
			else
				$map_descriptor.='3';
			$map_descriptor.='" class="item_list_endline item_list_title_type_'.$type.'"></td>
		</tr>
		</table>'."\n";
	}

    $content=$template;
    $content=str_replace('${TITLE}',$skill['name'][$current_lang],$content);
    $content=str_replace('${CONTENT}',$map_descriptor,$content);
    $content=str_replace('${AUTOGEN}',$automaticallygen,$content);
    $content=clean_html($content);
    $filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['skills/'].text_operation_do_for_url($skill['name'][$current_lang]).'.html';
    if(file_exists($filedestination))
        die('Skills The file already exists: '.$filedestination);
    filewrite($filedestination,$content);
}

$map_descriptor='';
foreach($skill_type_to_id as $type=>$id_list)
{
    $skill_count=0;
    $map_descriptor.='<div class="divfixedwithlist"><table class="item_list item_list_type_'.$type.' skill_list">
    <tr class="item_list_title item_list_title_type_'.$type.'">
        <th>'.$translation_list[$current_lang]['Skill'].'</th>
        <th>'.$translation_list[$current_lang]['Type'].'</th>
        <th>'.$translation_list[$current_lang]['Endurance'].'</th>'."\n";
    if(!$only_one_level)
        $map_descriptor.='<th>'.$translation_list[$current_lang]['Number of level'].'</th>'."\n";
    $map_descriptor.='</tr>'."\n";
    foreach($id_list as $skill_id)
    {
        $skill=$skill_meta[$skill_id];
        if(count($skill['level_list'])>0)
        {
            $skill_count++;
            if($skill_count>20)
            {
                $map_descriptor.='<tr>
                    <td colspan="';
                if(!$only_one_level)
                    $map_descriptor.='4';
                else
                    $map_descriptor.='3';
                $map_descriptor.='" class="item_list_endline item_list_title_type_'.$type.'"></td>
                </tr>
                </table></div>'."\n";
                $map_descriptor.='<div class="divfixedwithlist"><table class="item_list item_list_type_'.$type.' skill_list">
                <tr class="item_list_title item_list_title_type_'.$type.'">
                    <th>'.$translation_list[$current_lang]['Skill'].'</th>
                    <th>'.$translation_list[$current_lang]['Type'].'</th>
                    <th>'.$translation_list[$current_lang]['Endurance'].'</th>'."\n";
                if(!$only_one_level)
                    $map_descriptor.='<th>'.$translation_list[$current_lang]['Number of level'].'</th>'."\n";
                $map_descriptor.='</tr>'."\n";
                $skill_count=1;
            }
            $map_descriptor.='<tr class="value">'."\n";
            $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['skills/'].text_operation_do_for_url($skill['name'][$current_lang]).'.html">'.$skill['name'][$current_lang].'</a></td>'."\n";
            if(isset($type_meta[$skill['type']]))
                $map_descriptor.='<td><span class="type_label type_label_'.$skill['type'].'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$skill['type'].'.html">'.ucfirst($type_meta[$skill['type']]['name'][$current_lang]).'</a></span></td>'."\n";
            else
                $map_descriptor.='<td>&nbsp;</td>'."\n";
            if(isset($skill['level_list'][1]))
                $map_descriptor.='<td>'.$skill['level_list'][1]['endurance'].'</td>'."\n";
            else
                $map_descriptor.='<td>&nbsp;</td>'."\n";
            if(!$only_one_level)
                $map_descriptor.='<td>'.count($skill['level_list']).'</td>'."\n";
            $map_descriptor.='</tr>'."\n";
        }
    }
    $map_descriptor.='<tr>
        <td colspan="';
    if(!$only_one_level)
        $map_descriptor.='4';
    else
        $map_descriptor.='3';
    $map_descriptor.='" class="item_list_endline item_list_title_type_'.$type.'"></td>
    </tr>
    </table></div>'."\n";
}

$content=$template;
$content=str_replace('${TITLE}',$translation_list[$current_lang]['Skills list'],$content);
$content=str_replace('${CONTENT}',$map_descriptor,$content);
$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
$content=clean_html($content);
$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['skills.html'];
if(file_exists($filedestination))
    die('Skill 2 The file already exists: '.$filedestination);
filewrite($filedestination,$content);
