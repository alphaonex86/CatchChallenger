<?php

$dir = "./";
$dest='/home/user/Desktop/CatchChallenger/Grab-test/monsters/';

if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#[0-9]{3}-0\.png#',$file))
	{
		$number=(int)str_replace('-0.png','',$file);
		if(!is_dir($dest.$number.'/'))
			mkdir($dest.$number.'/');
		if(!file_exists($dest.$number.'/back.png'))
			copy($file,$dest.$number.'/back.png');
//        	echo "filename: $file : filetype: " . filetype($dir . $file) . "\n";
	}
    }
    closedir($dh);
}

