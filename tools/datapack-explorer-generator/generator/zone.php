<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator map'."\n");

if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['zones/']))
	if(!mkdir($datapack_explorer_local_path.$translation_list[$current_lang]['zones/']))
		die('Unable to make: '.$datapack_explorer_local_path.'zone/');

foreach($zone_to_map as $maindatapackcode=>$zone_list)
foreach($zone_list as $zone=>$map_by_zone)
{
	$map_descriptor='';

	if(isset($zone_meta[$maindatapackcode][$zone]))
		$zone_name=$zone_meta[$maindatapackcode][$zone]['name'][$current_lang];
	elseif($zone=='')
		$zone_name=$translation_list[$current_lang]['Unknown zone'];
	else
		$zone_name=$zone;

    {
        $map_type_found=false;
        foreach($map_by_zone as $map=>$map_content)
        {
            if(isset($maps_list[$maindatapackcode][$map]['type']))
            {
                $map_descriptor.='<div class="map map_type_'.$maps_list[$maindatapackcode][$map]['type'].'">'."\n";
                $map_type_found=true;
                break;
            }
            else
                echo '$maps_list['.$maindatapackcode.']['.$map.']'." not found\n";
        }
        if($map_type_found==false)
            $map_descriptor.='<div class="map map_type_outdoor">'."\n";
    }
	$map_descriptor.='<div class="subblock"><h1>'.$zone_name.'</h1></div>'."\n";
	$bot_count=0;
	if(isset($zone_to_bot_count[$maindatapackcode][$zone]))
		$bot_count=$zone_to_bot_count[$maindatapackcode][$zone];
	if($bot_count==0)
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Population'].'</div><div class="value">'.$translation_list[$current_lang]['No bots in this zone!'].'</div></div>'."\n";
	elseif($bot_count==1)
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Population'].'</div><div class="value">'.$translation_list[$current_lang]['1 bot'].'</div></div>'."\n";
	else
	{
		$map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Population'].'</div><div class="value">'.$zone_to_bot_count[$maindatapackcode][$zone].' '.$translation_list[$current_lang]['bots'].'<br />'."\n";

		$map_descriptor.='<center><table class="item_list item_list_type_outdoor"><tr class="item_list_title item_list_title_type_outdoor"><th>'.$translation_list[$current_lang]['Bots list'].'</th></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['shop']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:-32px 0px;" class="flags flags16" title="Shop"></div>'.$zone_to_function[$maindatapackcode][$zone]['shop'].' '.$translation_list[$current_lang]['shop(s)'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['fight']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:-16px -16px;" class="flags flags16" title="Fight"></div>'.$zone_to_function[$maindatapackcode][$zone]['fight'].' '.$translation_list[$current_lang]['bot(s) of fight'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['heal']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:0px 0px;" class="flags flags16" title="Heal"></div>'.$zone_to_function[$maindatapackcode][$zone]['heal'].' '.$translation_list[$current_lang]['bot(s) of heal'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['learn']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:-48px 0px;" class="flags flags16" title="Learn"></div>'.$zone_to_function[$maindatapackcode][$zone]['learn'].' '.$translation_list[$current_lang]['bot(s) of learn'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['warehouse']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:0px -16px;" class="flags flags16" title="Warehouse"></div>'.$zone_to_function[$maindatapackcode][$zone]['warehouse'].' '.$translation_list[$current_lang]['warehouse(s)'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['market']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:0px -16px;" class="flags flags16" title="Market"></div>'.$zone_to_function[$maindatapackcode][$zone]['market'].' '.$translation_list[$current_lang]['market(s)'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['clan']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:-48px -16px;" class="flags flags16" title="Clan"></div>'.$zone_to_function[$maindatapackcode][$zone]['clan'].' '.$translation_list[$current_lang]['bot(s) to create clan'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['sell']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:-32px 0px;" class="flags flags16" title="Sell"></div>'.$zone_to_function[$maindatapackcode][$zone]['sell'].' '.$translation_list[$current_lang]['bot(s) to sell your objects'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['zonecapture']))
			$map_descriptor.='<tr class="value"><td><div style="float:left;background-position:-32px -16px;" class="flags flags16" title="Zone capture"></div>'.$zone_to_function[$maindatapackcode][$zone]['zonecapture'].' '.$translation_list[$current_lang]['bot(s) to capture the zone'].'</td></tr>'."\n";
		if(isset($zone_to_function[$maindatapackcode][$zone]['industry']))
            $map_descriptor.='<tr class="value"><td><div style="float:left;background-position:-32px -16px;" class="flags flags16" title="Industry"></div>'.$zone_to_function[$maindatapackcode][$zone]['industry'].' '.$translation_list[$current_lang]['industries'].'</td></tr>'."\n";
        if(isset($zone_to_function[$maindatapackcode][$zone]['quests']))
            $map_descriptor.='<tr class="value"><td><div style="float:left;background-position:-16px 0px;" class="flags flags16" title="Quests"></div>'.$zone_to_function[$maindatapackcode][$zone]['quests'].' '.$translation_list[$current_lang]['quests to start'].'</td></tr>'."\n";
		$map_descriptor.='<tr>
		<td class="item_list_endline item_list_title_type_outdoor"></td>
		</tr></table></center><br style="clear:both;" />'."\n";

		$map_descriptor.='</div></div>'."\n";
	}
	$map_descriptor.='</div>'."\n";

	$map_descriptor.='<center><table class="item_list item_list_type_outdoor"><tr class="item_list_title item_list_title_type_outdoor">
	<th>'."\n";
	$map_descriptor.=$zone_name;
	$map_descriptor.='</th><th>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['shop']))
		$map_descriptor.='<div style="float:left;background-position:-32px 0px;" class="flags flags16" title="Shop"></div>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['fight']))
		$map_descriptor.='<div style="float:left;background-position:-16px -16px;" class="flags flags16" title="Fight"></div>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['heal']))
		$map_descriptor.='<div style="float:left;background-position:0px 0px;" class="flags flags16" title="Heal"></div>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['learn']))
		$map_descriptor.='<div style="float:left;background-position:-48px 0px;" class="flags flags16" title="Learn"></div>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['warehouse']))
		$map_descriptor.='<div style="float:left;background-position:0px -16px;" class="flags flags16" title="Warehouse"></div>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['market']))
		$map_descriptor.='<div style="float:left;background-position:0px -16px;" class="flags flags16" title="Market"></div>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['clan']))
		$map_descriptor.='<div style="float:left;background-position:-48px -16px;" class="flags flags16" title="Clan"></div>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['sell']))
		$map_descriptor.='<div style="float:left;background-position:-32px 0px;" class="flags flags16" title="Sell"></div>'."\n";
	if(isset($zone_to_function[$maindatapackcode][$zone]['zonecapture']))
		$map_descriptor.='<div style="float:left;background-position:-32px -16px;" class="flags flags16" title="Zone capture"></div>'."\n";
    if(isset($zone_to_function[$maindatapackcode][$zone]['industry']))
        $map_descriptor.='<div style="float:left;background-position:-32px -16px;" class="flags flags16" title="Industry"></div>'."\n";
    if(isset($zone_to_function[$maindatapackcode][$zone]['quests']))
        $map_descriptor.='<div style="float:left;background-position:-16px 0px;" class="flags flags16" title="Quests"></div>'."\n";
	$map_descriptor.='</th></tr>'."\n";
	asort($map_by_zone);
	foreach($map_by_zone as $map=>$map_content)
	{
        $map_descriptor.='<tr class="value"><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$map).'" title="'.$map_content[$current_lang].'">'.$map_content[$current_lang].'</a></td>'."\n";
        $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$map).'" title="'.$map_content[$current_lang].'">'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['shop']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['shop']; $i++)
				$map_descriptor.='<div style="float:left;background-position:-32px 0px;" class="flags flags16" title="Shop"></div>'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['fight']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['fight']; $i++)
				$map_descriptor.='<div style="float:left;background-position:-16px -16px;" class="flags flags16" title="Fight"></div>'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['heal']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['heal']; $i++)
				$map_descriptor.='<div style="float:left;background-position:0px 0px;" class="flags flags16" title="Heal"></div>'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['learn']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['learn']; $i++)
				$map_descriptor.='<div style="float:left;background-position:-48px 0px;" class="flags flags16" title="Learn"></div>'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['warehouse']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['warehouse']; $i++)
				$map_descriptor.='<div style="float:left;background-position:0px -16px;" class="flags flags16" title="Warehouse"></div>'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['market']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['market']; $i++)
				$map_descriptor.='<div style="float:left;background-position:0px -16px;" class="flags flags16" title="Market"></div>'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['clan']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['clan']; $i++)
				$map_descriptor.='<div style="float:left;background-position:-48px -16px;" class="flags flags16" title="Clan"></div>'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['sell']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['sell']; $i++)
				$map_descriptor.='<div style="float:left;background-position:-32px 0px;" class="flags flags16" title="Sell"></div>'."\n";
		if(isset($map_to_function[$maindatapackcode][$map]['zonecapture']))
			for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['zonecapture']; $i++)
				$map_descriptor.='<div style="float:left;background-position:-32px -16px;" class="flags flags16" title="Zone capture"></div>'."\n";
        if(isset($map_to_function[$maindatapackcode][$map]['industry']))
            for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['industry']; $i++)
                $map_descriptor.='<div style="float:left;background-position:-32px -16px;" class="flags flags16" title="Industry"></div>'."\n";
        if(isset($map_to_function[$maindatapackcode][$map]['quests']))
            for ($i = 1; $i <= $map_to_function[$maindatapackcode][$map]['quests']; $i++)
                $map_descriptor.='<div style="float:left;background-position:-16px 0px;" class="flags flags16" title="Quests"></div>'."\n";
		$map_descriptor.='</a></td></tr>'."\n";
	}
	$map_descriptor.='<tr>
	<td colspan="2" class="item_list_endline item_list_title_type_outdoor"></td>
	</tr></table></center>'."\n";

	$content=$template;
	$content=str_replace('${TITLE}',$zone_name,$content);
	$content=str_replace('${CONTENT}',$map_descriptor,$content);
	$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
	$content=clean_html($content);
	if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['zones/'].$maindatapackcode.'/'))
		mkpath($datapack_explorer_local_path.$translation_list[$current_lang]['zones/'].$maindatapackcode.'/');
	$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['zones/'].$maindatapackcode.'/'.text_operation_do_for_url($zone_name).'.html';
	if(file_exists($filedestination))
		die('zone The file already exists: '.$filedestination);
	filewrite($filedestination,$content);
}
