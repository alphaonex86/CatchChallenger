<?php
echo "<!--
multiplicator number need be multiple of 2: 2, 0.5, 0
All not into multiplicator is not multiplied or divided
The to value is separated by ;
-->
<types>\n";

$file=file_get_contents('battle/movetypes.xml');
preg_match_all('#<entry>(.*)</entry>#isU',$file,$entry_list);
$type_list=array();
foreach($entry_list[1] as $entry)
{
	if(!preg_match('#<type>[^<]+</type>#isU',$entry))
		continue;
	$type=preg_replace('#^.*<type>([^<]+)</type>.*$#isU','$1',$entry);
	$type_list[$type]=0;
}

foreach($type_list as $id=>$entry)
{
	?>
	<type name="<?php echo strtolower($id); ?>">
		<name><?php echo ucfirst($id); ?></name>
	</type>
<?php
}

echo "</types>\n";
