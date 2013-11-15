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
		imagecopyresized($dst,$image,$x*16,$y*24,8+$x*(32+6),$y*(48+4)-4,16,24,32,48);
		$y++;
	}
	$x++;
}
imagepng($dst);
