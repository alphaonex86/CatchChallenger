<?php
$dir='./';
if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#^-?[0-9]{1,3}\.png$#',$file))
	{
		$index=str_replace('.png','',$file);
		if(!($image = imagecreatefrompng($file)))
		{
			echo 'The file '.$file.' is not png image';
			continue;
		}
		elseif((imagesx($image)!=124 && imagesx($image)!=123) || (imagesy($image)!=204 && imagesy($image)!=203))
		{
			echo 'The file '.$file.' have not the good size';
			continue;
		}
		$dst = imagecreatetruecolor(48,96);
		imagesavealpha($dst, true);
		$col=imagecolorallocatealpha($dst,255,255,255,127);
		imagefill($dst, 0, 0, $col);

		$x=0;
		while($x<3)
		{
			$y=0;
			while($y<4)
			{
				$tempY=$y*(48+4)-4;
				//if($tempY<0)
				//	$tempY=0;
				imagecopyresized($dst,$image,$x*16,$y*24,8+$x*(32+6),$tempY,16,24,32,48);
				$y++;
			}
			$x++;
		}
		imagealphablending($dst, false);
		imagefilledrectangle($dst,0,0,imagesx($image),1,$col);
		imagecolordeallocate($dst,$col);
		imagealphablending($dst, true);

		if(!is_dir($index))
			mkdir($index);
		imagepng($dst,$index.'/trainer.png');
		//unlink($file);
	}
    }
    closedir($dh);
}
