<?php
function bot_to_wiki_name($bot_id)
{
    global $bots_meta,$current_lang,$bots_name_count,$maindatapackcode;
    if(!isset($bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]))
        $link='bot-'.$bot_id;
    else
    {
        $name=text_operation_do_for_url($bots_meta[$maindatapackcode][$bot_id]['name'][$current_lang]);
        if($name=='')
            $link='bot-'.$bot_id;
        else if($bots_name_count[$maindatapackcode][$current_lang][$name]==1)
            $link=$name;
        else
            $link=$bot_id.'-'.$name;
    }
    return $link;
} 
