<?php

function map_to_wiki_name($map)
{
    global $maps_list,$duplicate_detection_name,$duplicate_detection_name_and_zone,$current_lang,$zone_meta,$maindatapackcode;
    $zone=$maps_list[$maindatapackcode][$map]['zone'];
    $name=$maps_list[$maindatapackcode][$map]['name'][$current_lang];
    if(!isset($duplicate_detection_name[$maindatapackcode][$current_lang][$name]))
        return $maindatapackcode.'/'.$map;
    if($zone!='' && isset($zone_meta[$maindatapackcode][$zone]))
        $final_name_with_zone=$zone_meta[$maindatapackcode][$zone]['name'][$current_lang].' '.$name;
    else
        $final_name_with_zone=$name;
    if(!isset($duplicate_detection_name_and_zone[$maindatapackcode][$current_lang][$final_name_with_zone]))
        return $maindatapackcode.'/'.$map;

    if($duplicate_detection_name[$maindatapackcode][$current_lang][$name]==1)
        return $maindatapackcode.'/'.$name;
    if($duplicate_detection_name_and_zone[$maindatapackcode][$current_lang][$final_name_with_zone]==1)
        return $maindatapackcode.'/'.$final_name_with_zone;
    $map=str_replace('.tmx','',$map);
    return $maindatapackcode.'/'.$map;
} 

function textToProperty($text)
{
    $propertyList=array();
    preg_match_all('#<property .*/>#isU',$text,$propertyListText);
    foreach($propertyListText[0] as $entry)
    {
        $name=preg_replace('#^.* name="([^"]+)".*$#isU','$1',$entry);
        $value=preg_replace('#^.* value="([^"]+)".*$#isU','$1',$entry);
        $propertyList[$name]=$value;
    }
    return $propertyList;
}

function monsterMapOrderGreater($monsterA,$monsterB)
{
    $countMonsterA=count($monsterA['sub']);
    $countMonsterB=count($monsterB['sub']);
    if($countMonsterA==$countMonsterB)
    {
        if($countMonsterA==0)
            return 0;
        if(implode('',$monsterA['sub'])<implode('',$monsterB['sub']))
            return 1;
        else
            return -1;
    }
    if($countMonsterA<$countMonsterB)
        return 1;
    else
        return -1;
}

