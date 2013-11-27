<?php
if(!($image = imagecreatefrompng($argv[1])))
{
	echo 'The file '.$argv[1].' is not png image';
	continue;
}
elseif((imagesx($image)!=80) || (imagesy($image)!=80))
{
	echo 'The file '.$argv[1].' have not the good size';
	continue;
}
$index=0;
do
{
	$moveline=true;
	$x=0;
	while($x<80)
	{
		$col=imagecolorallocatealpha($image,255,255,255,127);
		$rgb = imagecolorat($image, $x, 79);
		$colors = imagecolorsforindex($image, $rgb);
		if($colors['alpha']!=127)
		{
			$moveline=false;
			break;
		}
		$x++;
	}
	if($moveline)
	{
		$dst = imagecreatetruecolor(80,80);
		$coldst=imagecolorallocatealpha($dst,255,255,255,127);
		imagesavealpha($dst, true);
		imagealphablending($dst, true);
		imagefill($dst, 0, 0, $coldst);
		imagecopyresized($dst,$image,0,1,0,0,80,79,80,79);
		$image=$dst;
	}
	$index++;
}
while($moveline && $index<80);

unlink($argv[1]);
imagepng($image,$argv[1]);

