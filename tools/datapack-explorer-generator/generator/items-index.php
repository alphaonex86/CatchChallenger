<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator items index'."\n");

$map_descriptor='';

$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#e5eaff;"></div>: Buy into shop<br style="clear:both" />';
$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#e0ffdd;"></div>: Drop or crafting<br style="clear:both" />';
$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#fbfdd3;"></div>: Quest or industry<br style="clear:both" />';
$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#ffefdb;"></div>: Hidden on map<br style="clear:both" />';
$map_descriptor.='<div style="width:16px;height:16px;float:left;background-color:#ffe5e5;"></div>: Fight<br style="clear:both" />';

$item_by_group=array();
foreach($item_meta as $id=>$item)
{
	if($item['group']!='')
		$group_name=$item_group[$item['group']]['name'][$current_lang];
	else if(isset($item_to_skill_of_monster[$id]))
		$group_name='Learn';
	else if(isset($item_to_crafting[$id]))
		$group_name='Crafting';
	else if(isset($item_to_plant[$id]))
		$group_name='Plant';
	else if(isset($item_to_regeneration[$id]))
		$group_name='Regeneration';
	else if(isset($item_to_trap[$id]))
		$group_name='Trap';
	else if(isset($item_to_evolution[$id]))
		$group_name='Evolution';
	else
		$group_name='Items';
	$item_by_group[$group_name][$id]=$item;
}
foreach($item_by_group as $group_name=>$item_meta_temp)
{
	$item_count_list=0;
	$map_descriptor.='<div class="divfixedwithlist"><table class="item_list item_list_type_normal map_list">
	<tr class="item_list_title item_list_title_type_normal">
		<th colspan="3">'.$group_name.'</th>
	</tr>
	<tr class="item_list_title item_list_title_type_normal">
		<th colspan="2">'.$translation_list[$current_lang]['Item'].'</th>
		<th>'.$translation_list[$current_lang]['Price'].'</th>
	</tr>'."\n";
	$max=15;
	/*if(count($item_meta_temp)>$max)
	{
		$number_of_table=(int)ceil((float)count($item_meta_temp)/(float)$max);
		$max=(int)ceil((float)count($item_meta_temp)/(float)$number_of_table);
	}*/
	foreach($item_meta_temp as $id=>$item)
	{
		$item_count_list++;
		//to prevent too long list
		if($item_count_list>$max)
		{
			$map_descriptor.='<tr>
				<td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
			</tr>
			</table></div>'."\n";
			$map_descriptor.='<div class="divfixedwithlist"><table class="item_list item_list_type_normal map_list">
			<tr class="item_list_title item_list_title_type_normal">
				<th colspan="3">'.$group_name.'</th>
			</tr>
			<tr class="item_list_title item_list_title_type_normal">
        <th colspan="2">'.$translation_list[$current_lang]['Item'].'</th>
        <th>'.$translation_list[$current_lang]['Price'].'</th>
			</tr>'."\n";
			$item_count_list=1;
		}
		$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item['name'][$current_lang]);
        $link.='.html';
		$name=$item['name'][$current_lang];
		if($item['image']!='' && file_exists($datapack_path.'items/'.$item['image']))
			$image=$base_datapack_site_path.'/items/'.$item['image'];
		else
			$image='';
		$map_descriptor.='<tr class="value"';
		
		//color flags
		$arraycolor=array();
        if(isset($item_to_shop[$id]))
            $arraycolor[]='#e5eaff';
        $fromwild=false;
        if(isset($item_to_monster[$id]))
            foreach($item_to_monster[$id] as $item_to_monster_list)
                if(isset($monster_meta[$item_to_monster_list['monster']]))
                    if(isset($monster_to_map[$item_to_monster_list['monster']]))
                        $fromwild=true;
        if($fromwild || isset($doItemId_to_crafting[$id]))
            $arraycolor[]='#e0ffdd';
        if(isset($items_to_quests[$id]) || isset($item_produced_by[$id]))
            $arraycolor[]='#fbfdd3';
        if(isset($item_to_map[$id]))
            $arraycolor[]='#ffefdb';
        if(isset($item_to_fight[$id]))
            $arraycolor[]='#ffe5e5';
        if(count($arraycolor)>0)
        {
            $map_descriptor.=' style="background: repeating-linear-gradient(-45deg, ';
            $colorlist=array();
            $px=0;
            foreach($arraycolor as $color)
            {
                $colorlist[]=$color.' '.$px.'px, '.$color.' '.($px+4).'px';
                $px+=5;
            }
            $map_descriptor.=implode(', ',$colorlist);
            $map_descriptor.=');"';
        }
		$map_descriptor.='>
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
		if($item['price']>0)
			$map_descriptor.='<td>'.$item['price'].'$</td>'."\n";
		else
			$map_descriptor.='<td>&nbsp;</td>'."\n";
		$map_descriptor.='</tr>'."\n";

	}
	$map_descriptor.='<tr>
		<td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
	</tr>
	</table></div>'."\n";
}

$content=$template;
$content=str_replace('${TITLE}',$translation_list[$current_lang]['Items list'],$content);
$content=str_replace('${CONTENT}',$map_descriptor,$content);
$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
$content=clean_html($content);
$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['items.html'];
if(file_exists($filedestination))
	die('Item index The file already exists: '.$filedestination);
filewrite($filedestination,$content);
