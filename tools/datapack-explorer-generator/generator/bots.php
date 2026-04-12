<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator bots'."\n");

$bots_by_zone=array(''=>array());
foreach($bots_meta as $maindatapackcode=>$bot_list)
foreach($bot_list as $bot_id=>$bot)
{
    $have_skin=true;
    if(isset($bot_id_to_skin[$bot_id][$maindatapackcode]))
    {
        if(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
            $have_skin=true;
        elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
            $have_skin=true;
        elseif(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
            $have_skin=true;
        elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
            $have_skin=true;
        else
            $have_skin=false;
    }
    else
        $have_skin=false;
    if($have_skin || $bot['onlytext']!=1)
    {
        if(isset($bot_id_to_map[$bot_id]))
        {
            $bottemp=$bot_id_to_map[$bot_id];
            if(isset($maps_list[$maindatapackcode][$bottemp[$maindatapackcode]['map']]))
            {
                if(isset($zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$bottemp[$maindatapackcode]['map']]['zone']]))
                {
                    $temp_name=$zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$bottemp[$maindatapackcode]['map']]['zone']]['name'][$current_lang];
                    if(!isset($bots_by_zone[$maindatapackcode][$temp_name]))
                        $bots_by_zone[$maindatapackcode][$temp_name]=array();
                    $bots_by_zone[$maindatapackcode][$temp_name][]=$bot_id;
                }
                else
                {
                    $temp_name=$maps_list[$maindatapackcode][$bottemp[$maindatapackcode]['map']]['name'][$current_lang];
                    if(!isset($bots_by_zone[$maindatapackcode][$temp_name]))
                        $bots_by_zone[$maindatapackcode][$temp_name]=array();
                    $bots_by_zone[$maindatapackcode][$temp_name][]=$bot_id;
                }
            }
            else
                $bots_by_zone[$maindatapackcode][''][]=$bot_id;
        }
        else
        $bots_by_zone[$maindatapackcode][''][]=$bot_id;
    }
	if(!is_dir($datapack_explorer_local_path.$translation_list[$current_lang]['bots/'].$maindatapackcode.'/'))
		mkpath($datapack_explorer_local_path.$translation_list[$current_lang]['bots/'].$maindatapackcode.'/');
	$map_descriptor='';

	$map_descriptor.='<div class="map item_details">'."\n";
        if($bot['name'][$current_lang]=='')
        {
            echo __FILE__." bot have only id for id $bot_id\n";
            $final_url_name='bot-'.$bot_id;
        }
        else
        {
            $n=text_operation_do_for_url($bot['name'][$current_lang]);
            if(!isset($bots_name_count[$maindatapackcode][$current_lang][$n]))
            {
                echo __FILE__."b ug name $n for id $bot_id\n";
                print_r($bots_name_count);
                die('bug into $bots_name_count not found');
            }
            if($bots_name_count[$maindatapackcode][$current_lang][$n]==1)
            {
                echo __FILE__." bot have unique name\n";
                $final_url_name=$bot['name'][$current_lang];
            }
            else
            {
                echo __FILE__." bot have duplicate name $n for id $bot_id\n";
                $final_url_name=$bot_id.'-'.$bot['name'][$current_lang];
            }
        }
		if($bot['name'][$current_lang]=='')
			$map_descriptor.='<div class="subblock"><h1>Bot #'.$bot_id.'</h1>'."\n";
		else
		{
			$map_descriptor.='<div class="subblock"><h1>'.$bot['name'][$current_lang].'</h1>'."\n";
			$map_descriptor.='<h2>Bot #'.$bot_id.'</h2>'."\n";
		}
		if(isset($bot_id_to_skin[$bot_id][$maindatapackcode]))
		{
			if(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
				$map_descriptor.='<center><h2><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png\');background-repeat:no-repeat;background-position:-16px -48px;" title="Skin: '.$bot_id_to_skin[$bot_id][$maindatapackcode].'"></div></h2></center>'."\n";
			elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
				$map_descriptor.='<center><h2><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png\');background-repeat:no-repeat;background-position:-16px -48px;" title="Skin: '.$bot_id_to_skin[$bot_id][$maindatapackcode].'"></div></h2></center>'."\n";
			elseif(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
				$map_descriptor.='<center><h2><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif\');background-repeat:no-repeat;background-position:-16px -48px;" title="Skin: '.$bot_id_to_skin[$bot_id][$maindatapackcode].'"></div></h2></center>'."\n";
			elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
				$map_descriptor.='<center><h2><div style="width:16px;height:24px;background-image:url(\''.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif\');background-repeat:no-repeat;background-position:-16px -48px;" title="Skin: '.$bot_id_to_skin[$bot_id][$maindatapackcode].'"></div></h2></center>'."\n";
		}
		$map_descriptor.='</div>'."\n";
		if(isset($bot_id_to_skin[$bot_id][$maindatapackcode]))
		{
			if(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.png'))
			{
				$map_descriptor.='<div class="value datapackscreenshot"><center>'."\n";
				$map_descriptor.='<img src="'.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.png" width="80" height="80" alt="" />'."\n";
				$map_descriptor.='</center></div>'."\n";
			}
			else if(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.png'))
			{
				$map_descriptor.='<div class="value datapackscreenshot"><center>'."\n";
				$map_descriptor.='<img src="'.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.png" width="80" height="80" alt="" />'."\n";
				$map_descriptor.='</center></div>'."\n";
			}
			elseif(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.gif'))
			{
				$map_descriptor.='<div class="value datapackscreenshot"><center>'."\n";
				$map_descriptor.='<img src="'.$base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.gif" width="80" height="80" alt="" />'."\n";
				$map_descriptor.='</center></div>'."\n";
			}
			else if(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.gif'))
			{
				$map_descriptor.='<div class="value datapackscreenshot"><center>'."\n";
				$map_descriptor.='<img src="'.$base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/front.gif" width="80" height="80" alt="" />'."\n";
				$map_descriptor.='</center></div>'."\n";
			}
		}
		if(isset($bot_id_to_map[$bot_id]))
		{
            $bottemp=$bot_id_to_map[$bot_id];
            if(isset($bottemp[$maindatapackcode]))
            {
                $mapTemp=$bottemp[$maindatapackcode]['map'];
                if(isset($maps_list[$maindatapackcode][$mapTemp]))
                {
                    $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Map'].'</div><div class="value">'."\n";
                    if(isset($zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$mapTemp]['zone']]))
                    {
                        $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$mapTemp).'" title="'.$maps_list[$maindatapackcode][$mapTemp]['name'][$current_lang].'">'.$maps_list[$maindatapackcode][$mapTemp]['name'][$current_lang].'</a>&nbsp;'."\n";
                        $zone_name=$zone_meta[$maindatapackcode][$maps_list[$maindatapackcode][$mapTemp]['zone']]['name'][$current_lang];
                        $map_descriptor.='(Zone: <a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['zones/'].$maindatapackcode.'/'.text_operation_do_for_url($zone_name).'.html" title="'.$zone_name.'">'.$zone_name.'</a>)'."\n";
                    }
                    else
                        $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$mapTemp).'" title="'.$maps_list[$maindatapackcode][$mapTemp]['name'][$current_lang].'">'.$maps_list[$maindatapackcode][$mapTemp]['name'][$current_lang].'</a>'."\n";
                    $map_descriptor.='</div></div>'."\n";
                }
			}
		}
        if(isset($bot_start_to_quests[$bot_id]))
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">'.$translation_list[$current_lang]['Quest start'].'</div><div class="value">'."\n";
            $map_descriptor.=questList($bot_start_to_quests[$bot_id],false);
            $map_descriptor.='</div></div>'."\n";
        }
		foreach($bot['step'] as $step_id=>$step)
		{
			if($step['type']=='text')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Text'].'</div><div class="value">'."\n";
				$tempText=$step['text'][$current_lang];
				preg_match_all('#a href="([^"]+)"#isU',$tempText,$matches);
				foreach($matches[1] as $link)
				{
                    if($link=='next')
                        $tempText=str_replace('a href="'.$link.'"','a href="#step'.($step_id+1).'"',$tempText);
                    else if(preg_match('#^[0-9]+$#',$link))
                        $tempText=str_replace('a href="'.$link.'"','a href="#step'.$link.'"',$tempText);
                    else
                        $tempText=str_replace('a href="'.$link.'"','a href=""',$tempText);
				}
				$map_descriptor.=$tempText;
				$map_descriptor.='</div></div>'."\n";
			}
			else if($step['type']=='shop')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Shop'].'</div>
				<center><div style="background-position:-32px 0px;" class="flags flags16"></div></center>
				<div class="value">'."\n";
				$map_descriptor.='<center><table class="item_list item_list_type_normal">
				<tr class="item_list_title item_list_title_type_normal">
					<th colspan="2">Item</th>
					<th>Price</th>
				</tr>'."\n";
				foreach($shop_meta[$maindatapackcode][$step['shop']]['products'] as $item=>$price)
				{
					if(isset($item_meta[$item]))
					{
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
							$map_descriptor.='Unknown item'."\n";
						if($link!='')
							$map_descriptor.='</a>'."\n";
						$map_descriptor.='</td>'."\n";
						$map_descriptor.='<td>'.$price.'$</td>'."\n";
						$map_descriptor.='</tr>'."\n";
					}
				}
				$map_descriptor.='<tr>
					<td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
				</tr>
				</table>'."\n";
				$map_descriptor.='</center>'."\n";
				$map_descriptor.='</div></div>'."\n";
			}
			else if($step['type']=='fight')
			{
				if(isset($fight_meta[$maindatapackcode][$step['fightid']]))
				{
					$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Fight'].'</div>
					<center><div style="background-position:-16px -16px;" class="flags flags16"></div></center>
					<div class="value">'."\n";
					if($fight_meta[$maindatapackcode][$step['fightid']]['cash']>0)
						$map_descriptor.=$translation_list[$current_lang]['Rewards'].': <b>'.$fight_meta[$maindatapackcode][$step['fightid']]['cash'].'$</b><br />'."\n";

                    if(count($fight_meta[$maindatapackcode][$step['fightid']]['items'])>0)
                    {
                        $map_descriptor.='<center><table class="item_list item_list_type_normal">
                        <tr class="item_list_title item_list_title_type_normal">
                            <th colspan="2">'.$translation_list[$current_lang]['Item'].'</th>
                        </tr>'."\n";
                        foreach($fight_meta[$maindatapackcode][$step['fightid']]['items'] as $item)
                        {
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
                            if($drop['quantity_min']>1)
                                $quantity_text=$item['quantity'].' ';
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
                                    $map_descriptor.=$quantity_text.'Unknown item'."\n";
                                if($link!='')
                                    $map_descriptor.='</a>'."\n";
                                $map_descriptor.='</td>'."\n";
                                $map_descriptor.='</tr>'."\n";
                        }
                        $map_descriptor.='<tr>
                            <td colspan="2" class="item_list_endline item_list_title_type_normal"></td>
                        </tr>
                        </table></center>'."\n";
                    }

					foreach($fight_meta[$maindatapackcode][$step['fightid']]['monsters'] as $monster)
					{
						if(isset($monster_meta[$monster['monster']]))
						{
							$monster_full=$monster_meta[$monster['monster']];
							$map_descriptor.='<table class="item_list item_list_type_'.$monster_full['type'][0].' map_list">
							<tr class="item_list_title item_list_title_type_'.$monster_full['type'][0].'">
								<th>'."\n";
							$map_descriptor.='</th>
							</tr>'."\n";
							$map_descriptor.='<tr class="value">'."\n";
							$map_descriptor.='<td>'."\n";
							$map_descriptor.='<table class="monsterforevolution">'."\n";
							if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/front.png'))
								$map_descriptor.='<tr><td><center><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$monster['monster'].'/front.png" width="80" height="80" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" /></center></a></td></tr>'."\n";
							else if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/front.gif'))
								$map_descriptor.='<tr><td><center><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$monster['monster'].'/front.gif" width="80" height="80" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" /></center></a></td></tr>'."\n";
							$map_descriptor.='<tr><td class="evolution_name"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html">'.$monster_full['name'][$current_lang].'</a></td></tr>'."\n";
                            $map_descriptor.='<tr><td>'."\n";
                            $type_list=array();
                            foreach($monster_meta[$monster['monster']]['type'] as $type)
                                if(isset($type_meta[$type]))
                                    $type_list[]='<span class="type_label type_label_'.$type.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>'."\n";
                            $map_descriptor.='<div class="type_label_list">'.implode(' ',$type_list).'</div></td></tr>'."\n";
							$map_descriptor.='<tr><td>Level '.$monster['level'].'</td></tr>'."\n";
							$map_descriptor.='</table>'."\n";
							$map_descriptor.='</td>'."\n";
							$map_descriptor.='</tr>'."\n";
							$map_descriptor.='<tr>
								<th class="item_list_endline item_list_title item_list_title_type_'.$monster_full['type'][0].'">'."\n";
							$map_descriptor.='</th>
							</tr>
							</table>'."\n";
						}
					}
					$map_descriptor.='<br style="clear:both;" />'."\n";

					$map_descriptor.='</div>'."\n";
				}
			}
			else if($step['type']=='heal')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Heal'].'</div>
				<div class="value">
					<center><div style="background-position:0px 0px;" class="flags flags128"></div></center>
				</div>'."\n";
			}
			else if($step['type']=='learn')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Learn'].'</div>
				<div class="value">
					<center><div style="background-position:-384px 0px;" class="flags flags128"></div></center>
				</div>'."\n";
			}
			else if($step['type']=='warehouse')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Warehouse'].'</div>
				<div class="value">
					<center><div style="background-position:0px -128px;" class="flags flags128"></div></center>
				</div>'."\n";
			}
			else if($step['type']=='market')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Market'].'</div>
				<div class="value">
					<center><div style="background-position:0px -128px;" class="flags flags128"></div></center>
				</div>'."\n";
			}
			else if($step['type']=='clan')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Clan'].'</div>
				<div class="value">
					<center><div style="background-position:-384px -128px;" class="flags flags128"></div></center>
				</div>'."\n";
			}
			else if($step['type']=='sell')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Sell'].'</div>
				<div class="value">
					<center><div style="background-position:-256px 0px;" class="flags flags128"></div></center>
				</div>'."\n";
			}
			else if($step['type']=='zonecapture')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Zone capture'].'</div>
				<div class="value">
					Zone: '."\n";
				if(isset($zone_meta[$maindatapackcode][$step['zone']]))
					$map_descriptor.=$zone_meta[$maindatapackcode][$step['zone']]['name'][$current_lang];
				else
					$map_descriptor.='Unknown zone'."\n";
				$map_descriptor.='<center><div style="background-position:-256px -128px;" class="flags flags128"></div></center>
				</div>'."\n";
			}
			else if($step['type']=='industry')
			{
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Industry'].'</div>
				<center><div style="background-position:0px -32px;" class="flags flags16"></div></center>
				<div class="value">'."\n";
                if(!isset($industrie_meta[$maindatapackcode][$step['industry']]) && !isset($industrie_meta[''][$step['industry']]))
                {
                    $map_descriptor.='Industry '.$step['industry'].' not found for bot '.$bot_id.'!</td>'."\n";
                    echo 'Industry '.$step['industry'].' not found for bot '.$bot_id.'!'."\n";
                }
                else
                {
                    $map_descriptor.='<center><table class="item_list item_list_type_normal">
                    <tr class="item_list_title item_list_title_type_normal">
                        <th>Industry</th>
                        <th>Resources</th>
                        <th>Products</th>
                    </tr>'."\n";
                    if(!isset($industrie_meta[$maindatapackcode][$step['industry']]))
                        $industry=$industrie_meta[''][$step['industry']];
                    else
                        $industry=$industrie_meta[$maindatapackcode][$step['industry']];
                    $map_descriptor.='<tr class="value">'."\n";
                    $map_descriptor.='<td>'."\n";
                    if(!isset($industrie_meta[$maindatapackcode][$step['industry']]))
                        $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['industries/'].$step['industry'].'.html">#'.$step['industry'].'</a>'."\n";
                    else
                        $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['industries/'].$maindatapackcode.'/'.$step['industry'].'.html">#'.$step['industry'].'</a>'."\n";
                    $map_descriptor.='</td>'."\n";
                    $map_descriptor.='<td>'."\n";
                    foreach($industry['resources'] as $resources)
                    {
                        $item=$resources['item'];
                        $quantity=$resources['quantity'];
                        if(isset($item_meta[$item]))
                        {
                            $link_item=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item]['name'][$current_lang]).'.html';
                            $link.='.html';
                            $name=$item_meta[$item]['name'][$current_lang];
                            if($item_meta[$item]['image']!='')
                                $image=$base_datapack_site_path.'/items/'.$item_meta[$item]['image'];
                            else
                                $image='';
                            $map_descriptor.='<div style="float:left;text-align:center;">'."\n";
                            if($image!='')
                            {
                                if($link_item!='')
                                    $map_descriptor.='<a href="'.$link_item.'">'."\n";
                                $map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
                                if($link_item!='')
                                    $map_descriptor.='</a>'."\n";
                            }
                            if($link_item!='')
                                $map_descriptor.='<a href="'.$link_item.'">'."\n";
                            if($name!='')
                                $map_descriptor.=$name;
                            else
                                $map_descriptor.='Unknown item'."\n";
                            if($link_item!='')
                                $map_descriptor.='</a></div>'."\n";
                        }
                        else
                            $map_descriptor.='Unknown item'."\n";
                    }
                    $map_descriptor.='</td>'."\n";
                    $map_descriptor.='<td>'."\n";
                    foreach($industry['products'] as $products)
                    {
                        $item=$products['item'];
                        $quantity=$products['quantity'];
                        if(isset($item_meta[$item]))
                        {
                            $link_item=$base_datapack_explorer_site_path.$translation_list[$current_lang]['items/'].text_operation_do_for_url($item_meta[$item]['name'][$current_lang]).'.html';
                            $link.='.html';
                            $name=$item_meta[$item]['name'][$current_lang];
                            if($item_meta[$item]['image']!='')
                                $image=$base_datapack_site_path.'/items/'.$item_meta[$item]['image'];
                            else
                                $image='';
                            $map_descriptor.='<div style="float:left;text-align:middle;">'."\n";
                            if($image!='')
                            {
                                if($link_item!='')
                                    $map_descriptor.='<a href="'.$link_item.'">'."\n";
                                $map_descriptor.='<img src="'.$image.'" width="24" height="24" alt="'.$name.'" title="'.$name.'" />'."\n";
                                if($link_item!='')
                                    $map_descriptor.='</a>'."\n";
                            }
                            if($link_item!='')
                                $map_descriptor.='<a href="'.$link_item.'">'."\n";
                            if($name!='')
                                $map_descriptor.=$name;
                            else
                                $map_descriptor.='Unknown item'."\n";
                            if($link_item!='')
                                $map_descriptor.='</a></div>'."\n";
                        }
                        else
                            $map_descriptor.='Unknown item'."\n";
                    }
                    $map_descriptor.='</td>'."\n";
                    $map_descriptor.='</tr>'."\n";
                    $map_descriptor.='<tr>
                        <td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
                    </tr>
                    </table></center>'."\n";
                }
                $map_descriptor.='
                </div>'."\n";
			}
			else
				$map_descriptor.='<div class="subblock"><div class="valuetitle" id="step'.$step_id.'">'.$translation_list[$current_lang]['Unknown type'].' ('.$step['type'].')</div></div>'."\n";
		}
	$map_descriptor.='</div>'."\n";

    $content=$template;
    if($bot['name'][$current_lang]=='')
        $content=str_replace('${TITLE}','Bot #'.$bot_id,$content);
    else
        $content=str_replace('${TITLE}',$bot['name'][$current_lang],$content);
    $content=str_replace('${CONTENT}',$map_descriptor,$content);
    $content=str_replace('${AUTOGEN}',$automaticallygen,$content);
    $content=clean_html($content);
    $filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['bots/'].$maindatapackcode.'/'.text_operation_do_for_url($final_url_name).'.html';
    if(file_exists($filedestination))
    {
        print_r($bot);
        echo "\n";
        die('Bots The file already exists: '.$filedestination);
    }
    filewrite($filedestination,$content);
}

$map_descriptor='';

$map_descriptor.='<!-- highest bot id: '.$highest_bot_id.' -->'."\n";
foreach($bots_by_zone as $maindatapackcode=>$bot_zone_list)
foreach($bot_zone_list as $zone=>$bot_id_list)
{
    if(count($bot_id_list))
    {
        $map_descriptor.='<div class="divfixedwithlist"><table class="item_list item_list_type_normal map_list">
        <tr class="item_list_title item_list_title_type_normal">
            <th colspan="2">'."\n";
        if($zone!='')
        {
            if(isset($zone_name_to_code[$current_lang][$zone]))
                $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['zones/'].text_operation_do_for_url($zone).'.html" title="'.$zone.'">'.$zone.'</a>'."\n";
            else
            {
                if(isset($maps_name_to_map[$zone]))
                    $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$maps_name_to_map[$zone]).'" title="'.$maps_list[$maps_name_to_map[$zone]]['name'][$current_lang].'">'.$zone.'</a>'."\n";
                else
                    $map_descriptor.=$zone;
            }
        }
        else
            $map_descriptor.='Unknown zone'."\n";
        $map_descriptor.='</th>
        </tr>'."\n";
        $bot_count=0;
        $name_count_for_zone=array();
        foreach($bot_id_list as $bot_id)
        {
            if(!isset($bots_meta[$maindatapackcode][$bot_id]))
            {
                if(!isset($bots_meta[$maindatapackcode]))
                    echo '$bots_meta['.$maindatapackcode.'] not found'."\n";
                else
                    echo '$bots_meta['.$maindatapackcode.']['.$bot_id.'] not found'."\n";
                exit;
            }
            $bot=$bots_meta[$maindatapackcode][$bot_id];
            if($bot['name'][$current_lang]!='')
            {
                if(isset($bot_id_to_map[$bot_id]))
                {
                    $bottemp=$bot_id_to_map[$bot_id];
                    if(isset($maps_list[$maindatapackcode][$bottemp[$maindatapackcode]['map']]))
                        $final_name=$name_count_for_zone[$bot['name'][$current_lang]][$maps_list[$maindatapackcode][$bottemp[$maindatapackcode]['map']]['name'][$current_lang]]=true;
                    else
                        $final_name=$name_count_for_zone[$bot['name'][$current_lang]]['']=true;
                }
                else
                    $final_name=$name_count_for_zone[$bot['name'][$current_lang]]['']=true;
            }
        }
        foreach($bot_id_list as $bot_id)
        {
            $bot_count++;
            if($bot_count>15)
            {
                $map_descriptor.='<tr>
                    <td colspan="2" class="item_list_endline item_list_title_type_normal"></td>
                </tr>
                </table></div>'."\n";
                        $map_descriptor.='<div class="divfixedwithlist"><table class="item_list item_list_type_normal map_list">
                <tr class="item_list_title item_list_title_type_normal">
                    <th colspan="2">'."\n";
                if($zone!='')
                {
                    if(isset($zone_name_to_code[$current_lang][$zone]))
                        $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['zones/'].text_operation_do_for_url($zone).'.html" title="'.$zone.'">'.$zone.'</a>'."\n";
                    else
                    {
                        if(isset($maps_name_to_map[$zone]))
                            $map_descriptor.='<a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$maindatapackcode.'/'.str_replace('.tmx','.html',$maps_name_to_map[$zone]).'" title="'.$maps_list[$maps_name_to_map[$zone]]['name'][$current_lang].'">'.$zone.'</a>'."\n";
                        else
                            $map_descriptor.=$zone;
                    }
                }
                else
                    $map_descriptor.='Unknown zone'."\n";
                $map_descriptor.='</th>
                </tr>'."\n";
                $bot_count=1;
            }
            if(!isset($bots_meta[$maindatapackcode][$bot_id]))
            {
                if(!isset($bots_meta[$maindatapackcode]))
                    echo '$bots_meta['.$maindatapackcode.'] not found'."\n";
                else
                    echo '$bots_meta['.$maindatapackcode.']['.$bot_id.'] not found'."\n";
                exit;
            }
            $bot=$bots_meta[$maindatapackcode][$bot_id];
            if($bot['name'][$current_lang]=='')
                $final_name='Bot #'.$bot_id;
            elseif(count($name_count_for_zone[$bot['name'][$current_lang]])==1)
                $final_name=$bot['name'][$current_lang];
            else
            {
                if(isset($bot_id_to_map[$bot_id]))
                {
                    $bottemp=$bot_id_to_map[$bot_id];
                    if(isset($maps_list[$maindatapackcode][$bottemp[$maindatapackcode]['map']]))
                        $final_name=$bot['name'][$current_lang].' ('.$maps_list[$maindatapackcode][$bottemp[$maindatapackcode]['map']]['name'][$current_lang].')'."\n";
                    else
                        $final_name=$bot['name'][$current_lang];
                }
                else
                    $final_name=$bot['name'][$current_lang];
            }
            if($bot['name'][$current_lang]=='')
                $final_url_name='bot-'.$bot_id;
            else if($bots_name_count[$maindatapackcode][$current_lang][text_operation_do_for_url($bot['name'][$current_lang])]==1)
                $final_url_name=$bot['name'][$current_lang];
            else
                $final_url_name=$bot_id.'-'.$bot['name'][$current_lang];
            $map_descriptor.='<tr class="value">'."\n";
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
            $map_descriptor.='</tr>'."\n";
        }
        $map_descriptor.='<tr>
            <td colspan="2" class="item_list_endline item_list_title_type_normal"></td>
        </tr>
        </table></div>'."\n";
    }
}

$content=$template;
$content=str_replace('${TITLE}',$translation_list[$current_lang]['Bots list'],$content);
$content=str_replace('${CONTENT}',$map_descriptor,$content);
$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
$content=clean_html($content);
$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['bots.html'];
if(file_exists($filedestination))
    die('Bots 2 The file already exists: '.$filedestination);
filewrite($filedestination,$content);

