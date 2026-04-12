<?php
function questList($id_list,$showbot=true,$wiki=false)
{
    global $maindatapackcode,$quests_meta,$bot_id_to_skin,$bots_meta,$bot_id_to_map,$base_datapack_explorer_site_path;
    global $datapack_path,$bots_name_count,$base_datapack_site_path,$item_meta,$base_datapack_site_http,$maps_list;
    global $zone_meta,$current_lang,$translation_list;

    $real_base_datapack_site_path=$base_datapack_site_path;
    $map_descriptor='';

    $map_descriptor.='<table class="item_list item_list_type_normal">
    <tr class="item_list_title item_list_title_type_normal">
        <th>'.$translation_list[$current_lang]['Quests'].'</th>
        <th>'.$translation_list[$current_lang]['Repeatable'].'</th>'."\n";
    if($showbot)
         $map_descriptor.='<th colspan="4">'.$translation_list[$current_lang]['Starting bot'].'</th>'."\n";
    $map_descriptor.='<th>'.$translation_list[$current_lang]['Rewards'].'</th>
    </tr>'."\n";
    foreach($id_list as $id)
    {
        if(isset($quests_meta[$maindatapackcode][$id]))
        {
            $map_descriptor.='<tr class="value">'."\n";
            $map_descriptor.='<td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['quests/'].$maindatapackcode.'/'.$id.'-'.text_operation_do_for_url(
            $quests_meta[$maindatapackcode][$id]['name'][$current_lang]
            ).'.html" title="'.
            $quests_meta[$maindatapackcode][$id]['name'][$current_lang]
            .'">'.
            $quests_meta[$maindatapackcode][$id]['name'][$current_lang]
            .'</a></td>'."\n";
            if(!isset($quests_meta[$maindatapackcode][$id]))
            {
                echo '$id ('.$id.') not found into:'."\n";
                print_r($quests_meta[$maindatapackcode]);
                exit;
            }
            if($quests_meta[$maindatapackcode][$id]['repeatable'])
                $map_descriptor.='<td>'.$translation_list[$current_lang]['Yes'].'</td>'."\n";
            else
                $map_descriptor.='<td>'.$translation_list[$current_lang]['No'].'</td>'."\n";
            if(isset($quests_meta[$maindatapackcode][$id]['steps'][1]))
                $bot_id=$quests_meta[$maindatapackcode][$id]['steps'][1]['bot'];
            else
                $bot_id=$quests_meta[$maindatapackcode][$id]['bot'];
            if(!isset($quests_meta[$maindatapackcode][$id]))
            {
                print_r($quests_meta[$maindatapackcode][$id]);
                exit;
            }
            if($showbot)
            {
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
                            $map_descriptor.='<td><center><div style="width:16px;height:24px;background-image:url(\''.$real_base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png\');background-repeat:no-repeat;background-position:-16px -48px;"></div></center></td>'."\n";
                        elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png'))
                            $map_descriptor.='<td><center><div style="width:16px;height:24px;background-image:url(\''.$real_base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.png\');background-repeat:no-repeat;background-position:-16px -48px;"></div></center></td>'."\n";
                        elseif(file_exists($datapack_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
                            $map_descriptor.='<td><center><div style="width:16px;height:24px;background-image:url(\''.$real_base_datapack_site_path.'skin/bot/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif\');background-repeat:no-repeat;background-position:-16px -48px;"></div></center></td>'."\n";
                        elseif(file_exists($datapack_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif'))
                            $map_descriptor.='<td><center><div style="width:16px;height:24px;background-image:url(\''.$real_base_datapack_site_path.'skin/fighter/'.$bot_id_to_skin[$bot_id][$maindatapackcode].'/trainer.gif\');background-repeat:no-repeat;background-position:-16px -48px;"></div></center></td>'."\n";
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
                        $map_descriptor.='<td colspan="2">&nbsp;</td>'."\n";
                }
                else
                    $map_descriptor.='<td colspan="4">&nbsp;</td>'."\n";
            }
            $map_descriptor.='<td>'."\n";
            if(count($quests_meta[$maindatapackcode][$id]['rewards'])>0)
            {
                $map_descriptor.='<div class="subblock"><div class="value">'."\n";
                if(isset($quests_meta[$maindatapackcode][$id]['rewards']['items']))
                {
                    $map_descriptor.='<table>'."\n";
                    foreach($quests_meta[$maindatapackcode][$id]['rewards']['items'] as $item)
                    {
                        $map_descriptor.='<tr class="value"><td>'."\n";
                        if(isset($item_meta[$item['item']]))
                        {
                            $link=$base_datapack_explorer_site_path.'items/'.text_operation_do_for_url($item_meta[$item['item']]['name'][$current_lang]).'.html';
                            $name=$item_meta[$item['item']]['name'][$current_lang];
                            if($item_meta[$item['item']]['image']!='')
                                $image=$real_base_datapack_site_path.'/items/'.$item_meta[$item['item']]['image'];
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
                            $map_descriptor.=$quantity_text.$name;
                        else
                            $map_descriptor.=$quantity_text.$translation_list[$current_lang]['Unknown item'];
                        if($link!='')
                            $map_descriptor.='</a>'."\n";
                        $map_descriptor.='</td></tr>'."\n";
                    }
                    $map_descriptor.='</table>'."\n";
                }
                if(isset($quests_meta[$maindatapackcode][$id]['rewards']['reputation']))
                    foreach($quests_meta[$maindatapackcode][$id]['rewards']['reputation'] as $reputation)
                    {
                        if($reputation['point']<0)
                            $map_descriptor.=$translation_list[$current_lang]['Less reputation in:'].' '.$reputation['type'];
                        else
                            $map_descriptor.=$translation_list[$current_lang]['More reputation in:'].' '.$reputation['type'];
                    }
                if(isset($quests_meta[$maindatapackcode][$id]['rewards']['allow']))
                    foreach($quests_meta[$maindatapackcode][$id]['rewards']['allow'] as $allow)
                    {
                        if($allow=='clan')
                            $map_descriptor.=$translation_list[$current_lang]['Able to create clan'];
                        else
                            $map_descriptor.=$translation_list[$current_lang]['Allow'].' '.$allow;
                    }
                $map_descriptor.='</div></div>'."\n";
            }
            $map_descriptor.='</td>'."\n";
            $map_descriptor.='</tr>'."\n";
        }
    }
    if($showbot)
        $map_descriptor.='<tr>
            <td colspan="7" class="item_list_endline item_list_title_type_normal"></td>
        </tr>
        </table>'."\n";
    else
        $map_descriptor.='<tr>
            <td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
        </tr>
        </table>'."\n";
    return $map_descriptor;
} 
