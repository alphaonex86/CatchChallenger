<?php

function filewrite($file,$content)
{
	if($filecurs=fopen($file, 'w'))
	{
		if(fwrite($filecurs,$content) === false)
			die('Unable to write the file: '.$file);
		fclose($filecurs);
	}
	else
		die('Unable to write or create the file: '.$file);
}

$dir = "./";

function cut_layer($layer,$width,$height)
{
	echo 'Data count: '.strlen(gzdeflate(base64_decode($layer_list[3][$id])))."\n";
	$newcontent=gzdeflate(base64_decode($layer_list[3][$id]));
	$newcontent=base64_encode(gzcompress($newcontent));
	echo 'Data count: '.strlen($newcontent)."\n";
}

function clean_map($file)
{
	$content=file_get_contents($file);
	preg_match_all('#<layer[^>]*>'."[ \r\n\t]+".'<data[^>]*>'."[ \r\n\t]+(".preg_quote('H4sIAAAAAAAAAO3BMQEAAADCoPVPbQwfoAAAAAC+BmbyAUigEAAA')."|".preg_quote('eJztwQEBAAAAgiD/r25IQAEAAPBoDhAAAQ==')."|".preg_quote('eNrtwTEBAAAAwqD1T20MH6AAAAAAvgYQoAAB').")[ \r\n\t]+".'</data>'."[ \r\n\t]+".'</layer>#isU',$content,$empty_layer);
	$content=preg_replace('#(tileheight="[0-9]+">[ \r\n\t]+)<properties>.*</properties>([ \r\n\t]+<tileset)#isU','$1$2',$content);
	foreach($empty_layer as $layer_content)
		$content=str_replace($layer_content,'',$content);
	/*$width=preg_replace('#^.*<map version="1.0" orientation="orthogonal" width="([0-9]+)" height="([0-9]+)".*$#isU','$1',$content);
	$width=preg_replace("#[ \r\n\t]+#isU",'',$width);
	$height=preg_replace('#^.*<map version="1.0" orientation="orthogonal" width="([0-9]+)" height="([0-9]+)".*$#isU','$2',$content);
	$height=preg_replace("#[ \r\n\t]+#isU",'',$height);
	preg_match_all('#<layer name="[^"]*" width="([0-9]+)" height="([0-9]+)">'."[ \r\n\t]+".'<data[^>]*>'."[ \r\n\t]+([^ \r\n\t]+)[ \r\n\t]+".'</data>'."[ \r\n\t]+".'</layer>#isU',$content,$layer_list);
	print_r($layer_list);
	foreach($layer_list[0] as $id=>$layer)
	{
		echo 'Layer size ('.$layer_list[1][$id].','.$layer_list[2][$id].') and with map size ('.$width.','.$height.')'."\n";
		if($width!=$layer_list[1][$id] || $height!=$layer_list[2][$id])
		{
			$index=0;
			echo 'Layer size ('.$layer_list[1][$id].','.$layer_list[2][$id].') don\'t match with map size ('.$width.','.$height.'), data count: '.strlen(gzdeflate(base64_decode($layer_list[3][$id])))."\n";
			$newcontent=gzdeflate(base64_decode($layer_list[3][$id]));
			$newcontent=base64_encode(gzcompress($newcontent));
			echo 'Layer size ('.$layer_list[1][$id].','.$layer_list[2][$id].') don\'t match with map size ('.$width.','.$height.'), data count: '.strlen($newcontent)."\n";
			//
		}
	}*/
	echo $content;
}

clean_map($argv[1]);
