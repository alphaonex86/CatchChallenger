<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into generator map'."\n");

$map_count=0;
foreach($temp_maps as $maindatapackcode=>$map_list)
foreach($map_list as $map)
$map_count++;

$bot_count=0;
foreach($bots_meta as $maindatapackcode=>$bot_list)
foreach($bot_list as $bot_id=>$bot)
$bot_count++;

$content=array('map_count'=>$map_count,'bot_count'=>$bot_count,'monster_count'=>count($monster_meta),'item_count'=>count($item_meta));

filewrite($datapack_explorer_local_path.'contentstat.json',json_encode($content));
