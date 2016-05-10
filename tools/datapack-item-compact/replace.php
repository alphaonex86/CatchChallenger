<?php

if(!isset($argc) || $argc!=3)
    die('pass the correcty arg count: '.$argc.', pass: datapack path, item list');
$datapack_path=$argv[1];
if(!is_dir($datapack_path))
    die('datapack not dir');

if(file_exists($argv[2]) && $filecurs=file_get_contents($argv[2]))
{
    $convertItemTo=json_decode($filecurs,true);
    if(is_array($convertItemTo))
    {
    }
    else
        die('convert invalid');
}
else
    die('convert not open');

function getXmlList($dir,$sub_dir='')
{
    $files_list=array();
    if(is_dir($dir.$sub_dir))
        if($handle = opendir($dir.$sub_dir)) {
            while(false !== ($entry = readdir($handle))) {
            if($entry != '.' && $entry != '..') {
                    if(is_dir($dir.$sub_dir.$entry))
                        $files_list=array_merge($files_list,getXmlList($dir,$sub_dir.$entry.'/'));
                    else if(preg_match('#\\.xml$#',$entry) || preg_match('#\\.tmx$#',$entry))
                        $files_list[]=$sub_dir.$entry;
                }
            }
            closedir($handle);
        }
    sort($files_list);
    return $files_list;
}
function fileopen($file)
{
    if($filecurs=file_get_contents($file))
        return $filecurs;
    else
        return '';
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
$folder=$datapack_path.'/crafting/';
$files=getXmlList($folder);
foreach($files as $file)
{
    $content=fileopen($folder.$file);
    $content=preg_replace_callback('# itemId="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' itemId="'.$matches[1].'"',' itemId="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('# itemToLearn="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' itemToLearn="'.$matches[1].'"',' itemToLearn="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('# doItemId="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' doItemId="'.$matches[1].'"',' doItemId="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    filewrite($folder.$file,$content);
}
$folder=$datapack_path.'/industries/';
$files=getXmlList($folder);
foreach($files as $file)
{
    $content=fileopen($folder.$file);
    $content=preg_replace_callback('#<resource( quantity="([0-9]+)")? id="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[3]]))
            die('item not found, die: '.$matches[3].', line: '.__LINE__);
        return str_replace(' id="'.$matches[3].'"',' id="'.$convertItemTo[$matches[3]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<product( quantity="([0-9]+)")? id="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[3]]))
            die('item not found, die: '.$matches[3].', line: '.__LINE__);
        return str_replace(' id="'.$matches[3].'"',' id="'.$convertItemTo[$matches[3]].'"',$matches[0]);
    },$content);
    filewrite($folder.$file,$content);
}
$folder=$datapack_path.'/map/main/';
$files=getXmlList($folder);
foreach($files as $file)
{
    $content=fileopen($folder.$file);
    $content=preg_replace_callback('#<property value="([0-9]+)" name="item"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' value="'.$matches[1].'"',' value="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<property name="item" value="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' value="'.$matches[1].'"',' value="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<item( quantity="([0-9]+)")? id="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[3]]))
            die('item not found, die: '.$matches[3].', line: '.__LINE__);
        return str_replace(' id="'.$matches[3].'"',' id="'.$convertItemTo[$matches[3]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<product( overridePrice="([0-9]+)")? itemId="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[3]]))
            die('item not found, die: '.$matches[3].', line: '.__LINE__);
        return str_replace(' itemId="'.$matches[3].'"',' itemId="'.$convertItemTo[$matches[3]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<gain( quantity="([0-9]+)")? item="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[3]]))
            die('item not found, die: '.$matches[3].', line: '.__LINE__);
        return str_replace(' item="'.$matches[3].'"',' item="'.$convertItemTo[$matches[3]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<condition( id="([0-9]+)")?( type="item")?( id="([0-9]+)")? item="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[6]]))
            die('item not found, die: '.$matches[6].', line: '.__LINE__);
        return str_replace(' item="'.$matches[6].'"',' item="'.$convertItemTo[$matches[6]].'"',$matches[0]);
    },$content);
    filewrite($folder.$file,$content);
}
$folder=$datapack_path.'/monsters/';
$files=getXmlList($folder);
foreach($files as $file)
{
    $content=fileopen($folder.$file);
    $content=preg_replace_callback('#<drop item="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' item="'.$matches[1].'"',' item="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<evolution type="item" item="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' item="'.$matches[1].'"',' item="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<attack byitem="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' byitem="'.$matches[1].'"',' byitem="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    filewrite($folder.$file,$content);
}
$folder=$datapack_path.'/items/';
$files=getXmlList($folder);
foreach($files as $file)
{
    $content=fileopen($folder.$file);
    $content=preg_replace_callback('#<item( image="([0-9]+).png")?( price="([0-9]+)")?( image="([0-9]+).png")? id="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[7]]))
            die('item not found, die: '.$matches[7].', line: '.__LINE__);
        return str_replace(' id="'.$matches[7].'"',' id="'.$convertItemTo[$matches[7]].'"',$matches[0]);
    },$content);
    filewrite($folder.$file,$content);
}
$folder=$datapack_path.'/plants/';
$files=getXmlList($folder);
foreach($files as $file)
{
    $content=fileopen($folder.$file);
    $content=preg_replace_callback('# itemUsed="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' itemUsed="'.$matches[1].'"',' itemUsed="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    filewrite($folder.$file,$content);
}
$folder=$datapack_path.'/player/';
$files=getXmlList($folder);
foreach($files as $file)
{
    $content=fileopen($folder.$file);
    $content=preg_replace_callback('# captured_with="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[1]]))
            die('item not found, die: '.$matches[1].', line: '.__LINE__);
        return str_replace(' captured_with="'.$matches[1].'"',' captured_with="'.$convertItemTo[$matches[1]].'"',$matches[0]);
    },$content);
    $content=preg_replace_callback('#<item( quantity="([0-9]+)")? id="([0-9]+)"#isU',function ($matches)
    {
        global $convertItemTo;
        if(!isset($convertItemTo[$matches[3]]))
            die('item not found, die: '.$matches[3].', line: '.__LINE__);
        return str_replace(' id="'.$matches[3].'"',' id="'.$convertItemTo[$matches[3]].'"',$matches[0]);
    },$content);
    filewrite($folder.$file,$content);
}




