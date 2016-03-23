<?php
while(1)
{
    $ctx = stream_context_create();
    $socket=@fsockopen('127.0.0.1',39034,$errno,$errstr,1);//,$ctx
    if($socket)
    {
        $contents=fread($socket,1);
        if(!isset($contents[0x00]))
            die('no encryption byte'."\n");
        else
        {
            if($contents[0x00]==0x01)
                die('encrypted, can\'t test'."\n");
            else
            {
                $tosend=hex2bin('a0019cd6498d10');
                $returnsize=fwrite($socket,$tosend,2+5);
                if($returnsize!=7)
                    die('wrong written size'."\n");
                else
                {
                    $contents=fread($socket,1+1+4+1);
                    if(!isset($contents[0x06]))
                        die('wrong read size'."\n");
                    else
                    {
                        echo 'All is ok'."\n";
                        usleep(10*1000);
                    }
                }
            }
        }
        fclose($socket);
    }
    else
        die('unable to connect'."\n");
}
