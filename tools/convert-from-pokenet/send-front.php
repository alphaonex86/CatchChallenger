<?php

$dir = "./";
$dest='/mnt/system/encrypted/datapack/monsters/';

if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#[0-9]{3}-2\.png#',$file))
	{
		$number=(int)str_replace('-2.png','',$file);
		if(!is_dir($dest.$number.'/'))
			mkdir($dest.$number.'/');
		if(!file_exists($dest.$number.'/front.png'))
			copy($file,$dest.$number.'/front.png');
//        	echo "filename: $file : filetype: " . filetype($dir . $file) . "\n";
	}
    }
    closedir($dh);
}

