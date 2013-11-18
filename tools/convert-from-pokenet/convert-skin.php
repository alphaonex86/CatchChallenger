<?php
if(!($image = imagecreatefrompng($argv[1])))
	echo 'The file is not png image';
elseif(imagesx($image)!=124 || imagesy($image)!=204)
{
	imagepng($image);
	exit;
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
imagepng($dst);
