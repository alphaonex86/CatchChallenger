<?php
$file=file_get_contents('items2.xml');
preg_match_all('#<entry>(.*)</entry>#isU',$file,$entry_list);
echo "<items>\n";
foreach($entry_list[1] as $entry)
{
	if(!preg_match('#<m_id>[0-9]+</m_id>#isU',$entry))
		continue;
	$id=preg_replace('#^.*<m_id>([0-9]+)</m_id>.*$#isU','$1',$entry);
	if(!preg_match('#<m_category>[^<]+</m_category>#isU',$entry))
		continue;
	$category=preg_replace('#^.*<m_category>([^<]+)</m_category>.*$#isU','$1',$entry);
	if($category!='TM')
		$image=$id.'.png';
	else
		$image='TM.png';
	if(file_exists($image))
	{
		if(!preg_match('#<m_name>[^<]+</m_name>#isU',$entry))
			continue;
		$name=preg_replace('#^.*<m_name>([^<]+)</m_name>.*$#isU','$1',$entry);
		if(!preg_match('#<m_description>[^<]+</m_description>#isU',$entry))
			continue;
		$description=preg_replace('#^.*<m_description>([^<]+)</m_description>.*$#isU','$1',$entry);
		$description=preg_replace("#[\n\r\t]+.*$#isU",'',$description);
		if(!preg_match('#<m_price>[0-9]+</m_price>#isU',$entry))
			continue;
		$price=preg_replace('#^.*<m_price>([0-9]+)</m_price>.*$#isU','$1',$entry);
		?>
    <item id="<?php echo $id; ?>" image="<?php echo $image; ?>" price="<?php echo $price; ?>">
        <name><?php echo $name; ?></name>
        <description><?php echo $description; ?></description>
    </item>
<?php
	}
}
echo "</items>\n";
