<?php
function reputationLevelToText($reputation,$level)
{
    global $reputation_meta,$current_lang,$translation_list;
    if(!isset($reputation_meta[$reputation]))
        return $translation_list[$current_lang]['Level'].' '.$reputation['level'].' '.$translation_list[$current_lang]['in'].' '.$reputation['type'];
    if(!isset($reputation_meta[$reputation]['level'][$level]))
        return $translation_list[$current_lang]['Level'].' '.$reputation['level'].' '.$translation_list[$current_lang]['in'].' '.$reputation_meta[$reputation]['name'][$current_lang];
    if(!isset($reputation_meta[$reputation]['level'][(int)$level][$current_lang]))
        return $translation_list[$current_lang]['Level'].' '.$level.' '.$translation_list[$current_lang]['in'].' '.$reputation_meta[$reputation]['name'][$current_lang].' (Level: '.$level.')';
    return $translation_list[$current_lang]['Level'].' '.$level.' '.$translation_list[$current_lang]['in'].' '.$reputation_meta[$reputation]['name'][$current_lang].' ('.$reputation_meta[$reputation]['level'][(int)$level][$current_lang].')';
}

function reputationToText($reputation)
{
    global $reputation_meta,$current_lang;
    if(!isset($reputation_meta[$reputation]))
        return $reputation['type'];
    return $reputation_meta[$reputation]['name'][$current_lang];
}
