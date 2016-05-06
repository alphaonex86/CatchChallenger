<?php

if(!isset($argc) || $argc!=(1+1+4))
    die('pass the correcty arg count: item list, dbname, user, password, host: '.$argc);

if(file_exists($argv[1]) && $filecurs=file_get_contents($argv[1]))
{
    $convertItemTo=json_decode($filecurs,true);
    if(!is_array($convertItemTo))
        die('convert invalid');
}
else
    die('convert not open');

if($argv[5]!='localhost')
    $postgres_link_common = pg_connect('dbname='.$argv[2].' user='.$argv[3].' password='.$argv[4].' host='.$argv[5]);
else
    $postgres_link_common = pg_connect('dbname='.$argv[2].' user='.$argv[3].' password='.$argv[4]);
if($postgres_link_common===FALSE)
    die('db not open');

function uint16HexInvert($hexa)
{
    if(strlen($hexa)!=4)
        die($hexa.' not 4 size');
    return substr($hexa,2,2).substr($hexa,0,2);
}
function convert_item($item_list,$convertItemTo)
{
    $newstring='';
    global $convertion_problem;
    $len=strlen($item_list);
    $pos=0;
    while($pos<$len)
    {
        $item_id=hexdec(uint16HexInvert(substr($item_list,$pos,4)));
        if(isset($convertItemTo[$item_id]))
            $item_id=$convertItemTo[$item_id];
        else
            $convertion_problem++;
        $newhex=str_pad(dechex($item_id),4,'0',STR_PAD_LEFT);
        $newstring.=uint16HexInvert($newhex);
        $pos+=4;
        //ignore quantity
        $newstring.=substr($item_list,$pos,8);
        $pos+=8;
    }
    return $newstring;
}

$convertion_problem=0;
$blob_version_incorrect_count=0;
$item_incorrect_count=0;

$result = pg_query($postgres_link_common, "SELECT id,blob_version,encode(item,'hex'),encode(item_warehouse,'hex') FROM character");
if(!$result)
    die("An error occurred.\n");
while($row = pg_fetch_row($result))
{
    if($row[1]!=0)
    {
        $blob_version_incorrect_count++;
        continue;
    }
    if(strlen($row[2])%((2+4)*2)!=0 || strlen($row[3])%((2+4)*2)!=0)
    {
        $item_incorrect_count++;
        continue;
    }
    $new_item=convert_item($row[2],$convertItemTo);
    if(strlen($new_item)!=strlen($row[2]))
        die('old and new string don\'t have the same size');
    $new_item_warehouse=convert_item($row[3],$convertItemTo);
    if(strlen($new_item_warehouse)!=strlen($row[3]))
        die('old and new string don\'t have the same size');

    {
        $result2 = pg_query($postgres_link_common, "SELECT id,captured_with FROM monster WHERE character=".$row[0]);
        if(!$result2)
            die("An error occurred.\n");
        while($row2 = pg_fetch_row($result2))
        {
            $item_id=$row2[1];
            if(isset($convertItemTo[$item_id]))
                pg_query($postgres_link_common, "UPDATE monster SET captured_with=".$convertItemTo[$item_id]." WHERE id=".$row2[0]);
            else
                $convertion_problem++;
        }
    }

    pg_query($postgres_link_common, "UPDATE character SET item='\\x".$new_item."',item_warehouse='\\x".$new_item_warehouse."' WHERE id=".$row[0]);
}

echo 'blob version incorrect count: '.$blob_version_incorrect_count."\n";
echo 'item incorrect count: '.$item_incorrect_count."\n";
echo 'convertion problem: '.$convertion_problem."\n";
