<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator skills'."\n");

$map_descriptor='';

$map_descriptor.='<div class="map map_type_city">'."\n";
    $map_descriptor.='<div class="subblock"><h1>'."\n";
    if(isset($informations_meta['name'][$current_lang]))
        $map_descriptor.=htmlspecialchars($informations_meta['name'][$current_lang]);
    else if(isset($informations_meta['name']['en']))
        $map_descriptor.=htmlspecialchars($informations_meta['name']['en']);
    else
        $map_descriptor.='Informations'."\n";
    $map_descriptor.='</h1></div>'."\n";
    if(isset($informations_meta['description'][$current_lang]))
        $map_descriptor.='<div class="type_label_list">'.htmlspecialchars(ucfirst($informations_meta['description'][$current_lang])).'</div>'."\n";
    else if(isset($informations_meta['description']['en']))
        $map_descriptor.='<div class="type_label_list">'.htmlspecialchars(ucfirst($informations_meta['description']['en'])).'</div>'."\n";

foreach($informations_meta['main'] as $maindatapackcode=>$mainContent)
{
    $map_descriptor.='<div class="map map_type_city">'."\n";
        $map_descriptor.='<div class="subblock"><h1';
        if($mainContent['initial']=='' && $mainContent['color']!='')
            $map_descriptor.=' style="color:'.$mainContent['color'].'"';
        $map_descriptor.='>'."\n";
        if(isset($mainContent['name'][$current_lang]))
            $map_descriptor.=htmlspecialchars($mainContent['name'][$current_lang]);
        else if(isset($mainContent['name']['en']))
            $map_descriptor.=htmlspecialchars($mainContent['name']['en']);
        else
            $map_descriptor.='Informations'."\n";
        if($mainContent['initial']!='')
        {
            $color_temp_sub='';
            if($mainContent['color']!='')
                $map_descriptor.='&nbsp;<span style="background-color:'.$mainContent['color'].';" class="datapackinital">'.$mainContent['initial'].'</span>'."\n";
            else
                $map_descriptor.='&nbsp;<span class="datapackinital">'.$mainContent['initial'].'</span>'."\n";
        }
        $map_descriptor.='</h1></div>'."\n";
        if(isset($mainContent['description'][$current_lang]))
            $map_descriptor.='<div class="type_label_list">'.htmlspecialchars(ucfirst($mainContent['description'][$current_lang])).'</div>'."\n";
        else if(isset($mainContent['description']['en']))
            $map_descriptor.='<div class="type_label_list">'.htmlspecialchars(ucfirst($mainContent['description']['en'])).'</div>'."\n";

        if(isset($start_map_meta[$maindatapackcode]['']) && count($start_map_meta[$maindatapackcode][''])>0)
        {
            $map_descriptor.='<div class="subblock"><div class="value">'."\n";
            
            $map_descriptor.='<table class="item_list item_list_type_normal">
            <tr class="item_list_title item_list_title_type_normal">
                <th colspan="1">'.$translation_list[$current_lang]['Start map'].'</th>
            </tr>'."\n";
            foreach($start_map_meta[$maindatapackcode][''] as $map)
            {
                if(isset($maps_list[$maindatapackcode][$map]))
                {
                    $map_current_object=$maps_list[$maindatapackcode][$map];
                    $map_html=$maindatapackcode.'/'.str_replace('.tmx','.html',$map);
                    $map_descriptor.='<tr class="value"><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$map_html.'" title="'.$map_current_object['name'][$current_lang].'">'.$map_current_object['name'][$current_lang].'</a></td></tr>'."\n";
                }
            }
            $map_descriptor.='<tr>'."\n";
                $map_descriptor.='<td colspan="1" class="item_list_endline item_list_title_type_normal"></td>'."\n";
            $map_descriptor.='</tr>
            </table>'."\n";
            
            $map_descriptor.='</div></div>'."\n";
        }
        /*if(count($mainContent['monsters']))
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">Monster specific</div><div class="value">'."\n";
            $map_descriptor.='<table class="item_list item_list_type_normal">
            <tr class="item_list_title item_list_title_type_normal">
                <th colspan="2">'.$translation_list[$current_lang]['Monster'].'</th>
            </tr>'."\n";
            foreach($mainContent['monsters'] as $item_to_monster_list)
            {
                $monsterId=$monsterId;
                if(isset($monster_meta[$monsterId]))
                {
                    if($item_to_monster_list['quantity_min']!=$item_to_monster_list['quantity_max'])
                        $quantity_text=$item_to_monster_list['quantity_min'].' to '.$item_to_monster_list['quantity_max'];
                    else
                        $quantity_text=$item_to_monster_list['quantity_min'];
                    $name=$monster_meta[$monsterId]['name'][$current_lang];
                    $link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
                    $link.='.html';
                    $map_descriptor.='<tr class="value">'."\n";
                    $map_descriptor.='<td>'."\n";
                    if(file_exists($datapack_path.'monsters/'.$monsterId.'/small.png'))
                        $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$monsterId.'/small.png" width="32" height="32" alt="'.$monster_meta[$monsterId]['name'][$current_lang].'" title="'.$monster_meta[$monsterId]['name'][$current_lang].'" /></a></div>'."\n";
                    else if(file_exists($datapack_path.'monsters/'.$monsterId.'/small.gif'))
                        $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$monsterId.'/small.gif" width="32" height="32" alt="'.$monster_meta[$monsterId]['name'][$current_lang].'" title="'.$monster_meta[$monsterId]['name'][$current_lang].'" /></a></div>'."\n";
                    $map_descriptor.='</td>
                    <td><a href="'.$link.'">'.$name.'</a></td>'."\n";
                    $map_descriptor.='</tr>'."\n";
                }
            }
            $map_descriptor.='<tr>'."\n";
                $map_descriptor.='<td colspan="2" class="item_list_endline item_list_title_type_normal"></td>'."\n";
            $map_descriptor.='</tr>
            </table>'."\n";
            $map_descriptor.='</div></div>'."\n";
        }*/
        
        if(isset($exclusive_monster[$maindatapackcode]['']))
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">Exclusive monsters</div><div class="value">'."\n";
            $map_descriptor.='<br style="clear:both" />';
            $map_descriptor.='<table class="item_list item_list_type_normal monster_list">
            <tr class="item_list_title item_list_title_type_normal">
                <th colspan="3">'.$translation_list[$current_lang]['Monster'].'</th>
            </tr>'."\n";
            $monster_count=0;
            foreach($exclusive_monster[$maindatapackcode][''] as $id)
            {
                $monster=$monster_meta[$id];
                $monster_count++;
                if($monster_count>20)
                {
                    $map_descriptor.='<tr>
                        <td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
                    </tr>
                    </table>'."\n";
                    $map_descriptor.='<table class="item_list item_list_type_normal monster_list">
                    <tr class="item_list_title item_list_title_type_normal">
                        <th colspan="3">'.$translation_list[$current_lang]['Monster'].'</th>
                    </tr>'."\n";
                    $monster_count=1;
                }

                $name=$monster['name'][$current_lang];
                $link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
                $link.='.html';
                $map_descriptor.='<tr class="value">'."\n";
                $map_descriptor.='<td>'."\n";
                if(file_exists($datapack_path.'monsters/'.$id.'/small.png'))
                    $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$id.'/small.png" width="32" height="32" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" /></a></div>'."\n";
                else if(file_exists($datapack_path.'monsters/'.$id.'/small.gif'))
                    $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$id.'/small.gif" width="32" height="32" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" /></a></div>'."\n";
                $map_descriptor.='</td>'."\n";
                
                $map_descriptor.='<td><a href="'.$link.'">'.$name.'</a>';
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
                <td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
            </tr>
            </table>'."\n";
            $map_descriptor.='<br style="clear:both;" /></div></div>'."\n";
        }

        if(count($mainContent['sub'])>0)
        {
            $map_descriptor.='<div class="subblock"><div class="valuetitle">Sub part(s)</div><div class="value">'."\n";
            foreach($mainContent['sub'] as $subdatapackcode=>$subContent)
            {
                $map_descriptor.='<div class="map map_type_city">'."\n";
                    $map_descriptor.='<div class="subblock"><h1';
                    if($subContent['initial']=='' && $subContent['color']!='')
                        $map_descriptor.=' style="color:'.$subContent['color'].'"';
                    $map_descriptor.='>'."\n";
                    if(isset($subContent['name'][$current_lang]))
                        $map_descriptor.=htmlspecialchars($subContent['name'][$current_lang]);
                    else if(isset($subContent['name']['en']))
                        $map_descriptor.=htmlspecialchars($subContent['name']['en']);
                    else
                        $map_descriptor.='Informations'."\n";
                    if($subContent['initial']!='')
                    {
                        $color_temp_sub='';
                        if($subContent['color']!='')
                            $map_descriptor.='&nbsp;<span style="background-color:'.$subContent['color'].';" class="datapackinital">'.$subContent['initial'].'</span>'."\n";
                        else
                            $map_descriptor.='&nbsp;<span class="datapackinital">'.$subContent['initial'].'</span>'."\n";
                    }
                    $map_descriptor.='</h1></div>'."\n";
                    if(isset($subContent['description'][$current_lang]))
                        $map_descriptor.='<div class="type_label_list">'.htmlspecialchars(ucfirst($subContent['description'][$current_lang])).'</div>'."\n";
                    else if(isset($subContent['description']['en']))
                        $map_descriptor.='<div class="type_label_list">'.htmlspecialchars(ucfirst($subContent['description']['en'])).'</div>'."\n";

                    if(isset($start_map_meta[$maindatapackcode][$subdatapackcode]) && count($start_map_meta[$maindatapackcode][$subdatapackcode])>0)
                    {
                        $map_descriptor.='<div class="subblock"><div class="value">'."\n";
                        
                        $map_descriptor.='<table class="item_list item_list_type_normal">
                        <tr class="item_list_title item_list_title_type_normal">
                            <th colspan="1">'.$translation_list[$current_lang]['Start map'].'</th>
                        </tr>'."\n";
                        foreach($start_map_meta[$maindatapackcode][$subdatapackcode] as $map)
                        {
                            if(isset($maps_list[$maindatapackcode][$map]))
                            {
                                $map_current_object=$maps_list[$maindatapackcode][$map];
                                $map_html=$maindatapackcode.'/'.str_replace('.tmx','.html',$map);
                                $map_descriptor.='<tr class="value"><td><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['maps/'].$map_html.'" title="'.$map_current_object['name'][$current_lang].'">'.$map_current_object['name'][$current_lang].'</a></td></tr>'."\n";
                            }
                        }
                        $map_descriptor.='<tr>'."\n";
                            $map_descriptor.='<td colspan="1" class="item_list_endline item_list_title_type_normal"></td>'."\n";
                        $map_descriptor.='</tr>
                        </table>'."\n";
                        
                        $map_descriptor.='</div></div>'."\n";
                    }
                    /*if(count($subContent['monsters']))
                    {
                        $map_descriptor.='<div class="subblock"><div class="valuetitle">Monster specific</div><div class="value">'."\n";
                        $map_descriptor.='<table class="item_list item_list_type_normal">
                        <tr class="item_list_title item_list_title_type_normal">
                            <th colspan="2">'.$translation_list[$current_lang]['Monster'].'</th>
                        </tr>'."\n";
                        foreach($mainContent['monsters'] as $item_to_monster_list)
                        {
                            $monsterId=$monsterId;
                            if(isset($monster_meta[$monsterId]))
                            {
                                if($item_to_monster_list['quantity_min']!=$item_to_monster_list['quantity_max'])
                                    $quantity_text=$item_to_monster_list['quantity_min'].' to '.$item_to_monster_list['quantity_max'];
                                else
                                    $quantity_text=$item_to_monster_list['quantity_min'];
                                $name=$monster_meta[$monsterId]['name'][$current_lang];
                                $link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
                                $link.='.html';
                                $map_descriptor.='<tr class="value">'."\n";
                                $map_descriptor.='<td>'."\n";
                                if(file_exists($datapack_path.'monsters/'.$monsterId.'/small.png'))
                                    $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$monsterId.'/small.png" width="32" height="32" alt="'.$monster_meta[$monsterId]['name'][$current_lang].'" title="'.$monster_meta[$monsterId]['name'][$current_lang].'" /></a></div>'."\n";
                                else if(file_exists($datapack_path.'monsters/'.$monsterId.'/small.gif'))
                                    $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$monsterId.'/small.gif" width="32" height="32" alt="'.$monster_meta[$monsterId]['name'][$current_lang].'" title="'.$monster_meta[$monsterId]['name'][$current_lang].'" /></a></div>'."\n";
                                $map_descriptor.='</td>
                                <td><a href="'.$link.'">'.$name.'</a></td>'."\n";
                                $map_descriptor.='</tr>'."\n";
                            }
                        }
                        $map_descriptor.='<tr>'."\n";
                            $map_descriptor.='<td colspan="2" class="item_list_endline item_list_title_type_normal"></td>'."\n";
                        $map_descriptor.='</tr>
                        </table>'."\n";
                        $map_descriptor.='</div></div>'."\n";
                    }*/

                $map_descriptor.='</div>'."\n";
                
                
                if(isset($exclusive_monster[$maindatapackcode][$subdatapackcode]))
                {
                    $map_descriptor.='<div class="subblock"><div class="valuetitle">Exclusive monsters</div><div class="value">'."\n";
                    $map_descriptor.='<br style="clear:both" />';
                    $map_descriptor.='<table class="item_list item_list_type_normal monster_list">
                    <tr class="item_list_title item_list_title_type_normal">
                        <th colspan="3">'.$translation_list[$current_lang]['Monster'].'</th>
                    </tr>'."\n";
                    $monster_count=0;
                    foreach($exclusive_monster[$maindatapackcode][$subdatapackcode] as $id)
                    {
                        $monster=$monster_meta[$id];
                        $monster_count++;
                        if($monster_count>20)
                        {
                            $map_descriptor.='<tr>
                                <td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
                            </tr>
                            </table>'."\n";
                            $map_descriptor.='<table class="item_list item_list_type_normal monster_list">
                            <tr class="item_list_title item_list_title_type_normal">
                                <th colspan="3">'.$translation_list[$current_lang]['Monster'].'</th>
                            </tr>'."\n";
                            $monster_count=1;
                        }

                        $name=$monster['name'][$current_lang];
                        $link=$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($name);
                        $link.='.html';
                        $map_descriptor.='<tr class="value">'."\n";
                        $map_descriptor.='<td>'."\n";
                        if(file_exists($datapack_path.'monsters/'.$id.'/small.png'))
                            $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$id.'/small.png" width="32" height="32" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" /></a></div>'."\n";
                        else if(file_exists($datapack_path.'monsters/'.$id.'/small.gif'))
                            $map_descriptor.='<div class="monstericon"><a href="'.$link.'"><img src="'.$base_datapack_site_path.'monsters/'.$id.'/small.gif" width="32" height="32" alt="'.$monster['name'][$current_lang].'" title="'.$monster['name'][$current_lang].'" /></a></div>'."\n";
                        $map_descriptor.='</td>'."\n";
                        
                        $map_descriptor.='<td><a href="'.$link.'">'.$name.'</a>';
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
                        <td colspan="3" class="item_list_endline item_list_title_type_normal"></td>
                    </tr>
                    </table>'."\n";
                    $map_descriptor.='<br style="clear:both;" /></div></div>'."\n";
                }
            }
            $map_descriptor.='</div></div>'."\n";
        }

    $map_descriptor.='</div>'."\n";
}

$map_descriptor.='</div>'."\n";

$content=$template;
if(isset($informations_meta['name'][$current_lang]))
    $content=str_replace('${TITLE}',htmlspecialchars($informations_meta['name'][$current_lang]),$content);
else if(isset($informations_meta['name']['en']))
    $content=str_replace('${TITLE}',htmlspecialchars($informations_meta['name']['en']),$content);
else
    $content=str_replace('${TITLE}','Informations',$content);
$content=str_replace('${CONTENT}',$map_descriptor,$content);
$content=str_replace('${AUTOGEN}',$automaticallygen,$content);
$content=clean_html($content);
$filedestination=$datapack_explorer_local_path.$translation_list[$current_lang]['tree.html'];
if(file_exists($filedestination))
    die('Tree The file already exists: '.$filedestination);
filewrite($filedestination,$content);
