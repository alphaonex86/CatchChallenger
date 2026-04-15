<?php
if(!isset($datapackexplorergeneratorinclude))
	die('abort into function'."\n");

function mkpath($path)
{
	if(file_exists($path) or @mkdir($path))
		return true;
	return (mkpath(dirname($path)) and mkdir($path));
}

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

function text_operation_lower_case($text)
{
	$text=strtolower($text);
	$text=str_replace('Â','â',$text);
	$text=str_replace('À','à',$text);
	$text=str_replace('Ä','ä',$text);
	$text=str_replace('Ç','ç',$text);
	$text=str_replace('Ê','ê',$text);
	$text=str_replace('È','è',$text);
	$text=str_replace('Ë','ë',$text);
	$text=str_replace('É','é',$text);
	$text=str_replace('Ï','ï',$text);
	$text=str_replace('Ö','ö',$text);
	$text=str_replace('Ô','ô',$text);
	$text=str_replace('Û','û',$text);
	$text=str_replace('Ù','ù',$text);
	$text=str_replace('Ü','ü',$text);
	return $text;
}

function clean_html($html)
{
    $html=preg_replace("#[\r\n\t]+#isU",'',$html);
    $html=preg_replace("#[ ]{2,}#",'',$html);
    $html=str_replace('> <','><',$html);
    return $html;
}

function text_operation_clean_text($text,$minimum_word_length=0,$minimum_string_length=15,$maximum_string_length=64)
{
	$text=text_operation_lower_case($text);

	$text=str_replace('â','a',str_replace('à','a',$text));
	$text=str_replace('ã','a',str_replace('á','a',str_replace('ã','a',str_replace('ä','a',$text))));

	$text=str_replace('ç','c',$text);

	$text=str_replace('é','e',str_replace('è','e',str_replace('ê','e',str_replace('ë','e',$text))));

	$text=str_replace('ì','i',str_replace('í','i',str_replace('î','i',str_replace('ï','i',$text))));

	$text=str_replace('ñ','n',$text);
	
	$text=str_replace('õ','o',str_replace('ö','o',str_replace('ó','o',str_replace('ô','o',$text))));
	$text=str_replace('ô','o',str_replace('ò','o',$text));
	
	$text=str_replace('û','u',str_replace('ü','u',str_replace('ú','u',str_replace('ù','u',$text))));
	
	$text=str_replace('ý','y',str_replace('ÿ','y',$text));
	
	if(strlen($text)>$maximum_string_length)
		$text=substr($text,0,$maximum_string_length);
	$text=preg_replace('#([0-9]+)(\.|-| )+#','$1 ',$text);
	$text=preg_replace('#[^a-zA-Z0-9_-]+#',' ',$text);
	$text=preg_replace('# +#',' ',$text);
	if($minimum_word_length>2)
	{
		$a=$minimum_word_length-1;
		do
		{
			$text_temp=preg_replace('#\b[a-zA-Z_-]{1,'.$a.'}\b#',' ',$text);
			$text_temp=preg_replace('# +#',' ',$text_temp);
			$text_temp=preg_replace('# +$#','',$text_temp);
			$text_temp=preg_replace('#^ +#','',$text_temp);
			$a--;
		}
		while(strlen($text_temp)<=$minimum_string_length && $a>1);
		if(strlen($text_temp)>$minimum_string_length && $a>1)
			$text=$text_temp;
	}
	$text=preg_replace('# +#',' ',$text);
	$text=preg_replace('# +$#','',$text);
	$text=preg_replace('#^ +#','',$text);
	return $text;
}

function text_operation_do_for_url($text,$minimum_word_length=0,$minimum_string_length=15,$maximum_string_length=64)
{
	$text=text_operation_clean_text($text,$minimum_word_length,$minimum_string_length,$maximum_string_length);
	$text=str_replace(' ','-',$text);
	$text=preg_replace('#-+#','-',$text);
	$text=preg_replace('#^-+#','',$text);
	$text=preg_replace('#-+$#','',$text);
	return $text;
}

function text_operation_lower_case_first_letter_upper($text)
{
	if(strlen($text)<=0)
		return $text;
	else if(strlen($text)==1)
		return strtoupper($text);
	else
		return strtoupper(substr($text,0,1)).text_operation_lower_case(substr($text,1,strlen($text)-1));
}

function text_operation_first_letter_upper($text)
{
	if(strlen($text)<=0)
		return $text;
	else if(strlen($text)==1)
		return strtoupper($text);
	else
		return strtoupper(substr($text,0,1)).substr($text,1,strlen($text)-1);
} 

function getTmxList($dir,$sub_dir='')
{
	$files_list=array();
	if(is_dir($dir.$sub_dir))
		if($handle = opendir($dir.$sub_dir)) {
			while(false !== ($entry = readdir($handle))) {
			if($entry != '.' && $entry != '..') {
					if(is_dir($dir.$sub_dir.$entry))
						$files_list=array_merge($files_list,getTmxList($dir,$sub_dir.$entry.'/'));
					else if(preg_match('#\\.tmx$#',$entry))
						$files_list[]=$sub_dir.$entry;
				}
			}
			closedir($handle);
		}
    sort($files_list);
	return $files_list;
}

function getXmlList($dir,$sub_dir='')
{
	$files_list=array();
	if(is_dir($dir.$sub_dir))
		if($handle = opendir($dir.$sub_dir)) {
			while(false !== ($entry = readdir($handle))) {
			if($entry != '.' && $entry != '..') {
					if(is_dir($dir.$sub_dir.$entry))
						$files_list=array_merge($files_list,getXmlList($dir,$sub_dir.$entry.'/'));
					else if(preg_match('#\\.xml$#',$entry))
						$files_list[]=$sub_dir.$entry;
				}
			}
			closedir($handle);
		}
    sort($files_list);
	return $files_list;
}

function getDefinitionXmlList($dir,$sub_dir='')
{
	$files_list=array();
	if(is_dir($dir.$sub_dir))
		if($handle = opendir($dir.$sub_dir)) {
			while(false !== ($entry = readdir($handle))) {
			if($entry != '.' && $entry != '..') {
					if(is_dir($dir.$sub_dir.$entry))
						$files_list=array_merge($files_list,getDefinitionXmlList($dir,$sub_dir.$entry.'/'));
					else if(preg_match('#definition\\.xml$#',$entry))
						$files_list[]=$sub_dir.$entry;
				}
			}
			closedir($handle);
		}
    sort($files_list);
	return $files_list;
}
