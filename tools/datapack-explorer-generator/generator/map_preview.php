<?php
if(!isset($datapackexplorergeneratorinclude))
    die('abort into generator map preview'."\n");

if(!is_dir($datapack_explorer_local_path.'maps/'))
    if(!mkdir($datapack_explorer_local_path.'maps/',0777,true))
        die('Unable to make: '.$datapack_explorer_local_path.'maps/');

foreach($temp_maps as $maindatapackcode=>$map_list)
foreach($map_list as $map)
{
    $map_html=str_replace('.tmx','.html',$map);
    $map_image=str_replace('.tmx','.png',$map);
    if(preg_match('#/#isU',$map))
    {
        $map_folder=preg_replace('#/[^/]+$#','',$maindatapackcode.'/'.$map).'/';
        if(!is_dir($datapack_explorer_local_path.'maps/'.$map_folder))
            if(!mkpath($datapack_explorer_local_path.'maps/'.$map_folder))
                die('Unable to make: '.$datapack_explorer_local_path.'maps/'.$map_folder);
    }
}

$temprand=rand(10000,99999);
if(isset($map_generator) && 
    (
        ($map_generator!='' && file_exists($map_generator)) ||
        (strpos($map_generator,' ')!==FALSE)
    )
)
{
    $qtargs=' -platform offscreen';
    $pwd=getcwd();
    $return_var=0;
    //echo 'cd '.$datapack_explorer_local_path.'maps/ && '.$map_generator.$qtargs.'  '.$pwd.'/'.$datapack_path.'map/';
    chdir($datapack_explorer_local_path.'maps/');
    
    //all map preview
    if(count($start_map_meta)>0)
    {
        $mapgroupdisplaygenerated=array();
        foreach($start_map_meta as $maindatapackcode=>$subdatapackcode_list)
        foreach($subdatapackcode_list as $subdatapackcode=>$map_list)
        foreach($map_list as $map)
        if(isset($maps_list[$maindatapackcode][$map]))
        {
            $overviweid=count($mapgroupdisplaygenerated)+1;
            //overview
            @unlink('overview-'.$overviweid.'.png');
            @unlink('preview-'.$overviweid.'.png');
            $before = microtime(true);
            //echo 'cd '.getcwd().';'.$map_generator.$qtargs.' '.$pwd.'/'.$datapack_path.'map/main/'.$maindatapackcode.'/'.$map.' overview-'.$overviweid.'.png --renderAll'."\n";
            echo exec($map_generator.$qtargs.' '.$pwd.'/'.$datapack_path.'map/main/'.$maindatapackcode.'/'.$map.' overview-'.$overviweid.'.png --renderAll',$output,$return_var);
            if($return_var!=0)
            {
                echo implode("\n",$output);
                echo 'Bug with ('.$return_var.'): cd '.getcwd().'/ && '.$map_generator.$qtargs.' '.$pwd.'/'.$datapack_path.'map/main/'.$maindatapackcode.'/'.$map.' overview-'.$overviweid.'.png --renderAll'."\n";
            }
            //echo implode($output,"\n");
            $after = microtime(true);
            echo 'Preview generation '.(int)($after-$before)."s\n";
            
            //preview
            if(file_exists('overview-'.$overviweid.'.png'))
            {
                if(is_executable('/usr/bin/convert'))
                {
                    $before = microtime(true);
                    exec('/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/convert -limit memory 2GiB -limit map 2GiB -limit disk 4GiB overview-'.$overviweid.'.png -resize 256x256 preview-'.$overviweid.'.png');
                    $after = microtime(true);
                    echo 'Preview resize generation '.(int)($after-$before)."s\n";

                    if(!in_array($maps_list[$maindatapackcode][$map]['mapgroup'],$mapgroupdisplaygenerated))
                        $mapgroupdisplaygenerated[]=$maps_list[$maindatapackcode][$map]['mapgroup'];
                }
                else
                    echo 'no /usr/bin/convert found, install imagemagick'."\n";
            }
            else
            {
                echo 'overview.png not found'."\n";
                echo 'cd '.$pwd.' && '.$map_generator.$qtargs.' '.$pwd.'/'.$datapack_path.'map/main/'.$maindatapackcode.'/'.$map.' overview-'.$overviweid.'.png --renderAll'."\n";
            }
        }
        else
            echo 'map for starter '.$map.' missing'."\n";
    }
    else
        echo 'starter to do overview map missing'."\n";

    //single map preview
    //echo 'cd '.getcwd().';'.$map_generator.$qtargs.' '.$pwd.'/'.$datapack_path.'map/main/'."\n";
    echo exec($map_generator.$qtargs.' '.$pwd.'/'.$datapack_path.'map/main/',$output,$return_var);
    echo implode("\n",$output);
    if($return_var!=0)
        echo 'Bug with ('.$return_var.'): cd '.$datapack_explorer_local_path.'maps/ && '.$map_generator.$qtargs.' '.$pwd.'/'.$datapack_path.'map/main/'.$maindatapackcode.'/'.$map.' overview-'.$overviweid.'.png --renderAll'."\n";
    /*
    if(!file_exists($datapack_explorer_local_path.'maps/preview-1.png'))
    //if($return_var!=0)
    {
        echo 'exec failed';
        exit;
    }*/
    if(is_executable('/usr/bin/mogrify'))
    {
        $before = microtime(true);
        echo exec('/usr/bin/find ./ -name \'*.png\' -exec /usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/mogrify -trim +repage {} \;',$output,$return_var);
        echo implode("\n",$output);
        $after = microtime(true);
        echo 'Png trim and repage into '.(int)($after-$before)."s\n";
    }
    else
        echo 'no /usr/bin/mogrify found, install imagemagick'."\n";

    //compression for all png found!
    if(isset($png_compress) && $png_compress!='')
    {
        $before = microtime(true);
        echo exec($png_compress,$output,$return_var);
        $after = microtime(true);
        echo 'Png compressed into '.(int)($after-$before)."s\n";
    }
    /*if(isset($png_compress_zopfli) && is_executable($png_compress_zopfli))
    {
        if(!isset($png_compress_zopfli_level))
            $png_compress_zopfli_level=100;
        $before = microtime(true);
        exec('/usr/bin/find ./ -name \'*.png\' -print -exec /usr/bin/ionice -c 3 /usr/bin/nice -n 19 '.$png_compress_zopfli.' --png --i'.$png_compress_zopfli_level.' {} \;');
        exec('/usr/bin/find ./ -name \'*.png\' -and ! -name \'*.png.png\' -exec mv {}.png {} \;');
        $after = microtime(true);
        echo 'Png trim and repage into '.(int)($after-$before)."s\n";
    }
    else
        echo 'zopfli for png don\'t installed, prefed install it'."\n";*/

    chdir($pwd);
}
else
{
    if(!isset($map_generator) || $map_generator=='')
        echo 'Map generator not found or disabled'."\n";
    else
        echo 'Map generator not found: '.$map_generator."\n";
}
 
