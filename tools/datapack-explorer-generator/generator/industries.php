<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator industries'."\n");

foreach($industrie_meta as $industrie_path=>$industry_list)
foreach($industry_list as $id=>$industry)
{
    if($industrie_path=='')
    {
        if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['industries/']))
            mkdir($datapack_explorer_local_path.$translation_list[$current_lang]['industries/'],0777,true);
    }
    else
    {
        if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['industries/'].$industrie_path.'/'))
            mkdir($datapack_explorer_local_path.$translation_list[$current_lang]['industries/'].$industrie_path.'/',0777,true);
    }
	$map_descriptor='';

	$map_descriptor.='<div class="map item_details">'."\n";
		$map_descriptor.='<div class="subblock"><h1>'.str_replace('[id]',$id,$translation_list[$current_lang]['Industry [id]']).'</h1>'."\n";
		$map_descriptor.='</div>'."\n";
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Time to complet a cycle'].'</div><div class="value">'."\n";
		if($industry['time']<(60*2))
			$map_descriptor.=$industry['time'].$translation_list[$current_lang]['s'];
		elseif($industry['time']<(60*60*2))
			$map_descriptor.=($industry['time']/60).$translation_list[$current_lang]['mins'];
		elseif($industry['time']<(60*60*24*2))
			$map_descriptor.=($industry['time']/(60*60)).$translation_list[$current_lang]['hours'];
		else
			$map_descriptor.=($industry['time']/(60*60*24)).$translation_list[$current_lang]['days'];
		$map_descriptor.='</div></div>'."\n";
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Cycle to be full'].'</div><div class="value">'.$industry['cycletobefull'].'</div></div>'."\n";

		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Resources'].'</div><div class="value">'."\n";
		foreach($industry['resources'] as $resources)
		{
            $material=$resources['item'];
            $quantity=$resources['quantity'];
			if(isset($item_meta[$material]))
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
			else
				$map_descriptor.='Unknown material: '.$material;
		}
		$map_descriptor.='</div></div>'."\n";

        if(isset($industrie_link_meta[$id]))
            if(count($industrie_link_meta[$id]['requirements'])>0)
            {
                $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Requirements'].'</div><div class="value">'."\n";
                if(isset($industrie_link_meta[$id]['requirements']['quests']))
                {
                    foreach($industrie_link_meta[$id]['requirements']['quests'] as $quest_id)
                    {
                        $map_descriptor.=$translation_list[$current_lang]['Quest:'].' <a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['quests/'].$quest_id.'-'.text_operation_do_for_url($quests_meta[$quest_id]['name'][$current_lang]).'.html" title="'.$quests_meta[$quest_id]['name'][$current_lang].'">'."\n";
                        $map_descriptor.=$quests_meta[$quest_id]['name'][$current_lang];
                        $map_descriptor.='</a><br />'."\n";
                    }
                }
                if(isset($industrie_link_meta[$id]['requirements']['reputation']))
                    foreach($industrie_link_meta[$id]['requirements']['reputation'] as $reputation)
                        $map_descriptor.=reputationLevelToText($reputation['type'],$reputation['level']).'<br />'."\n";
                $map_descriptor.='</div></div>'."\n";
            }

        if(isset($industry_to_bot[$id][$industrie_path]))
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">Location</div><div class="value">'."\n";
            $map_descriptor.='<table class="item_list item_list_type_normal map_list">
                    <tr class="item_list_title item_list_title_type_normal">
                        <th colspan="2">'.$translation_list[$current_lang]['Bot'].'</th>
                        <th colspan="2">'.$translation_list[$current_lang]['Map'].'</th>
                        </tr>'."\n";
            foreach($industry_to_bot[$id][$industrie_path] as $maindatapackcode=>$bot_id)
            {
                $map_descriptor.='<tr class="value">'."\n";
                if(isset($bots_meta[$maindatapackcode][$bot_id]))
                {
                    $bot=$bots_meta[$maindatapackcode][$bot_id];
                    if($bot['name'][$current_lang]=='')
                        $final_name='Bot #'.$bot_id;
                    else
                        $final_name=$bot['name'][$current_lang];
                    $skin_found=true;
                    if(isset($bot_id_to_skin[$bot_id]))
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
                    if($bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]=='')
                        $link_bot=text_operation_do_for_url('bot '.$bot_id);
                    else
                    {
                        $urlname=text_operation_do_for_url($bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]);
                        if($bots_name_count[$maindatapackcode][$current_lang][$urlname]==1)
                            $link_bot=$urlname;
                        else
                            $link_bot=text_operation_do_for_url($bot_id.'-'.$bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]);
                    }
                    $map_descriptor.='><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['bots/'].$maindatapackcode.'/'.text_operation_do_for_url($link_bot).'.html" title="'.$final_name.'">'.$final_name.'</a></td>'."\n";
                    if(isset($bot_id_to_map[$bot_id][$maindatapackcode]))
                    {
                        $entry=$bot_id_to_map[$bot_id][$maindatapackcode]['map'];
                        if(isset($maps_list[$maindatapackcode]) && isset($maps_list[$maindatapackcode][$entry]))
                        {
                            if(isset($zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$entry]['zone']]))
                            {
                                $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$entry).'" title="'.$maps_list[$maindatapackcode][$entry]['name'][$current_lang].'">'.$maps_list[$maindatapackcode][$entry]['name'][$current_lang].'</a></td>'."\n";
                                $map_descriptor.='<td>'.$zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$entry]['zone']]['name'][$current_lang].'</td>'."\n";
                            }
                            else
                                $map_descriptor.='<td colspan="2"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$entry).'" title="'.$maps_list[$maindatapackcode][$entry]['name'][$current_lang].'">'.$maps_list[$maindatapackcode][$entry]['name'][$current_lang].'</a></td>'."\n";
                        }
                        else
                            $map_descriptor.='<td colspan="2">'.$translation_list[$current_lang]['Unknown map'].'</td>'."\n";
                    }
                    else
                        $map_descriptor.='<td colspan="2">&nbsp;'.$final_name.': map not found</td>'."\n";
                }
                else
                    $map_descriptor.='<td colspan="4">Bot '.$bot_id.'</td>'."\n";
                $map_descriptor.='</tr>'."\n";
            }
            $map_descriptor.='<tr>
                <td colspan="4" class="item_list_endline item_list_title_type_normal"></td>
            </tr>
            </table><br style="clear:both;" />'."\n";
            $map_descriptor.='</div></div>'."\n";
        }

		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Products'].'</div><div class="value">'."\n";
		foreach($industry['products'] as $products)
		{
            $material=$products['item'];
            $quantity=$products['quantity'];
			if(isset($item_meta[$material]))
			{
				$map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$material]['name'][$current_lang]).'.html" title="'.$item_meta[$material]['name'][$current_lang].'">'."\n";
				if($item_meta[$material]['image']!='' && file_exists($datapack_path.'items/'.$item_meta[$material]['image']))
					$map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$material]['image'].'" width="24" height="24" alt="'.$item_meta[$material]['name'][$current_lang].'" title="'.$item_meta[$material]['name'][$current_lang].'" />'."\n";
				$map_descriptor.='</td><td>'."\n";
				if($quantity>1)
					$map_descriptor.=$quantity.'x ';
				$map_descriptor.=$item_meta[$material]['name'][$current_lang].'</td></tr></table>'."\n";
				$map_descriptor.='</a>'."\n";
			}
			else
				$map_descriptor.='Unknown material: '.$material;
		}
		$map_descriptor.='</div></div>'."\n";
	$map_descriptor.='</div>'."\n";

    $content=$template;
    $content=str_replace('${TITLE}','Industry #'.$id,$content);
    $content=str_replace('${CONTENT}',$map_descriptor,$content);
    $content=str_replace('${AUTOGEN}',$automaticallygen,$content);
    $content=clean_html($content);
    if($industrie_path=='')
        $filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['industries/'].$id.'.html';
    else
        $filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['industries/'].$industrie_path.'/'.$id.'.html';
    if(file_exists($filedestination))
        die('Industries The file already exists: '.$filedestination);
    filewrite($filedestination,$content);
}

$map_descriptor='';

$map_descriptor.='<table class="item_list item_list_type_normal">
<tr class="item_list_title item_list_title_type_normal">
	<th>'.$translation_list[$current_lang]['Industry'].'</th>
	<th>'.$translation_list[$current_lang]['Resources'].'</th>
	<th>'.$translation_list[$current_lang]['Products'].'</th>
    <th>'.$translation_list[$current_lang]['Location'].'</th>
</tr>'."\n";
foreach($industrie_meta as $industrie_path=>$industry_list)
foreach($industry_list as $id=>$industry)
{
	$map_descriptor.='<tr class="value">'."\n";
	$map_descriptor.='<td>'."\n";
    if($industrie_path=='')
        $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['industries/'].$id.'.html">#'.$id.'</a>'."\n";
    else
        $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['industries/'].$industrie_path.'/'.$id.'.html">#'.$id.'</a>'."\n";
	$map_descriptor.='</td>'."\n";
	$map_descriptor.='<td><center>'."\n";
	foreach($industry['resources'] as $resources)
	{
        $item=$resources['item'];
        $quantity=$resources['quantity'];
		if(isset($item_meta[$item]))
		{
			$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item]['name'][$current_lang]);
            $link.='.html';
			$name=$item_meta[$item]['name'][$current_lang];
			if($item_meta[$item]['image']!='')
				$image=$base_datapack_site_path.'/items/'.$item_meta[$item]['image'];
			else
				$image='';
			$map_descriptor.='<div style="float:left;text-align:middle;">'."\n";
			if($image!='')
			{
				if($link!='')
					$map_descriptor.='<a href="'.$link.'">'."\n";
				$map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
				if($link!='')
					$map_descriptor.='</a>'."\n";
			}
			if($link!='')
				$map_descriptor.='<a href="'.$link.'">'."\n";
			if($name!='')
				$map_descriptor.=$name;
			else
				$map_descriptor.='Unknown resources name ('.$item.')'."\n";
			if($link!='')
				$map_descriptor.='</a></div>'."\n";
		}
		else
			$map_descriptor.='Unknown resources ('.$item.')'."\n";
	}
	$map_descriptor.='</center></td>'."\n";
	$map_descriptor.='<td><center>'."\n";
	foreach($industry['products'] as $products)
	{
        $item=$products['item'];
        $quantity=$products['quantity'];
		if(isset($item_meta[$item]))
		{
			$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item]['name'][$current_lang]);
            $link.='.html';
			$name=$item_meta[$item]['name'][$current_lang];
			if($item_meta[$item]['image']!='')
				$image=$base_datapack_site_path.'/items/'.$item_meta[$item]['image'];
			else
				$image='';
			$map_descriptor.='<div style="float:left;text-align:middle;">'."\n";
			if($image!='')
			{
				if($link!='')
					$map_descriptor.='<a href="'.$link.'">'."\n";
				$map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
				if($link!='')
					$map_descriptor.='</a>'."\n";
			}
			if($link!='')
				$map_descriptor.='<a href="'.$link.'">'."\n";
			if($name!='')
				$map_descriptor.=$name;
			else
				$map_descriptor.='Unknown products name ('.$item.')'."\n";
			if($link!='')
				$map_descriptor.='</a></div>'."\n";
		}
		else
			$map_descriptor.='Unknown products ('.$item.')'."\n";
	}
	$map_descriptor.='</center></td><td>'."\n";
    if(isset($industry_to_bot[$id][$industrie_path]))
    {
        $map_descriptor.='<table class="item_list item_list_type_normal map_list">'."\n";
        foreach($industry_to_bot[$id][$industrie_path] as $maindatapackcode=>$bot_id)
        {
            $map_descriptor.='<tr class="value">'."\n";
            if(isset($bots_meta[$maindatapackcode][$bot_id]))
            {
                $bot=$bots_meta[$maindatapackcode][$bot_id];
                if($bot['name'][$current_lang]=='')
                    $final_url_name='bot-'.$bot_id;
                else
                {
                    $urlname=text_operation_do_for_url($bot['name'][$current_lang]);
                    if($bots_name_count[$maindatapackcode][$current_lang][$urlname]==1)
                        $final_url_name=$urlname;
                    else
                        $final_url_name=text_operation_do_for_url($bot_id.'-'.$bot['name'][$current_lang]);
                }
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
                if($bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]=='')
                    $link_bot=text_operation_do_for_url('bot '.$bot_id);
                else
                {
                    $urlname=text_operation_do_for_url($bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]);
                    if($bots_name_count[$maindatapackcode][$current_lang][$urlname]==1)
                        $link_bot=text_operation_do_for_url($bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]);
                    else
                        $link_bot=text_operation_do_for_url($bot_id.'-'.$bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]);
                }
                $map_descriptor.='><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['bots/'].$maindatapackcode.'/'.text_operation_do_for_url($link_bot).'.html" title="'.$final_name.'">'.$final_name.'</a></td>'."\n";
                if(isset($bot_id_to_map[$bot_id][$maindatapackcode]))
                {
                    $entry=$bot_id_to_map[$bot_id][$maindatapackcode]['map'];
                    if(isset($maps_list[$maindatapackcode][$entry]))
                    {
                        if(isset($zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$entry]['zone']]))
                        {
                            $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$entry).'" title="'.$maps_list[$maindatapackcode][$entry]['name'][$current_lang].'">'.$maps_list[$maindatapackcode][$entry]['name'][$current_lang].'</a></td>'."\n";
                            $map_descriptor.='<td>'.$zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$entry]['zone']]['name'][$current_lang].'</td>'."\n";
                        }
                        else
                            $map_descriptor.='<td colspan="2"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$entry).'" title="'.$maps_list[$maindatapackcode][$entry]['name'][$current_lang].'">'.$maps_list[$maindatapackcode][$entry]['name'][$current_lang].'</a></td>'."\n";
                    }
                    else
                        $map_descriptor.='<td colspan="2">'.$translation_list[$current_lang]['Unknown map'].'</td>'."\n";
                }
                else
                    $map_descriptor.='<td colspan="2">&nbsp;'.$final_name.': map not found</td>'."\n";
            }
            else
                $map_descriptor.='<td colspan="4">Bot '.$bot_id.'</td>'."\n";
            $map_descriptor.='</tr>'."\n";
        }
        $map_descriptor.='</table>'."\n";
    }
    $map_descriptor.='</td>'."\n";
	$map_descriptor.='</tr>'."\n";
}
$map_descriptor.='<tr>
	<td colspan="4" class="item_list_endline item_list_title_type_normal"></td>
</tr>
</table>'."\n";

$content=$template;
$content=str_replace('${TITLE}',$translation_list[$current_lang]['Industries list'],$content);
$content=str_replace('${CONTENT}',$map_descriptor,$content);
$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
$content=clean_html($content);
$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['industries.html'];
if(file_exists($filedestination))
    die('Industries 2 The file already exists: '.$filedestination);
filewrite($filedestination,$content);
