<?php
$content=file_get_contents($argv[1]);
$content=preg_replace('#'.preg_quote('<?xml ').'[^>]*'.preg_quote('?>').'#isU','',$content);
$content=preg_replace('#'.preg_quote('<!DOCTYPE ').'[^>]*'.preg_quote('>').'#isU','',$content);
$content=str_replace(' lang="en"','',$content);
$content=preg_replace('#<!--.*-->#isU','',$content);
//$content=preg_replace("#<[a-z]+ ?/>#isU",'',$content);////drop <br /> into bot test
$content=preg_replace("#^[ \t]+#isU",'',$content);
$content=preg_replace("#([\n\r])[ \t]+#",'$1',$content);
$content=preg_replace("#[ \t]+([\n\r])#",'$1',$content);
$content=preg_replace("#(<[^>]+)([\n\r])([^>]+>)#",'$1 $3',$content);
$content=preg_replace("#[\n\r]#",'',$content);
echo $content;
