<?php
function monsterAndLevelToDisplay($monster,$full=true,$wiki=false)
{
    global $monster_meta,$base_datapack_explorer_site_path,$datapack_path,$base_datapack_site_path,$type_meta,$base_datapack_site_http,$current_lang,$translation_list;
    $map_descriptor='';
    if(isset($monster_meta[$monster['monster']]))
    {
        $monster_full=$monster_meta[$monster['monster']];
        $map_descriptor.='<table class="item_list item_list_type_'.$monster_full['type'][0].' map_list">
        <tr class="item_list_title item_list_title_type_'.$monster_full['type'][0].'">
            <th';
        if(!$full)
            $map_descriptor.=' colspan="3"';
        $map_descriptor.='></th>
        </tr>';
        $map_descriptor.='<tr class="value">';
        if($full)
        {
            $map_descriptor.='<td>';
            $map_descriptor.='<table class="monsterforevolution">';
            if(!$wiki)
            {
                if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/front.png'))
                    $map_descriptor.='<tr><td><center><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$monster['monster'].'/front.png" width="80" height="80" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" /></a></center></td></tr>';
                else if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/front.gif'))
                    $map_descriptor.='<tr><td><center><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$monster['monster'].'/front.gif" width="80" height="80" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" /></a></center></td></tr>';
                $map_descriptor.='<tr><td class="evolution_name"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html">'.$monster_full['name'][$current_lang].'</a></td></tr>';
            }
            else
            {
                if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/front.png'))
                    $map_descriptor.='<tr><td><center>[['.$translation_list[$current_lang]['Monsters:'].$monster_full['name'][$current_lang].'|<img src="'.$base_datapack_site_http.$base_datapack_site_path.'monsters/'.$monster['monster'].'/front.png" width="80" height="80" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" />]]</center></td></tr>';
                else if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/front.gif'))
                    $map_descriptor.='<tr><td><center>[['.$translation_list[$current_lang]['Monsters:'].$monster_full['name'][$current_lang].'|<img src="'.$base_datapack_site_http.$base_datapack_site_path.'monsters/'.$monster['monster'].'/front.gif" width="80" height="80" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" />]]</center></td></tr>';
                $map_descriptor.='<tr><td class="evolution_name">[['.$translation_list[$current_lang]['Monsters:'].$monster_full['name'][$current_lang].'|'.$monster_full['name'][$current_lang].']]</td></tr>';
            }

            $map_descriptor.='<tr><td>';
            $type_list=array();
            if(!$wiki)
                foreach($monster_meta[$monster['monster']]['type'] as $type)
                    if(isset($type_meta[$type]))
                        $type_list[]='<span class="type_label type_label_'.$type.'"><a href="'.$base_datapack_explorer_site_path.'monsters/type-'.$type.'.html">'.ucfirst($type_meta[$type]['name'][$current_lang]).'</a></span>';
            else
                foreach($monster_meta[$monster['monster']]['type'] as $type)
                    if(isset($type_meta[$type]))
                        $type_list[]='<span class="type_label type_label_'.$type.'">[['.$translation_list[$current_lang]['Monsters type:'].$type_meta[$type]['name'][$current_lang].'|'.ucfirst($type_meta[$type]['name'][$current_lang]).']]</span>';
            $map_descriptor.='<div class="type_label_list">'.implode(' ',$type_list).'</div></td></tr>';
            
            $map_descriptor.='<tr><td>'.$translation_list[$current_lang]['Level'].' '.$monster['level'].'</td></tr>';
            $map_descriptor.='</table>';
            $map_descriptor.='</td>';
        }
        else
        {
            if(!$wiki)
            {
                if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/small.png'))
                    $map_descriptor.='<td><center><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$monster['monster'].'/small.png" width="32" height="32" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" /></a></center></td>';
                else if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/small.gif'))
                    $map_descriptor.='<td><center><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html"><img src="'.$base_datapack_site_path.'monsters/'.$monster['monster'].'/small.gif" width="32" height="32" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" /></a></center></td>';
                $map_descriptor.='<td class="evolution_name"><a href="'.$base_datapack_explorer_site_path.$translation_list[$current_lang]['monsters/'].text_operation_do_for_url($monster_full['name'][$current_lang]).'.html">'.$monster_full['name'][$current_lang].'</a></td>';
            }
            else
            {
                if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/small.png'))
                    $map_descriptor.='<td><center>[['.$translation_list[$current_lang]['Monsters:'].$monster_full['name'][$current_lang].'|<img src="'.$base_datapack_site_http.$base_datapack_site_path.'monsters/'.$monster['monster'].'/small.png" width="32" height="32" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" />]]</center></td>';
                else if(file_exists($datapack_path.'monsters/'.$monster['monster'].'/small.gif'))
                    $map_descriptor.='<td><center>[['.$translation_list[$current_lang]['Monsters:'].$monster_full['name'][$current_lang].'|<img src="'.$base_datapack_site_http.$base_datapack_site_path.'monsters/'.$monster['monster'].'/small.gif" width="32" height="32" alt="'.$monster_full['name'][$current_lang].'" title="'.$monster_full['name'][$current_lang].'" />]]</center></td>';
                $map_descriptor.='<td class="evolution_name">[['.$translation_list[$current_lang]['Monsters:'].$monster_full['name'][$current_lang].'|'.$monster_full['name'][$current_lang].']]</td>';
            }

            $map_descriptor.='<td>'.$translation_list[$current_lang]['Level'].' '.$monster['level'].'</td>';
        }
        $map_descriptor.='</tr>';
        $map_descriptor.='<tr>
            <th class="item_list_endline item_list_title item_list_title_type_'.$monster_full['type'][0].'"';
        if(!$full)
            $map_descriptor.=' colspan="3"';
        $map_descriptor.='>';
        $map_descriptor.='</th>
        </tr>
        </table>';
    }
    return $map_descriptor;
}
