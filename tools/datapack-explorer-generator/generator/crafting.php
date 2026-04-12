<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator crafting'."\n");

$map_descriptor='';

$map_descriptor.='<table class="item_list item_list_type_normal">
<tr class="item_list_title item_list_title_type_normal">
	<th colspan="2">'.$translation_list[$current_lang]['Item'].'</th>
<th>'.$translation_list[$current_lang]['Material'].'</th>
<th>'.$translation_list[$current_lang]['Product'].'</th>
	<th>'.$translation_list[$current_lang]['Price'].'</th>
</tr>'."\n";
foreach($crafting_meta as $id=>$crafting)
{
	if(isset($item_meta[$crafting['itemToLearn']]))
	{
		$link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$crafting['itemToLearn']]['name'][$current_lang]);
        $link.='.html';
		//$link=$base_datapack_explorer_site_path.'crafting/'.text_operation_do_for_url($item_meta[$crafting['itemToLearn']]['name'][$current_lang]).'.html';
		$name=$item_meta[$crafting['itemToLearn']]['name'][$current_lang];
		if($item_meta[$crafting['itemToLearn']]['image']!='')
			$image=$base_datapack_site_path.'/items/'.$item_meta[$crafting['itemToLearn']]['image'];
		else
			$image='';
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

        $map_descriptor.='<td>'."\n";
        foreach($crafting['material'] as $material=>$quantity)
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
        $map_descriptor.='</td>'."\n";

        $map_descriptor.='<td>'."\n";
        $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$crafting['doItemId']]['name'][$current_lang]).'.html" title="'.$item_meta[$crafting['doItemId']]['name'][$current_lang].'">'."\n";
            $map_descriptor.='<table><tr><td>'."\n";
            if($item_meta[$crafting['doItemId']]['image']!='' && file_exists($datapack_path.'items/'.$item_meta[$crafting['doItemId']]['image']))
                $map_descriptor.='<img src="'.$base_datapack_site_path.'items/'.$item_meta[$crafting['doItemId']]['image'].'" width="24" height="24" alt="'.$item_meta[$crafting['doItemId']]['name'][$current_lang].'" title="'.$item_meta[$crafting['doItemId']]['name'][$current_lang].'" />'."\n";
            $map_descriptor.='</td><td>'.$item_meta[$crafting['doItemId']]['name'][$current_lang].'</td></tr></table>'."\n";
        $map_descriptor.='</a>'."\n";
        $map_descriptor.='</td>'."\n";

        $map_descriptor.='<td>'.$item_meta[$crafting['itemToLearn']]['price'].'$</td>'."\n";

		$map_descriptor.='</tr>'."\n";
	}
	else
		$map_descriptor.='<tr class="value"><td colspan="3">'.$translation_list[$current_lang]['Item to learn missing: '].$crafting['itemToLearn'].'</td></tr>'."\n";
}
$map_descriptor.='<tr>
	<td colspan="5" class="item_list_endline item_list_title_type_normal"></td>
</tr>
</table>'."\n";

$content=$template;
$content=str_replace('${TITLE}',$translation_list[$current_lang]['Crafting list'],$content);
$content=str_replace('${CONTENT}',$map_descriptor,$content);
$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
$content=clean_html($content);
$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['crafting.html'];
if(file_exists($filedestination))
    die('Crafting The file already exists: '.$filedestination);
filewrite($filedestination,$content);
