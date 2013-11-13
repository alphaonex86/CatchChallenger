<?php

$dir = "./";
$dest='/mnt/system/encrypted/datapack/monsters/';

if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#[0-9]{3}\.gif#',$file))
	{
		$number=(int)str_replace('.gif','',$file);
		if(!is_dir($dest.$number.'/'))
			mkdir($dest.$number.'/');
		if(!file_exists($dest.$number.'/small.gif'))
			copy($file,$dest.$number.'/small.gif');
//        	echo "filename: $file : filetype: " . filetype($dir . $file) . "\n";
	}
    }
    closedir($dh);
}

