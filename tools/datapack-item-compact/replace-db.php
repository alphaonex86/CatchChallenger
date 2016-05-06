<?php

if(!isset($argc) || ($argc!=(1+1+1+4) && $argc!=(1+1+1+3)))
    die('pass the correcty arg count: item list, [test, replace], dbname, user, password, host: '.$argc);

if(file_exists($argv[1]) && $filecurs=file_get_contents($argv[1]))
{
    $convertItemTo=json_decode($filecurs,true);
    if(!is_array($convertItemTo))
        die('convert invalid');
}
else
    die('convert not open');

if($argv[2]!='test' && $argv[2]!='replace')
    die('segund arg not test or replace');

if(!isset($argv[6]))
    $argv[6]='localhost';

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

$workonsetsize=1000;
set_time_limit(0);
$character_converted=0;
$monster_converted=0;
$convertion_problem=0;
$blob_version_incorrect_count=0;
$item_incorrect_count=0;

///use cursor to prevent download all the database content
try {
if($argv[6]!='localhost')
    $pdo = new PDO("pgsql:host=".$argv[6]." dbname=".$argv[3],$argv[4],$argv[5]);
else
    $pdo = new PDO("pgsql:dbname=".$argv[3],$argv[4],$argv[5]);
} catch (PDOException $e) {
    die('Connection failed: '.$e->getMessage());
}
$pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
$sql = "SELECT id,blob_version,encode(item,'hex') as item,encode(item_warehouse,'hex') as item_warehouse FROM character";
$pdo->beginTransaction();
if($pdo->exec("DECLARE character_cursor NO SCROLL CURSOR FOR ({$sql})")===FALSE)
    die($pdo->errorInfo()[0].','.$pdo->errorInfo()[1].','.$pdo->errorInfo()[2]);
$stmt = $pdo->query("FETCH ".$workonsetsize." FROM character_cursor");
if($stmt===FALSE)
    die($pdo->errorInfo()[0].','.$pdo->errorInfo()[1].','.$pdo->errorInfo()[2]);
$results = $stmt->fetchAll(PDO::FETCH_ASSOC);
$c=count($results);
while($c>0)
{
    $index=0;
    while($index<$c)
    {
        $row=$results[$index];
        if($row['blob_version']!=0)
        {
            $index++;
            $blob_version_incorrect_count++;
            continue;
        }
        $item=str_replace('\\x','',$row['item']);
        $item_warehouse=str_replace('\\x','',$row['item_warehouse']);
        if(strlen($item)%((2+4)*2)!=0 || strlen($row['item_warehouse'])%((2+4)*2)!=0)
        {
            $index++;
            $item_incorrect_count++;
            continue;
        }
        $new_item=convert_item($item,$convertItemTo);
        if(strlen($new_item)!=strlen($item))
            die('old and new string don\'t have the same size');
        $new_item_warehouse=convert_item($item_warehouse,$convertItemTo);
        if(strlen($new_item_warehouse)!=strlen($item_warehouse))
            die('old and new string don\'t have the same size');

        {
            $stmt2 = $pdo->query("SELECT id,captured_with FROM monster WHERE character=".$row['id']);
            if($stmt2===FALSE)
                die($pdo->errorInfo()[0].','.$pdo->errorInfo()[1].','.$pdo->errorInfo()[2]);
            $results2 = $stmt2->fetchAll(PDO::FETCH_ASSOC);
            $c2=count($results2);
            $index2=0;
            while($index2<$c2)
            {
                $row2=$results2[$index2];
                $item_id=$row2['captured_with'];
                if(isset($convertItemTo[$item_id]))
                {
                    $monster_converted++;
                    if($argv[2]=='replace')
                        if($pdo->exec("UPDATE monster SET captured_with=".$convertItemTo[$item_id]." WHERE id=".$row2['id'])===NULL)
                            die("An error occurred. ".$pdo->errorInfo()[0].','.$pdo->errorInfo()[1].','.$pdo->errorInfo()[2]." at character id ".$row['id']." with monster ".$row2['id']."\n");
                }
                else
                    $convertion_problem++;
                $index2++;
            }
        }

        $character_converted++;
        if($argv[2]=='replace')
        {
            if($new_item!='' || $new_item_warehouse!='')
            {
                $query_item="UPDATE character SET ";
                if($new_item!='')
                    $query_item.="item='\\x".$new_item."'";
                if($new_item!='' && $new_item_warehouse!='')
                    $query_item.=",";
                if($new_item_warehouse!='')
                    $query_item.="item_warehouse='\\x".$new_item_warehouse."'";
                $query_item=" WHERE id=".$row['id'];
                if($pdo->exec()===NULL)
                    die("An error occurred. ".$pdo->errorInfo()[0].','.$pdo->errorInfo()[1].','.$pdo->errorInfo()[2]." at character id ".$row['id']."\n");
            }
        }
        $index++;
    }
    if($argv[2]=='replace')
        if(!$pdo->commit())
                die("An error occurred. ".$pdo->errorInfo()[0].','.$pdo->errorInfo()[1].','.$pdo->errorInfo()[2]." at character id ".$row['id']."\n");
    else
        if(!$pdo->rollback())
                die("An error occurred. ".$pdo->errorInfo()[0].','.$pdo->errorInfo()[1].','.$pdo->errorInfo()[2]." at character id ".$row['id']."\n");

    $stmt = $pdo->query("FETCH ".$workonsetsize." FROM character_cursor");
    if($stmt===FALSE)
        die($pdo->errorInfo()[0].','.$pdo->errorInfo()[1].','.$pdo->errorInfo()[2]);
    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);
    $c = count($results);
}

echo 'blob version incorrect count: '.$blob_version_incorrect_count."\n";
echo 'item incorrect count: '.$item_incorrect_count."\n";
echo 'convertion problem: '.$convertion_problem."\n";
echo '-------------------------------------'."\n";
echo 'character converted: '.$character_converted.', monster converted: '.$monster_converted."\n";
