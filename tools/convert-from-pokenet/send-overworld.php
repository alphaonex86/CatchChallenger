<?php

$dir = "./";
$dest='/home/user/Desktop/CatchChallenger/Grab-test/monsters/';

if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#[0-9]+\.gif#isU',$file))
	{
		$number=(int)str_replace('.gif','',$file);
		$number=(int)str_replace('.GIF','',$file);
		if(!is_dir($dest.$number.'/'))
			mkdir($dest.$number.'/');
		if(!file_exists($dest.$number.'/overworld.gif') && !file_exists($dest.$number.'/overworld.png'))
			copy($file,$dest.$number.'/overworld.gif');
//        	echo "filename: $file : filetype: " . filetype($dir . $file) . "\n";
	}
	else if(preg_match('#[0-9]+\.png#isU',$file))
	{
		$number=(int)str_replace('.png','',$file);
		$number=(int)str_replace('.PNG','',$file);
		if(!is_dir($dest.$number.'/'))
			mkdir($dest.$number.'/');
		if(!file_exists($dest.$number.'/overworld.gif') && !file_exists($dest.$number.'/overworld.png'))
			copy($file,$dest.$number.'/overworld.png');
//        	echo "filename: $file : filetype: " . filetype($dir . $file) . "\n";
	}
    }
    closedir($dh);
}

